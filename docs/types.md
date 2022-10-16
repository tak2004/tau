# Builtin Types
## Numeric
* u8
* u16
* u32
* u64
* u128
* u256
* u512
* i8
* i16
* i32
* i64
* i128
* i256
* i512
* f16
* f32
* f64
* f128

## Text
* strlit: String literal exists only at compile time. At runtime it depends but mostly it's converted to a str type.

## Logic
* bool

## Optimization
* simd< type >: SIMD template which accepts all numeric types and bool. It has a special padding.

## Memory
* size
* uptr: Arithmetic pointer representation. Size depends on memory architecture.
* arr< type >: An fixed size array is a structure which contains just the data but the size is inlined at compile-time.
* ptr< type >: A pointer to any type. This reduce ambigous syntax and is more readable.
* mem: A dynamic memory block which consists of a ptr<u8> and size.

## Language
* void: Can be used to express the absence of any type.

## Interfaces
* error