# C% Interpreter - 개발자 가이드

## 프로젝트 구조

```
{PROJECT_ROOT}/
├── cpct-interpreter/       # 인터프리터
│   └── src/                # 소스 코드
│       ├── main.cpp            # 진입점 (REPL + 파일 실행)
│       ├── token.h             # 토큰 타입 정의, 키워드 맵
│       ├── lexer.h / lexer.cpp # 렉서 (소스 → 토큰)
│       ├── ast.h               # AST 노드 정의 + 팩토리 함수
│       ├── parser.h / parser.cpp   # 파서 (토큰 → AST)
│       ├── interpreter.h / interpreter.cpp # 인터프리터 (AST 실행)
│       ├── bigint.h / bigint.cpp   # BigInt 구현 (임의 정밀도 정수)
├── cpct-vscode/            # VSCode 확장 (구문 강조)
├── docs/                   # 사용자 문서
│   └── builtin/            # 내장 함수 문서
├── docs_dev/               # 개발자 문서
├── examples/               # 예제 코드 (.cpc)
├── build.sh                # 빌드 스크립트
├── CMakeLists.txt          # CMake 빌드 (대안)
└── cpct.exe                # 빌드 결과물
```

## 빌드 방법

### 요구 사항
- C++17 이상 지원 컴파일러 (g++, clang++)
- Windows: msys2 환경 권장

### 빌드
```bash
# msys2 bash에서
C:/msys64/usr/bin/bash.exe -l -c "cd {PROJECT_ROOT} && bash build.sh"

# 또는 직접 컴파일
g++ -std=c++17 -O2 -Wall -Wextra cpct-interpreter/src/main.cpp cpct-interpreter/src/lexer.cpp cpct-interpreter/src/parser.cpp cpct-interpreter/src/interpreter.cpp cpct-interpreter/src/bigint.cpp -Icpct-interpreter/src -o cpct.exe
```

### 실행
```bash
# REPL 모드
./cpct.exe

# 파일 실행
./cpct.exe examples/hello.cpc
```

## 아키텍처

Tree-walking interpreter 구조:

```
소스 코드 (.cpc)
    ↓
[Lexer] 문자열 → 토큰 스트림
    ↓
[Parser] 토큰 → AST (Abstract Syntax Tree)
    ↓
[Interpreter] AST 순회 → 실행
```

### 1. Lexer (`lexer.h`, `lexer.cpp`)

소스 코드 문자열을 토큰으로 분리한다.

- `tokenize()` — 메인 루프. 문자를 읽고 적절한 토큰 생성
- `makeNumber()` — 정수/실수 리터럴
- `makeString()` — `"..."` 문자열 (이스케이프 처리)
- `makeRawString()` — `r"..."` raw 문자열 (이스케이프 없음)
- `makeFString()` — `f"..."` 포맷 문자열 (raw 저장, 파서에서 처리)
- `makeChar()` — `'x'` 문자 리터럴
- `makeIdentifierOrKeyword()` — 식별자 또는 예약어

토큰 추가 시: `token.h`의 `TokenType` enum에 추가 → `tokenTypeToString()`에 케이스 추가 → `lexer.cpp`의 `tokenize()`에서 인식 로직 작성.

### 2. Parser (`parser.h`, `parser.cpp`)

재귀 하강(recursive descent) 파서. 연산자 우선순위 기반 파싱.

**우선순위 체인** (낮은 것 → 높은 것):
```
parseAssignment → parseOr → parseAnd → parseEquality
→ parseComparison → parseAddition → parseMultiplication
→ parsePower → parseUnary → parsePostfix → parsePrimary
```

- 각 레벨은 자신보다 한 단계 높은 함수를 호출
- 새 연산자 추가 시: 해당 우선순위 레벨에 `check()` 조건 추가
- 새 우선순위 레벨 추가 시: 새 함수 작성 후 체인에 삽입

