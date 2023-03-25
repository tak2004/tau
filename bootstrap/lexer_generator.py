from lark import Lark
import os
import re

def convertRegex(pythonRegex:str):
    # Get rid of non-capturing group because it's not used.
    result = pythonRegex.replace('(?:','(')
    # Remove unnecessary groups.
    group = re.compile(r'\((?:(?!(\(|\))).)*\)(?!(\+|\*))')
    matches = group.search(result)
    while matches != None:
        result = result.replace(matches[0], matches[0][1:-1])
        matches = group.search(result)
    # Escape newline.
    result = result.replace('\n','\\n')
    result = result.replace('*?','*')
    return result

def extractRanges(regex:str):
    result = {}
    group = re.compile(r'\[([a-z]|[A-Z]|[0-9])\-([a-z]|[A-Z]|[0-9])\]')
    matches = group.findall(regex)
    if matches != None:
        for m in matches:
            result[m[0]+m[1]]=m
    return result

def writeExpressions(file, expr):
    file.write("struct LexerContext;\n")
#    file.write("enum class RegularExpression:uint8_t{\n\t")
    # Ambiguous streaming tokens occur if a token takes any byte code until it's terminated.
    # Commonly a token can be fixed in the merging step of multiple token lists but this
    # kind of case would need a whole rescan of all data following by the start of the rule.
    # To avoid this the lexer interprets the given buffer in the usual way and in parallel for
    # each of this ambiguous cases. If a termination of an ambiguous case occur the lexer can be
    # sure that this is the current state, disable the other one and continue regular lexing.
#    ambiguousTokens = 0
#    for e in expr:
#        file.write(e.name+", ")
#        if str(e.pattern.value).find("(.|\n)*") != -1:
#            ambiguousTokens += 1
#    file.write("\n};\nconstexpr size_t REGULAR_EXPRESSION_COUNT="+str(len(expr))+";\n")
#    file.write("size_t AMBIGUOUS_STREAMING_TOKENS="+str(ambiguousTokens)+";\n")
    #    rangeExpr = re.compile(r'\[(.)-(.)\]')
    for e in expr:
#        file.write("bool match_"+e.name+"(LexerContext& Context){\n")
        regex = convertRegex(e.pattern.value)
        range = extractRanges(regex)
        if len(range.items()):
            ranges.append(extractRanges(regex))
#        result = rangeExpr.findall(regex)
#        print(result)
#        file.write("R\"("+regex+")\",//"+e.name+"\n")
#        file.write("\treturn true;\n};\n")

def writeLexer(file):
    # State
    file.write("struct LexerState{\n\tbool hasErrors;\n};\n")
    # LexerContext
    file.write("struct LexerContext{\n\tLexerState States[AMBIGUOUS_STREAMING_TOKENS+1];\n")
    file.write("};\n")
    file.write("bool match(LexerContext& Context, size_t ByStateIndex){\n")
    file.write("\tauto* state = Context.States[ByStateIndex];")
    file.write("\treturn true;\n}\n")
    file.write("bool lexing(LexerContext& Context){\n")
    file.write("\tfor (auto i = 0; i < AMBIGUOUS_STREAMING_TOKENS+1; ++i){\n")
    file.write("\t\tContext.CreateJob(Context.States[i]);\n")
    file.write("\t}\n\treturn true;\n}\n")

def writeRanges(file):
    new_ranges=[]
    # Entries which occur in multiple groups are seperated in their own one.
    for r in ranges:
        group = {}
        for key in r.keys():
            occurance = 0
            for r2 in ranges:
                if key in r2:
                    occurance+=1
            if occurance == 1:
                group[key]=r[key]
            else:
                # Check if the range is already in any new group.
                isAlreadyInList = False
                for e in new_ranges:
                    if key in e:
                        isAlreadyInList = True
                        break
                if isAlreadyInList == False:
                    new_ranges.append({key:r[key]})
        if len(group.items())> 0:
            new_ranges.append(group)
    file.write("constexpr char LEXER_RANGES[]={\n\t")
    totalRanges = 0
    for grp in new_ranges:
        for r in grp.values():
            totalRanges+=1
            file.write("'"+r[0]+"', '"+r[1]+"',\n\t")
    for i in range(16-(totalRanges*2%16)):
        file.write("0,")
    file.write("\n};\nconstexpr uint8_t LEXER_RANGES_SHIFT_LEFT_RIGHT[]={")
    left = 16
    for grp in new_ranges:
        elements = len(grp.values())*2
        left -= elements
        file.write("\n\t"+str(left)+","+str(16-elements)+",")
    file.write("\n};\n")

