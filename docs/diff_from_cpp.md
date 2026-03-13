# C/C++와의 차이점

C%는 C/C++ 문법을 기반으로 하되, 일부 동작이 다르다. C/C++ 경험자가 혼동할 수 있는 항목을 정리한다.

## 타입 시스템

| 항목 | C/C++ | C% |
| --- | --- | --- |
| `float` 기본 크기 | 32비트 (`float`) | **64비트** (`double` 상당, `float64` alias) |
| `int8` | `char`와 동의어, 문자로 취급 | **순수 정수**, 문자 아님 |
| `int` 오버플로 | UB (C) / 구현 정의 | **wrap-around 보장** |
| 참조 | `int& r = x` | `ref int r = x` (`ref` 키워드 사용) |
| 참조 전달 | `void f(int& x)` → `f(x)` (암묵적) | `void f(ref int x)` → `f(ref x)` (호출부 명시) |
| 소유권 이전 | `auto y = std::move(x)` | `let int y = x` (`let` 키워드) |
| const | `const int x = 10;` | `const int x = 10;` (동일 문법) |
| static | `static int count = 0;` | `static int count = 0;` (동일 문법) |
| heap | `int* p = new int(42)` | `heap int x = 42` (스코프 기반 자동 해제) |
| 포인터 | `int* p = &x` | **없음** (미구현) |
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
| 참조 기호 | `&` (참조/주소 겸용) | `@` (참조 전용, `&`는 비트 AND) |
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

## 문자열 메서드

C/C++에서는 `<string>`, `<algorithm>`, `<cctype>` 등에 분산되어 있는 기능을 C%에서는 내장 함수로 제공한다.
모든 문자열 함수는 `func(s, ...)` 또는 `s.func(...)` 두 가지 호출 방식을 지원한다.

| 기능 | C/C++ | C% |
| --- | --- | --- |
| 문자열 분할 | 직접 구현 (getline 등) | `split(s, sep)` |
| 배열→문자열 결합 | 직접 구현 | `join(sep, arr)` |
| 대소문자 변환 | `toupper`/`tolower` (문자 단위) | `upper(s)` / `lower(s)` (문자열 단위) |
| 부분문자열 검색 | `str.find()` | `find(s, sub)` (없으면 -1) |
| 문자열 치환 | 직접 구현 | `replace(s, old, new)` (전체 치환) |
| 공백 제거 | 직접 구현 (C++20에도 없음) | `trim(s)` |
| 부분문자열 추출 | `str.substr()` | `substr(s, start, len)` |
| 문자열 뒤집기 | `std::reverse()` (in-place) | `reverse(s)` (새 문자열 반환) |
| 포함 여부 | `str.find() != npos` | `is_contains(s, sub)` |
| 접두사/접미사 | C++20 `starts_with`/`ends_with` | `is_starts_with(s, p)` / `is_ends_with(s, p)` |
| 문자 판별 | `isdigit()`, `isalpha()` (문자 단위) | `is_digit(s)`, `is_alpha(s)` (문자열 단위) |
| 대소문자 판별 | `isupper()`, `islower()` (문자 단위) | `is_upper(s)`, `is_lower(s)` (문자열 단위) |

**`is_` 접두사 규칙**: bool을 반환하는 판별 함수는 모두 `is_` 접두사를 사용한다 (`is_empty`, `is_contains`, `is_digit` 등).

## 추가 기능 (C/C++에 없는 것)

- `intbig`: int64 범위에서는 고속, 오버플로 시 자동 BigInt 승격
- `bigint`: 항상 임의 정밀도 정수
- `for(n) { ... }`: n번 반복 (카운터 변수 불필요)
- `for(0 <= i < 10) { ... }`: 연쇄 비교 기반 범위 반복
- `array<T>`: `T[]`의 제네릭 스타일 별칭 (예: `array<int>` = `int[]`)
- `vector<T>`: C++ `std::vector`와 유사한 동적 배열 (메서드: `push`, `pop`, `insert`, `erase`, `clear`, `is_empty`, `front`, `back`)
- `dict`: 키-값 쌍 (타입 자유)
- `map<K, V>`: 타입이 고정된 키-값 쌍
- 함수 매개변수/반환 타입에 복합 타입 사용 가능: `void f(int[] arr)`, `void f(vector<int> v)`, `int[] getArr()`
