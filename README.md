# C% (C Percent)

C%는 C/C++ 스타일 문법에 현대적 편의 기능을 더한 프로그래밍 언어입니다.
`#include`나 `main()` 없이 바로 코드를 작성할 수 있으며,
C++ 트랜스파일링을 기반으로 PC부터 임베디드까지 폭넓은 플랫폼을 지원합니다.

## 추구 방향

- C/C++ 스타일의 문법과 네이티브 성능
- Python, MATLAB과 같은 편의성과 안정성
- 명시적이고 예측 가능한 타입 시스템
- C++ 트랜스파일링을 통한 크로스 플랫폼 지원 (PC, Arduino, ARM 등)

## 특징

- 기본 라이브러리 내장: `string`, `print`, `len` 등 별도 `#include` 불필요
- 정수/실수 크기를 타입명에 명시: `int8`, `int16`, `int64`, `float32`, `float64`
- Fast 정수 타입: `int8f`, `int16f`, `int32f` (플랫폼 최적 크기)
- 임의 정밀도 정수 내장: `intbig` (자동 승격), `bigint` (항상 임의 정밀도)
- 변수 한정자: `const`, `static`, `heap`, `let` (소유권 이전), `ref` (참조)
- Python 스타일 F-string, Raw 문자열: `f"..."`, `r"..."`
- 연쇄 비교: `0 < x < 10`
- 범위 기반 for: `for (0 <= i < 10) { ... }`
- `main()` 있으면 진입점, 없으면 스크립트 모드
- `#define` / `#undef` 전처리기 지원

## 예제

```c
// 변수 선언
int x = 10;
float pi = 3.14159;
string name = "World";

// 출력
println(f"Hello, {name}!");
println(f"x = {x}, pi = {pi}");

// 함수
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
println(factorial(10));

// 범위 기반 for
for (0 <= i < 10) {
    print(i, " ");
}
```

더 많은 예제는 [examples/](examples/) 디렉토리를 참고하세요.

## 문서

- [기본 문법](docs/syntax.md)
- [데이터 타입](docs/datatype.md)
- [연산자](docs/operators.md)
- [자료구조](docs/datastructure.md)
- [C/C++와의 차이점](docs/diff_from_cpp.md)

---

## 설치

### 요구사항

- **Windows**: MSYS2 (MinGW-w64)
- **Linux / macOS**: GCC 9+ 또는 Clang 10+
- C++17 이상 (인터프리터), C++20 이상 (트랜스파일러)

### 빌드

```bash
# 인터프리터 빌드
bash build.sh

# 트랜스파일러 빌드
g++ -std=c++20 -I cpct-core/src \
    -o cpct-translate.exe \
    cpct-transpiler/src/main.cpp \
    cpct-core/src/lexer.cpp \
    cpct-core/src/parser.cpp \
    cpct-interpreter/src/bigint.cpp
```

### Cling JIT REPL (선택)

Cling JIT는 C++ JIT 컴파일러를 통해 네이티브 속도로 인터랙티브 실행을 제공합니다.
conda 설치가 필요합니다.

```bash
# 1. conda 환경 생성 + cling 설치
conda create -n cpct-cling -c conda-forge cling -y

# 2. cpct 라이브러리를 cling include 경로에 복사
cp -r cpct-cpp-lib/cpct ~/.conda/envs/cpct-cling/Library/include/

# 3. Cling JIT REPL 빌드
g++ -std=c++20 -I cpct-core/src \
    -o cpct-jit.exe \
    cpct-jit/src/cling_repl.cpp \
    cpct-core/src/lexer.cpp \
    cpct-core/src/parser.cpp \
    cpct-interpreter/src/bigint.cpp
```

---

## 사용법

### 인터프리터 (의존성 없음)

```bash
# REPL 모드
./cpct.exe

# 파일 실행
./cpct.exe script.cpc

# 플랫폼 지정 (fast 타입 크기 변경)
./cpct.exe --platform=avr script.cpc
```

### 트랜스파일러 (C++ 컴파일러 필요)

```bash
# C% → C++ 변환
./cpct-translate.exe script.cpc > output.cpp

# C++ 컴파일
g++ -std=c++20 -I cpct-cpp-lib output.cpp \
    cpct-cpp-lib/cpct/data/type/bigint.cpp \
    -o output.exe
```

### Cling JIT REPL (conda + cling 필요)

```bash
./cpct-jit.exe
c%> int x = 42;
c%> println("x =", x);
x =42
c%> vector<int> v = [1, 2, 3];
c%> v.push(4);
c%> println(v);
[1, 2, 3, 4]
```

---

## 실행 방식 비교

| 방식 | 실행 파일 | 속도 | 변수 유지 | 의존성 |
| ---- | --------- | ---- | --------- | ------ |
| 인터프리터 | `cpct.exe` | 보통 (트리워킹) | O (REPL) | 없음 |
| 트랜스파일러 | `cpct-translate.exe` + g++ | 빠름 (네이티브) | - (파일 단위) | g++ |
| Cling JIT | `cpct-jit.exe` | 빠름 (JIT) | O (REPL) | conda + cling |

## 프로젝트 구조

```
cpct-core/          공유 파싱 인프라 (lexer, parser, AST, preprocessor)
cpct-cpp-lib/       C++ 타입 라이브러리 (트랜스파일된 코드용)
cpct-interpreter/   트리워킹 인터프리터
cpct-transpiler/    C% → C++ 변환기 (translator, analyzer, compiler)
cpct-jit/           Cling JIT REPL
cpct-vscode/        VSCode 확장 (syntax highlighting)
tests/              라이브러리 테스트
docs/               사용자 문서
docs_dev/           개발자 문서
examples/           C% 예제 코드
```

## 라이선스

이 소프트웨어는 GPL v3 및 PolyForm Noncommercial License 1.0.0 하에 이중 라이선스됩니다.
자세한 내용은 [LICENSE](LICENSE) 파일을 참고하세요.

Copyright (C) 2026 MuteJack
