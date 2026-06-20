#!/usr/bin/env python3
"""Combine CASA reports with function-scoped Cachegrind measurements."""

import argparse
import csv
import json
from pathlib import Path

from cachegrind_parser import parse_functions

METRICS = ("accesses", "l1_misses", "ll_misses")


def _relative_error(actual: int, reference: int) -> float:
    return abs(actual - reference) / max(abs(reference), 1)


def _r_squared(pairs: list) -> float:
    if len(pairs) < 2:
        return float("nan")
    xs, ys = zip(*pairs)
    x_mean, y_mean = sum(xs) / len(xs), sum(ys) / len(ys)
    x_var = sum((x - x_mean) ** 2 for x in xs)
    y_var = sum((y - y_mean) ** 2 for y in ys)
    if x_var == 0 or y_var == 0:
        return float("nan")
    covariance = sum((x - x_mean) * (y - y_mean) for x, y in pairs)
    return covariance ** 2 / (x_var * y_var)


def _casa_values(report: dict) -> dict:
    return {
        "accesses": report["accesses"]["total"],
        "l1_misses": report["levels"]["L1"]["misses"],
        "ll_misses": report["levels"]["L2"]["misses"],
    }


def collect(manifest_path: Path, cachegrind_dir: Path, casa_dir: Path):
    """Collect comparison rows, failures, and CASA cause breakdowns."""
    manifest = json.loads(manifest_path.read_text())
    rows, statuses, causes = [], [], []
    for benchmark in manifest["benchmarks"]:
        name = benchmark["name"]
        raw = cachegrind_dir / f"cachegrind.out.{name}"
        report_path = casa_dir / f"{name}_ape.json"
        status, reason = "ok", ""
        try:
            cg = parse_functions(raw, benchmark["kernels"])
        except (OSError, ValueError) as error:
            status, reason = "cachegrind_failed", str(error)
            cg = None
        if not report_path.exists():
            status = "unsupported" if not benchmark.get("casa_supported", True) else "casa_failed"
            reason = benchmark.get("expected_failure", "CASA report is missing")
        if status == "ok":
            report = json.loads(report_path.read_text())
            casa = _casa_values(report)
            cachegrind = {
                "accesses": cg.accesses,
                "l1_misses": cg.l1_misses,
                "ll_misses": cg.ll_misses,
            }
            for metric in METRICS:
                rows.append({
                    "workload": name, "tier": benchmark["tier"],
                    "metric": metric, "casa_value": casa[metric],
                    "cachegrind_value": cachegrind[metric],
                    "absolute_error": abs(casa[metric] - cachegrind[metric]),
                    "relative_error": _relative_error(casa[metric], cachegrind[metric]),
                })
            cause = report.get("miss_breakdown", {}).get("cause", {})
            causes.append({"workload": name, "tier": benchmark["tier"], **{
                key: int(cause.get(key, report.get(key, 0)))
                for key in ("cold", "capacity", "conflict", "policy")}})
        statuses.append({"workload": name, "tier": benchmark["tier"],
                         "status": status, "reason": reason})
    return rows, statuses, causes


def summarize(rows: list) -> list:
    """Calculate MAPE and correlation R-squared by tier and metric."""
    summaries = []
    for tier in ("exact", "trend", "all"):
        for metric in METRICS:
            selected = [r for r in rows if r["metric"] == metric and
                        (tier == "all" or r["tier"] == tier)]
            pairs = [(r["cachegrind_value"], r["casa_value"]) for r in selected]
            summaries.append({
                "tier": tier, "metric": metric, "n": len(selected),
                "mape": sum(r["relative_error"] for r in selected) / max(len(selected), 1),
                "r_squared": _r_squared(pairs),
            })
    return summaries


def _write_csv(path: Path, rows: list) -> None:
    if not rows:
        return
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    base = Path(__file__).resolve().parent
    parser.add_argument("--manifest", type=Path, default=base / "benchmark_manifest.json")
    parser.add_argument("--cachegrind", type=Path, default=base / "cachegrind-results/raw")
    parser.add_argument("--casa", type=Path, default=base / "casa-results")
    parser.add_argument("--output", type=Path, default=base / "comparison-results")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rows, statuses, causes = collect(args.manifest, args.cachegrind, args.casa)
    _write_csv(args.output / "comparison.csv", rows)
    _write_csv(args.output / "summary.csv", summarize(rows))
    _write_csv(args.output / "status.csv", statuses)
    _write_csv(args.output / "miss_causes.csv", causes)
    failed = sum(row["status"] != "ok" for row in statuses)
    print(f"Compared {len(statuses) - failed}/{len(statuses)} workloads; {failed} unavailable")


if __name__ == "__main__":
    main()
