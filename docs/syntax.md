# rC% 기본 문법

## 주석

```
// 한 줄 주석

/* 블록 주석
   여러 줄 가능 */
```

## 변수 선언

```
타입 이름 = 값;
타입 이름;          // 기본값으로 초기화
```

```
int x = 10;
float pi = 3.14;
string name = "World";
bool flag = true;
char ch = 'A';
int8 small = 127;
```

타입 목록: `int`, `int8`, `int16`, `int32`, `int64`, `uint`, `uint8`, `uint16`, `uint32`, `uint64`, `intbig`, `bigint`, `float`, `float32`, `float64`, `char`, `string`, `bool`

자세한 내용은 [datatype.md](datatype.md) 참고.

## 출력

```
print(값);               // 줄바꿈 없이 출력
println(값);             // 줄바꿈 포함 출력
print(값1, 값2, 값3);    // 공백으로 구분하여 출력 (줄바꿈 없음)
println(값1, 값2, 값3);  // 공백으로 구분하여 출력 (줄바꿈 포함)
```

```
println("Hello, World!");  // Hello, World!\n
print("x =", x);           // x = 10 (줄바꿈 없음)
println(1, 2, 3);           // 1 2 3\n
```

## 입력

```
string s = input();      // 한 줄 입력받기
```

## 산술 연산

```
int a = 10 + 3;     // 13
int b = 10 - 3;     // 7
int c = 10 * 3;     // 30
int d = 10 / 3;     // 3 (정수 나눗셈)
int e = 10 % 3;     // 1 (나머지)
int f = 2 ** 10;    // 1024 (제곱)
```

- `**`는 우결합: `2 ** 3 ** 2` = `2 ** (3 ** 2)` = `2 ** 9` = `512`
- 음수 지수: `2 ** -1` = `0.5` (float 변환)

자세한 내용은 [operators.md](operators.md) 참고.

## 대입 연산자

```
x = 10;
x += 3;     // x = x + 3
x -= 3;     // x = x - 3
x *= 3;     // x = x * 3
x /= 3;     // x = x / 3
x %= 3;     // x = x % 3
```

## 증감 연산자

```
x++;    // 후위 증가 (현재 값 반환 후 +1)
++x;    // 전위 증가 (+1 후 값 반환)
x--;    // 후위 감소
--x;    // 전위 감소
```

## 비교 / 논리 연산

```
x == y    // 같음
x != y    // 다름
x < y     x > y     x <= y    x >= y

// 연쇄 비교
0 < x < 10           // (0 < x) && (x < 10)
0 <= x <= 100        // (0 <= x) && (x <= 100)
0 < a < b < c < 100  // 3개 이상 체이닝 가능

a && b    // AND
a || b    // OR
!a        // NOT
```

## 삼항 연산자

```
int max = x > y ? x : y;

// 중첩 가능 (우결합)
string grade = score >= 90 ? "A" : score >= 80 ? "B" : "C";
```

## 조건문

```
if (조건) {
    // ...
}

if (조건) {
    // ...
} else {
    // ...
}

if (조건1) {
    // ...
} else if (조건2) {
    // ...
} else {
    // ...
}
```

## 반복문

### while

```
while (조건) {
    // ...
}
```

### do-while

```
do {
    // ... (최소 1회 실행)
} while (조건);
```

```
int i = 0;
do {
    println(i);
    i++;
} while (i < 5);
```

- 본문을 먼저 실행한 후 조건을 검사
- 조건이 처음부터 `false`여도 최소 1회 실행
- `break`, `continue` 사용 가능

### for

#### C 스타일

```
for (int i = 0; i < 10; i++) {
    println(i);
}
```

#### for-each (범위 기반)

```
int arr[5] = [10, 20, 30, 40, 50];
for (int x : arr) {       // 타입 명시
    println(x);
}

for (x : arr) {            // 타입 생략
    println(x);
}

string s = "hello";
for (string c : s) {       // 문자열 순회
    print(c, " ");
}
```

#### for(n) — n번 반복

```
for(5) {
    print("x");            // xxxxx
}
```

#### 연쇄 비교 for

```
for (0 <= i < 10) {        // i: 0~9, 오름차순
    print(i, " ");
}

for (10 > i >= 0) {        // i: 9~0, 내림차순
    print(i, " ");
}

for (0 < i < 5) {          // i: 1~4 (strict 경계)
    print(i, " ");
}
```

### break / continue

```
for (int i = 0; i < 100; i++) {
    if (i % 2 == 0) continue;   // 짝수 건너뛰기
    if (i > 10) break;          // 10 초과 시 종료
    println(i);
}
```

## switch/case

```
switch (표현식) {
    case 값1:
        // ...
        break;
    case 값2:
        // ...
        break;
    default:
        // ...
        break;
}
```

```
int x = 2;
switch (x) {
    case 1:
        println("one");
        break;
    case 2:
        println("two");
        break;
    default:
        println("other");
        break;
}
```

- `int`, `string`, `bool` 타입 매칭 지원
- `float` 타입은 사용 불가 (부동소수점 정밀도 문제로 금지, C/C++과 동일)
- `break` 없으면 다음 case로 fall-through (C/C++과 동일)
- `default`는 어떤 case에도 매칭되지 않을 때 실행
- 루프 안에서 `continue` 사용 시 switch를 빠져나와 다음 반복으로 진행

## 함수

```
반환타입 함수이름(타입 매개변수1, 타입 매개변수2) {
    // ...
    return 값;
}
```

```
int add(int a, int b) {
    return a + b;
}

print(add(3, 4));    // 7
```

- 재귀 호출 지원
- `void` 반환 타입 사용 가능

