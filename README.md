# CASA

**Cache-Analysis-Framework-for-Staic-Access-Patterns (CASA)** — LLVM IR 기반
정적 캐시 분석 프레임워크.

[Access-Pattern-Extractor](https://github.com/OBC-SIM/Access-Pattern-Extractor)가 생성한
AP JSON을 입력으로 받아, 프로그램 실행 전에 캐시 동작을 시뮬레이션한다.

---

## 개요

```
C/C++ 소스 코드
  → Access-Pattern-Extractor  (frontend/ 서브모듈)
  → AP JSON
  → CASA
      ├─ AP Layer       : JSON 파싱 → AccessEvent 스트림 복원
      ├─ Memory Layer   : flat address 배치 → byte_address 계산
      ├─ Cache Layer    : YAML 설정형 multi-L1/L2/Memory 시뮬레이션
      ├─ Analysis Layer : cold/capacity/conflict miss 귀속 및 진단
      └─ Report Layer   : CSV / JSON / Markdown 출력
```

---

## 기능

- AP JSON → AccessEvent 선형 스트림 복원 (루프 언롤, call 인라이닝 + 인자 바인딩)
- YAML 기반 캐시 계층 설정 (`cache.yaml`) — private L1 × N + shared L2 + Memory
- LRU replacement, write-back / write-through, write-allocate / no-write-allocate 정책
- cold / capacity / conflict miss 분류
- L1/L2 hit율, cycle, write-through/writeback/dirty eviction 통계
- inclusive / exclusive Region 집계
- rule-based 최적화 진단 (padding, blocking, loop interchange 힌트)
- 출력: 입력명 기반 `<name>.csv`/`.json` (miss 분류 + L1/L2 hit율 + cycle/write traffic 통계), `<name>_diagnostics.md`, `<name>_objects.csv`

---

## 빌드

### 요구 사항

| 도구 | 최소 버전 |
|------|-----------|
| CMake | 3.20 |
| GCC / Clang | C++17 이상 |
| yaml-cpp | 0.7 |
| nlohmann/json | 3.10 |
| GTest | 1.11 |

Ubuntu 22.04 기준 의존성 설치:

```bash
sudo apt-get install -y \
    libyaml-cpp-dev \
    nlohmann-json3-dev \
    libgtest-dev
```

### 빌드 및 테스트

빌드는 용도에 따라 두 가지로 나뉜다. 개발·디버깅·테스트에는 기본 빌드(`-O0`,
assert 활성)를, 성능 측정이나 실사용에는 Release 빌드(`-O2`)를 쓴다. 빌드 타입은
빌드 디렉터리마다 고정되므로 두 빌드를 별도 디렉터리에 둔다 (Cargo의 dev/release처럼).

먼저 서브모듈을 초기화한다 (최초 1회).

```bash
git submodule update --init --recursive
```

개발 빌드 — `build/casa` 생성, `-O0`, assert 활성:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

Release 빌드 — `build-release/casa` 생성, `-O2 -DNDEBUG`:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
```

`-O3`가 필요하면 Release 설정에 `-DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"`를
덧붙인다.

---

## 사용법

```bash
# 단일 실행 (구현 완료) — 입력은 APE LAT v2 JSON
./build/casa run input_g_ape.json --cache cache.yaml [options]

# sweep: L1 용량 범위 탐색 (예정)
./build/casa sweep input_g_ape.json --cache cache.yaml --l1-sizes 4K,8K,16K,32K
```

`run`은 `--output`(기본 `results/`, 없으면 자동 생성)에 입력 파일명 기반으로
`<name>.csv`, `<name>.json`, `<name>_diagnostics.md`, `<name>_objects.csv`를 생성한다.
예를 들어 `examples/test_stencil_g_ape.json`은 `test_stencil_ape.*` 리포트로
저장된다.

### C 소스부터 실행

`casa run`은 APE JSON을 입력으로 받는다. C/LLVM IR 입력에서 APE JSON 생성과
cache simulation을 한 번에 실행하려면 루트의 `pipeline.py` wrapper를 사용한다.

```bash
python3 pipeline.py frontend/tasks/test_matmul.c \
  --cache settings/cache.yaml \
  --output results \
  --no-color
```

동작 흐름:

```text
C/LL -> clang-14 -> opt-14 APE JSON -> casa run -> reports
```

APE JSON까지만 생성하려면 `--ape-only`를 사용한다.

```bash
python3 pipeline.py frontend/tasks/test_matmul.c --ape-only
```

### Cachegrind 비교 실행

Valgrind Cachegrind와 비교하려면 먼저 같은 캐시 형상을 사용하는 CASA 설정을
사용한다. 예를 들어 `settings/cachegrind.yaml`은 D1 `32 KiB, 4-way, 32 B line`,
LL `2 MiB, 4-way, 32 B line`, write-allocate 정책을 맞추기 위한 비교용 설정이다.

전체 workload의 native compile, Cachegrind, CASA, 집계, figure 생성을 한 번에
재현하려면 다음 명령을 사용한다.

```bash
python3 comparisions/run_comparison.py all
```

단계별 재실행도 가능하다.

```bash
python3 comparisions/run_comparison.py compile
python3 comparisions/run_comparison.py cachegrind
python3 comparisions/run_comparison.py casa
python3 comparisions/run_comparison.py aggregate
python3 comparisions/run_comparison.py figures
```

`--workload test_1d`로 일부 workload만 선택할 수 있다. 실행 시간 측정은
`--repetitions 7`을 사용한다. Cachegrind는 native binary profiling을, CASA는
미리 생성된 AP JSON부터 cache simulation과 report 생성을 측정하며 각 단계의 값은
`comparisions/comparison-results/runtime.csv`에 저장된다.

`cg_annotate`의 주요 data-cache 열은 다음과 같이 해석한다.

| 열 | 의미 |
|----|------|
| `Dr` | data read references |
| `D1mr` | D1 read misses |
| `DLmr` | LL read misses |
| `Dw` | data write references |
| `D1mw` | D1 write misses |
| `DLmw` | LL write misses |

비교기는 manifest에 지정된 분석 kernel 함수만 raw 결과에서 집계한다. CASA L1
miss는 `D1mr + D1mw`, CASA L2 miss는 `DLmr + DLmw`와 비교한다. `Dr + Dw`는
`-O0`의 loop-variable spill과 stack 접근을 포함하므로 진단값으로만 사용한다.

단일 객체의 통제 workload는 `exact`, PolyBench와 multi-object workload는
`trend`로 구분한다. 후자는 native linker 주소와 CASA synthetic object 주소의
차이 때문에 절대값보다 경향을 중심으로 해석한다. 지원하지 않는 workload도
누락하지 않고 `comparison-results/status.csv`에 원인을 기록한다.

### CLI 옵션

| 옵션 | 설명 |
|------|------|
| `--cache <cache.yaml>` | 캐시 계층 설정 파일. `run`에서 필수 |
| `--output <dir>` | 리포트 출력 디렉터리. 기본값은 `results` |
| `--quiet` | 자동화용 최소 출력. 결과 디렉터리만 표시 |
| `--verbose` | 캐시 설정과 write traffic 요약까지 함께 출력 |
| `--no-color` | ANSI color 없이 plain text로 출력 |
| `-h`, `--help` | 도움말 출력 |

도움말은 아래 명령으로 확인할 수 있다.

```bash
./build/casa --help
./build/casa help
./build/casa run --help
```

### 설정 파일

**cache.yaml** — 캐시 계층 정의

```yaml
cores:
  count: 4
  mapping:
    - id: 0
      l1: L1D0

caches:
  - name: L1D0
    role: L1
    private_to: 0
    size_bytes: 32768
    line_size: 64
    associativity: 8
    replacement: LRU
    write_policy: write-back
    write_allocate: true
    delay_cycles: 4
    next: L2
  - name: L2
    role: LLC
    size_bytes: 262144
    line_size: 64
    associativity: 8
    replacement: LRU
    write_policy: write-back
    write_allocate: true
    delay_cycles: 12
    next: Memory

memory:
  name: Memory
  delay_cycles: 120
```

`write_policy`는 `write-back` 또는 `write-through`를 지원한다. `write_allocate`가
`false`인 store miss는 해당 계층에 cache line을 적재하지 않고 다음 계층으로
전달된다. 리포트의 `write_traffic`에는 `write_through_writes`, `writebacks`,
`dirty_evictions`, `writeback_cycles`가 포함된다.

> 배열 shape·elem_size·구조체 layout은 LAT v2 입력의 `metadata`가 제공하므로
> 별도 shapes.yaml은 필요 없다.

**mapping.yaml** — SMP 함수→core 정적 매핑 (선택)

```yaml
mappings:
  - function: kernel_2mm
    core: 0
  - function: init_tmp
    core: 1
```

### 리포트 시각화 (Python)

생성된 리포트를 플롯(PNG)으로 후처리한다. 저장소 루트에서 실행한다.

```bash
python3 backend/main.py results/
python3 backend/main.py results/test_matmul_ape.json
```

디렉터리를 입력하면 내부의 각 `<name>.json` + `<name>_objects.csv` 쌍마다
PNG 4종을 생성한다. 단일 JSON 파일을 입력하면 같은 basename의
`<name>_objects.csv`를 함께 읽는다.

출력 파일은 `<results>/plots/`에 `<name>_miss_breakdown.png`,
`<name>_object_misses.png`, `<name>_cache_hit_miss.png`,
`<name>_write_traffic.png` 형태로 저장된다.
의존성: `matplotlib`, `seaborn`.

---

## 구현 현황

| Phase | 내용 | 상태 |
|-------|------|------|
| 1 | CMake 골격 + 디렉터리 구조 | ✅ 완료 |
| 2 | AP Layer (ApLoader, EventBuilder) | ✅ 완료 |
| 3 | Memory Layer (MemoryLayout, AddressMapper) | ✅ 완료 |
| 4 | Cache Layer (YamlConfigParser, LRU 시뮬레이션) | ✅ 완료 |
| 5 | Analysis Layer (Attribution, MissClassifier, Diagnostics) | ✅ 완료 |
| 6 | Report Layer (CSV / JSON / Markdown) | ✅ 완료 |
| 7 | CLI (`run`) + 통합 테스트 | ✅ 완료 |
| 8 | Python 후처리·시각화 (`backend/`) | ✅ 완료 |
| 9 | LAT v2 입력 전환 (access_path, 구조체) | ✅ 완료 |

### 알려진 제약 (Known Limitations)

| 항목 | 현황 |
|------|------|
| `sweep` 모드 | 미구현 — `run`만 제공 |
| 멀티코어 | 설정은 가능하나 통합 검증은 단일 코어 기준 |

---

## 저장소 구조

```
CASA/
  CMakeLists.txt
  frontend/                    ← Access-Pattern-Extractor 서브모듈
  include/
    ap/                        ← ApNode, ApLoader(v2), ApProgram, AccessLayout,
                                 IndexExpr, AddressResolver, EventBuilder, AccessEvent
    memory/                    ← MemoryLayout, AddressMapper
    cache/                     ← CacheConfig, YamlConfigParser, CacheHierarchy
    analysis/                  ← Attribution, MissClassifier, Diagnostics
    report/                    ← CsvWriter, JsonWriter, MarkdownWriter
  src/                         ← 각 Layer 구현
  tests/                       ← GTest 단위 테스트
  examples/                    ← APE LAT v2 입력 샘플 (*_g_ape.json)
  settings/                    ← cache.yaml 등 실행 설정
  backend/                     ← Python 후처리·시각화 (Phase 8)
    reports.py                 ← 리포트 로딩·집계
    plotting/                  ← style, bars, figures
    main.py                    ← CLI (python3 backend/main.py)
    tests/                     ← pytest
  results/                     ← 출력 디렉터리 (gitignore)
```

---

## 라이선스

[MIT License](LICENSE) © 2026 OBC-SIM
