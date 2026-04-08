# C% 데이터 타입 내장 함수

> 모든 정보 함수는 자유 함수와 메서드 스타일 모두 지원한다.
> `type(x)` ↔ `x.type()` — 동일한 결과를 반환한다.

## 정보 함수

### `type(expr)` / `expr.type()`

변수 또는 표현식의 타입명을 문자열로 반환한다.

```
int x = 10;
print(type(x));          // "int"
print(x.type());         // "int"  (메서드 스타일)

float32 f = 3.14;
print(f.type());         // "float32"

int arr[] = [1, 2, 3];
print(arr.type());       // "int[]"

int m[][] = [[1,2],[3,4]];
print(type(m));          // "int[][]"
```

- 변수에 선언된 타입이 있으면 선언 타입을 반환 (`"int8"`, `"float32"`, `"int[]"` 등)
- 리터럴/표현식은 런타임 값에서 추론: `"int"`, `"bigint"`, `"float"`, `"bool"`, `"string"`, `"array"`

### `size(expr)` / `expr.size()`

변수 또는 표현식의 **네이티브 기준** 바이트 크기를 반환한다.

```
int x = 10;
print(size(x));          // 플랫폼 의존 (기본 8)
print(x.size());         // 플랫폼 의존 (기본 8, --platform으로 변경)

int64 y = 100;
print(y.size());         // 8

int8 z = 0;
print(z.size());         // 1

float f = 3.14;
print(f.size());         // 8

float32 g = 1.0;
print(size(g));          // 4

string s = "hello";
print(s.size());         // 5 (바이트 수)

int arr[] = [1, 2, 3];
print(arr.size());       // 12 (4 bytes × 3)
```

#### 타입별 반환값

| 타입 | `size()` 반환값 |
| ---- | --------------- |
| `bool` | 1 |
| `int8`, `char` | 1 |
| `int16` | 2 |
| `int` (`int16f`) | 플랫폼 의존 (기본 8, `--platform`으로 변경) |
| `int32` | 4 |
| `int64` | 8 |
| `intbig` | 8 (int64 범위) 또는 BigInt 바이트 수 |
| `bigint` | BigInt 바이트 수 |
| `float32` | 4 |
| `float`, `float64` | 8 |
| `string` | 문자열 바이트 수 |
| 배열 | 원소 크기 × 원소 수 |
| dict | 키 바이트 합 + 값 바이트 합 |

### `len(expr)` / `expr.len()`

배열의 길이 또는 문자열의 길이를 반환한다.

```
int arr[] = [10, 20, 30];
print(len(arr));         // 3
print(arr.len());        // 3  (메서드 스타일)

string s = "hello";
print(s.len());          // 5

int m[][] = [[1,2,3],[4,5,6]];
print(m.len());          // 2 (행 수)
print(m[0].len());       // 3 (열 수)
```

- 배열: 해당 차원의 원소 수
- 문자열: 바이트 수 (UTF-8 기준)
- 그 외 타입: 에러

### `shape(expr)` / `expr.shape()`

배열의 각 차원 크기를 배열로 반환한다. (MATLAB `size()` / NumPy `.shape`와 유사)

```
int arr[] = [10, 20, 30, 40, 50];
print(shape(arr));       // [5]
print(arr.shape());      // [5]  (메서드 스타일)

int m[3][4];
print(shape(m));         // [3, 4]

int t[2][3][4];
print(shape(t));         // [2, 3, 4]
```

- 1차원 배열: `[원소 수]`
- 다차원 배열: `[1차원 크기, 2차원 크기, ...]` (첫 번째 원소 기준으로 하위 차원 탐색)
- 배열이 아닌 타입: 에러

### `타입.max()` / `타입.min()`

타입의 최대/최소 표현 가능 값을 반환한다.

