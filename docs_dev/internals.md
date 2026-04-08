# C% 내부 구현

## 인터프리터 런타임 값 (CpctValue)

> 인터프리터는 폐기 예정. 이 섹션은 참고용으로 유지.

트리워킹 인터프리터에서 모든 스칼라 변수는 `CpctValue`(`std::variant`)에 저장된다.

```
CpctValue = std::variant<int64_t, uint64_t, double, bool, std::string,
                        std::vector<CpctValue>, BigInt, TypedArray, CpctDict>
sizeof(CpctValue) ≈ 40~48 bytes
```

---

## cpct-cpp-lib 타입 시스템

트랜스파일된 C++ 코드가 사용하는 타입 라이브러리. 각 타입은 네이티브 C++ 타입을 래핑하며, 암시적 변환을 통해 C++ built-in 연산자를 사용한다.

### 스칼라 타입

| C% 타입 | cpct 클래스 | native_type | 연산 |
| ------- | ----------- | ----------- | ---- |
| `int` | `Int` | `int_t` (platform.h) | built-in 위임 |
| `int8`~`int64` | `Int8`~`Int64` | `int8_t`~`int64_t` | built-in 위임 |
| `int8f`~`int32f` | `Int8f`~`Int32f` | `fast8_t`~`fast32_t` (platform.h) | built-in 위임 |
| `intbig` | `IntBig` | `variant<int64_t, BigInt>` | 자체 연산자 (오버플로 감지) |
| `float` | `Float` | `double` | built-in 위임 |
| `string` | `String` | `std::string` | 자체 메서드 |
| `bool` | `Bool` | `bool` | built-in 위임 |
| `char` | `Char` | `char` | built-in 위임 |

### 연산자 전략

대부분의 타입은 **산술 연산자를 정의하지 않는다.** `operator native_type()`으로 암시적 변환한 뒤 C++ built-in 연산자를 사용한다. 이 설계의 장점:
- 리터럴(`1`, `3.14`)과 혼합 연산 시 모호성 없음
- 코드량 최소화
- 네이티브 성능

예외: `IntBig`은 오버플로 감지가 필요하므로 자체 연산자를 가진다.

### 플랫폼 설정 (platform.h)

fast 타입의 실제 크기를 플랫폼별로 고정 크기 타입으로 매핑한다.
`int_fast*_t`를 직접 사용하면 컴파일러마다 다른 타입이 되어 모호성이 생기므로, 우리가 직접 결정한다.

| 프리셋 | fast8 | fast16 | fast32 |
| ------ | ----- | ------ | ------ |
| default (x64-linux, arm64) | int8_t | int64_t | int64_t |
| x86, x64-win, esp32 | int8_t | int32_t | int32_t |
| avr | int8_t | int16_t | int32_t |
| arm32 | int32_t | int32_t | int32_t |

### 비교 연산 (compare.h)

모든 비교 연산은 `cpct::cmp_*` 함수로 변환된다. 정수 타입은 `std::cmp_*`(C++20)를 사용하여 signed/unsigned 혼합 비교를 안전하게 처리한다. 비정수 타입(float, string, char, bool)은 기본 `<`, `>` 연산자를 사용한다. `if constexpr`로 컴파일 타임 분기하므로 런타임 오버헤드 없음.

---

## 자료구조

### Array<T, N>

`std::array<T, N>` 래핑. 고정 크기. 음수 인덱싱, 슬라이싱, sort, reverse, find, is_contains 지원.

### Vector<T>

`std::vector<T>` 래핑. 동적 크기. push/pop/insert/erase/clear + 음수 인덱싱, 슬라이싱, sort, reverse, find, is_contains 지원.

### Dict

비타입 딕셔너리. 키: string, int, bool, char (tagged hash로 구분). 값: `std::any`. 삽입 순서 유지. sortkey/sortval/sortvk 지원.

### Map<K, V>

타입 고정 맵. 해시 기반 + 체이닝으로 충돌 처리. 삽입 순서 유지. 타입 coercion 지원. sortkey/sortval/sortvk 지원. remove는 shift 방식 (순서 보존).

---

## 정적 분석 (Analyzer)

트랜스파일 전 AST를 순회하며 오류를 감지한다.

### move-after-use 검사

`let`으로 이동된 변수를 이후에 사용하면 컴파일 에러:
```
let int y = x;
println(x);       // Error: Variable 'x' has been moved
```

스코프 인식: 블록/함수별 moved set 관리. 재대입으로 복원 가능:
```
let int y = x;
x = 100;          // x 복원
println(x);       // OK
```

---

## 포맷 (format.h)

`cpct::format()`은 f-string 트랜스파일 대상. `std::format` 가능 시 사용, 아닌 경우 `std::to_string` + `append` fallback.

```cpp
#if CPCT_HAS_STD_FORMAT
    // std::format 사용
#else
    // to_string + append (크로스플랫폼)
#endif
```

---

## TODO

- [ ] fast 타입 오버플로 런타임 에러 (cpct-cpp-lib)
- [ ] const 위반 정적 분석 (Analyzer 확장)
- [ ] ref 반환 지원 + lifetime 검사
- [ ] `:=` 복사 대입 연산자
