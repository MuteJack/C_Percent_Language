# 언어 스펙 vs 인터프리터 차이점

현재 cpct-interpreter는 프로토타입 단계로, 언어 스펙과 내부 구현이 다른 부분이 있다.
향후 컴파일러 단계에서 스펙대로 구현할 예정.

## 정수 저장

| 항목 | 언어 스펙 | 인터프리터 |
| --- | --- | --- |
| `int` | 32비트 | `int64_t`로 저장, 범위 검사로 32비트 제한 |
| `int8` | 8비트 | `int64_t`로 저장, `checkIntRange()`로 wrap-around |
| `int16` | 16비트 | `int64_t`로 저장, `checkIntRange()`로 wrap-around |
| `int32` | 32비트 | `int64_t`로 저장, `checkIntRange()`로 wrap-around |
| `uint`~`uint32` | 각 크기별 unsigned | `uint64_t`로 저장, `checkIntRange()`로 범위 제한 |
| `uint64` | 64비트 unsigned | `uint64_t` 네이티브 저장 (스펙과 동일) |
| `char` | 8비트 정수 | `int64_t`로 저장 |

## 실수 저장

| 항목 | 언어 스펙 | 인터프리터 |
| --- | --- | --- |
| `float32` | 32비트 단정밀도 | `double`로 저장, `coerceFloat()`로 절삭 |
| `float` / `float64` | 64비트 배정밀도 | `double` (동일) |

## 문자열 리터럴

| 항목 | 언어 스펙 | 인터프리터 |
| --- | --- | --- |
| `"hello"` 내부 저장 | `char[]` (경량) | `std::string` 직접 생성 |
| `string` 대입 | `char[]` → `string` 승격 (복사) | 이미 `std::string`이므로 추가 비용 없음 |
| 리터럴 직접 사용 (`print("hello")`) | 힙 할당 없이 `char[]` 직접 접근 | `std::string` 생성 후 전달 |

## 배열 저장

| 항목 | 언어 스펙 | 인터프리터 |
| --- | --- | --- |
| `int8[]` | 1바이트 원소 | `TypedArray` (네이티브 크기 저장, 스펙과 동일) |
| `int[]` | 4바이트 원소 | `TypedArray` (스펙과 동일) |

## 스칼라 메모리

| 항목 | 언어 스펙 | 인터프리터 |
| --- | --- | --- |
| `int8` 변수 메모리 | 1바이트 | `CpctValue`(`std::variant`) ≈ 40~48바이트 |
| `bool` 변수 메모리 | 1비트 | 동일 variant 크기 |

> 트리워킹 인터프리터에서는 모든 값이 `std::variant`에 감싸져 있으므로, 타입별 메모리 최적화는 의미가 없다. 컴파일러/바이트코드 단계에서 네이티브 크기를 활용할 예정.
