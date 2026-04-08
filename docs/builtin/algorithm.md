# C% 정렬 내장 함수

## 배열 정렬

### `arr.sort(bool descending = false)`

배열을 **제자리(in-place)** 정렬한다. 원본 배열이 변경된다.

```
int arr[] = [3, 1, 4, 1, 5];
arr.sort();
print(arr);          // [1, 1, 3, 4, 5]

arr.sort(true);
print(arr);          // [5, 4, 3, 1, 1]
```

#### 타입별 정렬 기준

| 원소 타입                                                | 정렬 기준                  |
| -------------------------------------------------------- | -------------------------- |
| 정수 (`int`, `int8`~`int64`, `uint`~`uint64`) | 숫자 크기                  |
| 실수 (`float`, `float32`, `float64`)               | 숫자 크기                  |
| `bigint`, `intbig`                                   | 숫자 크기                  |
| `char`                                                 | ASCII 값                   |
| `string`                                               | 사전순 (lexicographic)     |
| `bool`                                                 | `false`(0) < `true`(1) |

#### 예시

```
float f[] = [3.14, 1.0, 2.71];
f.sort();
print(f);            // [1.0, 2.71, 3.14]

string s[] = ["banana", "apple", "cherry"];
s.sort();
print(s);            // ["apple", "banana", "cherry"]

char c[] = ['c', 'a', 'b'];
c.sort();
print(c);            // ['a', 'b', 'c']
```

#### 다차원 배열

다차원 배열에서는 1차원 하위 배열만 정렬 가능하다.

```
int matrix[][] = [[3, 1, 2], [6, 4, 5]];
matrix[0].sort();
print(matrix);       // [[1, 2, 3], [6, 4, 5]]

matrix.sort();       // Error: 다차원 배열은 직접 정렬 불가
```

---

## 딕셔너리/Map 정렬

딕셔너리, Map 정렬은 **제자리(in-place)**로 키-값 쌍의 순서를 재배치한다.

### `d.sortkey(bool descending = false)` / `d.sortk(...)`

키 기준으로 정렬한다.

```
dict d = {"banana": 2, "apple": 1, "cherry": 3};
d.sortkey();
print(d);            // {"apple": 1, "banana": 2, "cherry": 3}

d.sortkey(true);
print(d);            // {"cherry": 3, "banana": 2, "apple": 1}
```

```
dict d = {3: "c", 1: "a", 2: "b"};
d.sortk();
print(d);            // {1: "a", 2: "b", 3: "c"}
```

- 키는 중복 불가이므로 정렬 결과가 항상 결정적(deterministic)

### `d.sortval(bool descending = false)` / `d.sortv(...)`

값 기준으로 정렬한다. **안정 정렬(stable sort)** 사용.

```
dict scores = {"alice": 87, "bob": 95, "charlie": 87};
scores.sortval();
print(scores);       // {"alice": 87, "charlie": 87, "bob": 95}

scores.sortval(true);
print(scores);       // {"bob": 95, "alice": 87, "charlie": 87}
```

- 값이 같은 항목은 기존 순서를 유지 (stable sort)
- 위 예시에서 `"alice"`와 `"charlie"`는 둘 다 87이므로 삽입 순서 유지

### `d.sortvk(bool descending = false)`

값 기준 1차 정렬 후, 같은 값에 대해 키 기준 2차 정렬을 수행한다. 항상 결정적(deterministic).

```
dict scores = {"charlie": 87, "alice": 87, "bob": 95};
scores.sortvk();
print(scores);       // {"alice": 87, "charlie": 87, "bob": 95}
// 87이 같으므로 키 사전순: "alice" < "charlie"

scores.sortvk(true);
print(scores);       // {"bob": 95, "charlie": 87, "alice": 87}
// 내림차순: 95 > 87, 같은 87에서 키 내림차순: "charlie" > "alice"
```

- `sortval()`과 달리 동일 값에 대해 키 순서로 정렬하므로 삽입 순서에 의존하지 않음
- 결과가 항상 동일함을 보장

---

## 공통 규칙

- 모든 정렬은 **제자리(in-place)**: 원본을 변경하고 반환값 없음 (void)
- `descending = false` (기본): 오름차순
- `descending = true`: 내림차순
- 빈 배열/딕셔너리에 대한 정렬은 아무 동작 없음 (에러 아님)
- 원소가 1개인 경우도 동일 (아무 동작 없음)

## 메서드 별칭 요약

| 메서드        | 별칭        | 대상 |
| ------------- | ----------- | ---- |
| `sort()`    | -           | 배열 |
| `sortkey()` | `sortk()` | dict, map |
| `sortval()` | `sortv()` | dict, map |
| `sortvk()`  | -           | dict, map |
