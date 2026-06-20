#!/usr/bin/env python3
"""Generate paper figures from compact comparison CSV files."""

import argparse
import os
from pathlib import Path

from backend import comparison_reports
from backend.plotting import comparison_figures, style


def generate(input_dir, output_dir=None) -> list:
    """Generate all comparison figures as PNG and PDF files."""
    os.environ.setdefault("MPLBACKEND", "Agg")
    os.environ.setdefault("MPLCONFIGDIR", "/tmp/casa-matplotlib")
    source = Path(input_dir)
    output = Path(output_dir) if output_dir else source / "figures"
    output.mkdir(parents=True, exist_ok=True)
    style.setup_theme()
    rows = comparison_reports.load_comparison(source / "comparison.csv")
    summary = comparison_reports.load_summary(source / "summary.csv")
    causes = comparison_reports.load_causes(source / "miss_causes.csv")
    figures = {
        "l1_validation": comparison_figures.validation_scatter(
            rows, summary, "l1_misses", "L1 miss validation"),
        "ll_validation": comparison_figures.validation_scatter(
            rows, summary, "ll_misses", "Last-level miss validation"),
        "relative_error": comparison_figures.relative_error_bars(rows),
        "miss_causes": comparison_figures.cause_breakdown(causes),
        "optimization_effect": comparison_figures.optimization_effect(rows),
    }
    runtime_path = source / "runtime.csv"
    if runtime_path.exists():
        runtime = comparison_reports.load_runtime(runtime_path)
        polybench = {row["workload"] for row in runtime
                     if row["workload"].startswith("polybench_")}
        if len(polybench) == 5:
            figures["runtime_comparison"] = comparison_figures.runtime_comparison(runtime)
    saved = []
    import matplotlib.pyplot as plt
    for name, figure in figures.items():
        for suffix in ("png", "pdf"):
            path = output / f"{name}.{suffix}"
            figure.savefig(path, dpi=200, bbox_inches="tight",
                           facecolor="white", pad_inches=0.04)
            saved.append(path)
        plt.close(figure)
    return saved


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", default="comparisions/comparison-results")
    parser.add_argument("--output")
    args = parser.parse_args()
    for path in generate(args.input, args.output):
        print(path)


if __name__ == "__main__":
    main()
