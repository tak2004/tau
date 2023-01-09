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

def writeExpressions(file, expr):
    file.write("struct LexerContext;\n")
    file.write("enum class RegularExpression:uint8_t{\n\t")
    # Ambiguous streaming tokens occur if a token takes any byte code until it's terminated.
    # Commonly a token can be fixed in the merging step of multiple token lists but this
    # kind of case would need a whole rescan of all data following by the start of the rule.
    # To avoid this the lexer interprets the given buffer in the usual way and in parallel for
    # each of this ambiguous cases. If a termination of an ambiguous case occur the lexer can be
    # sure that this is the current state, disable the other one and continue regular lexing.
    ambiguousTokens = 0
    for e in expr:
        file.write(e.name+", ")
        if str(e.pattern.value).find("(.|\n)*") != -1:
            ambiguousTokens += 1
    file.write("\n};\nconstexpr size_t REGULAR_EXPRESSION_COUNT="+str(len(expr))+";\n")
    file.write("size_t AMBIGUOUS_STREAMING_TOKENS="+str(ambiguousTokens)+";\n")
    #    rangeExpr = re.compile(r'\[(.)-(.)\]')
    for e in expr:
        file.write("bool match_"+e.name+"(LexerContext& Context){\n")
        regex = convertRegex(e.pattern.value)
        print(regex)
#        result = rangeExpr.findall(regex)
#        print(result)
#        file.write("R\"("+regex+")\",//"+e.name+"\n")
        file.write("\treturn true;\n};\n")

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

for t in parser.terminals:
    if t.pattern.type == "str":
        if len(t.pattern.value) == 1:
            singleByte.append(t)
        if len(t.pattern.value) >= 2:
            keywords.append(t)
    else:
        expr.append(t)

with open(os.path.dirname(__file__)+"/../backend/vc++/tokenizer.ipp", 'wt+', encoding="utf8") as f:
    # Single byte character
    f.write("enum class SingleCharacterTerminals:uint8_t{\n\t")
    for e in singleByte:
        f.write(e.name+", ")
    f.write("\n};\nconstexpr size_t SINGLE_CHARACTER_TERMINALS_COUNT="+str(len(singleByte))+";\n")
    f.write("constexpr uint8_t SINGLE_CHARACTER_TERMINALS[SINGLE_CHARACTER_TERMINALS_COUNT]={\n")
    for e in singleByte:
        f.write("\t'"+str(e.pattern.value).replace('\n','\\n').replace('\r','\\r').replace('\t','\\t')+"', // SingleCharacterTerminals::"+e.name+"\n")
    f.write("};\n") 
    # Keywords
    f.write("enum class Keywords:uint8_t{\n\t")
    for e in keywords:
        f.write(e.name+", ")
    f.write("\n};\nconstexpr size_t KEYWORDS_COUNT="+str(len(keywords))+";\n")
    f.write("constexpr const char* KEYWORDS[KEYWORDS_COUNT]={\n")
    for e in keywords:
        f.write("\t\""+str(e.pattern.value)+"\", // Keywords::"+e.name+"\n")
    f.write("};\n")
    f.write("constexpr const uint8_t KEYWORDS_SIZE[KEYWORDS_COUNT]={\n\t")
    for e in keywords:
        f.write(str(len(str(e.pattern.value)))+", ")
    f.write("\n};\n")
    writeExpressions(f,expr)
    #writeLexer(f)