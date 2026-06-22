#!/usr/bin/env python3
"""Run reproducible CASA and Cachegrind comparison experiment stages."""

import argparse
import csv
import json
import subprocess
import sys
import time
from pathlib import Path


def _run(command: list, log: Path) -> tuple:
    log.parent.mkdir(parents=True, exist_ok=True)
    started = time.perf_counter()
    with log.open("w") as output:
        result = subprocess.run(command, stdout=output,
                                stderr=subprocess.STDOUT, check=False)
    return result.returncode, time.perf_counter() - started


def _write_runtime(path: Path, rows: list) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    stages = {row["stage"] for row in rows}
    if "casa_simulation" in stages:
        stages.add("casa_e2e")
    preserved = []
    if path.exists():
        with path.open(newline="") as source:
            preserved = [row for row in csv.DictReader(source)
                         if row["stage"] not in stages]
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=(
            "workload", "stage", "repetition", "seconds", "exit_code"))
        writer.writeheader()
        writer.writerows(preserved + rows)


def compile_workloads(base: Path, benchmarks: list, repetitions: int) -> list:
    """Compile native workload binaries used by Cachegrind."""
    result_dir = base / "cachegrind-results"
    (result_dir / "bin").mkdir(parents=True, exist_ok=True)
    rows = []
    for item in benchmarks:
        command = ["clang-14", "-O0", "-g", "-I", str(base / "tasks"),
                   str(base / item["source"]), "-lm", "-o",
                   str(result_dir / "bin" / item["name"])]
        code, seconds = _run(command, result_dir / "logs" /
                             f"{item['name']}.compile.log")
        rows.append(_timing(item["name"], "compile", 1, seconds, code))
    return rows


def run_cachegrind(base: Path, manifest: dict, benchmarks: list,
                   repetitions: int) -> list:
    """Profile native workload binaries with the manifest cache geometry."""
    result_dir = base / "cachegrind-results"
    (result_dir / "raw").mkdir(parents=True, exist_ok=True)
    rows = []
    for item in benchmarks:
        binary = result_dir / "bin" / item["name"]
        if not binary.exists():
            rows.append(_timing(item["name"], "cachegrind", 0, 0, 127))
            continue
        for repetition in range(0, repetitions + 1):
            raw = result_dir / "raw" / f"cachegrind.out.{item['name']}"
            command = ["valgrind", "--tool=cachegrind",
                       f"--D1={manifest['cachegrind']['d1']}",
                       f"--LL={manifest['cachegrind']['ll']}",
                       f"--cachegrind-out-file={raw}", str(binary)]
            code, seconds = _run(command, result_dir / "logs" /
                                 f"{item['name']}.valgrind.log")
            if repetition:
                rows.append(_timing(item["name"], "cachegrind", repetition, seconds, code))
            if code:
                break
    return rows


def run_casa(repo: Path, base: Path, benchmarks: list,
             repetitions: int, casa_binary: Path) -> list:
    """Measure CASA simulation from prepared AP JSON through report output."""
    output = base / "casa-results"
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    for item in benchmarks:
        ap_json = base / "tasks" / f"{item['name']}_g_ape.json"
        if not ap_json.exists():
            prepare = [sys.executable, str(repo / "pipeline.py"),
                       str(base / item["source"]), "--include", str(base / "tasks"),
                       "--ape-only"]
            code, _ = _run(prepare, output / f"{item['name']}.prepare.log")
            if code:
                continue
        for repetition in range(0, repetitions + 1):
            command = [str(casa_binary), "run", str(ap_json),
                       "--cache", str(repo / "settings/cachegrind.yaml"),
                       "--output", str(output), "--quiet", "--no-color"]
            code, seconds = _run(command, output / f"{item['name']}.simulation.log")
            if repetition:
                rows.append(_timing(item["name"], "casa_simulation", repetition, seconds, code))
            if code:
                break
    return rows


def _timing(name, stage, repetition, seconds, code) -> dict:
    return {"workload": name, "stage": stage, "repetition": repetition,
            "seconds": f"{seconds:.9f}", "exit_code": code}


def _select(benchmarks: list, names: list) -> list:
    if not names:
        return benchmarks
    selected = [item for item in benchmarks if item["name"] in names]
    missing = set(names) - {item["name"] for item in selected}
    if missing:
        raise SystemExit(f"unknown workloads: {', '.join(sorted(missing))}")
    return selected


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", choices=("compile", "cachegrind", "casa",
                                          "aggregate", "figures", "all"))
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).with_name("benchmark_manifest.json"))
    parser.add_argument("--workload", action="append", default=[])
    parser.add_argument("--repetitions", type=int, default=1)
    parser.add_argument("--casa-bin", type=Path, default=Path("build/casa"),
                        help="CASA executable used for simulation timing")
    args = parser.parse_args()
    repo, base = Path(__file__).resolve().parent.parent, args.manifest.resolve().parent
    manifest = json.loads(args.manifest.read_text())
    benchmarks = _select(manifest["benchmarks"], args.workload)
    timings = []
    if args.stage in ("compile", "all"):
        timings += compile_workloads(base, benchmarks, args.repetitions)
    if args.stage in ("cachegrind", "all"):
        timings += run_cachegrind(base, manifest, benchmarks, args.repetitions)
    if args.stage in ("casa", "all"):
        casa_binary = args.casa_bin.resolve()
        if not casa_binary.is_file():
            raise SystemExit(f"CASA executable not found: {casa_binary}")
        timings += run_casa(repo, base, benchmarks, args.repetitions,
                            casa_binary)
    if timings:
        _write_runtime(base / "comparison-results/runtime.csv", timings)
    if args.stage in ("aggregate", "all"):
        subprocess.run([sys.executable, str(base / "experiment_stats.py")], check=True)
    if args.stage in ("figures", "all"):
        subprocess.run([sys.executable, "-m", "backend.comparison_main"],
                       cwd=repo, check=True)


if __name__ == "__main__":
    main()
