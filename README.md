# C% (C Percent)

C%는 C/C++ 스타일 문법에 현대적 편의 기능을 더한 실험적인 프로그래밍 언어입니다.
`#include`나 `main()` 없이 바로 코드를 작성할 수 있으며,
명시적 타입 크기, F-string, 임의 정밀도 정수 등 다양한 편의 기능을 제공합니다.

## 추구 방향

- C/C++ 스타일의 문법과 성능
- Python, MATLAB과 같은 편의성, 안정성의 혼합
- 명시적이고 예측 가능한 타입 시스템
- 최소한의 보일러플레이트로 바로 실행 가능한 코드

## 특징

- 기본 라이브러리 내장: `string`, `print`, `len` 등 별도 `#include` 불필요
- 정수/실수 크기를 타입명에 명시: `int8`, `int16`, `int64`, `float32`, `float64`
- 임의 정밀도 정수 내장: `intbig` (자동 승격), `bigint` (항상 임의 정밀도)
- Python 스타일 F-string, Raw 문자열 지원: `f"..."`, `r"..."`
- 연쇄 비교: `0 < x < 10`
- 범위 기반 for: `for (0 <= i < 10) { ... }`
- REPL 모드 지원 (cpct-interpreter)

----------------

## 사용법 (인터프리터)

```bash
# REPL 모드
./cpct.exe

# 파일 실행
./cpct.exe examples/hello.cpc
```

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

-----------------------------------

## 개발 환경
권장 환경: Windows 10 or Later, MSYS2

- C++17 이상을 지원하는 컴파일러
  - **Windows**: MSYS2 (MinGW-w64) 또는 Visual Studio 2017+
  - **Linux / macOS**: GCC 7+ 또는 Clang 5+

- CMake 3.15+ (선택)

## 빌드

### Windows (MSYS2)

```bash
# MSYS2 MinGW 64-bit 터미널에서 실행
bash build.sh
```

### Linux / macOS

```bash
bash build.sh

# 또는 직접 빌드
g++ -std=c++17 -O2 -Wall -Wextra \
    cpct-interpreter/src/main.cpp \
    cpct-interpreter/src/lexer.cpp \
    cpct-interpreter/src/parser.cpp \
    cpct-interpreter/src/interpreter.cpp \
    cpct-interpreter/src/bigint.cpp \
    -Icpct-interpreter/src -o cpct
```

### CMake (공통)

```bash
cmake -B build && cmake --build build
```

## 프로젝트 구조

```
cpct-interpreter/   인터프리터 소스 코드
cpct-translater/    C% to C++ 번역기 (개발 예정)
cpct-compiler/      컴파일러 (개발 고려중)
cpct-vscode/        VSCode 확장 (syntax 등)
docs/               사용자 문서
docs_dev/           개발자 문서
examples/           C% 예제 코드
```

## 라이선스

이 소프트웨어는 GPL v3 및 PolyForm Noncommercial License 1.0.0 하에 이중 라이선스됩니다.
자세한 내용은 [LICENSE](LICENSE) 파일을 참고하세요.

Copyright (C) 2026 MuteJack
