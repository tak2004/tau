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

# File Benchmark findings

I did several benchmarks for different scenarios to get a clue about the numerics.
My test setup is a 6core(12lp),32GB RAM, NVME 980 evo pro on Windows 10 64bit.

## File read througput

The highest reading performance is achived by using as less files as possible because the reading into the memory takes plenty of time and many files are split across the whole disc which reduce the disk and memory file cache efficiency. The OS detects sequential reading and reads ahead of request into the caches. On Windows you could enforce it by specifiing this behaviour during the file open call but it's not necessary as the OS does this anyway except you explicitly forbid it with another flag. 

The throughput also increase if a file is larger which can be an increase of complexity if compression/decompression is involved. A slow decompression would reduce the advantage of memory over disk read performance especially if you run a flash disk.

Multithreading makes a huge difference compared to single threaded and amplifies with faster storage and decompression. With roughly 4 time the speed compared to single thread read the multithreading approach should be implemented. A fast decompression can rise the speed by additional 2,5 times. A handfull of large compressed files gets the best performance.

Hot cache access is the biggest impact on performance with a factor of rougly 100 times. On a single build process running once this can't be utilized but a rebuild benefits very well from the hot cache. Valve did alot of research for their pak format in the past. They choose rougly 200mb files as sweet spot for sequential read cache, decompression and file system caching.
I think 200mb works very well on normal dev machines, in fact own benchmarks in the past revealed 280mb as a sweet spot for virtual texture archives. But there's a paradigma shift which favors small memory docker container for build environments. The default size of a file should be 250mb and would be reduced it the file cache in the memory is smaller.  

Case|time|files|total size(bytes)|throughput(mb)/s
-|-|-|-|-
single thread, sync, hot cache|37ms|16|2.105.344|57
single thread, sync, cold cache|12.884ms|2.468|229.130.240|18
multi thread, mapped, cold cache|3.046ms|2.468|229.130.240|75
multi thread, mapped, cold cache|4.289ms|858|12.636.160|2,9
multi thread, mapped, cold cache, compressed|1.743ms|6(858)|3.022.848|1,7(7,2)
multi thread, mapped, hot cache|26ms|6|266.305.536|10.242

## File system search

The tests also show that the search through the file tree takes only a little fraction of the time because the file system information are cached on memory. Which means that iterate through thousand of files and check their file system attributes(bytes,last write, create time,last access) can be done in two digit ms time span. Reading the content of the files takes the majority of time. But even at this topic less is more but the impact is less of a deal.

## Single build performance
A single build can only benefit from a little amount of larger files. C++ introduced the module concept to reduce the amount of files and increase the build speed. Many languages collect multiple source files into a single package/module but many deflate the package into the filesystem and lose the advantage of access time. The internal caching system which fix this bad decision for rebuild don't work on single build. Which means poor build performance for single builds. Java allows to bundle jar files and read the jar file with all it's content instead of deflate it and read the files. This increase the performance on single build and rebuild.

It's crucial to the performance to enforce a single file for 3rd party code. Which means a package concept must be included in the ecosystem. Github and Gitlab support zip compression of projects and Gitlab also supports several tar flavours but previous development on a own package manager revealed that they are pretty bad formats and a much simpler and more restrictive format is better.
Compression of the whole file would enforce a single compression algorithm and also longer wait time for decompression. Compression of a single file inside an archive allows streaming, better compression/decompression ratio and timings. In some cases even no compression is a valid choice(already compressed files).
A package benefits more from fast decompression as they tend to be only a couple of kb or mb large.

## Rebuild performance
The rebuild performance benefits mostly from hot cache. The decission to use packages instead of their deflated content benefits the caching system too. Many uncompressed files would lower the amount of cache hits compared to less compressed files. A rough compression of 1:4 would allow 4 times more source code in the cache and with this more cache hits. 

The second important technique is the file mapping instead of reading the data in whole, because this allows to work directly on the file cache instead of copy the memory block into the process memory.

A huge project with many files is the usual case where rebuild time matters and to cover this case the code of the current project should be optimized too. Many compiler do this by tracking the file changes and cache the results of the last builds. This way only the changed files must be processed again. As already mentioned the access to the file system information is fast because it's cached on the memory and only the file data read can be slow.

