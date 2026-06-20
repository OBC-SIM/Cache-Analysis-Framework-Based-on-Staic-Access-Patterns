"""Load compact CSV artifacts produced by the comparison experiment."""

import csv
from pathlib import Path


def _read(path: Path) -> list:
    with Path(path).open(newline="") as source:
        return list(csv.DictReader(source))


def load_comparison(path) -> list:
    """Load per-workload comparison values with numeric conversion."""
    rows = _read(Path(path))
    for row in rows:
        for key in ("casa_value", "cachegrind_value", "absolute_error"):
            row[key] = int(row[key])
        row["relative_error"] = float(row["relative_error"])
    return rows


def load_summary(path) -> list:
    """Load aggregate validation statistics."""
    rows = _read(Path(path))
    for row in rows:
        row["n"] = int(row["n"])
        row["mape"] = float(row["mape"])
        row["r_squared"] = float(row["r_squared"])
    return rows


def load_causes(path) -> list:
    """Load CASA miss-cause counts."""
    rows = _read(Path(path))
    for row in rows:
        for key in ("cold", "capacity", "conflict", "policy"):
            row[key] = int(row[key])
    return rows


def load_runtime(path) -> list:
    """Load successful runtime repetitions."""
    rows = _read(Path(path))
    return [{**row, "repetition": int(row["repetition"]),
             "seconds": float(row["seconds"]),
             "exit_code": int(row["exit_code"])}
            for row in rows if int(row["exit_code"]) == 0]


def metric_rows(rows: list, metric: str) -> list:
    """Select rows for one comparison metric."""
    return [row for row in rows if row["metric"] == metric]


def summary_row(rows: list, tier: str, metric: str) -> dict:
    """Return the aggregate row matching a tier and metric."""
    return next(row for row in rows
                if row["tier"] == tier and row["metric"] == metric)