**AST 노드** (`ast.h`):
- `Expr` — 모든 표현식의 단일 구조체 (kind로 구분)
- `Stmt` — 모든 문장의 단일 구조체 (kind로 구분)
- 팩토리 함수: `makeIntLiteral()`, `makeBinaryOp()`, `makeVarDecl()` 등

### 3. Interpreter (`interpreter.h`, `interpreter.cpp`)

AST를 직접 순회하며 실행한다.

**런타임 값** (`CmmValue`):
```cpp
std::variant<int64_t, double, bool, std::string, std::vector<CmmValue>, BigInt, TypedArray>
```

- `int64_t` — 정수 (빠른 경로)
- `BigInt` — 오버플로 시 자동 승격되는 임의 정밀도 정수
- `double` — 실수
- `bool`, `string`, `vector<CmmValue>` — 기타 타입

**환경(Environment)**: 변수 스코프를 관리하는 체인 구조. `parent_` 포인터로 외부 스코프 참조.

**제어 흐름**: C++ 예외를 시그널로 사용
- `ReturnSignal` — 함수 반환
- `BreakSignal` — 반복문 탈출
- `ContinueSignal` — 반복문 계속

**정수 타입 시스템**:
- `int` — int32 고정 (오버플로 시 wrap-around)
- `int8/16/32/64` — 고정 크기 (오버플로 시 wrap-around)
- `intbig` — int64 fast path + BigInt 자동 승격
- `bigint` — 항상 BigInt (임의 정밀도)
- `char` — int8 범위, print 시 문자로 출력

**실수 타입 시스템**:
- `float`, `float64` — double 정밀도
- `float32` — 단정밀도 (`static_cast<float>` 후 다시 double 저장)

## 새 기능 추가 가이드

### 새 연산자 추가 (예: `**`)
1. `token.h` — `TokenType` enum에 추가 + `tokenTypeToString()`
2. `lexer.cpp` — `tokenize()`의 switch에서 문자 조합 인식
3. `parser.cpp` — 적절한 우선순위 레벨에서 `check()` 추가
4. `interpreter.cpp` — `evalBinaryOp()`의 각 경로(int64, BigInt, float)에 처리 추가

### 새 내장 함수 추가 (예: `len()`)
1. `interpreter.cpp` — `callFunction()`에 `if (name == "함수명")` 블록 추가
2. (선택) `cpct-vscode/syntaxes/cpct.tmLanguage.json` — `builtins` 패턴에 함수명 추가

### 새 데이터 타입 추가 (예: `char`)
1. `token.h` — `KW_타입명` 토큰 + 키워드 맵에 등록
2. `ast.h` — 필요 시 새 ExprKind (예: `CharLiteral`)
3. `lexer.cpp` — 리터럴 파싱 로직
4. `parser.cpp` — `parsePrimary()`에서 리터럴 처리, `isTypeKeyword()`에 추가
5. `interpreter.h` — 타입 판별 헬퍼 (예: `isCharType()`)
6. `interpreter.cpp` — `execVarDecl()`, `eval()`, `execPrint()` 등에서 처리

### 새 문(Statement) 추가 (예: `switch`)
1. `ast.h` — `StmtKind` enum에 추가 + 팩토리 함수
2. `parser.cpp` — `parseStatement()`에서 키워드 감지 + 파싱 함수 작성
3. `interpreter.cpp` — `execStmt()`에 케이스 추가 + 실행 함수 작성

## VSCode 확장

`cpct-vscode/` 디렉토리에 위치. TextMate 문법 기반 구문 강조.

### 설치
```bash
# extensions 폴더에 복사
cp -r cpct-vscode/* ~/.vscode/extensions/cpct-language/
# VSCode 재시작
```

### 수정 후 반영
```bash
cp -r cpct-vscode/* ~/.vscode/extensions/cpct-language/
# VSCode에서 Ctrl+Shift+P → "Developer: Reload Window"
```

### 문법 파일
- `syntaxes/cpct.tmLanguage.json` — 구문 강조 규칙
- `language-configuration.json` — 괄호 매칭, 주석, 들여쓰기
- `package.json` — 확장 메타데이터 (파일 확장자: `.cpc`)
