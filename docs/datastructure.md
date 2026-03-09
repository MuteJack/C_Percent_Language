# C% 자료구조

## 배열

### 개요

- **고정 크기** 배열 (1차원 및 다차원)
- 선언 시 크기가 결정되며, 이후 크기 변경 불가
- 모든 원소는 **동일한 타입**

### 선언

```
타입 이름[] = [값1, 값2, ...];     // 크기 = 초기화자 개수
타입 이름[크기];                    // 기본값으로 초기화
타입 이름[크기] = [값1, 값2, ...];  // 부족분은 기본값으로 패딩
```

#### 예시

```
int arr[] = [1, 2, 3];          // 크기 3
float data[] = [1.0, 2.5, 3.7]; // 크기 3

int arr[5];                      // [0, 0, 0, 0, 0]
float32 f[3];                    // [0.0, 0.0, 0.0]
bool flags[4];                   // [false, false, false, false]
char buf[10];                    // [0, 0, ..., 0] (널 문자)
string names[3];                 // ["", "", ""]

int arr[5] = [1, 2, 3];         // [1, 2, 3, 0, 0] (부족분 0 패딩)
float arr[4] = [1.0];           // [1.0, 0.0, 0.0, 0.0]
```

#### 에러 케이스

```
int arr[];                       // Error: 크기 없음, 초기화자 없음
int arr[3] = [1, 2, 3, 4];      // Error: 초기화자가 크기보다 많음
```

### 지원 타입

| 원소 타입   | 기본값    | 네이티브 크기 | 인터프리터 크기 | 비고                    |
| ----------- | --------- | ------------- | --------------- | ----------------------- |
| `int`     | `0`     | 4 bytes  | 8 bytes (int64_t) | int32 고정, wrap-around |
| `int8`    | `0`     | 1 byte   | 8 bytes (int64_t) | 오버플로 시 wrap-around |
| `int16`   | `0`     | 2 bytes  | 8 bytes (int64_t) | 오버플로 시 wrap-around |
| `int32`   | `0`     | 4 bytes  | 8 bytes (int64_t) | 오버플로 시 wrap-around |
| `int64`   | `0`     | 8 bytes  | 8 bytes (int64_t) | 오버플로 시 wrap-around |
| `intbig`  | `0`     | 가변     | 8B 또는 가변     | int64 + BigInt 자동 승격 |
| `bigint`  | `0`     | 가변     | 가변 (BigInt)    | 무제한 정수 (BigInt)     |
| `float`   | `0.0`   | 8 bytes  | 8 bytes (double) | `float64` alias       |
| `float32` | `0.0`   | 4 bytes  | 8 bytes (double) | 단정밀도 절삭           |
| `float64` | `0.0`   | 8 bytes  | 8 bytes (double) | 배정밀도                |
| `char`    | `0`     | 1 byte   | 8 bytes (int64_t) | 내부 정수, 출력 시 문자 |
| `bool`    | `false` | 1 byte   | 8 bytes (int64_t)¹ | ¹variant 내 bool 저장  |
| `string`  | `""`    | 가변     | 32 bytes + 힙²   | ²std::string SSO 포함  |

> **참고**: 인터프리터(트리워킹) 구현에서 스칼라 변수는 `std::variant` 기반 `CmmValue`에 저장되므로,
> 모든 스칼라가 variant의 최대 멤버 크기(≈40~48B)를 차지한다. 네이티브 크기는 `size()` 내장 함수가
> 반환하는 언어 사양 기준 크기이며, 인터프리터 크기는 실제 C++ 런타임 저장 크기이다.
> 배열(TypedArray)은 네이티브 크기로 저장되므로 이 차이가 없다.

### 원소 접근

```
int arr[] = [10, 20, 30, 40, 50];
print(arr[0]);       // 10
print(arr[2]);       // 30
print(arr[-1]);      // 50 (마지막 원소)
print(arr[-2]);      // 40 (뒤에서 두 번째)
print(arr[-5]);      // 10 (첫 번째)
print(arr[5]);       // Error: 인덱스 범위 초과
print(arr[-6]);      // Error: 인덱스 범위 초과
```

- 인덱스는 `0`부터 시작
- 음수 인덱스: `arr[-n]`은 `arr[len(arr) - n]`과 동일
- 유효 범위: `-len(arr) <= index < len(arr)` (그 외 런타임 에러)

### 원소 대입

```
int arr[] = [1, 2, 3];
arr[0] = 10;
arr[2] = 30;
print(arr);          // [10, 2, 30]
```

#### 복합 대입 연산자

