"""APEX-Cache 리포트 시각화 CLI.

`run --output <dir>`이 만든 리포트를 읽어 PNG 3종을 생성한다:
  miss_breakdown.png  — cold/capacity/conflict miss 수
  object_misses.png   — 객체별 load/store miss (상위 N; 격차 크면 broken axis)
  cache_hit_miss.png  — L1/L2 hit/miss 비율 (격차 크면 broken axis)

사용법 (저장소 루트에서):
  python3 backend/main.py --results results/ [--output results/plots] [--top 10]
"""

import argparse
import os
import sys
from pathlib import Path

# 파일을 직접 실행할 때 `backend` 패키지를 import할 수 있도록 레포 루트를 경로에 추가.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from backend import reports  # noqa: E402
from backend.plotting import figures, style  # noqa: E402


def generate(results_dir, output_dir=None, top_n: int = 10) -> list:
    """리포트를 읽어 PNG 3종을 생성하고 저장 경로 목록을 반환한다."""
    os.environ.setdefault("MPLBACKEND", "Agg")
    import matplotlib.pyplot as plt

    summary_path, objects_path = reports.find_reports(results_dir)
    summary = reports.load_summary(summary_path)
    objects = reports.load_objects(objects_path)

    out = Path(output_dir) if output_dir else Path(results_dir) / "plots"
    out.mkdir(parents=True, exist_ok=True)
    style.setup_theme()

    plots = (
        ("miss_breakdown", lambda: figures.figure_miss_breakdown(summary)),
        ("object_misses", lambda: figures.figure_object_misses(objects, top_n)),
        ("cache_hit_miss", lambda: figures.figure_hit_miss(summary)),
    )

    saved = []
    for name, make_fig in plots:
        fig = make_fig()
        path = out / f"{name}.png"
        style.save_figure(fig, path)
        plt.close(fig)
        saved.append(path)
    return saved


def main() -> None:
    p = argparse.ArgumentParser(description="APEX-Cache 리포트 시각화")
    p.add_argument("--results", required=True,
                   help="run --output이 가리킨 리포트 디렉터리")
    p.add_argument("--output", default=None,
                   help="플롯 출력 디렉터리 (기본: <results>/plots)")
    p.add_argument("--top", type=int, default=10,
                   help="object_misses에 표시할 객체 수 (기본 10)")
    args = p.parse_args()
    for path in generate(args.results, args.output, args.top):
        print(f"  플롯 저장 → {path}")


if __name__ == "__main__":
    main()
