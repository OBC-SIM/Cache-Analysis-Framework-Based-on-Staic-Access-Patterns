#!/usr/bin/env python3
"""Benchmark CASA simulation scaling on parameterized PolyBench workloads."""

import argparse
import csv
import math
import os
import re
import statistics
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BASELINE = Path("/tmp/casa-baseline/build-release/casa")
DEFAULT_CURRENT = ROOT / "build-release/casa"
WORKLOADS = {
    "polybench_2mm": ("polybench_2mm.c", ("NI", "NJ", "NK", "NL")),
    "polybench_atax": ("polybench_atax.c", ("M", "N")),
    "polybench_correlation": ("polybench_correlation.c", ("M", "N")),
    "polybench_gemm": ("polybench_gemm.c", ("N",)),
    "polybench_jacobi": ("polybench_jacobi.c", ("N",)),
}


def parse_args() -> argparse.Namespace:
    """Parse the isolated scaling-experiment configuration."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scales", type=int, nargs="+", default=(1, 2, 3))
    parser.add_argument("--repetitions", type=int, default=7)
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    parser.add_argument("--current", type=Path, default=DEFAULT_CURRENT)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "comparisions/scale-results")
    return parser.parse_args()


def run(command: list[str], log: Path) -> None:
    """Run a command and retain stdout/stderr in its experiment log."""
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("w") as output:
        result = subprocess.run(command, stdout=output,
                                stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(command)}")


def scaled_source(source: Path, names: tuple[str, ...], scale: int) -> str:
    """Return source with named integer dimension macros multiplied by scale."""
    text = source.read_text()
    for name in names:
        pattern = rf"^(#define\s+{name}\s+)(\d+)(.*)$"
        match = re.search(pattern, text, flags=re.MULTILINE)
        if not match:
            raise ValueError(f"dimension macro {name} not found in {source}")
        value = int(match.group(2)) * scale
        text = re.sub(pattern, rf"\g<1>{value}\g<3>", text,
                      count=1, flags=re.MULTILINE)
    return text


def prepare_inputs(output: Path, scales: list[int]) -> dict[tuple[str, int], Path]:
    """Generate one AP JSON per workload/array-size pair for both binaries."""
    generated = output / "generated"
    include_dir = ROOT / "comparisions/tasks"
    inputs = {}
    for workload, (filename, dimensions) in WORKLOADS.items():
        source = ROOT / "comparisions/tasks" / filename
        for scale in scales:
            destination = generated / f"{workload}_x{scale}.c"
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(scaled_source(source, dimensions, scale))
            run([sys.executable, str(ROOT / "pipeline.py"), str(destination),
                 "--include", str(include_dir), "--ape-only"],
                output / "logs" / f"prepare_{workload}_x{scale}.log")
            inputs[workload, scale] = destination.with_name(
                f"{destination.stem}_g_ape.json")
    return inputs


def time_simulation(binary: Path, ap_json: Path, report_dir: Path) -> float:
    """Measure one AP JSON to report simulation invocation."""
    command = [str(binary), "run", str(ap_json), "--cache",
               str(ROOT / "settings/cachegrind.yaml"), "--output",
               str(report_dir), "--quiet", "--no-color"]
    started = time.perf_counter()
    result = subprocess.run(command, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, check=False)
    elapsed = time.perf_counter() - started
    if result.returncode:
        raise RuntimeError(f"simulation failed ({result.returncode}): {' '.join(command)}")
    return elapsed


def load_runtime(path: Path) -> list[dict]:
    """Load previously checkpointed samples so interrupted runs can resume."""
    if not path.exists():
        return []
    with path.open(newline="") as source:
        return [{"workload": row["workload"], "scale": int(row["scale"]),
                 "variant": row["variant"], "repetition": int(row["repetition"]),
                 "seconds": float(row["seconds"])}
                for row in csv.DictReader(source)]


def checkpoint_runtime(path: Path, rows: list[dict]) -> None:
    """Persist every completed sample before an external time limit can interrupt."""
    write_csv(path, rows)


def measure(inputs: dict, output: Path, baseline: Path, current: Path,
            scales: list[int], repetitions: int, rows: list[dict]) -> list[dict]:
    """Run paired, alternating baseline/current timings for every scale."""
    completed = {(row["workload"], row["scale"], row["variant"],
                  row["repetition"]) for row in rows}
    for workload in WORKLOADS:
        for scale in scales:
            ap_json = inputs[workload, scale]
            for repetition in range(1, repetitions + 1):
                variants = (("baseline", baseline), ("current", current))
                if repetition % 2 == 0:
                    variants = tuple(reversed(variants))
                for variant, binary in variants:
                    key = workload, scale, variant, repetition
                    if key in completed:
                        continue
                    seconds = time_simulation(binary, ap_json,
                                              output / "reports" / variant)
                    rows.append({"workload": workload, "scale": scale,
                                 "variant": variant, "repetition": repetition,
                                 "seconds": seconds})
                    completed.add(key)
                    checkpoint_runtime(output / "runtime.csv", rows)
                    print(f"{workload} x{scale} rep {repetition}: "
                          f"{variant} {seconds:.6f}s", flush=True)
    return rows


def summarize(rows: list[dict]) -> list[dict]:
    """Calculate per-workload median, mean, and relative speedup."""
    summary = []
    for workload in WORKLOADS:
        for scale in sorted({row["scale"] for row in rows}):
            samples = {variant: [row["seconds"] for row in rows
                                 if row["workload"] == workload
                                 and row["scale"] == scale
                                 and row["variant"] == variant]
                       for variant in ("baseline", "current")}
            before, after = samples["baseline"], samples["current"]
            median_before, median_after = statistics.median(before), statistics.median(after)
            summary.append({
                "workload": workload, "scale": scale,
                "baseline_median_s": median_before,
                "current_median_s": median_after,
                "baseline_mean_s": statistics.mean(before),
                "current_mean_s": statistics.mean(after),
                "median_speedup": median_before / median_after,
                "mean_speedup": statistics.mean(before) / statistics.mean(after),
            })
    return summary


def geometric_mean(values: list[float]) -> float:
    """Return a geometric mean suitable for multiplicative speedups."""
    return math.exp(statistics.mean(math.log(value) for value in values))


def write_csv(path: Path, rows: list[dict]) -> None:
    """Write result dictionaries using their stable first-row field order."""
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)


def write_report(path: Path, summary: list[dict], scales: list[int]) -> None:
    """Write a compact result table and per-scale aggregate improvement."""
    lines = ["# PolyBench Array-Scaling Performance", "",
             "Simulation-only wall time; median and mean are each based on seven runs.",
             "Baseline and current runs alternate order within every repetition.", "",
             "| Workload | Scale | Baseline median (s) | Current median (s) | Speedup | Improvement |",
             "|---|---:|---:|---:|---:|---:|"]
    for row in summary:
        lines.append("| {workload} | {scale}x | {baseline_median_s:.6f} | "
                     "{current_median_s:.6f} | {median_speedup:.3f}x | "
                     "{improvement:.1f}% |".format(
                         **row, improvement=(row["median_speedup"] - 1) * 100))
    lines.extend(("", "## Aggregate", "",
                  "| Scale | Geometric-mean speedup | Improvement |",
                  "|---:|---:|---:|"))
    all_speedups = []
    for scale in scales:
        speedup = geometric_mean([row["median_speedup"] for row in summary
                                  if row["scale"] == scale])
        all_speedups.extend(row["median_speedup"] for row in summary
                            if row["scale"] == scale)
        lines.append(f"| {scale}x | {speedup:.3f}x | {(speedup - 1) * 100:.1f}% |")
    overall = geometric_mean(all_speedups)
    lines.extend(("", f"Overall geometric-mean speedup: **{overall:.3f}x** "
                  f"(**{(overall - 1) * 100:.1f}%** faster).", ""))
    path.write_text("\n".join(lines))


def plot(summary: list[dict], scales: list[int], path: Path) -> None:
    """Plot per-workload runtime scaling and aggregate speedup."""
    os.environ.setdefault("MPLBACKEND", "Agg")
    os.environ.setdefault("MPLCONFIGDIR", "/tmp/casa-matplotlib")
    import matplotlib.pyplot as plt

    figure, axes = plt.subplots(2, 3, figsize=(11.2, 6.2), constrained_layout=True)
    flat_axes = axes.flat
    for axis, workload in zip(flat_axes, WORKLOADS):
        points = [row for row in summary if row["workload"] == workload]
        axis.plot(scales, [row["baseline_median_s"] for row in points], "o-",
                  color="#C44E52", label="baseline")
        axis.plot(scales, [row["current_median_s"] for row in points], "s-",
                  color="#4C72B0", label="current")
        axis.set(title=workload.removeprefix("polybench_"), xlabel="Array scale",
                 ylabel="Median runtime (s)", yscale="log")
        axis.set_xticks(scales, [f"{scale}x" for scale in scales])
        axis.grid(axis="y", alpha=0.25)
    axes[0, 0].legend(frameon=False, fontsize=8)
    aggregate = [geometric_mean([row["median_speedup"] for row in summary
                                 if row["scale"] == scale]) for scale in scales]
    aggregate_axis = axes[1, 2]
    aggregate_axis.bar([f"{scale}x" for scale in scales], aggregate,
                       color="#55A868", edgecolor="black", linewidth=0.5)
    aggregate_axis.axhline(1, color="black", linewidth=0.8)
    aggregate_axis.bar_label(aggregate_axis.containers[0],
                             labels=[f"{value:.2f}x" for value in aggregate],
                             padding=2, fontsize=8)
    aggregate_axis.set(title="Geometric-mean speedup", ylabel="Baseline / current")
    aggregate_axis.set_ylim(0, max(aggregate) * 1.25)
    figure.suptitle("CASA simulation performance as PolyBench arrays scale", fontsize=13)
    figure.savefig(path, dpi=200, facecolor="white")
    plt.close(figure)


def main() -> None:
    """Execute the scaling study and save reproducible artifacts."""
    args = parse_args()
    if any(scale <= 0 for scale in args.scales) or args.repetitions <= 0:
        raise SystemExit("scales and repetitions must be positive")
    for binary in (args.baseline, args.current):
        if not binary.is_file():
            raise SystemExit(f"CASA executable not found: {binary}")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    inputs = prepare_inputs(output, sorted(args.scales))
    rows = load_runtime(output / "runtime.csv")
    rows = measure(inputs, output, args.baseline.resolve(), args.current.resolve(),
                   sorted(args.scales), args.repetitions, rows)
    summary = summarize(rows)
    write_csv(output / "runtime.csv", rows)
    write_csv(output / "summary.csv", summary)
    write_report(output / "README.md", summary, sorted(args.scales))
    plot(summary, sorted(args.scales), output / "performance_scaling.png")


if __name__ == "__main__":
    main()