```
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

## 배열

```
int arr[] = [1, 2, 3, 4, 5];    // 크기 = 초기화자 개수
int arr2[5];                     // 기본값(0)으로 초기화
int arr3[5] = [1, 2, 3];        // 부족분은 기본값 패딩

print(arr[0]);       // 1
print(len(arr));     // 5
arr[0] = 10;         // 원소 대입
```

## 문자열

### 일반 문자열

```
string s = "hello\nworld";    // \n은 줄바꿈으로 처리
```

이스케이프 시퀀스: `\n` (줄바꿈), `\t` (탭), `\\` (역슬래시), `\"` (따옴표), `\0` (널), `\r` (캐리지 리턴)

### Raw 문자열

```
string path = r"C:\Users\test\newfile.txt";
// 이스케이프 시퀀스를 처리하지 않음
// \n, \t 등이 그대로 출력됨
```

### F-string (포맷 문자열)

```
string name = "World";
int age = 25;

print(f"Hello, {name}!");               // Hello, World!
print(f"Age: {age}, Double: {age * 2}");  // Age: 25, Double: 50
print(f"2 + 3 = {2 + 3}");              // 2 + 3 = 5
print(f"len = {len("hello")}");          // len = 5
```

- `{}` 안에 임의의 표현식 사용 가능 (변수, 연산, 함수 호출 등)

## 내장 함수

| 함수               | 설명                       | 예시                           |
| ------------------ | -------------------------- | ------------------------------ |
| `len(x)`         | 배열/문자열 길이           | `len("hello")` → `5`      |
| `int(x)`         | 정수로 변환                | `int("42")` → `42`        |
| `float(x)`       | 실수로 변환                | `float("3.14")` → `3.14`  |
| `string(x)`      | 문자열로 변환              | `string(42)` → `"42"`     |
| `bool(x)`        | 불리언으로 변환            | `bool(0)` → `false`       |
| `type(x)`        | 타입 이름 반환             | `type(42)` → `"int"`      |
| `size(x)`        | 바이트 크기 반환           | `size(42)` → `8`          |
| `shape(x)`       | 배열 차원 크기 반환        | `shape(m)` → `[3, 4]`     |
| `divmod(a, b)`   | 몫과 나머지 배열 반환      | `divmod(7, 3)` → `[2, 1]` |
| `input()`        | 한 줄 입력                 | `string s = input();`        |
| `keys(d)`        | dict 키 배열               | `keys(scores)`               |
| `values(d)`      | dict 값 배열               | `values(scores)`             |
| `has(d, k)`      | dict 키 존재 여부          | `has(scores, "alice")`       |
| `remove(d, k)`   | dict 키 삭제               | `d.remove("alice")`          |
| `v.push(x)`      | vector 끝에 원소 추가      | `v.push(42)`                 |
| `v.pop()`        | vector 끝 원소 제거 (반환) | `int x = v.pop()`            |
| `v.insert(i, x)` | vector i번째에 삽입        | `v.insert(0, 99)`            |
| `v.erase(i)`     | vector i번째 원소 제거     | `v.erase(0)`                 |
| `v.clear()`      | vector 전체 삭제           | `v.clear()`                  |
| `v.is_empty()`   | vector 비었는지 확인       | `v.is_empty()` → `true`   |
| `v.front()`      | vector 첫 번째 원소        | `v.front()`                  |
| `v.back()`       | vector 마지막 원소         | `v.back()`                   |
| `split(s, sep)`  | 문자열 분할 (배열 반환)    | `split("a-b", "-")` → `["a", "b"]` |
| `join(sep, arr)` | 배열을 문자열로 결합       | `join("-", ["a", "b"])` → `"a-b"` |
| `upper(s)`       | 대문자로 변환              | `upper("hello")` → `"HELLO"` |
| `lower(s)`       | 소문자로 변환              | `lower("HELLO")` → `"hello"` |
| `find(s, sub)`   | 부분문자열 위치 (-1=없음)  | `find("abc", "bc")` → `1`   |
| `replace(s, o, n)` | 문자열 치환 (전체)       | `replace("ab", "a", "x")` → `"xb"` |
| `trim(s)`        | 앞뒤 공백 제거             | `trim("  hi  ")` → `"hi"`   |
| `substr(s, i, n)` | 부분문자열 추출           | `substr("abcd", 1, 2)` → `"bc"` |
| `reverse(s)`     | 문자열 뒤집기              | `reverse("abc")` → `"cba"`  |
| `is_contains(s, sub)` | 부분문자열 포함 여부  | `is_contains("hello", "ell")` → `true` |
| `is_starts_with(s, p)` | 접두사 일치 여부     | `is_starts_with("hello", "he")` → `true` |
| `is_ends_with(s, p)` | 접미사 일치 여부       | `is_ends_with("hello", "lo")` → `true` |
| `is_digit(s)`    | 모든 문자가 숫자인지       | `is_digit("123")` → `true`  |
| `is_alpha(s)`    | 모든 문자가 알파벳인지     | `is_alpha("abc")` → `true`  |
| `is_upper(s)`    | 모든 문자가 대문자인지     | `is_upper("ABC")` → `true`  |
| `is_lower(s)`    | 모든 문자가 소문자인지     | `is_lower("abc")` → `true`  |

## 실행 방법

```bash
# REPL 모드
./cpct.exe

# 파일 실행
./cpct.exe 파일경로.cpc
```

## 예제: FizzBuzz

```
for (int i = 1; i <= 30; i++) {
    if (i % 15 == 0) {
        print("FizzBuzz");
    } else if (i % 3 == 0) {
        print("Fizz");
    } else if (i % 5 == 0) {
        print("Buzz");
    } else {
        print(i);
    }
}
```