```
arr[0] += 5;         // arr[0] = arr[0] + 5
arr[1] -= 1;         // arr[1] = arr[1] - 1
arr[2] *= 2;         // arr[2] = arr[2] * 2
arr[0] /= 3;         // arr[0] = arr[0] / 3
arr[1] %= 2;         // arr[1] = arr[1] % 2
```

### 타입 강제

배열 원소에 대한 타입 강제는 스칼라 변수와 동일한 규칙을 따른다.

```
// 암시적 변환 (스칼라와 동일)
int arr[] = [1, 2.7, 3];       // [1, 2, 3] — float→int 절삭
float arr[] = [1, 2, 3];       // [1.0, 2.0, 3.0] — int→float

// sized 정수 wrap-around
int8 arr[] = [127, 128, 200];  // [127, -128, -56]
int32 arr[2] = [2147483648];   // [-2147483648, 0]

// float32 정밀도 절삭
float32 arr[] = [3.14159265358979];  // [3.1415927]

// 에러
int arr[] = [1, "hello", 3];   // Error: string → int 변환 불가
```

대입 시에도 동일:

```
int8 arr[] = [0, 0];
arr[0] = 200;                  // arr[0] == -56 (wrap-around)

float32 f[] = [0.0];
f[0] = 3.14159265358979;       // f[0] == 3.1415927 (절삭)
```

### 내장 함수

배열에서 사용 가능한 내장 함수는 [builtin/datatype.md](builtin/datatype.md) 참조.

- `len(arr)` — 배열 길이
- `shape(arr)` — 각 차원 크기 배열 (`shape(m)` → `[3, 4]`)
- `type(arr)` — 타입 이름 (`"int[]"`, `"float32[][]"` 등)
- `size(arr)` — 네이티브 기준 바이트 크기

### 출력

```
int arr[] = [1, 2, 3];
print(arr);          // [1, 2, 3]
print(arr[0]);       // 1

string s[] = ["hello", "world"];
print(s);            // ["hello", "world"]
```

### 반복문에서 사용

#### for-each

```
int arr[] = [10, 20, 30, 40, 50];
for (int x : arr) {
    println(x);              // 10, 20, 30, 40, 50
}

for (x : arr) {              // 타입 생략 가능
    println(x);
}
```

#### C 스타일

```
int arr[] = [10, 20, 30, 40, 50];
for (int i = 0; i < len(arr); i++) {
    println(arr[i]);
}
```

### 범위 접근

`arr[start:end]`로 배열의 부분 범위에 접근한다. `end`는 미포함(exclusive).

```
int arr[] = [10, 20, 30, 40, 50];

print(arr[0:3]);     // [10, 20, 30]   — 인덱스 0, 1, 2
print(arr[1:4]);     // [20, 30, 40]   — 인덱스 1, 2, 3
print(arr[2:5]);     // [30, 40, 50]   — 인덱스 2, 3, 4
```

#### 규칙

- `start`와 `end`는 정수 표현식
- 음수 사용 가능: 음수는 `len(arr) + 값`으로 변환
- 변환 후 `[0, len(arr)]` 범위로 클램핑 (에러 없음)
- `start >= end`이면 빈 배열 반환
- 결과는 새로운 배열 (원본에 영향 없음)

```
int arr[] = [10, 20, 30, 40, 50];
print(arr[-3:-1]);   // [30, 40]         — 뒤에서 3번째 ~ 뒤에서 1번째 직전
print(arr[-2:5]);    // [40, 50]         — 음수와 양수 혼합 가능
print(arr[0:-1]);    // [10, 20, 30, 40] — 마지막 제외
print(arr[0:100]);   // [10, 20, 30, 40, 50] — end 클램핑
print(arr[-100:3]);  // [10, 20, 30]     — start 클램핑
print(arr[3:1]);     // []               — start >= end → 빈 배열
print(arr[-1:-4]);   // []               — 변환 후 [4:0] → 빈 배열
```

#### 다차원 배열에서

```
int matrix[][] = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
print(matrix[0:2]);          // [[1, 2, 3], [4, 5, 6]] — 행 범위
print(matrix[0][1:3]);       // [2, 3] — 특정 행의 열 범위
print(matrix[-1]);           // [7, 8, 9] — 마지막 행
```

### 다차원 배열

#### 선언

`[]`를 중첩하여 다차원 배열을 선언한다.

```
타입 이름[][] = [[값, ...], [값, ...], ...];     // 크기 추론
타입 이름[행][열];                                 // 기본값 초기화
타입 이름[행][열] = [[값, ...], [값, ...]];        // 부족분 패딩
```

#### 예시