If a database with each processed file, it's processed content, timestamp, content hash and size is created then it would be possible to utilize it on a build process. The build process would check if such a database exists and if this is the case then it could be loaded and testing the timestamp and size with the one from the filesystem and allows a fast skip of the file. If one or both are different then the file will be loaded, hashed and if the content didn't changed then the timestamp would be updated and the processed data returned else the file must be processed and all attributes must be updated too.
The database should be a different format then the package format because the database relies on a good defragmentation algorithm and can be more generous with disk space.

### Live build

The rebuild could be speed up by implementing a live build feature which establish a file watcher on the source code and rebuild if a file change event happes. With this approach only the changed file will be checked on the database and if it changed a update will be done. This needs a process which runs in the background and triggers the compiler instead of manual execution. Combined with hot-reloading this is a game-changer on rapid changes and debugging.

# Other optimizations

## Multi threading

It's easy to use multi threading a set of files by assign each file a thread. Further optimization of multiple threads per file is complex and depeends on the language.

To allow parallel processing of a single file it's possible to split heavy work on the threads or cut the data into chunks and assign a chunk to each thread.
To split the data it's mendatory to avoid unrecoverable missinterpretation. This can happen if a token is detected by the same start and end condition. Commonly this is the case for string literal.
To get most performance out of the language the start and end condition should be destinguishable e.g. instead of " for start and end of a string a " can start the string and a "r(raw) or "e(escape) end it. With such a rule the lexer can correct the content on the fly. As soon a "r or "e occurs all the previous content can be replaced by a string literal token. After each thread is done with the work the token lists can be merged. An alternative is a two byte start and end condition like C/C++ comments use /* and \*/ a string could be using "> and <". No escape sequence would be needed, C/C++ user-literal and raw/processed string literal would be possible. Don't use single line comments with only a open tag like C/C++ single line comment starting with // and end with a newline \n. Instead only block comments should be able. Now it's possible to parse the code from any spot by telling the thread the buffer to parse and that the first bytes could be part of a comment or string block. The parser can run in 3 states parallel, no comment or string/open comment/open string. If all three states fail before the end was reached then there's a lexer error. If the end of the buffer is reached and it could still be a comment or string then return the possible state information. This way the merge process can handle the issue. 

## SIMD

This optimization will not be faster as processing byte by byte until one of the following conditions is met.

* The byte array will be processed several times with different operations. E.g. byte is ansi space or tab.
* The byte array is larger then a cache line. E.g. detecting line count of file.
* Special operations which are more expensive will be done. E.g. SIMD hashing.

### Comments

SIMD can shine on comment processing because they take serveral bytes and multi line comments can be very large. The content of the content isn't important for the lexer or parser and only the span of the comment is important.
The start and end of a comment is easy to search and can be done without complex conditions.

### Identifier names

A identifier name is used for functions, variables, enums, class and other language elements. The rule for this names are commonly easy and they tend to be long on many cases.

### String literal

A string literal can be optimized if the start and end of the literal is easy to detect. Commonly there are two kind of string literals, raw and standard one. The raw string literals are used to avoid the usage of escape sequences. The rules are commonly very easy. E.g. start is r"( and end is )" for raw string literals and start with " and end with " for string literal. It's important to ensure that the end isn't missinterpreted by a escape sequence like \".

### Keywords

The keyword list is static but can be pretty long and brute force would be slower with more complex languages. There are multiple approaches to speed this up. The important fact that we already know which keywords are possible at the current position reduce the pool mostly to a single value. This would couple the lexer tightly to the parser. Which whould introduce a complex state and with this the possibility of multi threading would be gone.

First approach would be to check if the current byte could be the start of a keyword and write fixed code to reduce byte by byte the possible checks. As soon no candidate matches the check can be abort. This would work well with many keywords but bad with less keywords.

The second approach would be memory compare by copy the keyword to the register with the offset of the input and check if the result of the comparement matches all bytes. Most CPUs are capable to run multiple SIMD compare instructions parallel. Which would allow to check each keyword with one instruction and multiple keywords at the same time.

The third approach would be to priotize the keyword over a identifier name and decouple the parser. At this condition multithreading can be used and the list of candidates is always the total amount of keywords. To reduce the possible keywords down the first input byte should be used for a lookup. A 32Byte bitmask represents all 256 possible values. Each bit will be set to one if one of the keywords start with the byte value. To use SIMD 