def writeKeywordNeedles(file):
    leading = set()
    trailing = set()
    for k in keywords:
        leading.add(k.pattern.value[0])
        trailing.add(k.pattern.value[-1])
    file.write("constexpr char LEXER_KEYWORD_NEEDLE_LEFT[]={\n\t")
    for l in leading:
        file.write("'"+l+"', ")
    for i in range(16-(len(leading)%16)):
        file.write('0,')
    file.write("\n};\n")
    file.write("constexpr char LEXER_KEYWORD_NEEDLE_RIGHT[]={\n\t")
    for t in trailing:
        file.write("'"+t+"', ")
    for i in range(16-(len(trailing)%16)):
        file.write('0,')
    file.write("\n};\n")

def writeSingleByteTerminals(file):
    file.write("constexpr char LEXER_SB_TERMINALS[]={\n")
    for e in singleByte:
        file.write("\t'"+str(e.pattern.value).replace('\n','\\n').replace('\r','\\r').replace('\t','\\t')+"', // SingleCharacterTerminals::"+e.name+"\n")
    for i in range(16-(len(singleByte)%16)):
        file.write('0,')
    file.write("\n};\n") 

def writeExpressionNeedles(file):
    file.write(r"""constexpr char LEXER_REGEX_TOKENS_LEFT[] = {
	'/','*','"','<','0',
	0,0,0,0,0,0,0,0,0,0,0
};
constexpr char LEXER_REGEX_TOKENS_RIGHT[] = {
	'*','/','>','"','x',
	0,0,0,0,0,0,0,0,0,0,0
};""")

def writeLexerContextFreeGrammar(file):
    file.write(r"""void LexerContextFreeGrammar(uint8_t* Buffer, size_t Bytes, uint8_t* Tokens) {
	__m128i* p = reinterpret_cast<__m128i*>(Buffer);
	__m128i* tokens = reinterpret_cast<__m128i*>(Tokens);
	__m128i terminal1 = _mm_setzero_si128();
	__m128i terminal2 = _mm_setzero_si128();
	__m128i ranges = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_RANGES));
	__m128i sbt1 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_SB_TERMINALS));
	__m128i sbt2 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_SB_TERMINALS) + 1);
	__m128i kwcl1 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_KEYWORD_NEEDLE_LEFT));
	__m128i kwcl2 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_KEYWORD_NEEDLE_LEFT) + 1);
	__m128i kwct1 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_KEYWORD_NEEDLE_RIGHT));
	__m128i kwct2 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_KEYWORD_NEEDLE_RIGHT) + 1);
    __m128i expl1 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_REGEX_TOKENS_LEFT));
    __m128i expl2 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_REGEX_TOKENS_LEFT)+1);
    __m128i expt1 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_REGEX_TOKENS_RIGHT));
    __m128i expt2 = _mm_load_si128(reinterpret_cast<const __m128i*>(LEXER_REGEX_TOKENS_RIGHT)+1);
	__m128i a, b;

	for (size_t i = 0; i < (Bytes / 32); ++i) {
		// Read the next view of the buffer which depends the available register and cache-line size.
		a = _mm_load_si128(p);
		b = _mm_load_si128(p + 1);

		// Parallel processing of the current view.
		// Fuse the two 128 bit masks(only 16bit used) into one 32bit mask. This frees the 128bit register.
		// range check: is letter
		__m128i range = _mm_slli_si128(ranges, LEXER_RANGES_SHIFT_LEFT_RIGHT[0]);
		range = _mm_srli_si128(range, LEXER_RANGES_SHIFT_LEFT_RIGHT[1]);
		auto letterA = _mm_cmpistrm(range, a, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_BIT_MASK);
		auto letterB = _mm_cmpistrm(range, b, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_BIT_MASK);
		uint32_t letters = (_mm_cvtsi128_si32(letterB) << 16) | _mm_cvtsi128_si32(letterA);
		// range check: is digit
		range = _mm_slli_si128(ranges, LEXER_RANGES_SHIFT_LEFT_RIGHT[2]);
		range = _mm_srli_si128(range, LEXER_RANGES_SHIFT_LEFT_RIGHT[3]);
		auto digitA = _mm_cmpistrm(range, a, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_BIT_MASK);
		auto digitB = _mm_cmpistrm(range, b, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_BIT_MASK);
		uint32_t digits = (_mm_cvtsi128_si32(digitB) << 16) | _mm_cvtsi128_si32(digitA);
		// range check: is hex letter
		range = _mm_slli_si128(ranges, LEXER_RANGES_SHIFT_LEFT_RIGHT[4]);
		range = _mm_srli_si128(range, LEXER_RANGES_SHIFT_LEFT_RIGHT[5]);
		auto hexA = _mm_cmpistrm(range, a, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_BIT_MASK);
		auto hexB = _mm_cmpistrm(range, b, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_BIT_MASK);
		uint32_t hexes = (_mm_cvtsi128_si32(hexB) << 16) | _mm_cvtsi128_si32(hexA);
		// any necessary left terminal
		auto sbt1A = _mm_cmpistrm(sbt1, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto sbt1B = _mm_cmpistrm(sbt1, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto sbTerminals = (_mm_cvtsi128_si32(sbt1B) << 16) | _mm_cvtsi128_si32(sbt1A);
		auto sbt2A = _mm_cmpistrm(sbt2, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto sbt2B = _mm_cmpistrm(sbt2, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		sbTerminals = (_mm_cvtsi128_si32(sbt2B) << 16) | _mm_cvtsi128_si32(sbt2A) | sbTerminals;
		// keyword needle optimization
		auto kwcl1A = _mm_cmpistrm(kwcl1, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto kwcl1B = _mm_cmpistrm(kwcl1, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto keywordsLeading = (_mm_cvtsi128_si32(kwcl1B) << 16) | _mm_cvtsi128_si32(kwcl1A);
		auto kwcl2A = _mm_cmpistrm(kwcl2, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto kwcl2B = _mm_cmpistrm(kwcl2, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		keywordsLeading = (_mm_cvtsi128_si32(kwcl2B) << 16) | _mm_cvtsi128_si32(kwcl2A) | keywordsLeading;
		auto kwct1A = _mm_cmpistrm(kwct1, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto kwct1B = _mm_cmpistrm(kwct1, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto keywordsTrailing = (_mm_cvtsi128_si32(kwct1B) << 16) | _mm_cvtsi128_si32(kwct1A);
		auto kwct2A = _mm_cmpistrm(kwct2, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto kwct2B = _mm_cmpistrm(kwct2, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		keywordsTrailing = (_mm_cvtsi128_si32(kwct2B) << 16) | _mm_cvtsi128_si32(kwct2A) | keywordsTrailing;
        // expression needle optimization
		auto expl1A = _mm_cmpistrm(expl1, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto expl1B = _mm_cmpistrm(expl1, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto expLeading = (_mm_cvtsi128_si32(expl1B) << 16) | _mm_cvtsi128_si32(expl1A);
		auto expl2A = _mm_cmpistrm(expl2, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto expl2B = _mm_cmpistrm(expl2, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		expLeading = (_mm_cvtsi128_si32(expl2B) << 16) | _mm_cvtsi128_si32(expl2A) | expLeading;
		auto expt1A = _mm_cmpistrm(expt1, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto expt1B = _mm_cmpistrm(expt1, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto expTrailing = (_mm_cvtsi128_si32(expt1B) << 16) | _mm_cvtsi128_si32(expt1A);
		auto expt2A = _mm_cmpistrm(expt2, a, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		auto expt2B = _mm_cmpistrm(expt2, b, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
		expTrailing = (_mm_cvtsi128_si32(expt2B) << 16) | _mm_cvtsi128_si32(expt2A) | expTrailing;

		a = _mm_setr_epi32(letters, digits, hexes, sbTerminals);
		b = _mm_setr_epi32(keywordsLeading, keywordsTrailing, expLeading, expTrailing);
		_mm_store_si128(tokens, a);
		_mm_store_si128(tokens+1, b);

		tokens += 2;
        p += 2;
	}
}""")

