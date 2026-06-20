"""Paper figures for CASA versus Cachegrind validation."""

from backend import comparison_reports

TIER_STYLE = {
    "exact": ("#4C72B0", "o", "controlled"),
    "trend": ("#DD8452", "s", "layout-sensitive"),
}
CAUSE_COLORS = {
    "cold": "#55A868", "capacity": "#4C72B0",
    "conflict": "#C44E52", "policy": "#8172B3",
}


def validation_scatter(rows: list, summary: list, metric: str, title: str):
    """Create a log-log CASA versus Cachegrind validation scatter."""
    import matplotlib.pyplot as plt

    selected = comparison_reports.metric_rows(rows, metric)
    fig, ax = plt.subplots(figsize=(4.5, 3.4))
    positive = [row for row in selected
                if row["casa_value"] > 0 and row["cachegrind_value"] > 0]
    for tier, (color, marker, label) in TIER_STYLE.items():
        tier_rows = [row for row in positive if row["tier"] == tier]
        ax.scatter([row["cachegrind_value"] for row in tier_rows],
                   [row["casa_value"] for row in tier_rows],
                   color=color, marker=marker, label=label, s=34,
                   edgecolor="black", linewidth=0.45, alpha=0.9)
    values = [value for row in positive for value in
              (row["casa_value"], row["cachegrind_value"])]
    low, high = min(values) * 0.75, max(values) * 1.35
    ax.plot([low, high], [low, high], color="black", linewidth=0.9,
            linestyle="--", label="ideal")
    stats = comparison_reports.summary_row(summary, "all", metric)
    ax.text(0.04, 0.96, f"$R^2$ = {stats['r_squared']:.3f}\n"
            f"MAPE = {stats['mape'] * 100:.1f}%\n$n$ = {stats['n']}",
            transform=ax.transAxes, ha="left", va="top", fontsize=8)
    ax.set(xscale="log", yscale="log", xlim=(low, high), ylim=(low, high),
           xlabel="Cachegrind misses", ylabel="CASA misses", title=title)
    ax.legend(loc="lower right", frameon=False, fontsize=8)
    return fig


def relative_error_bars(rows: list):
    """Create workload-level grouped relative-error bars for L1 and LL."""
    import matplotlib.pyplot as plt
    import numpy as np

    l1 = {row["workload"]: row for row in comparison_reports.metric_rows(rows, "l1_misses")}
    ll = {row["workload"]: row for row in comparison_reports.metric_rows(rows, "ll_misses")}
    labels = list(l1)
    x = np.arange(len(labels))
    fig, ax = plt.subplots(figsize=(8.2, 3.7))
    ax.bar(x - 0.19, [l1[name]["relative_error"] * 100 for name in labels],
           0.38, label="L1", color="#4C72B0", edgecolor="black", linewidth=0.4)
    ax.bar(x + 0.19, [ll[name]["relative_error"] * 100 for name in labels],
           0.38, label="LL", color="#DD8452", edgecolor="black", linewidth=0.4)
    ax.set_xticks(x, labels, rotation=55, ha="right", fontsize=7)
    ax.set_ylabel("Relative error (%)")
    ax.set_title("Validation error by workload")
    ax.legend(frameon=False)
    fig.subplots_adjust(bottom=0.34, left=0.09, right=0.98, top=0.88)
    return fig


def cause_breakdown(rows: list):
    """Create a normalized stacked bar chart of CASA miss causes."""
    import matplotlib.pyplot as plt
    import numpy as np

    visible = [row for row in rows if sum(row[key] for key in CAUSE_COLORS) > 0]
    labels = [row["workload"] for row in visible]
    totals = [sum(row[key] for key in CAUSE_COLORS) for row in visible]
    x, bottom = np.arange(len(labels)), np.zeros(len(labels))
    fig, ax = plt.subplots(figsize=(8.2, 3.7))
    for cause, color in CAUSE_COLORS.items():
        values = np.array([row[cause] / total * 100
                           for row, total in zip(visible, totals)])
        ax.bar(x, values, bottom=bottom, label=cause, color=color,
               edgecolor="black", linewidth=0.35, width=0.75)
        bottom += values
    ax.set_xticks(x, labels, rotation=55, ha="right", fontsize=7)
    ax.set_ylim(0, 100)
    ax.set_ylabel("Share of L1 misses (%)")
    ax.set_title("CASA miss-cause attribution")
    ax.legend(loc="upper left", bbox_to_anchor=(1.005, 1), frameon=False)
    fig.subplots_adjust(bottom=0.34, left=0.09, right=0.86, top=0.88)
    return fig


def optimization_effect(rows: list):
    """Compare ordinary and cache-friendly matrix multiplication misses."""
    import matplotlib.pyplot as plt
    import numpy as np

    names = ("test_matmul", "test_matmul_cache_friendly")
    l1 = {row["workload"]: row for row in comparison_reports.metric_rows(rows, "l1_misses")}
    x = np.arange(2)
    fig, ax = plt.subplots(figsize=(4.5, 3.3))
    ax.bar(x - 0.19, [l1[name]["cachegrind_value"] for name in names], 0.38,
           label="Cachegrind", color="#DD8452", edgecolor="black", linewidth=0.5)
    ax.bar(x + 0.19, [l1[name]["casa_value"] for name in names], 0.38,
           label="CASA", color="#4C72B0", edgecolor="black", linewidth=0.5)
    ax.set_xticks(x, ("baseline", "cache-friendly"))
    ax.set_ylabel("L1 misses")
    ax.set_title("Matrix-multiplication optimization effect")
    ax.legend(frameon=False)
    return fig


def runtime_comparison(rows: list):
    """Create PolyBench median/IQR runtime bars for both analysis tools."""
    import matplotlib.pyplot as plt
    import numpy as np

    names = sorted({row["workload"] for row in rows
                    if row["workload"].startswith("polybench_")})
    stages = (("cachegrind", "Cachegrind profiling", "#DD8452"),
              ("casa_simulation", "CASA simulation", "#4C72B0"))
    x = np.arange(len(names))
    fig, ax = plt.subplots(figsize=(6.4, 3.6))
    for offset, (stage, label, color) in zip((-0.19, 0.19), stages):
        samples = [[row["seconds"] for row in rows
                    if row["workload"] == name and row["stage"] == stage]
                   for name in names]
        medians = np.array([np.median(values) for values in samples])
        lower = medians - np.array([np.percentile(values, 25) for values in samples])
        upper = np.array([np.percentile(values, 75) for values in samples]) - medians
        ax.bar(x + offset, medians, 0.38, yerr=(lower, upper), label=label,
               color=color, edgecolor="black", linewidth=0.45, capsize=2)
    ax.set_xticks(x, [name.removeprefix("polybench_") for name in names])
    ax.set_ylabel("Runtime (s), median and IQR")
    ax.set_title("Observed analysis runtime")
    ax.legend(frameon=False, fontsize=8)
    return fig
