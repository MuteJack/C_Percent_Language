# TODO LIST

## DOCS

- [ ] 영문 markdown 추가

## Sub-Projects

| Sub-Project 명     | 역할                   | 상태                 | 비고                                                    |
| ------------------ | ---------------------- | -------------------- | ------------------------------------------------------- |
| C% vscode          | 문법 하이라이팅        | 개발 중 (프로토타입) | LSP/DAP 미구현                                          |
| C% Interpreter     | 인터프리터             | 개발 중 (프로토타입) | 트리워킹 방식                                           |
| C% JIT Interpreter | 인터프리터             | 개발예정             | 트리워킹 방식                                           |
| C% Translator      | C% to C++ 등 언어 변환 | 고려대상             | 타언어 지원여부 고려중                                  |
| Compiler           | C% 컴파일러            | 고려대상             | C% to Cpp Translator 기반으로 Cpp 컴파일을 할 지 고려중 |

---

## 우선순위: 높음

### 연산자

| 기능      | C++                  | Python | C%                         | 상태     |
| --------- | -------------------- | ------ | -------------------------- | -------- |
| 비트 연산 | `& \| ^ ~ << >>`    | 동일   | `& \| ^ ~ << >>`          | 구현완료 |
| 비트 대입 | `&= \|= ^= <<= >>=` | 동일   | `&= \|= ^= <<= >>=`      | 구현완료 |

### 문자열 메서드

| 기능         | C++              | Python          | C%                                  | 상태     |
| ------------ | ---------------- | --------------- | ----------------------------------- | -------- |
| split/join   | 직접 구현        | `str.split()` | `split(s, sep)` / `join(sep, arr)` | 구현완료 |
| upper/lower  | `toupper`      | `str.upper()` | `upper(s)` / `lower(s)`           | 구현완료 |
| find/replace | `str.find()`   | `str.find()`  | `find(s, sub)` / `replace(s,o,n)` | 구현완료 |
| trim/strip   | (C++20 없음)     | `str.strip()` | `trim(s)`                          | 구현완료 |
| contains     | `str.find()`   | `in` 연산자   | `is_contains(s, sub)`              | 구현완료 |
| substring    | `str.substr()` | `s[1:3]`      | `substr(s, start, len)`            | 구현완료 |

---

## 우선순위: 중간

### 함수

| 기능           | C++                  | Python            | C%             | 상태   |
| -------------- | -------------------- | ----------------- | -------------- | ------ |
| 기본 매개변수  | `int f(int x = 0)` | `def f(x=0)`    | **없음** | 미구현 |
| 참조 전달 (ref) | `void f(int& x)`   | (뮤터블 객체)     | `void f(ref int x)` → `f(ref x)` | 구현완료 |
| 복합 타입 매개변수 | `void f(int arr[])` | `def f(arr)` | `void f(int[] arr)` / `void f(array<int> arr)` | 구현완료 |
| 가변 인자      | `variadic`         | `*args`         | **없음** | 미구현 |
| 람다/익명 함수 | `[](int x){...}`   | `lambda x: ...` | **없음** | 미구현 |

### 타입/데이터

| 기능           | C++                          | Python    | C%             | 상태   |
| -------------- | ---------------------------- | --------- | -------------- | ------ |
| null/none      | `nullptr`                  | `None`  | **없음** | 미구현 |
| enum           | `enum`                     | `Enum`  | **없음** | 미구현 |
| struct/class   | `struct`/`class`         | `class` | **없음** | 미구현 |
| 튜플/다중 반환 | `tuple`/structured binding | `tuple` | **없음** | 미구현 |
| const          | `const`                    | (없음)    | `const int x = 10;`        | 구현완료 |
| static         | `static`                   | (없음)    | `static int count = 0;`    | 구현완료 |
| let (소유권)   | `std::move(x)`             | (기본 동작) | `let int y = x;`          | 구현완료 |
| heap           | `new int(42)`              | (없음)    | `heap int x = 42;`         | 구현완료 |
| 포인터         | `int* p = &x`              | (없음)    | **없음** | 미구현 |
| 주소/역참조    | `&x`, `*p`               | (없음)    | **없음** | 미구현 |

### 자료구조

| 기능   | C++                | Python               | C%             | 상태   |
| ------ | ------------------ | -------------------- | -------------- | ------ |
| 행렬   | (없음, 라이브러리) | `numpy.ndarray`    | **없음** | 미구현 |
| 테이블 | (없음, 라이브러리) | `pandas.DataFrame` | **없음** | 미구현 |

- **행렬**: 수학 연산 특화 2D 자료구조 (행렬곱, 전치 등)
- **테이블**: 열 기반 데이터 구조 (CSV/DB 스타일, 열 이름으로 접근)

### 배열 동적 조작

| 기능        | C++                 | Python               | C%                         | 상태     |
| ----------- | ------------------- | -------------------- | -------------------------- | -------- |
| push/append | `vec.push_back()` | `list.append()`    | `v.push(x)`              | 구현완료 |
| pop         | `vec.pop_back()`  | `list.pop()`       | `v.pop()`                | 구현완료 |
| insert      | `vec.insert()`    | `list.insert()`    | `v.insert(i, x)`         | 구현완료 |
| erase       | `vec.erase()`     | `del list[i]`      | `v.erase(i)`             | 구현완료 |
| clear       | `vec.clear()`     | `list.clear()`     | `v.clear()`              | 구현완료 |
| is_empty    | `vec.empty()`     | `not list`         | `v.is_empty()`           | 구현완료 |
| front/back  | `vec.front()`     | `list[0]`/`[-1]` | `v.front()`/`v.back()` | 구현완료 |
| reverse     | `std::reverse()`  | `list.reverse()`   | **없음**             | 미구현   |
| find/index  | `std::find()`     | `list.index()`     | **없음**             | 미구현   |
| contains/in | `std::find()`     | `x in list`        | **없음**             | 미구현   |

---

## 우선순위: 낮음

### 에러 처리 / 기타

| 기능        | C++           | Python         | C%             | 상태   |
| ----------- | ------------- | -------------- | -------------- | ------ |
| try/catch   | `try/catch` | `try/except` | **없음** | 미구현 |
| 모듈/import | `#include`  | `import`     | **없음** | 미구현 |
| 파일 I/O    | `fstream`   | `open()`     | **없음** | 미구현 |

---