grammar = None
with open(os.path.dirname(__file__)+"/../resources/tau.lalr", 'r') as f:
    grammar = f.read()
parser = Lark(grammar, start='unit_', parser="lalr", maybe_placeholders=True)
with open(os.path.dirname(__file__)+"/../frontend/main.tau", 'r') as f:
    source = f.read()
    #print(parser.parse(source))

singleByte = []
singleByteRange = []
expr = []
keywords = []
ranges = []

for t in parser.terminals:
    if t.pattern.type == "str":
        if len(t.pattern.value) == 1:
            singleByte.append(t)
        if len(t.pattern.value) >= 2:
            keywords.append(t)
    else:
        expr.append(t)

with open(os.path.dirname(__file__)+"/../backend/vc++/tokenizer.ipp", 'wt+', encoding="utf8") as f:
    # Keywords
    #f.write("enum class Keywords:uint8_t{\n\t")
    #for e in keywords:
    #    f.write(e.name+", ")
    #f.write("\n};\nconstexpr size_t KEYWORDS_COUNT="+str(len(keywords))+";\n")
    f.write("constexpr size_t KEYWORDS_COUNT="+str(len(keywords))+";\n")
    f.write("constexpr const char* KEYWORDS[KEYWORDS_COUNT]={\n")
    for e in keywords:
        f.write("\t\""+str(e.pattern.value)+"\", // Keywords::"+e.name+"\n")
    f.write("};\n")
    f.write("constexpr const uint8_t KEYWORDS_SIZE[KEYWORDS_COUNT]={\n\t")
    for e in keywords:
        f.write(str(len(str(e.pattern.value)))+", ")
    f.write("\n};\n")
    writeExpressions(f,expr)
    writeRanges(f)
    writeKeywordNeedles(f)
    writeSingleByteTerminals(f)
    writeExpressionNeedles(f)
    writeLexerContextFreeGrammar(f)
    #writeLexer(f)