"""backend/main.py 플롯 생성 흐름 테스트."""

import json

from backend import main as backend_main


def _write_summary_json(path):
    path.write_text(json.dumps({
        "cold": 10,
        "capacity": 2,
        "conflict": 1,
        "accesses": {"total": 100, "load": 80, "store": 20},
        "levels": {
            "L1": {"hits": 90, "misses": 10, "hit_rate": 0.9},
            "L2": {"hits": 8, "misses": 2, "hit_rate": 0.8},
            "Memory": {"accesses": 2},
        },
        "cycles": {"total": 200, "average_per_access": 2.0},
        "write_traffic": {
            "write_through_writes": 20,
            "writebacks": 1,
            "dirty_evictions": 1,
            "writeback_cycles": 40,
        },
    }))


def _write_objects_csv(path):
    path.write_text(
        "object,accesses,hits,misses,miss_rate,load_accesses,store_accesses,load_misses,store_misses\n"
        "global::A,100,90,10,0.1,80,20,8,2\n"
    )


def test_generate_writes_four_prefixed_plots_for_summary_file(tmp_path):
    _write_summary_json(tmp_path / "bench_ape.json")
    _write_objects_csv(tmp_path / "bench_ape_objects.csv")
    out = tmp_path / "plots"

    saved = backend_main.generate(tmp_path / "bench_ape.json", out)

    assert [p.name for p in saved] == [
        "bench_ape_miss_breakdown.png",
        "bench_ape_object_misses.png",
        "bench_ape_cache_hit_miss.png",
        "bench_ape_write_traffic.png",
    ]
    assert all(p.exists() for p in saved)


def test_generate_writes_prefixed_plots_for_each_report_pair(tmp_path):
    for stem in ("a_ape", "b_ape"):
        _write_summary_json(tmp_path / f"{stem}.json")
        _write_objects_csv(tmp_path / f"{stem}_objects.csv")
    out = tmp_path / "plots"

    saved = backend_main.generate(tmp_path, out)

    assert [p.name for p in saved] == [
        "a_ape_miss_breakdown.png",
        "a_ape_object_misses.png",
        "a_ape_cache_hit_miss.png",
        "a_ape_write_traffic.png",
        "b_ape_miss_breakdown.png",
        "b_ape_object_misses.png",
        "b_ape_cache_hit_miss.png",
        "b_ape_write_traffic.png",
    ]
    assert all(p.exists() for p in saved)
