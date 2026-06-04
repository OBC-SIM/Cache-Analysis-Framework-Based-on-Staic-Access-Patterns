# APEX-Cache

**Access Pattern Executor for Cache** — LLVM IR 기반 정적 캐시 분석 도구.

[Access-Pattern-Extractor](https://github.com/OBC-SIM/Access-Pattern-Extractor)가 생성한
AP JSON을 입력으로 받아, 프로그램 실행 전에 캐시 동작을 시뮬레이션한다.

---

## 개요

```
C/C++ 소스 코드
  → Access-Pattern-Extractor  (frontend/ 서브모듈)
  → AP JSON
  → APEX-Cache
      ├─ AP Layer       : JSON 파싱 → AccessEvent 스트림 복원
      ├─ Memory Layer   : flat address 배치 → byte_address 계산
      ├─ Cache Layer    : XML 설정형 multi-L1/L2/Memory 시뮬레이션
      ├─ Analysis Layer : cold/capacity/conflict miss 귀속 및 진단
      └─ Report Layer   : CSV / JSON / Markdown 출력
```

---

## 기능

- AP JSON → AccessEvent 선형 스트림 복원 (루프 언롤, call 인라이닝 + 인자 바인딩)
- YAML 기반 캐시 계층 설정 (`cache.yaml`) — private L1 × N + shared L2 + Memory
- LRU replacement, write-back / write-through 정책
- cold / capacity / conflict miss 분류
- inclusive / exclusive Region 집계
- rule-based 최적화 진단 (padding, blocking, loop interchange 힌트)
- 출력: 입력명 기반 `<name>.csv`/`.json` (miss 분류 + L1/L2 hit율 + cycle 통계), `<name>_diagnostics.md`, `<name>_objects.csv`

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

```bash
# 서브모듈 초기화
git submodule update --init --recursive

# 빌드
cmake -S . -B build
cmake --build build

# 테스트
ctest --test-dir build
```

---

## 사용법

```bash
# 단일 실행 (구현 완료) — 입력은 APE LAT v2 JSON
./build/apex-cache run input_g_ape.json --cache cache.yaml [options]

# sweep: L1 용량 범위 탐색 (예정)
./build/apex-cache sweep input_g_ape.json --cache cache.yaml --l1-sizes 4K,8K,16K,32K
```

`run`은 `--output`(기본 `results/`, 없으면 자동 생성)에 입력 파일명 기반으로
`<name>.csv`, `<name>.json`, `<name>_diagnostics.md`, `<name>_objects.csv`를 생성한다.
예를 들어 `examples/test_stencil_g_ape.json`은 `test_stencil_ape.*` 리포트로
저장된다.

### CLI 옵션

| 옵션 | 설명 |
|------|------|
| `--cache <cache.yaml>` | 캐시 계층 설정 파일. `run`에서 필수 |
| `--output <dir>` | 리포트 출력 디렉터리. 기본값은 `results` |
| `--quiet` | 자동화용 최소 출력. 결과 디렉터리만 표시 |
| `--verbose` | 캐시 설정 요약까지 함께 출력 |
| `--no-color` | ANSI color 없이 plain text로 출력 |
| `-h`, `--help` | 도움말 출력 |

도움말은 아래 명령으로 확인할 수 있다.

```bash
./build/apex-cache --help
./build/apex-cache help
./build/apex-cache run --help
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
APEX-Cache/
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
    __main__.py                ← CLI (python3 -m backend)
    tests/                     ← pytest
  results/                     ← 출력 디렉터리 (gitignore)
```

---

## 라이선스

TBD
