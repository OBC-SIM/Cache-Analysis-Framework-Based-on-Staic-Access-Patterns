"""reports.py — APEX-Cache 리포트 로딩·집계 (순수 함수).

`run`이 생성한 리포트(`<name>.json`, `<name>_objects.csv`)를 읽어
플롯에 쓸 형태로 가공한다. 렌더링과 분리되어 단위 테스트 가능하다.
"""

import csv
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Tuple

_INT_FIELDS = ("accesses", "hits", "misses", "load_misses", "store_misses")
_WRITE_TRAFFIC_FIELDS = (
    ("write-through", "write_through_writes"),
    ("writebacks", "writebacks"),
    ("dirty evict", "dirty_evictions"),
    ("wb cycles", "writeback_cycles"),
)


@dataclass(frozen=True)
class ReportPair:
    """summary JSON과 object CSV의 같은 basename 리포트 쌍."""

    summary: Path
    objects: Path
    stem: str


def find_reports(results_dir) -> Tuple[Path, Path]:
    """results 디렉터리에서 (summary.json, objects.csv) 경로를 찾는다.

    @param results_dir  `run --output`이 가리킨 디렉터리.
    @return (summary_json_path, objects_csv_path)
    @raises FileNotFoundError  둘 중 하나라도 없을 때.
    """
    pair = find_report_pairs(results_dir)[0]
    return pair.summary, pair.objects


def find_report_pairs(input_path) -> list:
    """summary JSON 파일 또는 디렉터리에서 basename이 맞는 리포트 쌍을 찾는다.

    @param input_path  `<name>.json` 파일 또는 리포트 디렉터리.
    @return ReportPair 목록. 디렉터리는 이름순으로 정렬한다.
    @raises FileNotFoundError  대응하는 `<name>_objects.csv`가 없을 때.
    """
    path = Path(input_path)
    if path.is_file():
        return [_pair_for_summary(path)]
    if path.is_dir():
        pairs = []
        for summary in sorted(path.glob("*.json")):
            objects = summary.with_name(f"{summary.stem}_objects.csv")
            if objects.exists():
                pairs.append(ReportPair(summary, objects, summary.stem))
        if pairs:
            return pairs
    raise FileNotFoundError(
        f"리포트를 찾을 수 없음: {path} (<name>.json + <name>_objects.csv 필요)")


def _pair_for_summary(summary: Path) -> ReportPair:
    objects = summary.with_name(f"{summary.stem}_objects.csv")
    if not objects.exists():
        raise FileNotFoundError(
            f"object 리포트를 찾을 수 없음: {objects}")
    return ReportPair(summary, objects, summary.stem)


def load_summary(path) -> dict:
    """summary `<name>.json`을 dict로 로드한다."""
    return json.loads(Path(path).read_text())


def load_objects(path) -> list:
    """`<name>_objects.csv`를 행 dict 목록으로 로드한다(숫자 필드 형변환)."""
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            row = {"object": r["object"], "miss_rate": float(r["miss_rate"])}
            for k in _INT_FIELDS:
                row[k] = int(r[k])
            rows.append(row)
    return rows


def miss_breakdown(summary: dict) -> dict:
    """summary에서 cold/capacity/conflict miss 수를 추출한다."""
    return {k: summary[k] for k in ("cold", "capacity", "conflict")}


def hit_miss_rates(summary: dict) -> dict:
    """L1/L2의 hit·miss 비율을 반환한다 (miss = 1 - hit)."""
    levels = summary["levels"]
    out = {}
    for name in ("L1", "L2"):
        hit = levels[name]["hit_rate"]
        out[name] = {"hit": hit, "miss": 1.0 - hit}
    return out


def write_traffic(summary: dict) -> dict:
    """write-side traffic counters를 plotting label 기준으로 반환한다."""
    traffic = summary.get("write_traffic", {})
    return {label: int(traffic.get(key, 0))
            for label, key in _WRITE_TRAFFIC_FIELDS}


def top_objects_by_misses(objects: list, n: int = 10) -> list:
    """miss 수 내림차순 상위 n개 객체를 반환한다."""
    return sorted(objects, key=lambda o: o["misses"], reverse=True)[:n]