```
// 2차원 배열 — 리터럴 초기화
int matrix[][] = [[1, 2, 3], [4, 5, 6]];     // 2x3

// 2차원 배열 — 크기 지정
int grid[3][4];                               // 3x4, 모두 0

// 2차원 배열 — 크기 + 부분 초기화
int m[3][3] = [[1, 2], [3]];                  // [[1,2,0], [3,0,0], [0,0,0]]

// 3차원 배열
int cube[2][3][4];                            // 2x3x4, 모두 0
```

#### 원소 접근 및 대입

```
int matrix[][] = [[1, 2, 3], [4, 5, 6]];
print(matrix[0][1]);     // 2
print(matrix[1][2]);     // 6

matrix[0][0] = 10;
matrix[1][2] += 4;
print(matrix);           // [[10, 2, 3], [4, 5, 10]]
```

#### 행 접근

```
int matrix[][] = [[1, 2, 3], [4, 5, 6]];
print(matrix[0]);        // [1, 2, 3] (행 단위 접근)
print(len(matrix));      // 2 (행 수)
print(len(matrix[0]));   // 3 (열 수)
```

#### 규칙

- 각 행의 열 수는 동일해야 한다 (직사각형 배열)
- `len()`은 해당 차원의 크기를 반환
- 타입 강제 규칙은 1차원과 동일
- `type()`은 `"int[][]"`, `"float[][][]"` 등 차원 수를 반영

#### 에러 케이스

```
int m[][] = [[1, 2], [3, 4, 5]];   // Error: 행 길이 불일치 (2 vs 3)
int m[][];                          // Error: 크기 없음, 초기화자 없음
int m[2][3] = [[1,2,3,4], [5,6]];  // Error: 초기화자가 열 크기보다 많음
```

### 제한사항

- **고정 크기**: 선언 후 크기 변경 불가 (`push`, `pop` 없음)
- **크기 표현식**: `타입 이름[n]`에서 `n`은 양의 정수로 평가되는 표현식

---

## dict (비타입 딕셔너리)

### 개요

- **동적 크기** 키-값 매핑
- 삽입 순서 유지
- 키와 값의 타입 제한 없음 (Python의 dict와 유사)
- 하나의 dict에 서로 다른 타입의 키/값 혼용 가능

### 선언

```
dict 이름 = {키1: 값1, 키2: 값2, ...};
dict 이름;                              // 빈 dict
dict 이름 = {};                          // 빈 dict
```

#### 예시

```
dict scores = {"alice": 95, "bob": 87};
dict mixed = {1: "one", "two": 2, true: 3.14};  // 타입 혼용 가능
dict empty = {};
dict flags;
```

### 키 타입

| 허용 키 타입 | 비고 |
| ------------ | ---- |
| `string` | 가장 일반적 |
| `int`, `int8`~`int64` | 정수 키 |
| `char` | 문자 키 |
| `bool` | 2개 키만 가능 |

- `float`, 배열, dict는 키로 사용 불가 (해시 불가)
- 서로 다른 타입의 키는 구분됨: `d[1]`과 `d["1"]`은 다른 키

### 값 타입

모든 스칼라 타입 허용, 타입 혼용 가능

### 접근

```
dict scores = {"alice": 95, "bob": 87};
print(scores["alice"]);      // 95
print(scores["unknown"]);    // Runtime Error: Key not found
```

### 대입

```
// 기존 키 수정
scores["alice"] = 100;

// 새 키 추가 (자동, 어떤 타입이든 가능)
scores["charlie"] = 92;
scores[42] = "answer";

// 복합 대입
scores["bob"] += 10;
```

- 없는 키에 대입 → 새 항목 추가
- 없는 키를 읽기 → 런타임 에러

### 내장 함수

```
dict d = {"a": 1, "b": 2, "c": 3};

d.len()          // 3 (키 개수)
d.size()         // 15 (키 바이트 합 + 값 바이트 합)
d.type()         // "dict"
d.keys()         // ["a", "b", "c"]
d.values()       // [1, 2, 3]
d.has("a")       // true
d.has("z")       // false
d.remove("b")    // "b" 키 삭제
```

모든 내장 함수는 자유 함수와 메서드 스타일 모두 지원:
`len(d)` ↔ `d.len()`, `has(d, "key")` ↔ `d.has("key")`

### 출력

```
dict d = {"alice": 95, "bob": 87};
print(d);        // {"alice": 95, "bob": 87}
```

### 반복문에서 사용

```
dict scores = {"alice": 95, "bob": 87, "charlie": 92};

// for-each: 키 순회
for (name : scores) {
    println(name, ": ", scores[name]);
}
```

