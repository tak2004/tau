# Operator precedence

| Precedence | Operator | Description | Associativity|
|-|-|-|-|
| 1 | a++ <br> a-- | Suffix/postfix increment and decrement | Left-to-right |
| | a() | Function call | |
| | a[] | Subscript | |
| | a.b | Member access | |
| 2 | ++a <br> --a | Prefix increment and decrement | Right-to-left |
| | +a <br> -a | Unary plus and minus | |
| | !a <br> ~a | Logical NOT and bitwise NOT | |
| 3 | a*b <br> a/b <br> a%b | Multiplication, division, and remainder | Left-to-right |
| 4 | a+b <br> a-b | Addition and subtraction | |
| 5 | a<\<b <br> a>>b | Bitwise left shift and right shift | |
| 6 | a<b <br> a<=b <br> a>b <br> a>=b | For relational operators < and ≤ and > and ≥ respectively | |
| 7 | == <br> != | For equality operators = and ≠ respectively | |
| 8 | a&b | Bitwise AND | |
| 9 | ^a | Bitwise XOR (exclusive or) | |
| 10 | \| | Bitwise OR (inclusive or) | |
| 11 | && | Logical AND | |
| 12 | \|\| | Logical OR | |
| 13 | a=b | Direct assignment | Right-to-left |
| | a+=b <br> a-=b | Compound assignment by sum and difference | |
| | a*=b <br> a/=b <br> a%=b | Compound assignment by product, quotient, and remainder | |
| | a<<=b <br> a>>=b | 	Compound assignment by bitwise left shift and right shift | |
| | a&=b <br> a^=b <br> a\|=b | Compound assignment by bitwise AND, XOR, and OR | |
| 14 | a,b | Comma | Left-to-right |