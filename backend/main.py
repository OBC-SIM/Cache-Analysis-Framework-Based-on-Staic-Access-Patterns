"""CASA 리포트 시각화 CLI.

`run --output <dir>`이 만든 리포트를 읽어 PNG 4종을 생성한다:
  miss_breakdown.png  — cold/capacity/conflict miss 수
  object_misses.png   — 객체별 load/store miss (상위 N; 격차 크면 broken axis)
  cache_hit_miss.png  — L1/L2 hit/miss 비율 (격차 크면 broken axis)
  write_traffic.png   — write-through/writeback/dirty eviction 통계

사용법 (저장소 루트에서):
  python3 backend/main.py results/ [--output results/plots] [--top 10]
  python3 backend/main.py results/test_matmul_ape.json [--top 10]
"""

import argparse
import os
import sys
from pathlib import Path

# 파일을 직접 실행할 때 `backend` 패키지를 import할 수 있도록 레포 루트를 경로에 추가.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from backend import reports  # noqa: E402
from backend.plotting import figures, style  # noqa: E402


def generate(input_path, output_dir=None, top_n: int = 10) -> list:
    """리포트를 읽어 PNG 4종을 생성하고 저장 경로 목록을 반환한다."""
    os.environ.setdefault("MPLBACKEND", "Agg")
    os.environ.setdefault("MPLCONFIGDIR", "/tmp/casa-matplotlib")
    Path(os.environ["MPLCONFIGDIR"]).mkdir(parents=True, exist_ok=True)
    import matplotlib.pyplot as plt

    source = Path(input_path)
    out_base = source if source.is_dir() else source.parent
    out = Path(output_dir) if output_dir else out_base / "plots"
    out.mkdir(parents=True, exist_ok=True)
    style.setup_theme()

    saved = []
    for pair in reports.find_report_pairs(source):
        summary = reports.load_summary(pair.summary)
        objects = reports.load_objects(pair.objects)
        plots = (
            ("miss_breakdown", lambda: figures.figure_miss_breakdown(summary)),
            ("object_misses",
             lambda: figures.figure_object_misses(objects, top_n)),
            ("cache_hit_miss", lambda: figures.figure_hit_miss(summary)),
            ("write_traffic", lambda: figures.figure_write_traffic(summary)),
        )
        for name, make_fig in plots:
            fig = make_fig()
            path = out / f"{pair.stem}_{name}.png"
            style.save_figure(fig, path)
            plt.close(fig)
            saved.append(path)
    return saved


def main() -> None:
    p = argparse.ArgumentParser(description="CASA 리포트 시각화")
    p.add_argument("input",
                   help="summary JSON 파일 또는 run --output 리포트 디렉터리")
    p.add_argument("--output", default=None,
                   help="플롯 출력 디렉터리 (기본: <input-dir>/plots)")
    p.add_argument("--top", type=int, default=10,
                   help="object_misses에 표시할 객체 수 (기본 10)")
    args = p.parse_args()
    for path in generate(args.input, args.output, args.top):
        print(f"  플롯 저장 → {path}")


if __name__ == "__main__":
    main()