- for-each로 dict를 순회하면 **키**가 반복 변수에 대입됨
- 삽입 순서대로 순회

---

## map (타입 지정 맵)

### 개요

- **동적 크기** 키-값 매핑 (C++ `std::unordered_map`과 유사)
- 삽입 순서 유지
- 키와 값의 타입이 선언 시 고정
- 타입 안전성과 성능이 `dict`보다 우수

### 선언

```
map<키타입, 값타입> 이름 = {키1: 값1, 키2: 값2, ...};
map<키타입, 값타입> 이름;                              // 빈 map
map<키타입, 값타입> 이름 = {};                          // 빈 map
```

#### 예시

```
map<string, int> scores = {"alice": 95, "bob": 87};
map<int, string> names = {1: "one", 2: "two"};
map<string, float> empty = {};
map<string, bool> flags;
```

### 키 타입 제한

| 허용 키 타입 | 비고 |
| ------------ | ---- |
| `string` | 가장 일반적 |
| `int`, `int8`~`int64` | 정수 키 |
| `char` | 문자 키 |
| `bool` | 2개 키만 가능 |

- `float`, 배열, dict는 키로 사용 불가 (해시 불가)

### 값 타입

모든 스칼라 타입 허용: `int`, `float`, `string`, `char`, `bool` 등
선언된 타입 외의 값 대입 시 타입 강제(coercion) 적용

### 타입 강제

```
map<string, int> m = {"a": 3.7};    // 값 3.7 → 3 (float→int 절삭)
m["b"] = 2.9;                        // 값 2.9 → 2
map<int, float> m2 = {1: 10};       // 값 10 → 10.0 (int→float)
```

- 키와 값 모두 선언된 타입으로 강제 변환
- 변환 불가능한 타입 대입 시 런타임 에러

### 접근

```
map<string, int> scores = {"alice": 95, "bob": 87};
print(scores["alice"]);      // 95
print(scores["unknown"]);    // Runtime Error: Key not found
```

### 대입

```
// 기존 키 수정
scores["alice"] = 100;

// 새 키 추가 (자동)
scores["charlie"] = 92;

// 복합 대입
scores["bob"] += 10;
```

- 없는 키에 대입 → 새 항목 추가
- 없는 키를 읽기 → 런타임 에러

### 내장 함수

```
map<string, int> d = {"a": 1, "b": 2, "c": 3};

d.len()          // 3 (키 개수)
d.size()         // 15 (키 바이트 합 + 값 바이트 합: 1+1+1 + 4+4+4)
d.type()         // "map<string,int>"
d.keys()         // ["a", "b", "c"]
d.values()       // [1, 2, 3]
d.has("a")       // true
d.has("z")       // false
d.remove("b")    // "b" 키 삭제
```

모든 내장 함수는 자유 함수와 메서드 스타일 모두 지원:
`len(d)` ↔ `d.len()`, `has(d, "key")` ↔ `d.has("key")`

### 출력

```
map<string, int> d = {"alice": 95, "bob": 87};
print(d);        // {"alice": 95, "bob": 87}
```

### 반복문에서 사용

```
map<string, int> scores = {"alice": 95, "bob": 87, "charlie": 92};

// for-each: 키 순회
for (string name : scores) {
    println(name, ": ", scores[name]);
}

// C 스타일: keys() 사용
string k[] = keys(scores);
for (int i = 0; i < len(k); i++) {
    println(k[i], ": ", scores[k[i]]);
}
```

- for-each로 map을 순회하면 **키**가 반복 변수에 대입됨
- 삽입 순서대로 순회

---

## dict vs map 비교

| 특성 | `dict` | `map<K,V>` |
| ---- | ------ | ---------- |
| 타입 지정 | 없음 (자유) | 키/값 타입 고정 |
| 타입 혼용 | 가능 | 불가 (coercion 적용) |
| 타입 안전성 | 낮음 | 높음 |
| 성능 | 태그 해싱 오버헤드 | 전용 인덱스로 최적화 |
| 용도 | 프로토타이핑, 유연한 데이터 | 성능/안전이 중요한 코드 |

### 제한사항

- dict를 값으로 가지는 중첩 dict/map은 향후 확장

---

## vector (동적 배열)

### 개요

- **동적 크기** 배열 (C++ `std::vector`와 유사)
- 원소 타입을 선언 시 명시
- `push`, `pop`, `insert`, `erase` 등 크기 변경 연산 지원
- 배열과 달리 선언 후에도 크기 변경 가능

### 선언

```
vector<타입> 이름 = [값1, 값2, ...];   // 초기화
vector<타입> 이름;                       // 빈 vector
vector<타입> 이름 = [];                  // 빈 vector
```

