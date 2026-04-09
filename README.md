# C% (C Percent)

C%는 C/C++ 스타일 문법에 현대적 편의 기능을 더한 프로그래밍 언어입니다.
기본 라이브러리에 대해 `#include`나 `main()` 없이 바로 코드를 작성할 수 있으며,
C++ 트랜스파일링을 기반으로 PC부터 임베디드까지 폭넓은 플랫폼을 지원할 예정입니다.

## 1. 추구 방향

- C/C++ 스타일의 문법과 네이티브 성능
- Python, MATLAB과 같은 편의성과 안정성
- 명시적이고 예측 가능한 타입 시스템
- C++ 트랜스파일링을 통한 크로스 플랫폼 지원 (PC, Arduino, ARM 등)

## 2. 특징

- 기본 라이브러리 내장: `string`, `print`, `len` 등 기본 함수에 대해 별도 `#include` 불필요
- 정수/실수 크기를 타입명에 명시: `int8`, `int16`, `int64`, `float32`, `float64`
- Fast 정수 타입: `int8f`, `int16f`, `int32f` (플랫폼 최적 크기)
- Python 스타일 F-string, Raw 문자열 지원: `f"..."`, `r"..."`
- 임의 정밀도 정수 내장: `intbig` (int64/bigint 자동 승격), `bigint` (항상 임의 정밀도)
- 변수 한정자: `const`, `static`, `heap`, `let` (소유권 이전), `ref` (참조)
- 연쇄 비교: `0 < x < 10`
- 범위 기반 for: `for (0 <= i < 10) { ... }`
- `main()` 있으면 진입점, 없으면 스크립트 모드
- `#define` / `#undef` 등 전처리기 지원

## 3. 예제

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

## 4. Documents

- [기본 문법](docs/syntax.md)
- [데이터 타입](docs/datatype.md)
- [연산자](docs/operators.md)
- [자료구조](docs/datastructure.md)
- [C/C++와의 차이점](docs/diff_from_cpp.md)

---

## 5. Installation

### 5.1. Requirements

- **Windows**:
  - Environment: Windows 10 or Later
  - Dependencies:
    - MSVC (Visual Studio 2022 LTSC 17.8 or Later)
    - cmake (Optional, Already Included in VS)
    - git
    - Python 3.8 or later
- **Linux**:
  - Environment: Ubuntu 24.04 LTS Recommended
  - Dependencies: g++, git, cmake, python3 (3.8 or later)
- **macOS**:
  - Environment: Not officially tested/supported yet
  - Dependencies: Xcode Command Line Tools (g++, git, cmake, python3)

### 5.2. Install on Windows 10 or Later

1. `Visual Studio 2022` 설치
2. `x64 Native Tools Command Prompt for VS 2022` 실행
3. 프로젝트 경로에서 build.bat 실행
   ```bash
    cd /d {ProjectDir}
    build.bat
   ```

### 5.3. Install on Linux

1. 의존성 설치 (git, g++, cmake)

   ```bash
   sudo apt update
   sudo apt install g++ git cmake
   ```
2. 프로젝트 빌드

```bash
cd /path/to/cmm_interpreter
bash build.sh
```

### 5.4. Install on MacOS

현재 MacOS에 대해서는 해당 장비의 미비로 테스트되지 않았습니다.

---

## 6. 사용법

```bash
cpct                              # REPL 모드 (JIT)
cpct --help                       # 도움말
cpct --version                    # 버전 확인
cpct script.cpc                   # c% 코드를 JIT로 실행
cpct --translate script.cpc       # c% -> C++ 변환
cpct --compile script.cpc -o out  # c% 코드 컴파일
cpct --run script.cpc             # c% 코드 컴파일 + 실행
```

### REPL 예시

```bash
$ ./cpct
C% v0.0.2 (JIT via Cling)
Type 'exit' to quit.

c%> int x = 42;
c%> println("x =", x);
x =42
c%> vector<int> v = [1, 2, 3];
c%> v.push(4);
c%> println(v);
[1, 2, 3, 4]
c%> exit
Bye!
```

## 7. 실행 방식 비교

| 방식     | 명령                                 | 속도            | REPL |
| -------- | ------------------------------------ | --------------- | ---- |
| JIT 실행 | `cpct script.cpc`                  | 빠름 (JIT)      | O    |
| 컴파일   | `cpct --compile script.cpc -o out` | 빠름 (네이티브) | -    |
| C++ 변환 | `cpct --translate script.cpc`      | -               | -    |

## 8. 프로젝트 구조

```
src/
├── main.cpp            cpct (통합 CLI 진입점)
├── core/               공유 파싱 인프라 (lexer, parser, AST, preprocessor)
├── lib/                C++ 타입 라이브러리 (트랜스파일된 코드용)
├── translate/          cpct-translate (C% → C++ 변환기)
├── compile/            cpct-compile (cpp 변환 후, g++(Linux) or MSVC(Windows) 컴파일)
└── jit/                cpct-jit (Cling JIT REPL)
tools/
├── .src/               Cling/LLVM 소스 (빌드 시 자동 clone)
└── cling-build/        Cling 빌드 결과물 (bin/, lib/, include/)
cpct-vscode/            VSCode 확장 (syntax highlighting)
tests/                  라이브러리 테스트
docs/                   사용자 문서
docs_dev/               개발자 문서
examples/               C% 예제 코드
```

## 9. 라이선스

이 소프트웨어는 GPL v3 및 PolyForm Noncommercial License 1.0.0 하에 이중 라이선스됩니다.
자세한 내용은 [LICENSE](LICENSE) 파일을 참고하세요.

Copyright (C) 2026 MuteJack
