"""Run the full CASA source-to-report pipeline.

Usage:
    python3 pipeline.py [options] FILE [FILE ...]

The wrapper accepts C or LLVM IR input, emits APE JSON with the frontend
LLVM pass, then runs `casa run` to generate cache reports.
"""

import argparse
import subprocess
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parent
_PLUGIN_CANDIDATES = (
    _ROOT / "frontend" / "build" / "libLoopAnnotatedTrace.so",
    _ROOT / "build" / "libLoopAnnotatedTrace.so",
)
_DEFAULT_CASA = _ROOT / "build" / "casa"
_DEFAULT_CACHE = _ROOT / "settings" / "cache.yaml"
_DEFAULT_OUTPUT = _ROOT / "results"
_DEFAULT_INCLUDE = _ROOT / "frontend" / "tasks"


def _default_plugin() -> Path:
    for candidate in _PLUGIN_CANDIDATES:
        if candidate.exists():
            return candidate
    return _PLUGIN_CANDIDATES[0]


def compile_c(c_path: Path, include_dir: Path) -> Path:
    abs_c = c_path.resolve()
    out_ll = abs_c.parent / f"{abs_c.stem}_g.ll"
    subprocess.run(
        [
            "clang-14",
            "-O0",
            "-Xclang",
            "-disable-O0-optnone",
            "-g",
            "-I",
            str(include_dir.resolve()),
            "-emit-llvm",
            "-S",
            "-o",
            str(out_ll),
            str(abs_c),
        ],
        check=True,
        stderr=subprocess.PIPE,
        cwd=abs_c.parent,
    )
    return out_ll


def to_ll(path: Path, include_dir: Path, total_steps: int) -> Path:
    if path.suffix == ".c":
        print(f"  [1/{total_steps}] clang-14...", end=" ", flush=True)
        ll_path = compile_c(path, include_dir)
        print(f"done -> {ll_path}")
        return ll_path
    return path.resolve()


def _json_candidates(ll_path: Path) -> list:
    stem_path = ll_path.with_suffix("")
    return [
        stem_path.parent / f"{stem_path.name}_ape.json",
        stem_path.parent / f"{stem_path.name}_lat.json",
    ]


def run_ape(ll_path: Path, plugin_path: Path) -> Path:
    abs_ll = ll_path.resolve()
    subprocess.run(
        [
            "opt-14",
            f"-load-pass-plugin={plugin_path.resolve()}",
            "-passes=function(mem2reg),loop-simplify,loop-annotated-trace",
            str(abs_ll),
            "-o",
            "/dev/null",
        ],
        check=True,
        stderr=subprocess.PIPE,
        cwd=abs_ll.parent,
    )
    for candidate in _json_candidates(abs_ll):
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"APE JSON was not generated for {abs_ll}")


def run_casa(json_path: Path, casa_bin: Path, cache_yaml: Path,
                   output_dir: Path, verbose: bool, no_color: bool) -> None:
    cmd = [
        str(casa_bin.resolve()),
        "run",
        str(json_path.resolve()),
        "--cache",
        str(cache_yaml.resolve()),
        "--output",
        str(output_dir.resolve()),
    ]
    if verbose:
        cmd.append("--verbose")
    if no_color:
        cmd.append("--no-color")
    subprocess.run(cmd, check=True, cwd=_ROOT)


def _check_file(path: Path, label: str) -> None:
    if not path.exists():
        raise FileNotFoundError(f"{label} not found: {path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="C/LLVM IR -> APE JSON -> CASA reports"
    )
    parser.add_argument("files", nargs="+", metavar="FILE",
                        help=".c or .ll input file")
    parser.add_argument("--plugin", default=str(_default_plugin()),
                        metavar="PATH", help="LLVM pass plugin path")
    parser.add_argument("--casa", default=str(_DEFAULT_CASA),
                        metavar="PATH", help="casa binary path")
    parser.add_argument("--cache", default=str(_DEFAULT_CACHE),
                        metavar="PATH", help="cache.yaml path")
    parser.add_argument("--output", default=str(_DEFAULT_OUTPUT),
                        metavar="DIR", help="report output directory")
    parser.add_argument("--include", default=str(_DEFAULT_INCLUDE),
                        metavar="DIR", help="C include directory")
    parser.add_argument("--ape-only", action="store_true",
                        help="stop after generating APE JSON")
    parser.add_argument("--verbose", action="store_true",
                        help="forward --verbose to casa run")
    parser.add_argument("--no-color", action="store_true",
                        help="forward --no-color to casa run")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    plugin = Path(args.plugin)
    casa_bin = Path(args.casa)
    cache_yaml = Path(args.cache)
    output_dir = Path(args.output)
    include_dir = Path(args.include)

    try:
        _check_file(plugin, "plugin")
        _check_file(include_dir, "include directory")
        if not args.ape_only:
            _check_file(casa_bin, "casa binary")
            _check_file(cache_yaml, "cache config")
    except FileNotFoundError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    had_error = False
    for file_str in args.files:
        source = Path(file_str)
        if not source.exists() or source.suffix not in (".c", ".ll"):
            print(f"error: unsupported or missing input: {source}", file=sys.stderr)
            had_error = True
            continue
        try:
            total_steps = 2 if args.ape_only else 3
            print(f"Pipeline: {source}")
            ll_path = to_ll(source, include_dir, total_steps)
            print(f"  [2/{total_steps}] opt-14...", end=" ", flush=True)
            json_path = run_ape(ll_path, plugin)
            print(f"done -> {json_path}", flush=True)
            if not args.ape_only:
                print(f"  [3/{total_steps}] casa...", flush=True)
                run_casa(json_path, casa_bin, cache_yaml, output_dir,
                               args.verbose, args.no_color)
                print(f"Reports -> {output_dir.resolve()}")
        except subprocess.CalledProcessError as exc:
            stderr = exc.stderr.decode(errors="replace") if exc.stderr else ""
            failed = exc.cmd[0] if isinstance(exc.cmd, list) else exc.cmd
            print(f"error: {failed} failed\n{stderr}", file=sys.stderr)
            had_error = True
        except Exception as exc:
            print(f"error: {exc}", file=sys.stderr)
            had_error = True
    return 1 if had_error else 0


if __name__ == "__main__":
    raise SystemExit(main())