```
print(int.max());        // 플랫폼 의존 (기본: int64 범위)
print(int.min());        // 플랫폼 의존 (기본: int64 범위)
print(int8.max());       // 127
print(int8.min());       // -128
print(int16.max());      // 32767
print(int16.min());      // -32768
print(int32.max());      // 2147483647
print(int64.max());      // 9223372036854775807
print(float32.max());    // 3.40282e+38
print(float.max());      // 1.79769e+308
print(char.max());       // 127
print(bool.max());       // 1

print(uint.max());       // 플랫폼 의존 (기본: uint64 범위)
print(uint8.max());      // 255
print(uint16.max());     // 65535
print(uint32.max());     // 4294967295
print(uint64.max());     // 18446744073709551615 (BigInt)
print(uint.min());       // 0
```

#### 타입별 반환값

| 타입 | `max()` | `min()` |
| ---- | ------- | ------- |
| `int8` | 127 | -128 |
| `int16` | 32767 | -32768 |
| `int` (`int16f`) | 플랫폼 의존 | 플랫폼 의존 |
| `int32` | 2147483647 | -2147483648 |
| `int64` | 9223372036854775807 | -9223372036854775808 |
| `uint8` | 255 | 0 |
| `uint16` | 65535 | 0 |
| `uint` (`uint16f`) | 플랫폼 의존 | 0 |
| `uint32` | 4294967295 | 0 |
| `uint64` | 18446744073709551615 (BigInt) | 0 |
| `float32` | 3.40282e+38 | -3.40282e+38 |
| `float`, `float64` | 1.79769e+308 | -1.79769e+308 |
| `char` | 127 | -128 |
| `bool` | 1 | 0 |

- `intbig`, `bigint`: 범위 제한 없음 → `max()`/`min()` 미지원 (에러)
- `string`: 해당 없음 → 에러
- `uint64.max()`는 int64 범위를 초과하므로 BigInt로 반환

## 타입 캐스트 함수

타입 이름을 함수처럼 호출하여 값을 변환한다. 모든 타입 키워드가 캐스트 함수로 사용 가능하다.

### 정수 캐스트

```
print(int(3.7));         // 3 (소수점 절삭, fast 타입은 오버플로 시 에러)
print(int8(200));        // -56 (wrap-around)
print(int16(40000));     // -25536 (wrap-around)
print(int32(3.14));      // 3
print(int64(3.99));      // 3
print(intbig("999999999999999999999"));  // BigInt
print(bigint(42));       // BigInt 42
```

- `float` → 정수: 소수점 이하 절삭
- `string` → 정수: 숫자 문자열 파싱
- `bool` → 정수: `true` = 1, `false` = 0
- 고정 크기 정수(`int8`~`int64`): 범위 초과 시 wrap-around
- fast 정수(`int`, `int8f`~`int32f`): 범위 초과 시 런타임 에러
- `intbig`: int64 범위 유지, 초과 시 BigInt
- `bigint`: 항상 BigInt로 변환
- `uint`, `uint8`~`uint64`: unsigned 범위로 wrap-around
- 변환 불가 시 에러

### unsigned 정수 캐스트

```
print(uint8(256));       // 0 (wrap-around)
print(uint8(-1));        // 255 (wrap-around)
print(uint16(70000));    // 4464 (wrap-around)
print(uint(3.7));        // 3 (소수점 절삭)
```

### 실수 캐스트

```
print(float(42));        // 42.0
print(float("3.14"));    // 3.14
print(float32(3.14159265358979));  // 3.14159274101257 (단정밀도 절삭)
print(float64(42));      // 42.0
```

### `char(expr)`

```
print(char(65));         // A
print(char("hello"));   // h (첫 번째 문자)
print(char(200));        // wrap-around
```

### `string(expr)`

```
print(string(42));       // "42"
print(string(3.14));     // "3.14"
print(string(true));     // "true"
```

### `bool(expr)`

```
print(bool(0));          // false
print(bool(1));          // true
print(bool(""));         // false
print(bool("hello"));   // true
```

### 중첩 캐스트

```
int x = int(float(int8(300)));  // 300→int8(-44)→float(-44.0)→int(-44)
print(x);               // 44
```
