constexpr size_t KEYWORDS_COUNT=36;
constexpr const char* KEYWORDS[KEYWORDS_COUNT]={
	"const", // Keywords::IS_CONST
	"mut", // Keywords::IS_MUTATE
	"module", // Keywords::MODULE
	"use", // Keywords::USE
	"match", // Keywords::MATCH
	"enum", // Keywords::ENUM
	"interface", // Keywords::INTERFACE
	"operator", // Keywords::OPERATOR
	"free", // Keywords::FREE
	"init", // Keywords::INIT
	"struct", // Keywords::STRUCT
	"constants", // Keywords::CONSTANTS
	"aliases", // Keywords::ALIASES
	"decorator", // Keywords::DECORATOR
	"__AST__", // Keywords::__AST__
	"{}", // Keywords::__ANON_0
	"embed", // Keywords::EMBED
	"->", // Keywords::__ANON_1
	"[[", // Keywords::__ANON_2
	"]]", // Keywords::__ANON_3
	"for", // Keywords::FOR
	"in", // Keywords::IN
	"loop", // Keywords::LOOP
	"while", // Keywords::WHILE
	"if", // Keywords::IF
	"else", // Keywords::ELSE
	"return", // Keywords::RETURN
	"&&", // Keywords::__ANON_4
	"||", // Keywords::__ANON_5
	"!=", // Keywords::__ANON_6
	"==", // Keywords::__ANON_7
	"<=", // Keywords::__ANON_8
	">=", // Keywords::__ANON_9
	"++", // Keywords::__ANON_10
	"--", // Keywords::__ANON_11
	"::", // Keywords::__ANON_12
};
constexpr const uint8_t KEYWORDS_SIZE[KEYWORDS_COUNT]={
	5, 3, 6, 3, 5, 4, 9, 8, 4, 4, 6, 9, 7, 9, 7, 2, 5, 2, 2, 2, 3, 2, 4, 5, 2, 4, 6, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
};
struct LexerContext;
constexpr char LEXER_RANGES[]={
	'0', '9',
	'A', 'Z',
	'a', 'z',
	'a', 'f',
	'A', 'F',
	0,0,0,0,0,0,
};
constexpr uint8_t LEXER_RANGES_SHIFT_LEFT_RIGHT[]={
	14,14,
	10,12,
	6,12,
};
constexpr char LEXER_KEYWORD_NEEDLE_LEFT[]={
	'm', 'u', '>', ']', 'd', '+', 'e', '_', '<', '=', 'a', '-', 'w', '|', 'o', 's', ':', '[', 'r', 'f', 'l', '!', 'i', '{', '&', 'c', 0,0,0,0,0,0,
};
constexpr char LEXER_KEYWORD_NEEDLE_RIGHT[]={
	'p', 'h', 'm', '>', ']', 'd', '+', 'e', '_', '=', 'n', '-', '|', 's', ':', '[', 'r', 'f', 't', '}', '&', 0,0,0,0,0,0,0,0,0,0,0,
};
constexpr char LEXER_SB_TERMINALS[]={
	'\r', // SingleCharacterTerminals::CARRIAGE_RETURN
	'\n', // SingleCharacterTerminals::NEW_LINE
	' ', // SingleCharacterTerminals::WS
	'\t', // SingleCharacterTerminals::TAB
	',', // SingleCharacterTerminals::COMMA
	'{', // SingleCharacterTerminals::LBRACE
	'}', // SingleCharacterTerminals::RBRACE
	'=', // SingleCharacterTerminals::EQUAL
	'(', // SingleCharacterTerminals::LPAR
	')', // SingleCharacterTerminals::RPAR
	'<', // SingleCharacterTerminals::LESSTHAN
	'>', // SingleCharacterTerminals::MORETHAN
	'[', // SingleCharacterTerminals::LSQB
	']', // SingleCharacterTerminals::RSQB
	':', // SingleCharacterTerminals::COLON
	'.', // SingleCharacterTerminals::DOT
	';', // SingleCharacterTerminals::SEMICOLON
	'&', // SingleCharacterTerminals::AMPERSAND
	'^', // SingleCharacterTerminals::CIRCUMFLEX
	'|', // SingleCharacterTerminals::VBAR
	'+', // SingleCharacterTerminals::PLUS
	'-', // SingleCharacterTerminals::MINUS
	'*', // SingleCharacterTerminals::STAR
	'/', // SingleCharacterTerminals::SLASH
	'%', // SingleCharacterTerminals::PERCENT
	'!', // SingleCharacterTerminals::BANG
	'~', // SingleCharacterTerminals::TILDE
0,0,0,0,0,
};
constexpr char LEXER_REGEX_TOKENS_LEFT[] = {
	'/','*','"','<','0',
	0,0,0,0,0,0,0,0,0,0,0
};
constexpr char LEXER_REGEX_TOKENS_RIGHT[] = {
	'*','/','>','"','x',
	0,0,0,0,0,0,0,0,0,0,0
};void LexerContextFreeGrammar(uint8_t* Buffer, size_t Bytes, uint8_t* Tokens) {
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
}