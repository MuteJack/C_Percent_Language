# 언어 스펙 vs 구현 차이점

C%는 인터프리터, 트랜스파일러(cpct-cpp-lib), Cling JIT 세 가지 실행 경로가 있다.
각 경로의 구현 차이를 정리한다.

## 정수 저장

| 항목 | 언어 스펙 | 인터프리터 | cpct-cpp-lib |
| --- | --- | --- | --- |
| `int` | `int16f` (플랫폼 최적) | `int64_t`로 저장, 범위 검사 | `cpct::int_t` (platform.h에 의해 고정 크기) |
| `int8`~`int64` | 각 크기 고정 | `int64_t`로 저장, `checkIntRange()` wrap | 각 `intN_t` 네이티브 |
| `uint`~`uint64` | 각 크기 unsigned | `uint64_t`로 저장, 범위 제한 | 각 `uintN_t` 네이티브 |
| `int8f`~`int32f` | 플랫폼 최적 크기 | `int64_t`로 저장, 오버플로 런타임 에러 | `cpct::fast*_t` (platform.h) |
| `intbig` | int64 + BigInt 자동 승격 | `int64_t` / `BigInt` 전환 | `cpct::IntBig` (variant) |
| `bigint` | 항상 임의 정밀도 | `BigInt` | `BigInt` |

## 실수 저장

| 항목 | 언어 스펙 | 인터프리터 | cpct-cpp-lib |
| --- | --- | --- | --- |
| `float32` | 32비트 단정밀도 | `double`로 저장, `coerceFloat()` 절삭 | `float` 네이티브 |
| `float` / `float64` | 64비트 배정밀도 | `double` (동일) | `double` 네이티브 |

## 비교 연산

| 항목 | 인터프리터 | cpct-cpp-lib |
| --- | --- | --- |
| signed/unsigned 혼합 비교 | `safeCmpLess()` 등 헬퍼 | `cpct::cmp_*` → `std::cmp_*` (C++20) |

## 소유권/이동

| 항목 | 인터프리터 | 트랜스파일러 |
| --- | --- | --- |
| use-after-move 검사 | 런타임 (`moved_` set) | **정적 분석** (Analyzer, 컴파일 전 차단) |
| const 위반 검사 | 런타임 (`consts_` set) | C++ `const` 키워드에 위임 |

## 메모리 모델

| 항목 | 인터프리터 | cpct-cpp-lib |
| --- | --- | --- |
| 스칼라 변수 | `CpctValue` (variant, ~48B) | 네이티브 크기 (래퍼 클래스) |
| 배열 | `TypedArray` (네이티브 크기) | `cpct::Array<T,N>` / `cpct::Vector<T>` |
| dict/map | `CpctDict` (tagged hash) | `cpct::Dict` / `cpct::Map<K,V>` |

## 산술 연산

| 항목 | 인터프리터 | cpct-cpp-lib |
| --- | --- | --- |
| 실행 방식 | variant 타입 체크 → 분기 | C++ 네이티브 연산 (암시적 변환) |
| 오버헤드 | 높음 (매 연산마다 태그 체크) | 없음 (컴파일 타임 타입 결정) |
| 성능 | 느림 (트리워킹) | 빠름 (네이티브 / JIT) |

> 인터프리터는 폐기 예정. 향후 Cling JIT가 대체하며, cpct-cpp-lib를 공유하므로 구현 차이가 없어진다.
