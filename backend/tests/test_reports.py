"""reports 집계 로직 단위 테스트 (pytest).

리포트 파싱·정렬·추출은 순수 로직이므로 TDD로 검증한다.
각 테스트는 하나의 동작만 확인한다.

실행: 저장소 루트에서 `python3 -m pytest backend/tests`.
"""

import json

from backend import reports


def _write_objects_csv(path):
    path.write_text(
        "object,accesses,hits,misses,miss_rate,load_accesses,store_accesses,load_misses,store_misses\n"
        "global::C,160000,147767,12233,0.0764563,160000,0,12233,0\n"
        "global::B,140000,138131,1869,0.01335,140000,0,1869,0\n"
        "global::D,323200,322400,800,0.00247525,160000,163200,0,800\n"
    )


def _write_summary_json(path):
    path.write_text(json.dumps({
        "cold": 3875, "capacity": 11733, "conflict": 994,
        "accesses": {"total": 1205200, "load": 900000, "store": 305200},
        "levels": {
            "L1": {"hits": 1188598, "misses": 16602, "hit_rate": 0.986},
            "L2": {"hits": 12727, "misses": 3875, "hit_rate": 0.767},
            "Memory": {"accesses": 3875},
        },
        "cycles": {"total": 5485024, "average_per_access": 4.551},
        "write_traffic": {
            "write_through_writes": 305200,
            "writebacks": 12,
            "dirty_evictions": 12,
            "writeback_cycles": 480,
        },
    }))


def test_load_objects_parses_numeric_fields(tmp_path):
    p = tmp_path / "x_objects.csv"
    _write_objects_csv(p)
    rows = reports.load_objects(p)
    assert rows[0]["object"] == "global::C"
    assert rows[0]["misses"] == 12233  # int
    assert rows[0]["store_misses"] == 0
    assert abs(rows[2]["miss_rate"] - 0.00247525) < 1e-9  # float


def test_top_objects_sorted_by_misses_desc(tmp_path):
    p = tmp_path / "x_objects.csv"
    _write_objects_csv(p)
    rows = reports.load_objects(p)
    top = reports.top_objects_by_misses(rows, n=2)
    assert [o["object"] for o in top] == ["global::C", "global::B"]  # 12233 > 1869 > 800


def test_top_objects_limits_to_n(tmp_path):
    p = tmp_path / "x_objects.csv"
    _write_objects_csv(p)
    rows = reports.load_objects(p)
    assert len(reports.top_objects_by_misses(rows, n=2)) == 2


def test_miss_breakdown_extracts_three_types(tmp_path):
    p = tmp_path / "x.json"
    _write_summary_json(p)
    s = reports.load_summary(p)
    assert reports.miss_breakdown(s) == {"cold": 3875, "capacity": 11733, "conflict": 994}


def test_hit_miss_rates_splits_each_level(tmp_path):
    p = tmp_path / "x.json"
    _write_summary_json(p)
    s = reports.load_summary(p)
    hm = reports.hit_miss_rates(s)
    assert hm["L1"]["hit"] == 0.986
    assert abs(hm["L1"]["miss"] - 0.014) < 1e-9  # 1 - hit
    assert abs(hm["L2"]["miss"] - 0.233) < 1e-9


def test_write_traffic_extracts_summary_values(tmp_path):
    p = tmp_path / "x.json"
    _write_summary_json(p)
    s = reports.load_summary(p)
    assert reports.write_traffic(s) == {
        "write-through": 305200,
        "writebacks": 12,
        "dirty evict": 12,
        "wb cycles": 480,
    }


def test_write_traffic_defaults_missing_fields_to_zero():
    assert reports.write_traffic({}) == {
        "write-through": 0,
        "writebacks": 0,
        "dirty evict": 0,
        "wb cycles": 0,
    }


def test_find_reports_locates_summary_and_objects(tmp_path):
    _write_summary_json(tmp_path / "bench_ape.json")
    _write_objects_csv(tmp_path / "bench_ape_objects.csv")
    summary, objects = reports.find_reports(tmp_path)
    assert summary.name == "bench_ape.json"
    assert objects.name == "bench_ape_objects.csv"


def test_find_report_pairs_from_summary_file(tmp_path):
    _write_summary_json(tmp_path / "bench_ape.json")
    _write_objects_csv(tmp_path / "bench_ape_objects.csv")
    pairs = reports.find_report_pairs(tmp_path / "bench_ape.json")
    assert [(p.summary.name, p.objects.name, p.stem) for p in pairs] == [
        ("bench_ape.json", "bench_ape_objects.csv", "bench_ape")
    ]


def test_find_report_pairs_from_directory_matches_each_basename(tmp_path):
    _write_summary_json(tmp_path / "a_ape.json")
    _write_objects_csv(tmp_path / "a_ape_objects.csv")
    _write_summary_json(tmp_path / "b_ape.json")
    _write_objects_csv(tmp_path / "b_ape_objects.csv")
    _write_summary_json(tmp_path / "summary.json")

    pairs = reports.find_report_pairs(tmp_path)

    assert [(p.summary.name, p.objects.name, p.stem) for p in pairs] == [
        ("a_ape.json", "a_ape_objects.csv", "a_ape"),
        ("b_ape.json", "b_ape_objects.csv", "b_ape"),
    ]


def test_find_report_pairs_raises_when_summary_file_lacks_objects(tmp_path):
    import pytest
    _write_summary_json(tmp_path / "bench_ape.json")

    with pytest.raises(FileNotFoundError):
        reports.find_report_pairs(tmp_path / "bench_ape.json")


def test_find_reports_raises_when_missing(tmp_path):
    import pytest
    with pytest.raises(FileNotFoundError):
        reports.find_reports(tmp_path)
