# C% - 개발자 가이드

## 프로젝트 구조

```
{PROJECT_ROOT}/
├── cpct-core/              # 공유 파싱 인프라
│   └── src/
│       ├── token.h             # 토큰 타입 정의, 키워드 맵
│       ├── lexer.h / lexer.cpp # 렉서 (소스 → 토큰)
│       ├── ast.h               # AST 노드 정의 + 팩토리 함수
│       ├── parser.h / parser.cpp   # 파서 (토큰 → AST)
│       └── preprocessor.h     # 전처리기 (#define, #undef)
├── cpct-cpp-lib/           # C++ 타입 라이브러리 (트랜스파일된 코드용)
│   └── cpct/
│       ├── types.h             # 전체 umbrella header
│       ├── io.h                # I/O umbrella header
│       ├── platform.h          # 플랫폼별 fast 타입 크기 설정
│       ├── data/type/          # 스칼라 타입 (Int, Float, String, Bool, Char, IntBig, BigInt)
│       ├── data/structure/     # 자료구조 (Array, Vector, Dict, Map)
│       ├── data/               # 유틸리티 (math, format, cast, compare)
│       └── io/                 # I/O (console)
├── cpct-interpreter/       # 트리워킹 인터프리터 (폐기 예정)
│   └── src/
│       ├── main.cpp            # 진입점 (REPL + 파일 실행)
│       ├── interpreter.h / interpreter.cpp # 인터프리터 (AST 실행)
│       └── bigint.h / bigint.cpp   # BigInt 구현
├── cpct-transpiler/        # C% → C++ 변환기
│   └── src/
│       ├── translator.h        # AST → C++ 코드 생성
│       ├── analyzer.h          # 정적 분석 (move-after-use 등)
│       ├── compiler.h          # 빌드 매니저 (g++ 호출)
│       └── main.cpp            # CLI 진입점
├── cpct-jit/               # Cling JIT REPL
│   └── src/
│       └── cling_repl.cpp      # Translator + Cling 파이프 연동
├── cpct-vscode/            # VSCode 확장 (구문 강조)
├── tests/                  # 라이브러리 테스트
├── docs/                   # 사용자 문서
│   └── builtin/            # 내장 함수 문서
├── docs_dev/               # 개발자 문서
├── examples/               # 예제 코드 (.cpc)
├── build.sh                # 인터프리터 빌드 스크립트
└── CMakeLists.txt          # CMake 빌드 (대안)
```

## 실행 방식

C%는 3가지 실행 방식을 지원한다:

| 방식 | 실행 파일 | 속도 | 의존성 |
| ---- | --------- | ---- | ------ |
| 인터프리터 | `cpct.exe` | 보통 (트리워킹) | 없음 |
| 트랜스파일러 | `cpct-translate.exe` + g++ | 빠름 (네이티브) | g++ |
| Cling JIT | `cpct-jit.exe` | 빠름 (JIT) | conda + cling |

## 빌드 방법

### 인터프리터
```bash
bash build.sh
```

### 트랜스파일러
```bash
g++ -std=c++20 -I cpct-core/src \
    -o cpct-translate.exe \
    cpct-transpiler/src/main.cpp \
    cpct-core/src/lexer.cpp \
    cpct-core/src/parser.cpp \
    cpct-interpreter/src/bigint.cpp
```

### Cling JIT REPL (conda + cling 필요)
```bash
conda create -n cpct-cling -c conda-forge cling -y
g++ -std=c++20 -I cpct-core/src \
    -o cpct-jit.exe \
    cpct-jit/src/cling_repl.cpp \
    cpct-core/src/lexer.cpp \
    cpct-core/src/parser.cpp \
    cpct-interpreter/src/bigint.cpp
```

## 아키텍처

### 인터프리터 파이프라인
```
소스 코드 → [Preprocessor] → [Lexer] → [Parser] → AST → [Interpreter] → 실행
```

### 트랜스파일러 파이프라인
```
소스 코드 → [Preprocessor] → [Lexer] → [Parser] → AST
    → [Analyzer] → 정적 분석 (move-after-use 등)
    → [Translator] → C++ 소스
    → [g++] → 실행 파일
```

### Cling JIT 파이프라인
```
사용자 입력 → [Translator] → C++ 코드 → [Cling JIT] → 즉시 실행
```

### 공유 컴포넌트 (cpct-core)

| 모듈 | 파일 | 역할 |
| ---- | ---- | ---- |
| Preprocessor | `preprocessor.h` | `#define`/`#undef` 텍스트 치환 |
| Lexer | `lexer.h/.cpp` | 소스 → 토큰 |
| Parser | `parser.h/.cpp` | 토큰 → AST |
| AST | `ast.h` | 표현식/문장 노드 정의 |

### Analyzer (정적 분석)

AST를 순회하며 트랜스파일 전에 오류를 감지한다:
- **move-after-use**: `let`으로 이동된 변수 사용 시 컴파일 에러
- 재대입으로 이동된 변수 복원 가능

### Translator (코드 생성)

AST → C++20 코드 변환. 주요 변환:
- 타입 매핑: `int` → `Int`, `string` → `String`
- `ref` → `&`, `let` → `std::move`
- `f"..."` → `cpct::format(...)`
- `**` → `cpct::pow()`, `/%` → `cpct::divmod()`
- 연쇄 비교 → `&&` 체인
- string switch → if-else 체인
- 비교 연산 → `cpct::cmp_*` (signed/unsigned 안전)
- script mode → `int main()` 자동 래핑

## 새 기능 추가 가이드

### 새 기능 추가 시 수정 대상

| 단계 | 파일 | 비고 |
| ---- | ---- | ---- |
| 토큰 | `cpct-core/src/token.h` | 키워드/연산자 등록 |
| 파싱 | `cpct-core/src/parser.cpp` | AST 생성 |
| AST | `cpct-core/src/ast.h` | 노드 정의 |
| 인터프리터 | `cpct-interpreter/src/interpreter.cpp` | 실행 (폐기 예정) |
| 라이브러리 | `cpct-cpp-lib/cpct/...` | C++ 타입/함수 |
| 트랜스파일러 | `cpct-transpiler/src/translator.h` | C++ 코드 생성 |
| 분석기 | `cpct-transpiler/src/analyzer.h` | 정적 분석 (필요 시) |

> **참고**: cpct-interpreter는 폐기 예정. 새 기능은 cpct-cpp-lib + cpct-transpiler에만 구현하는 것을 권장.

## VSCode 확장

`cpct-vscode/` 디렉토리에 위치. TextMate 문법 기반 구문 강조.

### 설치
```bash
cp -r cpct-vscode/* ~/.vscode/extensions/cpct-language/
# VSCode 재시작
```
