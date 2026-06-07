"""figures.py — APEX-Cache 리포트 figure 빌더.

각 함수는 matplotlib Figure를 만들어 반환한다(저장은 호출자가).
값 격차가 크면 broken y-axis로 작은 값과 큰 값을 함께 보여준다.
"""

from backend import reports
from backend.plotting import bars, style

_MISS_COLORS = {"cold": "#55A868", "capacity": "#4C72B0", "conflict": "#C44E52"}
_WRITE_COLORS = ("#4C72B0", "#DD8452", "#55A868", "#8172B3")


def figure_miss_breakdown(summary: dict):
    """cold/capacity/conflict miss 수 막대 figure를 만든다."""
    import matplotlib.pyplot as plt

    bd = reports.miss_breakdown(summary)
    labels = list(bd)
    values = [bd[k] for k in labels]
    fig, ax = plt.subplots(figsize=(4.5, 3.2))
    rects = ax.bar(range(len(labels)), values,
                   color=[_MISS_COLORS[k] for k in labels],
                   edgecolor="black", linewidth=0.5, width=0.7)
    ax.set_ylim(0, max(values + [1]) * 1.18)
    bars.label_bars(ax, rects, values)
    ax.set_ylabel("Misses", labelpad=8)
    ax.set_xlabel("Miss type")
    ax.set_title("Miss breakdown by type", fontsize=13, pad=14)
    bars.style_bars(ax, labels)
    return fig


def _grouped_figure(labels, series, *, title, ylabel, xlabel,
                    rotate=False, ymax=None, percent=False):
    """2계열 그룹 막대 figure를 만든다.

    값의 scale 격차가 크면 y축을 broken axis(상=큰 값, 하=작은 값)로 분리한다.

    @param ymax     상단 y한계. None이면 max(counts)*1.18.
    @param percent  y눈금을 백분율(%)로 표기.
    """
    import matplotlib.pyplot as plt
    import seaborn as sns
    from matplotlib.ticker import PercentFormatter

    counts = series[0][0] + series[1][0]
    top_lim = ymax if ymax is not None else max(counts + [1]) * 1.18
    limits = bars.broken_axis_limits(counts)
    vfmt = (lambda v: f"{v * 100:.1f}%") if percent else str

    def fmt(ax):
        if percent:
            ax.yaxis.set_major_formatter(PercentFormatter(xmax=1.0))

    if limits is None:
        fig, ax = plt.subplots(figsize=(4.5, 3.4))
        rects = bars.grouped_bars(ax, labels, series)
        ax.set_ylim(0, top_lim)
        fmt(ax)
        bars.label_bars(ax, rects, counts, fmt=vfmt)
        ax.set_ylabel(ylabel, labelpad=8)
        ax.set_xlabel(xlabel)
        ax.set_title(title, fontsize=13, pad=14)
        ax.legend(loc="upper left", bbox_to_anchor=(1.02, 1.0),
                  frameon=False, fontsize=9)
        bars.style_bars(ax, labels, rotate=rotate)
        fig.subplots_adjust(left=0.15, right=0.78, bottom=0.30, top=0.84)
        return fig

    low_max, high_min = limits
    fig = plt.figure(figsize=(4.5, 3.8))
    gs = fig.add_gridspec(2, 1, height_ratios=[3, 1], hspace=0.0)
    ax_top = fig.add_subplot(gs[0])
    ax_bot = fig.add_subplot(gs[1])
    rects_top = bars.grouped_bars(ax_top, labels, series)
    rects_bot = bars.grouped_bars(ax_bot, labels, series)
    ax_top.set_ylim(high_min, top_lim)
    ax_bot.set_ylim(0, low_max)
    fmt(ax_top)
    fmt(ax_bot)
    bars.label_bars(ax_top, rects_top, counts, fmt=vfmt, ylim_min=high_min)
    bars.label_bars(ax_bot, rects_bot, counts, fmt=vfmt, inside=True)
    ax_top.tick_params(labelbottom=False, bottom=False)
    ax_top.set_ylabel(ylabel, labelpad=12)
    ax_top.set_title(title, fontsize=13, pad=14)
    ax_top.legend(loc="upper left", bbox_to_anchor=(1.02, 1.0),
                  frameon=False, fontsize=9)
    bars.style_bars(ax_bot, labels, rotate=rotate)
    ax_bot.set_xlabel(xlabel)
    sns.despine(ax=ax_top)
    bars.add_break_marks(ax_top, ax_bot)
    fig.subplots_adjust(left=0.15, right=0.78, bottom=0.30, top=0.84, hspace=0.0)
    bars.add_break_band(ax_top, ax_bot)
    return fig


def figure_object_misses(objects: list, top_n: int = 10):
    """객체별 load/store miss(상위 N) figure. 격차 크면 broken axis."""
    top = reports.top_objects_by_misses(objects, top_n)
    labels = [o["object"] for o in top]
    series = [
        ([o["load_misses"] for o in top], "load", style.PRIMARY),
        ([o["store_misses"] for o in top], "store", style.SECONDARY),
    ]
    return _grouped_figure(labels, series, rotate=True, ylabel="Misses",
                           xlabel="Object",
                           title=f"Per-object miss (top {len(labels)})")


def figure_hit_miss(summary: dict):
    """계층별 hit/miss 비율 figure (hit=빨강, miss=파랑). 격차 크면 broken axis."""
    hm = reports.hit_miss_rates(summary)
    labels = list(hm)
    series = [
        ([hm[k]["hit"] for k in labels], "hit", style.HIT_COLOR),
        ([hm[k]["miss"] for k in labels], "miss", style.MISS_COLOR),
    ]
    return _grouped_figure(labels, series, ylabel="Ratio", xlabel="Cache level",
                           title="Cache hit/miss rate", ymax=1.0, percent=True)


def figure_write_traffic(summary: dict):
    """write-through/writeback traffic figure를 만든다."""
    import matplotlib.pyplot as plt

    traffic = reports.write_traffic(summary)
    labels = list(traffic)
    values = [traffic[k] for k in labels]
    top = max(values + [1])
    fig, ax = plt.subplots(figsize=(4.8, 3.4))
    rects = ax.bar(range(len(labels)), values, color=_WRITE_COLORS,
                   edgecolor="black", linewidth=0.5, width=0.68)
    ax.set_ylim(0, top * 1.18)
    for rect, value in zip(rects, values):
        cx = rect.get_x() + rect.get_width() / 2
        ax.text(cx, rect.get_height() + top * 0.02, str(value),
                ha="center", va="bottom", fontsize=7, clip_on=False)
    ax.set_ylabel("Count / cycles", labelpad=8)
    ax.set_xlabel("Write event")
    ax.set_title("Write traffic", fontsize=13, pad=14)
    bars.style_bars(ax, labels, rotate=True)
    fig.subplots_adjust(left=0.16, right=0.97, bottom=0.30, top=0.84)
    return fig
