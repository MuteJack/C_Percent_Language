# C/C++와의 차이점

C%는 C/C++ 문법을 기반으로 하되, 일부 동작이 다르다. C/C++ 경험자가 혼동할 수 있는 항목을 정리한다.

## 타입 시스템

| 항목 | C/C++ | C% |
| --- | --- | --- |
| `float` 기본 크기 | 32비트 (`float`) | **64비트** (`double` 상당, `float64` alias) |
| `int8` | `char`와 동의어, 문자로 취급 | **순수 정수**, 문자 아님 |
| `int` 오버플로 | UB (C) / 구현 정의 | **wrap-around 보장** |
| 포인터/참조 | 핵심 기능 (`int* p`, `int& r`) | **없음** (메모리 모델 없음) |
| `null`/`nullptr` | 존재 | **없음** (미구현) |

## Truthy / Falsy

| 항목 | C/C++ | C% |
| --- | --- | --- |
| 빈 문자열 `""` | truthy (non-null 포인터) | **falsy** (비어있으면 false) |
| `std::string` 조건문 | 컴파일 에러 (암묵 변환 불가) | **허용** (`""` = false, 그 외 = true) |
| 빈 배열 | truthy (포인터) | **falsy** |
| `0`, `0.0`, `false` | falsy | falsy (동일) |

**C% 원칙**: 데이터가 없으면 false, 있으면 true (Python과 동일)

## 문법

| 항목 | C/C++ | C% |
| --- | --- | --- |
| 배열 초기화 | `int arr[] = {1, 2, 3};` | `int arr[] = [1, 2, 3];` |
| 제곱 연산 | 없음 (`pow()` 사용) | `**` 연산자 |
| divmod 연산 | 없음 | `/%` 연산자, `divmod()` 함수 |
| 연쇄 비교 | `0 < x < 10` → `(0<x) < 10` (버그) | `0 < x < 10` → `(0<x) && (x<10)` |
| 삼항 연산자 | `a ? b : c` | `a ? b : c` (동일) |
| 타입 변환 | `(int)x`, `static_cast<int>(x)` | `int(x)`, `float(x)`, `string(x)` |
| switch + float | 컴파일 에러 (정수/enum만) | 런타임 에러 (금지, C/C++과 동일 정책) |

## 문자열

| 항목 | C/C++ | C% |
| --- | --- | --- |
| 문자열 리터럴 내부 저장 | `const char[]` (읽기 전용) | `char[]` (내부 저장, `string` 대입 시 승격) |
| `"hello" + 2` | 포인터 산술 → `"llo"` (함정) | **문자열 연결** → `"hello2"` |
| `"hello" + "world"` | 컴파일 에러 (`const char*` + `const char*`) | **문자열 연결** → `"helloworld"` |
| 문자열 연결 `+` | `std::string`끼리만 | **자동 타입 변환** 후 연결 (리터럴 포함) |
| F-string | 없음 (`std::format` C++20) | `f"Hello, {name}!"` |
| Raw string | `R"(...)"`  | `r"..."` |

## 추가 기능 (C/C++에 없는 것)

- `intbig`: int64 범위에서는 고속, 오버플로 시 자동 BigInt 승격
- `bigint`: 항상 임의 정밀도 정수
- `for(n) { ... }`: n번 반복 (카운터 변수 불필요)
- `for(0 <= i < 10) { ... }`: 연쇄 비교 기반 범위 반복
- `vector<T>`: C++ `std::vector`와 유사한 동적 배열 (메서드: `push`, `pop`, `insert`, `erase`, `clear`, `empty`, `front`, `back`)
- `dict`: 키-값 쌍 (타입 자유)
- `map<K, V>`: 타입이 고정된 키-값 쌍
