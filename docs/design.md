# Notepad

# EBNF vs hand written
On python I used the library Lark which generates regular expressions out of EBNF like rules.
It adds some special commands like ignore/import and distinguish tokenizer and parser rules by upper/lower-case rule names. It generates a parser ruleset and build a parse-tree as result. The parse-tree can be optimized by declaring data classes which will be filled by the nodes it match and replace the actual part of the tree.

Intel wrote a library called hyperscan which is used for high performance matching of data streams.
It use EBNF and regular expressions to build NFA and compose DFA if possible. On top it does some special operations for string matching with SIMD.

I wrote a very simple lexer by hand interpreting the EBNF rules and optimized the checks by ordering and reducing the checks to the necessary one. No SIMD, cache optimization or simillar thinks.

The performance difference between the hand written lexer and Lark are unexpected huge. I want to compare the hyperscan to the hand written lexer but the library and it's pcre ruleset taking to much effort.

I wrote the lexer in the following way.
* Test single byte terminals <- It's a loop but everything runs on register and L1-cache
* If the terminal is unknown then test the range rules(digit,letter) <- VS2019 and gcc optimized it into two single bit-array check operation without any branching or loop.
At this point we have a error if the token is still unknown.
* Check the two byte terminals by first check if the current terminal is the right one. <- very poor performance and code bloating but easy to understand
* Special rules with more complex regular expressions(SingleLineComment,MultiLineComment,String,Name,Decimal/Hex-literals)
* If the current token is a Name then check if it's a keyword and change it to Keyword if this is the case.
* Check if the current token should be skipped
* Generate the TokenLocationInfos if requested.

I split the token information into a list of TokenKind and a list of TokenLocationInfos. Only very late steps like type validation take the TokenLocationInfos to find the values in the source code but but earlier stages like syntax check and node generation only need the TokenKind list.
Reading 4k of source code took ~120usec for the lexer on single core and no optimization on my i7-9850H.

I don't like the fact that the lexer and the parser are written by hand because it's a dead-end. Optimizing and changing the syntax get cumbersome and error prone. On the other side we have regex parser, FNA and DFA generation and poor choices on the rule and language design.
I think a hand crafted state machine with the already defined phases and a variable input allows me a flexible, less error prone and optimizeable lexer generation.

# Multithreading
The parser should create a thread pool with the amount of logial processors. Loading files take more time than lexing them. Because of that the files should be requested in async mode. Request additional files and then start processing one. Each file resides in the memory besides the token, parse-tree and tau-tree until the tau-tree is merged into the tau-tree of the module. Because of this the worker should lex, parse, validate, translate, merge and then look for a new task.
The funnel is the tau-tree of the module. If no file processing tasks are left the workers can join the module processing. 

# Optimization
A SIMD optimization for checking the one and two terminals is a no brainer. Load the current byte or two byte into one SIMD register and load all matching terminals in multiple other registers, do a comparement and look for the index. Two byte pattern must be checked a second time with a 1 byte shift. Most CPUs can do multiple SIMD instructions in parallel. Multiple byte tokens like comments, names and strings can benefit most because instead of testing multiple cases against the current byte they can test multiple bytes by one or more cases. A multiline comment on AVX512 could process 64 bytes at a time.

It's easier and more efficient to optimize the lexer then the parser because the lexer is the compressor and left only a fraction of data which the parser can interpret. The amount of possible states and symbols is very small. The lexer must work with a symbol set of 0-255 and the parser only with the set of tokens generated. The optimization for this part is mostly defined by the design of the language you want to parse. The rule of thumb is avoid optional cases.

A rule with multiple options should be able to switch to the right option by just checking the first token. The rule 

```block: struct | function | constants | aliases | ast_decorator | use | operator | interface | enum```

Has many options which is no performance issue if each sub-rule can start with a different token. Like struct with STRUCT, function with NAME, constants with CONSTANTS, aliases with ALIASES, ast_decorator with \_\_AST\_\_ and so on. I decided to declare a function without a NAME because no other rule starts with it and it's the most used rule in the code and reduce the amount of typing.
