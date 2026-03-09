# C% 인터프리터 내부 구현

## 런타임 값 (CmmValue)

트리워킹 인터프리터에서 모든 스칼라 변수는 `CmmValue`(`std::variant`)에 저장된다.
`variant`는 가장 큰 멤버 크기로 고정되므로, `int8`이든 `int64`이든 동일한 메모리를 차지한다.
범위 제한(wrap-around, 정밀도 절삭)은 대입/연산 시점에 소프트웨어적으로 적용된다.

```
CmmValue = std::variant<int64_t, double, bool, std::string,
                        std::vector<CmmValue>, BigInt, TypedArray, CmmDict>
sizeof(CmmValue) ≈ 40~48 bytes (플랫폼/컴파일러에 따라 상이)
```

| 스칼라 타입 | 실제 저장 타입 | 범위 제한 방식 |
| ----------- | -------------- | -------------- |
| `int`, `int8`~`int64`, `char` | `int64_t` | `checkIntRange()` wrap-around |
| `intbig` | `int64_t` 또는 `BigInt` | 오버플로 시 BigInt 승격 |
| `bigint` | `BigInt` | 제한 없음 |
| `float`, `float64` | `double` | IEEE 754 배정밀도 |
| `float32` | `double` | `coerceFloat()` 단정밀도 절삭 |
| `bool` | `bool` | — |
| `string` | `std::string` | — |

---

## 배열 (TypedArray)

배열은 선언된 원소 타입에 따라 **타입별 특화 저장소(TypedArray)**를 사용한다.
원소가 실제 해당 크기로 저장되므로 C와 동일한 메모리 효율을 가진다.

| 배열 타입 | 원소 크기 | 100개 배열 메모리 | C 동등 타입 |
| --------- | --------- | ----------------- | ----------- |
| `int8[]`    | 1 byte  | 100 bytes         | `int8_t[]`  |
| `int16[]`   | 2 bytes | 200 bytes         | `int16_t[]` |
| `int[]`, `int32[]` | 4 bytes | 400 bytes  | `int32_t[]` |
| `int64[]`   | 8 bytes | 800 bytes         | `int64_t[]` |
| `intbig[]`  | 가변    | 요소별 가변         | BigInt 지원  |
| `bigint[]`  | 가변    | 가변 (BigInt)      | BigInt 지원  |
| `uint8[]`   | 1 byte  | 100 bytes         | `uint8_t[]` |
| `uint16[]`  | 2 bytes | 200 bytes         | `uint16_t[]`|
| `uint[]`, `uint32[]` | 4 bytes | 400 bytes | `uint32_t[]`|
| `uint64[]`  | 8 bytes | 800 bytes         | `uint64_t[]`|
| `float32[]` | 4 bytes | 400 bytes         | `float[]`   |
| `float[]`, `float64[]` | 8 bytes | 800 bytes | `double[]` |
| `char[]`    | 1 byte  | 100 bytes         | `char[]`    |
| `bool[]`    | 1 byte  | 100 bytes         | `bool[]`    |

- sized 타입 배열(`int8[]`, `float32[]` 등)은 네이티브 크기로 직접 저장
- `string[]`은 문자열 포인터 배열 (원소 크기 동적)
- 다차원 배열은 연속 메모리 또는 행 포인터 배열로 구현

---

## dict / map (CmmDict)

dict와 map은 내부적으로 동일한 `CmmDict` 구조체를 사용하되, 인덱싱 전략이 다르다.

### 공통 구조

```
CmmDict {
    keyType: string                  // "" (dict) 또는 "string"/"int"/... (map)
    valueType: string                // "" (dict) 또는 "int"/"float"/... (map)
    keys: vector<CmmValue>           // 삽입 순서 유지
    values: vector<CmmValue>         // keys와 1:1 대응
    strIndex: unordered_map<string, size_t>   // 문자열 해시 인덱스
    intIndex: unordered_map<int64_t, size_t>  // 정수 해시 인덱스
}
```

### dict (untyped) — 안전한 타입 구분

- `strIndex` 사용, 타입 접두사(tagged hash)로 키 충돌 방지
- `d[1]`과 `d["1"]`이 다른 키로 구분됨

```
hashKey(42)      → "i:42"
hashKey("hello") → "s:hello"
hashKey(true)    → "b:true"
hashKey(BigInt)  → "B:12345..."
```

### map (typed) — 성능 최적화

키 타입이 선언 시 고정되므로, 타입별 전용 인덱스를 사용한다.

| 키 타입 | 인덱스 | 해시 방식 | 장점 |
| ------- | ------ | --------- | ---- |
| `string` | `strIndex` | raw string (태그 없음) | prefix 오버헤드 제거 |
| `int`, `char`, `bool` 등 | `intIndex` | raw `int64_t` | string 변환 비용 제거, O(1) int hash |

### 복잡도

| 연산 | dict | map<int, V> | map<string, V> |
| ---- | ---- | ----------- | -------------- |
| 조회 | O(1) + string alloc | O(1) no alloc | O(1) no prefix |
| 삽입 | O(1) + string alloc | O(1) no alloc | O(1) no prefix |
| 삭제 | O(1) swap-remove | O(1) swap-remove | O(1) swap-remove |
| 타입 검사 | 없음 | coercion 강제 | coercion 강제 |

### 삭제 구현 (swap-remove)

마지막 원소를 삭제 위치로 이동하여 O(1)에 삭제한다.
삽입 순서가 일부 변경될 수 있다.

```
remove(key):
  1. 인덱스에서 위치 pos 조회
  2. keys[pos] = keys[last], values[pos] = values[last]
  3. 인덱스 갱신: moved element의 위치를 pos로
  4. keys/values pop_back, 인덱스에서 원래 키 제거
```

---

## TODO

- [ ] 플랫폼별 최적 정수 크기: 현재 인터프리터는 모든 연산을 C++ `int64_t`로 수행. 향후 컴파일러/바이트코드 전환 시 `int_fast8_t`, `int_fast16_t` 등 플랫폼 최적 크기를 활용하여 레지스터 연산 효율 개선 검토
- [ ] map: typed value storage — value 타입이 고정이므로 `vector<int64_t>` 등 flat 저장으로 variant 오버헤드 제거 가능