#### 예시

```
vector<int> v = [1, 2, 3, 4, 5];
vector<string> names = ["alice", "bob"];
vector<float> data;
vector<int> empty = [];
```

### 원소 접근

```
vector<int> v = [10, 20, 30, 40, 50];
print(v[0]);         // 10
print(v[2]);         // 30
print(v[-1]);        // 50 (마지막 원소)
print(v[-2]);        // 40 (뒤에서 두 번째)
print(v[5]);         // Error: 인덱스 범위 초과
```

- 인덱스는 `0`부터 시작
- 음수 인덱스: `v[-n]`은 `v[len(v) - n]`과 동일
- 유효 범위: `-len(v) <= index < len(v)` (그 외 런타임 에러)

### 원소 대입

```
vector<int> v = [1, 2, 3];
v[0] = 10;
v[2] = 30;
print(v);            // [10, 2, 30]

// 복합 대입
v[0] += 5;
v[1] -= 1;
v[2] *= 2;
```

### 크기 변경 연산

```
vector<int> v = [1, 2, 3];

// push — 끝에 추가
v.push(4);               // [1, 2, 3, 4]
v.push(5);               // [1, 2, 3, 4, 5]

// pop — 마지막 원소 제거 + 반환
int last = v.pop();      // last == 5, v == [1, 2, 3, 4]

// insert — 지정 위치에 삽입 (O(n))
v.insert(1, 99);         // [1, 99, 2, 3, 4]

// erase — 지정 위치 제거 (O(n))
v.erase(1);              // [1, 2, 3, 4]

// clear — 모든 원소 제거
v.clear();               // []
```

- `pop()`: 빈 vector에서 호출 시 런타임 에러
- `insert(i, val)`: `i`는 `0 <= i <= len(v)` (끝에 삽입 가능)
- `erase(i)`: `i`는 유효 인덱스 범위 내

### 내장 함수

```
vector<int> v = [1, 2, 3, 4, 5];

v.len()              // 5 (원소 개수)
v.empty()            // false (비어있는지)
v.front()            // 1 (첫 번째 원소)
v.back()             // 5 (마지막 원소)
v.type()             // "vector<int>"
v.size()             // 바이트 크기
```

모든 내장 함수는 자유 함수와 메서드 스타일 모두 지원:
`len(v)` ↔ `v.len()`, `empty(v)` ↔ `v.empty()`

### 타입 강제

배열과 동일한 규칙을 따른다.

```
vector<int> v = [1, 2.7, 3];       // [1, 2, 3] — float→int 절삭
vector<float> f = [1, 2, 3];       // [1.0, 2.0, 3.0] — int→float

vector<int8> s = [127, 128, 200];  // [127, -128, -56] — wrap-around

// 에러
vector<int> v = [1, "hello", 3];   // Error: string → int 변환 불가
```

대입/push 시에도 동일:

```
vector<int8> v = [0];
v[0] = 200;                        // v[0] == -56 (wrap-around)
v.push(300);                       // push 시에도 wrap-around 적용
```

### 출력

```
vector<int> v = [1, 2, 3];
print(v);            // [1, 2, 3]

vector<string> s = ["hello", "world"];
print(s);            // ["hello", "world"]
```

### 반복문에서 사용

#### for-each

```
vector<int> v = [10, 20, 30, 40, 50];
for (int x : v) {
    println(x);
}

for (x : v) {               // 타입 생략 가능
    println(x);
}
```

#### C 스타일

```
vector<int> v = [10, 20, 30, 40, 50];
for (int i = 0; i < len(v); i++) {
    println(v[i]);
}
```

### 범위 접근

배열과 동일한 슬라이싱 지원.

```
vector<int> v = [10, 20, 30, 40, 50];
print(v[0:3]);       // [10, 20, 30]
print(v[-2:5]);      // [40, 50]
print(v[3:1]);       // [] (빈 vector)
```

- 결과는 새로운 vector (원본에 영향 없음)
- 규칙은 배열의 범위 접근과 동일

### 배열 vs vector 비교

| 특성 | 배열 (`int arr[]`) | `vector<int>` |
| ---- | ------------------ | ------------- |
| 크기 | 고정 (선언 시 결정) | 동적 (변경 가능) |
| push/pop | 불가 | 가능 |
| insert/erase | 불가 | 가능 |
| 메모리 | 네이티브 크기 저장 | 동적 할당 |
| 인덱스 접근 | O(1) | O(1) |
| 용도 | 크기 고정 데이터 | 크기 가변 데이터 |

### 제한사항

- `vector<vector<int>>` 같은 중첩 vector는 향후 확장
