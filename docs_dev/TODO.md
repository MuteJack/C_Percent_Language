# TODO LIST

## DOCS

- [ ] 영문 markdown 추가
- [ ] 라이선스 변경 (GPL+PolyForm → PSF 또는 BSD-3-Clause 검토)

## Sub-Projects

| Sub-Project 명     | 역할                       | 상태                 | 비고                                    |
| ------------------ | -------------------------- | -------------------- | --------------------------------------- |
| cpct-core          | 공유 파싱 인프라           | 구현완료             | lexer, parser, AST, preprocessor        |
| cpct-cpp-lib       | C++ 타입 라이브러리        | 구현완료             | 트랜스파일된 코드용                     |
| cpct-interpreter   | 트리워킹 인터프리터        | 구현완료 (폐기 예정) | Cling JIT로 대체 예정                   |
| cpct-transpiler    | C% → C++ 변환기            | 구현완료             | translator + analyzer + compiler        |
| cpct-jit           | Cling JIT REPL             | 구현완료 (프로토타입)| conda + cling 필요                      |
| cpct-vscode        | 문법 하이라이팅            | 개발 중 (프로토타입) | LSP/DAP 미구현                          |

---

## 구현완료

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

### 함수

| 기능           | C++                  | Python            | C%             | 상태   |
| -------------- | -------------------- | ----------------- | -------------- | ------ |
| 참조 전달 (ref) | `void f(int& x)`   | (뮤터블 객체)     | `void f(ref int x)` → `f(ref x)` | 구현완료 |
| 복합 타입 매개변수 | `void f(int arr[])` | `def f(arr)` | `void f(int[] arr)` / `void f(array<int> arr)` | 구현완료 |
| main() 진입점 | `int main()`       | `if __name__`  | `int main()` / `void main()` | 구현완료 |

### 타입/데이터

| 기능           | C++                          | Python    | C%             | 상태   |
| -------------- | ---------------------------- | --------- | -------------- | ------ |
| const          | `const`                    | (없음)    | `const int x = 10;`        | 구현완료 |
| static         | `static`                   | (없음)    | `static int count = 0;`    | 구현완료 |
| let (소유권)   | `std::move(x)`             | (기본 동작) | `let int y = x;`          | 구현완료 |
| heap           | `new int(42)`              | (없음)    | `heap int x = 42;`         | 구현완료 |
| fast 정수      | `int_fast16_t`             | (없음)    | `int16f`, `int` = `int16f` 별칭 | 구현완료 |
| --platform     | (없음)                     | (없음)    | `--platform=avr` 등 7개 프리셋  | 구현완료 |

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
| reverse     | `std::reverse()`  | `list.reverse()`   | `v.reverse()`            | 구현완료 |
| find/index  | `std::find()`     | `list.index()`     | `v.find(val)`            | 구현완료 |
| contains/in | `std::find()`     | `x in list`        | `v.is_contains(val)`     | 구현완료 |

### 전처리기 / 기타

| 기능        | C++           | Python         | C%             | 상태   |
| ----------- | ------------- | -------------- | -------------- | ------ |
| #define/#undef | `#define`, `#undef` | (없음) | `#define`, `#undef` (C/C++과 동일) | 구현완료 |

### 트랜스파일러

| 기능 | 상태 |
| ---- | ---- |
| C% → C++ 코드 변환 (27개 예제 통과) | 구현완료 |
| move-after-use 정적 분석 | 구현완료 |
| safe signed/unsigned 비교 (std::cmp_*) | 구현완료 |
| 플랫폼별 fast 타입 크기 매핑 (platform.h) | 구현완료 |
| Cling JIT REPL | 구현완료 (프로토타입) |

---

## 우선순위: 중간

### 함수

| 기능           | C++                  | Python            | C%             | 상태   |
| -------------- | -------------------- | ----------------- | -------------- | ------ |
| 기본 매개변수  | `int f(int x = 0)` | `def f(x=0)`    | **없음** | 미구현 |
| 가변 인자      | `variadic`         | `*args`         | **없음** | 미구현 |
| 람다/익명 함수 | `[](int x){...}`   | `lambda x: ...` | **없음** | 미구현 |
| ref 반환       | `int& f()`         | (없음)           | **없음** | 설계중 |

### 타입/데이터

| 기능           | C++                          | Python    | C%             | 상태   |
| -------------- | ---------------------------- | --------- | -------------- | ------ |
| null/none      | `nullptr`                  | `None`  | **없음** | 미구현 |
| enum           | `enum`                     | `Enum`  | **없음** | 미구현 |
| struct/class   | `struct`/`class`         | `class` | **없음** | 미구현 |
| 튜플/다중 반환 | `tuple`/structured binding | `tuple` | **없음** | 미구현 |
| 포인터         | `int* p = &x`              | (없음)    | **없음** | 미구현 |
| `:=` 복사 대입 | (없음)                     | (없음)    | **없음** | 설계중 |

### 자료구조

| 기능   | C++                | Python               | C%             | 상태   |
| ------ | ------------------ | -------------------- | -------------- | ------ |
| 행렬   | (없음, 라이브러리) | `numpy.ndarray`    | **없음** | 미구현 |
| 테이블 | (없음, 라이브러리) | `pandas.DataFrame` | **없음** | 미구현 |

---

## 우선순위: 낮음

### 에러 처리 / 모듈

| 기능        | C++           | Python         | C%             | 상태   |
| ----------- | ------------- | -------------- | -------------- | ------ |
| try/catch   | `try/catch` | `try/except` | **없음** | 미구현 |
| import      | `#include`/`import` | `import`/`from...import` | **없음** | 설계중 |
| 파일 I/O    | `fstream`   | `open()`     | **없음** | 설계중 |

---
