#ifdef __PRE
#include <cstdint>
#include <cmath>
extern "C"
{
    uint32_t _tls_index{};
    __declspec(dllimport) void* GetStdHandle(int_fast32_t StdHandle);
    __declspec(dllimport) int_fast32_t WriteConsoleA(void* ConsoleOutput,const void* Buffer,uint_fast32_t NumberOfCharsToWrite,uint_fast32_t* NumberOfCharsWritten,void* Reserved);
}

void print(const void* Text, size_t bytes){
    void* handle = GetStdHandle(-11);
    WriteConsoleA(handle, Text, bytes+1,0,0);
}

template<typename T, size_t N>
struct arr{
    T data[N];
    T& operator[](int index){
        return data[index];
    }
};

template<typename T>
struct bitmask{
    T data;
};

template<typename T>
bitmask<T> initbitmask(){
    return bitmask<T>{0};
}

template<typename T, typename T1 = T>
bool isSet(bitmask<T> Self, const T1 Value){
    return Self.data & (const T)Value;
}

template<typename T, typename T1 = T>
void set(bitmask<T>& Self, const T1 Value, bool On){
    if (On) Self.data |= (const T)Value; else Self.data &= ~(const T)Value;
}

template<typename T>
T* ptr(T& Value){
    return &Value;
}

void* offset(void* Ptr, size_t Bytes){
    return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(Ptr)+Bytes);
}

struct strlit{const char* ptr; size_t bytes;};

void print(strlit string){
    void* handle = GetStdHandle(-11);
    WriteConsoleA(handle, string.ptr, string.bytes+1,0,0);
}

void* ptr(strlit str){
    return reinterpret_cast<void*>(const_cast<char*>(str.ptr));
}

size_t capacity(strlit str){
    size_t result = 0;
    for(;str.ptr[result] != 0; ++result){}
    return result+1;
}

size_t length(strlit str){
    return capacity(str);
}

size_t bytes(strlit str){
    return capacity(str);
}

static char stackbuf[64];

strlit toString(size_t value){
    char buf[33];
    int offset = 0;
    if (value < 0) {
        buf[offset] = '-';
        ++offset;
    }
    for(;;++offset){
        auto remainder = value % 10;        
        value = value / 10;
        buf[offset] = '0' + remainder;        
        if (value == 0) break;        
    }
    for (auto i = 0; i <= offset;++i){
        stackbuf[i] = buf[offset-i];
    }
    offset++;
    stackbuf[offset] = 0;
    strlit result = {stackbuf, offset};
    return result;
}



template<typename T>
struct vec{
    T* data;
    size_t bytes;
    size_t elements;
    size_t capacity;
    
    T& operator[](int Index){
        return data[Index];
    }
};

template<typename T>
T& at(vec<T>& Vector, size_t Index){
    return Vector[Index];
}

struct mem;
mem initmem(size_t Bytes);
void freemem(mem Self);

template <class T>
vec<T>& resize(vec<T>& Self, size_t TargetElementCount){
    if (TargetElementCount > Self.capacity){
        auto bytes = (((TargetElementCount * sizeof(T)) >> 12)+1)*4096;
        auto memory = initmem(bytes);
        for (auto i = 0; i<Self.elements; ++i){reinterpret_cast<T*>(memory.data)[i]=Self.data[i];}
        Self.data = reinterpret_cast<T*>(memory.data);
        Self.bytes = bytes;
        Self.capacity = bytes / sizeof(T);
    }
    return Self;
}

template <class T>
vec<T>& append(vec<T>& Self, T Value){
    if (Self.elements == Self.capacity){
        resize<T>(Self,Self.elements+1);
    }
    Self.data[Self.elements]=Value;
    ++Self.elements;
    return Self;
}

template <class T>
T& add(vec<T>& Self){    
    if (Self.elements == Self.capacity){
        resize<T>(Self,Self.elements+1);
    }    
    ++Self.elements;
    return Self.data[Self.elements-1];
}

template<class T>
vec<T>& reset(vec<T>& Self){
    Self.elements = 0;
    return Self;
}

template<typename A, typename B=A>
struct pair{
    A left;
    B right;
};

struct strview{
    const uint8_t* str;
    size_t start;
    size_t bytes;
};

strview initstrview(){
    return {nullptr, 0, 0};
}

template<typename T>
vec<T> initvec(){
    return vec<T>{0};
}

void* ptr(strview string){
    return reinterpret_cast<void*>(const_cast<uint8_t*>(string.str+string.start));
}

size_t bytes(strview string){
    return string.bytes;
}

bool isUpperCase(strview string){
    bool result = true;
    for(auto i = string.start; i < string.start+string.bytes; ++i){
        if (string.str[i] < 'A' || string.str[i] > 'Z'){
            result = false;
            break;
        }
    }
    return result;
}

struct cstr{
    const uint8_t* data;
};

void* ptr(cstr cstring){
    return reinterpret_cast<void*>(const_cast<uint8_t*>(cstring.data));
}

size_t capacity(cstr cstring){
    size_t result = 0;
    for(;cstring.data[result] != 0; ++result){}
    return result+1;
}

size_t length(cstr cstring){
    return capacity(cstring)-1;
}

size_t bytes(cstr cstring){
    return capacity(cstring);
}

size_t split(cstr string, strlit tokens, vec<strview>& storage){
    size_t len = length(string);
    size_t last = 0;
    for(size_t i=0; i<len; ++i){
        if (string.data[i]==tokens.ptr[0]){
            strview entry{string.data,last,i-last};
            append(storage, entry);
            last = i+1;
        }        
    }
    if (last < len){
        strview entry{string.data,last,(len+1)-last};
        append(storage, entry);
    }
    return storage.elements;
}

void writeSize(void* BasePtr, size_t Bytes, size_t Offset, size_t Value){
    if (Offset + sizeof(size_t) <= Bytes){
        auto p = reinterpret_cast<size_t*>(reinterpret_cast<char*>(BasePtr)+Offset);
        *p = Value;
    }   
}

size_t readSize(void* BasePtr, size_t Bytes, size_t Offset){
    if (Offset + sizeof(size_t) <= Bytes){
        auto p = reinterpret_cast<size_t*>(reinterpret_cast<char*>(BasePtr)+Offset);
        return *p;
    }       
    return 0;
}

template<class T>
const uint8_t* toConstU8(const T* Ptr){
    return reinterpret_cast<const uint8_t*>(Ptr);
}

size_t bytes(uint8_t Value){
    return 1;
}

struct longstring;
struct mem;
struct shortstring;
struct TokenLocationInfo;
struct Node;
struct Lexer;

using TokenList = pair<vec<uint8_t>,vec<TokenLocationInfo>>;

TokenList lexer(uint8_t* Buffer, size_t Bytes, bool IgnoreSkipables, bool GenerateTokenInfos = false);

template<class T>
T* parser(TokenList& Tokens, vec<T>& pool,uint8_t* Buffer, size_t Bytes);

using TreeFunction = void (*)(const Node*, TokenList&, uint8_t*, size_t, size_t);

void topDown(const Node* Node, TreeFunction Function, TokenList& Tokens,
             uint8_t* Buffer, size_t Bytes, size_t Depth = 0);

void bottomUp(const Node* Node, TreeFunction Function, TokenList& Tokens,
              uint8_t* Buffer, size_t Bytes, size_t Depth = 0);

void printNode(const Node* Node, TokenList& Tokens, uint8_t* Buffer, size_t Bytes, size_t Depth);
void test();
#endif
#ifdef __POST
enum SingleCharacterTerminals:uint8_t{
    Comma=0, LeftBracket, RightBracket, Equal, Colon, Semicolon, Exclamationmark, Tilde,
    BitAnd, BitXor, Star, Pipe, LeftParenthesis, RightParenthesis, Plus, LeftSquareBracket,
    RightSquareBracket, DoubleQuotation, Percent, Minus, SmallerThan, GreaterThan, Underscore,
    Space, Tab, NewLine, Slash, CarriageReturn, Dot, Questionmark, EndOfStream
};

enum RangeTerminals : uint8_t{
    Letter = SingleCharacterTerminals::EndOfStream+1,
    Digit
};
enum MultipleCharacterTerminals: uint8_t{
    Module=RangeTerminals::Digit+1,
    Use, Match, Enum, Interface, Operator, Struct, Constants, Aliases, Decoractor,
    AST, Const, Embed, For, In, Loop, While, If, Else, Return, Mut,
    // Rules
    Arrow,// ->
    DecorationsStart,// [[
    DecorationsEnd,// ]]
    LogicalAnd,// &&
    LogicalOr,// ||
    Unequal,// !=
    LogicalEqual,// ==
    SmallerEqual,// <=
    LargerEqual,// >=
    Increment,// ++
    Decrement,// --
    ScopeAccess,// ::
    Name,// _ | a..z | A..Z ( _ | a..z | A..Z | 0..9)*
    String,// ".*"
    HexNumber,// 0x (0..9 | a..f | A..F)+
    DecimalNumber,// (0..9)+
    SinglelineComment,// // !(\r|\n)*
    MultilineComment// /* .* */
};

TokenList lexer(uint8_t* Buffer, size_t Bytes, bool IgnoreSkipables, bool GenerateTokenInfos){
    TokenList result;
    result.left = initvec<uint8_t>();
    result.right = initvec<TokenLocationInfo>();
    uint8_t terminals[SingleCharacterTerminals::EndOfStream+1] = {',', '{', '}', '=', ':', ';', '!', '~', '&','^','*', '|', '(', ')',  '+','[',']','\"', '%', '-', '<', '>', '_', ' ', '\t', '\n', '/', '\r','.', '?', 0};
    const size_t SkipablesCount = 6;
    uint8_t skipables[SkipablesCount] = {SingleCharacterTerminals::Space, SingleCharacterTerminals::Tab,
        SingleCharacterTerminals::CarriageReturn,SingleCharacterTerminals::NewLine, 
        MultipleCharacterTerminals::SinglelineComment,MultipleCharacterTerminals::MultilineComment};
    const size_t KeywordCount = MultipleCharacterTerminals::Mut-RangeTerminals::Digit;
    const char* Keywords[KeywordCount] = {"module", "use", "match", "enum", "interface", "operator",
        "struct", "constants", "aliases", "decorator", "__AST__", "const", "embed", "for", "in", "loop",
        "while", "if", "else", "return", "mut"};
    const uint8_t KeywordSize[KeywordCount] = {6,3,5,4,9,8, 6,9,7,9,7,5,5,3,2,4, 5,2,4,6,3};
    // token infos
    uint64_t col = 1;
    uint64_t line = 1;
    for (auto c = 0; c < Bytes;){
        uint8_t terminal = 255;
        auto start = c;
        // Single byte terminals.
        for (auto i = 0; i< SingleCharacterTerminals::EndOfStream+1; ++i)
            if (Buffer[c] == terminals[i]){
                terminal = i;
                c+=1;
                break;
            }
        // Range terminals.
        // Rule: letter or digit        
        if (terminal == 255){
            switch(Buffer[c]){
                // letter
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
                case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
                case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': 
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': 
                case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
                case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                    terminal = RangeTerminals::Letter;
                    break;
                // digit
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    terminal = RangeTerminals::Digit;
                    break;
            }
            // Go to the next byte even if the current one couldn't solved.
            c+=1; 
        }

        // Multi byte terminals.
        // Rule: Arrow
        if (terminal == SingleCharacterTerminals::Minus){// "->"
            if (Buffer[c] == '>'){
                terminal = MultipleCharacterTerminals::Arrow;
                c+=1;
            }
        }
        // Rule: DecorationsStart
        if (terminal == SingleCharacterTerminals::LeftSquareBracket){// "[["
            if (Buffer[c] == '['){
                terminal = MultipleCharacterTerminals::DecorationsStart;
                c+=1;
            }
        }
        // Rule: DecorationsEnd
        if (terminal == SingleCharacterTerminals::RightSquareBracket){// "]]"
            if (Buffer[c] == ']'){
                terminal = MultipleCharacterTerminals::DecorationsEnd;
                c+=1;
            }
        }
        // Rule: LogicalAnd
        if (terminal == SingleCharacterTerminals::BitAnd){// "&&"
            if (Buffer[c] == '&'){
                terminal = MultipleCharacterTerminals::LogicalAnd;
                c+=1;
            }
        }
        // Rule: LogicalOr
        if (terminal == SingleCharacterTerminals::Pipe){// "||"
            if (Buffer[c] == '|'){
                terminal = MultipleCharacterTerminals::LogicalOr;
                c+=1;
            }
        }
        // Rule: Unequal
        if (terminal == SingleCharacterTerminals::Exclamationmark){// "!="
            if (Buffer[c] == '='){
                terminal = MultipleCharacterTerminals::Unequal;
                c+=1;
            }
        }
        // Rule: Equal
        if (terminal == SingleCharacterTerminals::Equal){// "=="
            if (Buffer[c] == '='){
                terminal = MultipleCharacterTerminals::LogicalEqual;
                c+=1;
            }
        }
        // Rule: SmallerEqual
        if (terminal == SingleCharacterTerminals::SmallerThan){// "<="
            if (Buffer[c] == '='){
                terminal = MultipleCharacterTerminals::SmallerEqual;
                c+=1;
            }
        }
        // Rule: LargerEqual
        if (terminal == SingleCharacterTerminals::GreaterThan){// ">="
            if (Buffer[c] == '='){
                terminal = MultipleCharacterTerminals::LargerEqual;
                c+=1;
            }
        }
        // Rule: Increment
        if (terminal == SingleCharacterTerminals::Plus){// "++"
            if (Buffer[c] == '+'){
                terminal = MultipleCharacterTerminals::Increment;
                c+=1;
            }
        }
        // Rule: Decrement
        if (terminal == SingleCharacterTerminals::Minus){// "--"
            if (Buffer[c] == '-'){
                terminal = MultipleCharacterTerminals::Decrement;
                c+=1;
            }
        }
        // Rule: ScopeAccess
        if (terminal == SingleCharacterTerminals::Colon){// "::"
            if (Buffer[c] == ':'){
                terminal = MultipleCharacterTerminals::ScopeAccess;
                c+=1;
            }
        }
        // Rule: SinglelineComment
        if (terminal == SingleCharacterTerminals::Slash){// "//" !("\n"|"\r")*
            if (Buffer[c] == '/'){
                auto lookAhead = 0;
                for(;;){
                    ++lookAhead;
                    if (Buffer[c+lookAhead] == '\n' || Buffer[c+lookAhead] == '\r'){
                        break;
                    }
                }
                c+=lookAhead;
                terminal = MultipleCharacterTerminals::SinglelineComment;
            }
        }
        // Rule: MultilineComment
        if (terminal == SingleCharacterTerminals::Slash){// /* .* */
            if (Buffer[c] == '*'){
                auto lookAhead = 0;
                for(;;){
                    ++lookAhead;
                    if (Buffer[c+lookAhead] == '*' && Buffer[c+lookAhead+1] == '/'){
                        ++lookAhead;
                        break;
                    }
                }
                c+=lookAhead;
                terminal = MultipleCharacterTerminals::MultilineComment;
            }
        }
        // Rule: HexNumber | DecimalNumber
        if (terminal == RangeTerminals::Digit){
            // Rule: HexNumber 0x (0..9 | a..f | A..F)+
            if (Buffer[c-1] == '0' && (Buffer[c] == 'x' || Buffer[c] == 'X')){
                auto lookAhead = 0;
                for(;;lookAhead++){
                    switch(Buffer[c+lookAhead]){
                    // hex-letter
                    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
                    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                    // digit
                    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                        continue;
                    }
                    break;
                }            
                c+=lookAhead;
                terminal = MultipleCharacterTerminals::HexNumber;
            } else {
                // Rule: DecimalNumber (0..9)+
                auto lookAhead = 0;
                for(;;lookAhead++){
                    switch(Buffer[c+lookAhead]){
                    // digit
                    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                        continue;
                    }
                    break;
                }            
                c+=lookAhead;
                terminal = MultipleCharacterTerminals::DecimalNumber;
            }
        }
        // Rule: string
        if (terminal==SingleCharacterTerminals::DoubleQuotation){// "\"" .* "\""
            auto lookAhead = 0;
            for(;;){
                ++lookAhead;
                if ((c+lookAhead)==Bytes || Buffer[c+lookAhead] == '\"'){
                    break;
                }
            }
            if (Buffer[c+lookAhead] == '\"'){
                ++lookAhead;
                c+=lookAhead;
                terminal = MultipleCharacterTerminals::String;
            }
        }
        // Rule: name
        if (terminal==RangeTerminals::Letter || terminal==SingleCharacterTerminals::Underscore){ // ("_" | LETTER) ("_" | LETTER | DIGIT)*
            auto lookAhead = 0;
            for(;;lookAhead++){
                switch(Buffer[c+lookAhead]){
                // letter
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
                case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
                case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': 
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': 
                case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
                case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                // digit
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                // underscore
                case '_':
                    continue;
                }
                break;
            }            
            c+=lookAhead;
            terminal = MultipleCharacterTerminals::Name;
            // Check keywords.
            for (auto i = 0; i < KeywordCount; ++i){
                auto bytes = KeywordSize[i];
                if (c-start == bytes){
                    auto j = 0;
                    for (; j < bytes; ++j){
                        if (Buffer[start+j] != Keywords[i][j]){
                            break;
                        }
                    }
                    if (j == bytes){
                        terminal = i+static_cast<uint8_t>(MultipleCharacterTerminals::Module);
                        break;
                    }
                }                
            }            
        }

        bool skip = false;
        if (IgnoreSkipables){
            for (auto i = 0; i < SkipablesCount; ++i){
                if (skipables[i] == terminal){
                    skip = true;
                    break;
                }
            }
        }

        if (skip == false){            
            append(result.left, terminal);
            if (GenerateTokenInfos){
                TokenLocationInfo v = {start, c-start, line, col};
                append(result.right, v);
            }
        }

        if (GenerateTokenInfos){
            if (terminal == SingleCharacterTerminals::NewLine){
                col = 1;
                line++;
            } else {
                col+=c-start;
            }
        }
    }
    return result;
}

enum class NodeKind:uint8_t{
    Unit=0, Module, Name, Struct, Function, Constants, Aliases, AST_Decorator, Use, Operator,
    Interface, Enum, String, Match, InterfaceFunction, FunctionDeclaration, ReturnType,
    ParameterList, VariableDeclaration, TypeDeclaration, Unmutable, TypeStaticArray, DecimalNumber,
    Decorations, Decor, Block, AliasDeclaration, Embed, StatementList, Return, For, While, Loop,
    VariableDefinition, Constant, Mutable, InitList, LogicalOperation, BinaryOperation, UnaryOperation,
    Expression, HexNumber, FunctionCall, GetEnumValue, GetAttribute, PostExpression, Assign,
    IfExpression
};

const size_t NodeKindCount = 48;
const char* NodeKindString[NodeKindCount]={"Unit","Module","Name","Struct","Function","Constants",
    "Aliases", "AST_Decorator", "Use", "Operator", "Interface", "Enum", "String", "Match", "InterfaceFunction",
    "FunctionDeclaration", "ReturnType", "ParameterList", "VariableDeclaration", "TypeDeclaration", "Unmutable",
    "TypeStaticArray","DecimalNumber", "Decorations", "Decor", "Block", "AliasDeclaration", "Embed", "StatementList",
    "Return", "For", "While", "Loop", "VariableDefinition", "Constant", "Mutable", "InitList", "LogicalOperation",
    "BinaryOperation", "UnaryOperation", "Expression", "HexNumber", "FunctionCall", "GetEnumValue",
    "GetAttribute", "PostExpression", "Assign","IfExpression"};
const size_t NodeKindStringSize[NodeKindCount]={4,6,4,6,8,9,7,13,3,8,9,4,6,5,17,19,10,13,19,15,9,15,13,11,5,5,16,5, 
    13,6,3,5,4,18,8,7,8, 16,15,14,10,9,12,12,12,14,6,12};

Node& createNode(vec<Node>& Pool, NodeKind Kind){
    Node& result = add(Pool);
    result.Kind = static_cast<uint8_t>(Kind);
    result.FirstChild = nullptr;
    result.Next = nullptr;
    return result;
}

void attachNext(Node& Current, Node& Last){
    Node* current = &Current;
    while(current->Next != nullptr){
        current = current->Next;
    }
    current->Next = &Last;
}

void attachChild(Node& Parent, Node& Child){
    if (Parent.FirstChild == nullptr){
        Parent.FirstChild = &Child;
    } else {
        attachNext(*Parent.FirstChild, Child);
    }
}

void moveLastNodeTo(Node& Parent, Node& Target){
    if (Parent.FirstChild!=nullptr){
        Node* current = Parent.FirstChild;
        while(current->Next != nullptr && current->Next->Next != nullptr){
            current = current->Next;
        }
        if (current->Next == nullptr){// only FirstChild set
            attachChild(Target, *Parent.FirstChild);
        } else {
            attachChild(Target, *current->Next);
            current->Next = nullptr;
        }
    }
}

enum RegularExpressionOperator{
    ZeroOrMore=0,// X* KleeneStar
    OneOrMore,// X+ -> X,X*
    ZeroOrOne,// X? -> 
    GroupStart,// ( ...
    GroupEnd,// ... )
    Concatanation,// X,Y
    Union// X|Y
};

// Convert a regex into a postfix notatation rule.
// The algorithm used is Shunting-Yard.
vec<uint8_t> ConvertRegexToPostfixExpression(vec<uint8_t>& Input){
    vec<uint8_t> result = initvec<uint8_t>();
    uint8_t stack[256];
    uint8_t stackNextIndex = 0;
    if (Input.elements > 0){
        for (auto i = 0; i < Input.elements; ++i){
            switch(Input[i]){
                case RegularExpressionOperator::ZeroOrMore:
                case RegularExpressionOperator::OneOrMore:
                case RegularExpressionOperator::ZeroOrOne:
                case RegularExpressionOperator::Union:
                case RegularExpressionOperator::Concatanation:
                    if (stackNextIndex == 0 || stack[stackNextIndex-1]>Input[i]){
                        stack[stackNextIndex]=Input[i];
                        ++stackNextIndex;
                    } else {
                        --stackNextIndex;
                        append(result,stack[stackNextIndex]);
                        --i;
                    }
                    break;            
                case RegularExpressionOperator::GroupStart:
                    stack[stackNextIndex]=Input[i];
                    ++stackNextIndex;
                    break;
                case RegularExpressionOperator::GroupEnd:
                    while(stack[stackNextIndex-1]!=RegularExpressionOperator::GroupStart){
                        --stackNextIndex;
                        append(result,stack[stackNextIndex]);                    
                    }
                    --stackNextIndex;
                    break;
                default:
                    append(result,Input[i]);
                    break;
            }
        }
        while(stackNextIndex > 0){
            --stackNextIndex;
            append(result,stack[stackNextIndex]);
        }
    }    
    return result;
}

void appendTerminal(vec<uint8_t>& In, uint8_t Terminal){
    append(In,static_cast<uint8_t>(RegularExpressionOperator::Union+1+Terminal));
}

void appendRule(vec<uint8_t>& In, uint8_t Rule){
    append(In,static_cast<uint8_t>(RegularExpressionOperator::Union+1+MultipleCharacterTerminals::MultilineComment+1+Rule));
}

void appendOperator(vec<uint8_t>& In, uint8_t Operator){
    append(In,static_cast<uint8_t>(Operator));
}

struct NFANode;

struct NFATransition{
    uint8_t Input;
    NFANode* Target;
    NFATransition* Next;
};

struct NFANode{
    NFATransition* FirstTransition;
};

struct NFA{
    NFANode* Start;
    NFANode* End;
};

const uint8_t Empty = 255;

NFA ConvertPostfixExpressionToNFA(vec<uint8_t>& PostfixExpression, vec<NFATransition>& TransitionPool, vec<NFANode>& NodePool){
    NFA result={nullptr,nullptr};
    
    NFA ruleStack[128];
    uint8_t ruleStackIndex = 0;

    for(auto i = 0; i < PostfixExpression.elements; ++i){
        switch (PostfixExpression[i]){
            case RegularExpressionOperator::ZeroOrMore:
                result.Start = &add(NodePool);// new start
                result.End = &add(NodePool);// new end
                // new start ----> old start
                result.Start->FirstTransition=&add(TransitionPool);
                result.Start->FirstTransition->Target = ruleStack[ruleStackIndex-1].Start;
                result.Start->FirstTransition->Input = Empty;
                // new start ----> new end
                result.Start->FirstTransition->Next=&add(TransitionPool);
                result.Start->FirstTransition->Next->Target = result.End;
                result.Start->FirstTransition->Next->Input = Empty;
                // old end ----> new end
                ruleStack[ruleStackIndex-1].End->FirstTransition=&add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Target = result.End;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Input = Empty;
                // old end ----> old start
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next=&add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next->Target = ruleStack[ruleStackIndex-1].Start;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next->Input = Empty;
                // replace the latest nfa
                ruleStack[ruleStackIndex-1].Start = result.Start;
                ruleStack[ruleStackIndex-1].End = result.End;
                break;
            case RegularExpressionOperator::OneOrMore:
                result.Start = &add(NodePool);// new start
                result.End = &add(NodePool);// new end
                // new start ----> old start
                result.Start->FirstTransition=&add(TransitionPool);
                result.Start->FirstTransition->Target = ruleStack[ruleStackIndex-1].Start;
                result.Start->FirstTransition->Input = Empty;
                // old end ----> new end
                ruleStack[ruleStackIndex-1].End->FirstTransition=&add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Target = result.End;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Input = Empty;
                // old end ----> old start
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next=&add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next->Target = ruleStack[ruleStackIndex-1].Start;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next->Input = Empty;
                // replace the latest nfa
                ruleStack[ruleStackIndex-1].Start = result.Start;
                ruleStack[ruleStackIndex-1].End = result.End;            
                break;
            case RegularExpressionOperator::ZeroOrOne:
                result.Start = &add(NodePool);// new start
                result.End = &add(NodePool);// new end
                // new start ----> old start
                result.Start->FirstTransition=&add(TransitionPool);
                result.Start->FirstTransition->Target = ruleStack[ruleStackIndex-1].Start;
                result.Start->FirstTransition->Input = Empty;
                // new start ----> new end
                result.Start->FirstTransition->Next=&add(TransitionPool);
                result.Start->FirstTransition->Next->Target = result.End;
                result.Start->FirstTransition->Next->Input = Empty;
                // old end ----> new end
                ruleStack[ruleStackIndex-1].End->FirstTransition=&add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Target = result.End;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Input = Empty;
                // replace the latest nfa
                ruleStack[ruleStackIndex-1].Start = result.Start;
                ruleStack[ruleStackIndex-1].End = result.End;            
                break;
            case RegularExpressionOperator::Union:
                result.Start = &add(NodePool);// new start
                result.End = &add(NodePool);// new end
                // new start ----> old start
                result.Start->FirstTransition=&add(TransitionPool);
                result.Start->FirstTransition->Target = ruleStack[ruleStackIndex-1].Start;
                result.Start->FirstTransition->Input = Empty;
                // new start ----> 2nd oldest start
                result.Start->FirstTransition->Next=&add(TransitionPool);
                result.Start->FirstTransition->Next->Target = ruleStack[ruleStackIndex-2].Start;
                result.Start->FirstTransition->Next->Input = Empty;
                // old end ----> new end
                ruleStack[ruleStackIndex-1].End->FirstTransition=&add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Target = result.End;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Input = Empty;
                // 2nd oldest end ----> new end
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next = &add(TransitionPool);
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next->Target = result.End;
                ruleStack[ruleStackIndex-1].End->FirstTransition->Next->Input = Empty;
                // replace the latest nfa
                ruleStack[ruleStackIndex-2].Start = result.Start;
                ruleStack[ruleStackIndex-2].End = result.End;
                // we used two nfa from the stack but put only one back
                --ruleStackIndex;
                break;
            case RegularExpressionOperator::Concatanation:
                // 2nd oldest end merge with old start
                ruleStack[ruleStackIndex-2].End->FirstTransition=ruleStack[ruleStackIndex-1].Start->FirstTransition;
                ruleStack[ruleStackIndex-2].End = ruleStack[ruleStackIndex-1].End;
                // we used two nfa from the stack but put only one back
                --ruleStackIndex;
                break;
            default:                
                ruleStack[ruleStackIndex].Start=&add(NodePool);
                ruleStack[ruleStackIndex].End=&add(NodePool);
                ruleStack[ruleStackIndex].Start->FirstTransition=&add(TransitionPool);
                ruleStack[ruleStackIndex].Start->FirstTransition->Input = PostfixExpression[i]-RegularExpressionOperator::Union-1;
                ruleStack[ruleStackIndex].Start->FirstTransition->Target = ruleStack[ruleStackIndex].End;
                ruleStack[ruleStackIndex].Start->FirstTransition->Next = nullptr;
                ruleStack[ruleStackIndex].End->FirstTransition = nullptr;
                ++ruleStackIndex;
                break;
        }
    }
    if (ruleStackIndex==1){
        --ruleStackIndex;
        result.Start = ruleStack[ruleStackIndex].Start;
        result.End = ruleStack[ruleStackIndex].End;
    } else {// Error
        result.Start = nullptr;
        result.End = nullptr;
    }
    return result;
}

size_t matchNFA(vec<NFANode*>& Rules, TokenList& Tokens, size_t StartRule, size_t TokenOffset){
    auto* currentTransition = Rules[StartRule]->FirstTransition;
    size_t currentToken = 0;
    while(currentTransition != nullptr && currentToken < Tokens.left.elements){
        if (currentTransition->Input > RegularExpressionOperator::Union+1+MultipleCharacterTerminals::MultilineComment &&
            currentTransition->Input < 255){
            auto ruleIndex = currentTransition->Input-(RegularExpressionOperator::Union+1+MultipleCharacterTerminals::MultilineComment+1);
            currentToken += matchNFA(Rules,Tokens,ruleIndex,currentToken);
            currentTransition = currentTransition->Target->FirstTransition;
        } else {
            if (currentTransition->Input == Tokens.left[currentToken]){
                currentToken++;
                currentTransition = currentTransition->Target->FirstTransition;
            } else {
                if (currentTransition->Input == 255){
                        
                } else {
                    currentTransition = currentTransition->Next;
                } 
            }
        }        
    }
    return currentToken;
}

void test(){    
    vec<uint8_t> in = initvec<uint8_t>();
    vec<NFATransition> transitions=initvec<NFATransition>();
    vec<NFANode> nodes=initvec<NFANode>();
    vec<NFANode*> rules=initvec<NFANode*>();
    // loop: "loop" "{" statement+ "}"
    appendTerminal(in,MultipleCharacterTerminals::Loop);
    appendOperator(in,RegularExpressionOperator::Concatanation);
    appendTerminal(in,SingleCharacterTerminals::LeftBracket);
    appendOperator(in,RegularExpressionOperator::Concatanation);
    appendRule(in,rules.elements+1);
    appendOperator(in,RegularExpressionOperator::OneOrMore);
    appendOperator(in,RegularExpressionOperator::Concatanation);
    appendTerminal(in,SingleCharacterTerminals::RightBracket);
    auto postfixExpression = ConvertRegexToPostfixExpression(in);
    append(rules, ConvertPostfixExpressionToNFA(postfixExpression,transitions,nodes).Start);
    reset(in);

    // statement: "return"
    appendTerminal(in,MultipleCharacterTerminals::Return);
    postfixExpression = ConvertRegexToPostfixExpression(in);
    append(rules,ConvertPostfixExpressionToNFA(postfixExpression,transitions,nodes).Start);
    reset(in);

    auto input = strlit("loop { return }",16);
    auto tokens = lexer(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(input.ptr)), input.bytes,true, true);
    auto processedTokens = matchNFA(rules, tokens,0,0);
}

bool parseMethodCall(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseTestExpression(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseFunctionCall(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseTypeDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseFunctionDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseVariableDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseStatement(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);
bool parseTypeStructure(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes);

bool parseName(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Name){
        auto& name = createNode(pool, NodeKind::Name);
        name.TokenIndex = Offset;
        attachChild(Parent, name);
        ++Offset;
        return true;
    }
    return false;    
}

bool parseString(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::String){
        auto& name = createNode(pool, NodeKind::String);
        name.TokenIndex = Offset;
        attachChild(Parent, name);
        ++Offset;
        return true;
    }
    return false;    
}

bool parseDecNumber(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::DecimalNumber){
        auto& dec = createNode(pool, NodeKind::DecimalNumber);
        dec.TokenIndex = Offset;
        attachChild(Parent, dec);
        ++Offset;
        return true;
    }
    return false;    
}

bool parseHexNumber(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::HexNumber){
        auto& hex = createNode(pool, NodeKind::HexNumber);
        hex.TokenIndex = Offset;
        attachChild(Parent, hex);
        ++Offset;
        return true;
    }
    return false;    
}

bool parseMatch(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Match){
        auto& name = createNode(pool, NodeKind::Match);
        name.TokenIndex = Offset;
        attachChild(Parent, name);
        ++Offset;
        return true;
    }
    return false;    
}

bool parseLeftBracket(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::LeftBracket){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseRightBracket(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::RightBracket){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseLeftParenthesis(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::LeftParenthesis){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseRightParenthesis(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::RightParenthesis){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseSmallerThan(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::SmallerThan){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseGreaterThan(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::GreaterThan){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseColon(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::Colon){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseComma(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::Comma){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseEqual(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::Equal){
        ++Offset;
        return true;
    }
    return false;    
}

bool parseIsMutate(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Mut){
        ++Offset;
        auto& mut = createNode(pool, NodeKind::Mutable);
        attachChild(Parent, mut);
        return true;
    }
    return false;  
}

bool parseIsConst(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Const){
        ++Offset;
        auto& constant = createNode(pool, NodeKind::Constant);
        attachChild(Parent, constant);
        return true;
    }
    return false;  
}

// ?decor: NAME [":" (DEC_NUMBER | ESCAPED_STRING)]
bool parseDecor(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& deco = createNode(pool, NodeKind::Decor);
    deco.TokenIndex = Offset;
    attachChild(Parent, deco);
    if (parseName(deco,Offset, Tokens,pool,Buffer,Bytes)){
        if (parseColon(deco,Offset, Tokens,pool,Buffer,Bytes)){
            return parseDecNumber(deco,Offset, Tokens,pool,Buffer,Bytes) ||
                   parseString(deco,Offset, Tokens,pool,Buffer,Bytes);
        }
    }
    return false;    
}

// decorations : "[[" decor ("," decor)* "]]"
bool parseDecorations(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::DecorationsStart){
        Offset++;
        auto& decorations = createNode(pool, NodeKind::Decorations);
        attachChild(Parent, decorations);
        bool result = parseDecor(decorations,Offset, Tokens,pool,Buffer,Bytes);
        while(parseComma(decorations, Offset, Tokens,pool,Buffer,Bytes)){
            result = result && parseDecor(decorations,Offset, Tokens,pool,Buffer,Bytes);
        }
        if (result && Tokens.left[Offset] == MultipleCharacterTerminals::DecorationsEnd){
            Offset++;
            return true;
        }
    }
    return false;
}

// embed: "embed" NAME
bool parseEmbed(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if(Tokens.left[Offset] == MultipleCharacterTerminals::Embed){
        auto& embed = createNode(pool, NodeKind::Embed);
        attachChild(Parent, embed);
        Offset++;
        return parseName(embed,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// struct: "struct" NAME "{" (variable_declaration|embed)* "}"
bool parseStruct(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Struct){
        Offset++;
        auto& _struct = createNode(pool, NodeKind::Struct);
        attachChild(Parent, _struct);
        if (parseName(_struct, Offset, Tokens,pool,Buffer,Bytes) && parseLeftBracket(_struct, Offset, Tokens,pool,Buffer,Bytes)){
            while(Tokens.left[Offset] != SingleCharacterTerminals::RightBracket && (
                parseVariableDeclaration(_struct,Offset, Tokens,pool,Buffer,Bytes) || parseEmbed(_struct,Offset, Tokens,pool,Buffer,Bytes))){}
            return parseRightBracket(_struct,Offset, Tokens,pool,Buffer,Bytes);
        }
    }
    return false;
}

bool parseFunction(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return false;
}

bool parseConstants(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return false;
}

// alias_declaration: NAME "=" (type_declaration | function_declaration)
bool parseAliasDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& decl = createNode(pool, NodeKind::AliasDeclaration);
    attachChild(Parent, decl);
    if (parseName(decl, Offset, Tokens,pool,Buffer,Bytes) && parseEqual(decl, Offset, Tokens,pool,Buffer,Bytes)){
        return parseTypeDeclaration(decl, Offset, Tokens,pool,Buffer,Bytes) || parseFunctionDeclaration(decl, Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// aliases: "aliases" "{" alias_declaration+ "}"
bool parseAliases(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Aliases){
        Offset++;
        auto& aliases = createNode(pool, NodeKind::Aliases);
        attachChild(Parent, aliases);
        if (parseLeftBracket(aliases, Offset, Tokens,pool,Buffer,Bytes)){
            bool result = parseAliasDeclaration(aliases, Offset, Tokens,pool,Buffer,Bytes);
            while(Tokens.left[Offset] != SingleCharacterTerminals::RightBracket && 
                  parseAliasDeclaration(aliases, Offset, Tokens,pool,Buffer,Bytes)){}
            if (result && parseRightBracket(aliases, Offset, Tokens,pool,Buffer,Bytes)){
                return true;
            }
        }
    }
    return false;
}

bool parseAST_Decorator(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return false;
}

// subscript_list: "[" SIGNED_DEC_NUMBER "]" -> subscript_offset
//               | subscript_slice | "[" NAME "]" -> subscript_variable
// subscript_slice: "[" molecule ":" molecule "]"
bool parseSubscriptList(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return false;
}

// atom: "(" test_expr ")" -> compound
//     | NAME | HEX_NUMBER -> number | DEC_NUMBER -> number | ESCAPED_STRING -> string   
bool parseAtom(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (parseLeftParenthesis(Parent, Offset, Tokens, pool, Buffer,Bytes)){
        auto& expr = createNode(pool, NodeKind::Expression);
        attachChild(Parent, expr);
        return parseTestExpression(expr, Offset, Tokens, pool, Buffer,Bytes) && 
               parseRightParenthesis(Parent, Offset, Tokens, pool, Buffer,Bytes);
    } else {
        return parseName(Parent, Offset, Tokens, pool, Buffer,Bytes)||
               parseDecNumber(Parent, Offset, Tokens, pool, Buffer,Bytes)||
               parseString(Parent, Offset, Tokens, pool, Buffer,Bytes) ||
               parseHexNumber(Parent, Offset, Tokens, pool, Buffer,Bytes);
    }
    return false;
}

// molecule: function_call | atom
bool parseMolecule(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return parseFunctionCall(Parent, Offset, Tokens, pool, Buffer, Bytes) || parseAtom(Parent, Offset, Tokens, pool, Buffer,Bytes);
}

// get_attribute: "." NAME
bool parseGetAttribute(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == SingleCharacterTerminals::Dot &&
        Tokens.left[Offset+1] == MultipleCharacterTerminals::Name){
        auto& getAttrib = createNode(pool, NodeKind::GetAttribute);
        Offset++;
        moveLastNodeTo(Parent,getAttrib);
        parseName(getAttrib, Offset, Tokens, pool, Buffer, Bytes);
        attachChild(Parent, getAttrib);
        return true;
    }
    return false;
}

// get_enum_value: "::"" NAME
bool parseGetEnumValue(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::ScopeAccess &&
        Tokens.left[Offset+1] == MultipleCharacterTerminals::Name){
        auto& getEnumVal = createNode(pool, NodeKind::GetEnumValue);
        getEnumVal.TokenIndex = Offset+1;
        moveLastNodeTo(Parent,getEnumVal);
        attachChild(Parent, getEnumVal);
        Offset+=2;
        return true;
    }
    return false;
}

// post_expr: post_expr ("++" | "--" | subscript_list | get_attribute | get_enum_value | method_call)
//          | molecule
bool parsePostExpression(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseMolecule(Parent, Offset, Tokens, pool, Buffer,Bytes);
    bool matchedOneOrMoreTimes = false;
    while (result && (parseMethodCall(Parent, Offset, Tokens, pool, Buffer, Bytes) ||
        Tokens.left[Offset] == MultipleCharacterTerminals::Increment ||
        Tokens.left[Offset] == MultipleCharacterTerminals::Decrement ||
        parseSubscriptList(Parent, Offset, Tokens, pool, Buffer, Bytes) ||
        parseGetAttribute(Parent, Offset, Tokens, pool, Buffer, Bytes) ||
        parseGetEnumValue(Parent, Offset, Tokens, pool, Buffer, Bytes))){
        matchedOneOrMoreTimes = true;
        if(Tokens.left[Offset] == MultipleCharacterTerminals::Increment ||
           Tokens.left[Offset] == MultipleCharacterTerminals::Decrement){
            auto& expr = createNode(pool, NodeKind::PostExpression);
            attachChild(Parent, expr);
            expr.TokenIndex = Offset;
            Offset++;
        }
    }
    return result && matchedOneOrMoreTimes; 
}

// unary_expr: ("++" | "--" | "-" | "+" | "!" | "~") unary_expr
//           | post_expr       
bool parseUnaryExpression(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if(Tokens.left[Offset] == MultipleCharacterTerminals::Increment ||
       Tokens.left[Offset] == MultipleCharacterTerminals::Decrement ||
       Tokens.left[Offset] == SingleCharacterTerminals::Minus ||
       Tokens.left[Offset] == SingleCharacterTerminals::Plus ||
       Tokens.left[Offset] == SingleCharacterTerminals::Exclamationmark ||
       Tokens.left[Offset] == SingleCharacterTerminals::Tilde){
        auto& op = createNode(pool, NodeKind::UnaryOperation);
        op.TokenIndex = Offset;
        attachChild(Parent, op);
        Offset++;
        return parseUnaryExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    } else {
        return parsePostExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    }
}

// binary_expr: unary_expr binary_expr_right*
// binary_expr_right: ("&" | "^" | "|" | "+" | "-" | "*" | "/" | "%") unary_expr
bool parseBinaryExpression(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseUnaryExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    while(Tokens.left[Offset] == SingleCharacterTerminals::BitAnd ||
          Tokens.left[Offset] == SingleCharacterTerminals::BitXor ||
          Tokens.left[Offset] == SingleCharacterTerminals::Pipe ||
          Tokens.left[Offset] == SingleCharacterTerminals::Plus ||
          Tokens.left[Offset] == SingleCharacterTerminals::Minus ||
          Tokens.left[Offset] == SingleCharacterTerminals::Star ||
          Tokens.left[Offset] == SingleCharacterTerminals::Slash ||
          Tokens.left[Offset] == SingleCharacterTerminals::Percent ){
        auto& op = createNode(pool, NodeKind::BinaryOperation);
        op.TokenIndex = Offset;
        attachChild(Parent, op);
        Offset++;
        return parseUnaryExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    }
    return result;
}

// test_expr: binary_expr test_expr_right*
// test_expr_right: ("&&" | "||" | "!=" | "==" | "<" | ">" | "<=" | ">=") binary_expr
bool parseTestExpression(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseBinaryExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    while(Tokens.left[Offset] == MultipleCharacterTerminals::LogicalAnd ||
          Tokens.left[Offset] == MultipleCharacterTerminals::LogicalOr ||
          Tokens.left[Offset] == MultipleCharacterTerminals::Unequal ||
          Tokens.left[Offset] == MultipleCharacterTerminals::LogicalEqual ||
          Tokens.left[Offset] == SingleCharacterTerminals::SmallerThan ||
          Tokens.left[Offset] == SingleCharacterTerminals::GreaterThan ||
          Tokens.left[Offset] == MultipleCharacterTerminals::SmallerEqual ||
          Tokens.left[Offset] == MultipleCharacterTerminals::LargerEqual ){
        auto& op = createNode(pool, NodeKind::LogicalOperation);
        op.TokenIndex = Offset;
        attachChild(Parent, op);
        Offset++;
        return parseBinaryExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    }
    return result;
}

// arglist: test_expr ("," test_expr)*
bool parseArgList(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseTestExpression(Parent, Offset, Tokens, pool, Buffer,Bytes);
    while(result && parseComma(Parent, Offset, Tokens, pool, Buffer,Bytes) &&
        parseTestExpression(Parent, Offset, Tokens, pool, Buffer,Bytes)){}
    return result;
}

// init_list: "{}" -> init_default | "{" arglist "}"
bool parseInitList(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (parseLeftBracket(Parent, Offset, Tokens, pool, Buffer,Bytes)){
        auto& init = createNode(pool, NodeKind::InitList);
        attachChild(Parent, init);
        if (parseRightBracket(init, Offset, Tokens, pool, Buffer,Bytes)){
            return true;
        } else {
            return parseArgList(init, Offset, Tokens, pool, Buffer, Bytes) &&
                   parseRightBracket(init, Offset, Tokens, pool, Buffer,Bytes);
        }
    }
    return false;
}

// assign: molecule "=" test_expr
bool parseAssign(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){    
    bool result = parsePostExpression(Parent,Offset, Tokens,pool,Buffer,Bytes) && parseEqual(Parent,Offset, Tokens,pool,Buffer,Bytes);
    if (result){
        auto& assign = createNode(pool, NodeKind::Assign);
        moveLastNodeTo(Parent,assign);
        attachChild(Parent, assign);
        return parseTestExpression(assign,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// variable_definition: (IS_CONST|IS_MUTATE) type_structure NAME "=" (test_expr|init_list)
bool parseVariableDefinition(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Const ||
        Tokens.left[Offset] == MultipleCharacterTerminals::Mut){
        auto& def = createNode(pool, NodeKind::VariableDefinition);
        attachChild(Parent, def);
        auto& type = createNode(pool, NodeKind::TypeDeclaration);
        attachChild(def, type);
        parseIsConst(type,Offset, Tokens,pool,Buffer,Bytes)||parseIsMutate(type,Offset, Tokens,pool,Buffer,Bytes);
        return parseTypeStructure(type,Offset, Tokens,pool,Buffer,Bytes) && parseName(def,Offset, Tokens,pool,Buffer,Bytes) &&
                parseEqual(def,Offset, Tokens,pool,Buffer,Bytes) && (
                parseTestExpression(def,Offset, Tokens,pool,Buffer,Bytes)||parseInitList(def,Offset, Tokens,pool,Buffer,Bytes));
    }
    return false;
}

// return: "return" test_expr
bool parseReturn(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Return){
        Offset++;
        auto& ret = createNode(pool, NodeKind::Return);
        attachChild(Parent, ret);
        return parseTestExpression(ret,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// statements: statement*
bool parseStatements(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& statements = createNode(pool, NodeKind::StatementList);
    attachChild(Parent, statements);
    while(parseStatement(statements, Offset, Tokens,pool,Buffer,Bytes)){}
    return true;
}

// function_call: NAME "(" [arglist] ")"
bool parseFunctionCall(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Name &&
        Tokens.left[Offset+1] == SingleCharacterTerminals::LeftParenthesis){
        auto& fncCall = createNode(pool, NodeKind::FunctionCall);
        attachChild(Parent, fncCall);
        parseName(fncCall, Offset, Tokens, pool, Buffer,Bytes);
        parseLeftParenthesis(Parent, Offset, Tokens, pool, Buffer,Bytes);
        auto& args = createNode(pool, NodeKind::ParameterList);
        attachChild(fncCall, args);
        if (Tokens.left[Offset] != SingleCharacterTerminals::RightParenthesis){
            parseArgList(args, Offset, Tokens, pool, Buffer,Bytes);
        }
        return parseRightParenthesis(Parent, Offset, Tokens, pool, Buffer,Bytes);
    }    
    return false;
}

// method_call: molecule "." NAME "(" [arglist] ")"
bool parseMethodCall(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return false;
}

// for_loop: assign ("," assign)* ";" test_expr
bool parseForLoop(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
return false;
}

// for_range: NAME "in" (molecule | subscript_slice)
bool parseForRange(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
return false;
}

// for_declaration: "for" (for_range|for_loop) "{" statements "}"
bool parseForDeclaraction(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::For){
        Offset++;
        auto& forDecl = createNode(pool, NodeKind::For);
        attachChild(Parent, forDecl);
        return (parseForRange(forDecl,Offset, Tokens,pool,Buffer,Bytes) || parseForLoop(forDecl,Offset, Tokens,pool,Buffer,Bytes)) &&
            parseLeftBracket(forDecl,Offset, Tokens,pool,Buffer,Bytes) && parseStatements(forDecl,Offset, Tokens,pool,Buffer,Bytes) &&
            parseRightBracket(forDecl,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// while: "while" test_expr "{" statement* "}"
bool parseWhile(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::While){
        Offset++;
        auto& whileDecl = createNode(pool, NodeKind::While);
        attachChild(Parent, whileDecl);
        return parseTestExpression(whileDecl,Offset, Tokens,pool,Buffer,Bytes) && parseLeftBracket(whileDecl,Offset, Tokens,pool,Buffer,Bytes) && 
            parseStatements(whileDecl,Offset, Tokens,pool,Buffer,Bytes) && parseRightBracket(whileDecl,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// if_declaration: "if" test_expr "{" statement+ "}" ["else" "{" statement+ "}"]
bool parseIfDeclacration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::If){
        Offset++;
        auto& ifExpr = createNode(pool, NodeKind::IfExpression);
        attachChild(Parent, ifExpr);
        bool result = parseTestExpression(ifExpr,Offset, Tokens,pool,Buffer,Bytes) &&
            parseLeftBracket(ifExpr,Offset, Tokens,pool,Buffer,Bytes) &&
            parseStatements(ifExpr,Offset, Tokens,pool,Buffer,Bytes) &&
            parseRightBracket(ifExpr,Offset, Tokens,pool,Buffer,Bytes);
        if (result && Tokens.left[Offset] == MultipleCharacterTerminals::Else){
            result = result && parseLeftBracket(ifExpr,Offset, Tokens,pool,Buffer,Bytes) &&
                parseStatements(ifExpr,Offset, Tokens,pool,Buffer,Bytes) &&
                parseRightBracket(ifExpr,Offset, Tokens,pool,Buffer,Bytes);
        }
        return result;
    }
    return false;
}

// loop: "loop" "{" statement+ "}"
bool parseLoop(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Loop){
        Offset++;
        auto& loop = createNode(pool, NodeKind::Loop);
        attachChild(Parent, loop);
        return parseLeftBracket(loop,Offset, Tokens,pool,Buffer,Bytes) &&  parseStatements(loop,Offset, Tokens,pool,Buffer,Bytes) && 
            parseRightBracket(loop,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// statement: assign | variable_definition | return | function_call | method_call | for_declaration | while | if_declaration | loop
bool parseStatement(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (parseAssign(Parent, Offset, Tokens,pool,Buffer,Bytes) || 
        parseVariableDefinition(Parent, Offset, Tokens,pool,Buffer,Bytes) || 
        parseReturn(Parent, Offset, Tokens,pool,Buffer,Bytes) ||
        parseFunctionCall(Parent, Offset, Tokens,pool,Buffer,Bytes) ||
        parseMethodCall(Parent, Offset, Tokens,pool,Buffer,Bytes) ||
        parseForDeclaraction(Parent, Offset, Tokens,pool,Buffer,Bytes) ||
        parseWhile(Parent, Offset, Tokens,pool,Buffer,Bytes) ||
        parseIfDeclacration(Parent, Offset, Tokens,pool,Buffer,Bytes) ||
        parseLoop(Parent, Offset, Tokens,pool,Buffer,Bytes)){
        return true;
    }
    return false;
}

// operator: "operator" NAME function_declaration "{" statements "}"
bool parseOperator(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Operator){
        Offset++;
        auto& op = createNode(pool, NodeKind::Operator);
        attachChild(Parent, op);
        return parseName(op,Offset, Tokens,pool,Buffer,Bytes) && parseFunctionDeclaration(op,Offset, Tokens,pool,Buffer,Bytes) && 
            parseLeftBracket(op,Offset, Tokens,pool,Buffer,Bytes) && parseStatements(op,Offset, Tokens,pool,Buffer,Bytes) && 
            parseRightBracket(op,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// variable_declaration: type_declaration NAME
bool parseVariableDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& var = createNode(pool, NodeKind::VariableDeclaration);
    attachChild(Parent, var);
    return parseTypeDeclaration(var, Offset, Tokens,pool,Buffer,Bytes) &&
           parseName(var,Offset, Tokens,pool,Buffer,Bytes);
}

// paramlist : ("(" variable_declaration ("," variable_declaration)* ")" | "(" ")")
bool parseParamList(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& paramList = createNode(pool, NodeKind::ParameterList);
    attachChild(Parent, paramList);
    if (parseLeftParenthesis(paramList, Offset, Tokens,pool,Buffer,Bytes)){
        if (parseRightParenthesis(paramList, Offset, Tokens,pool,Buffer,Bytes)){
                return true;
        } else {
            if (parseVariableDeclaration(paramList, Offset, Tokens,pool,Buffer,Bytes)){
                while(parseComma(paramList, Offset, Tokens,pool,Buffer,Bytes) && 
                    parseVariableDeclaration(paramList, Offset, Tokens,pool,Buffer,Bytes)){
                }
                return parseRightParenthesis(paramList, Offset, Tokens,pool,Buffer,Bytes);
            }
        }
    }    
    return false;
}

// typelist: type_declaration ("," type_declaration)*
bool parseTypeList(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseTypeDeclaration(Parent, Offset, Tokens,pool,Buffer,Bytes);
    while(parseComma(Parent, Offset, Tokens,pool,Buffer,Bytes)){
        result = parseTypeDeclaration(Parent, Offset, Tokens,pool,Buffer,Bytes);
    }
    return result;
}

// type_static_array: "[" DEC_NUMBER "]"
bool parseTypeStaticArray(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (parseLeftBracket(Parent, Offset, Tokens,pool,Buffer,Bytes)){
        auto& array = createNode(pool, NodeKind::TypeStaticArray);
        attachChild(Parent, array);
        return parseDecNumber(array, Offset, Tokens,pool,Buffer,Bytes) &&
               parseRightBracket(Parent, Offset, Tokens,pool,Buffer,Bytes);
    }    
    return false;
}

// type_structure: NAME ["<" typelist ">"] [type_static_array]
bool parseTypeStructure(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseName(Parent, Offset, Tokens,pool,Buffer,Bytes);
    parseSmallerThan(Parent, Offset, Tokens,pool,Buffer,Bytes) && parseTypeList(Parent, Offset, Tokens,pool,Buffer,Bytes) && parseGreaterThan(Parent, Offset, Tokens,pool,Buffer,Bytes);
    parseTypeStaticArray(Parent, Offset, Tokens,pool,Buffer,Bytes);
    return result;
}

// type_declaration: [IS_CONST|IS_MUTATE] type_structure
bool parseTypeDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& type = createNode(pool, NodeKind::TypeDeclaration);
    attachChild(Parent, type);
    parseIsConst(type, Offset, Tokens,pool,Buffer,Bytes) || parseIsMutate(type, Offset, Tokens,pool,Buffer,Bytes);
    
    return parseTypeStructure(type, Offset, Tokens,pool,Buffer,Bytes);
}

// function_declaration: paramlist [ARROW type_declaration]
bool parseFunctionDeclaration(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    bool result = parseParamList(Parent, Offset, Tokens,pool,Buffer,Bytes);
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Arrow){
        ++Offset;
        auto& returnType = createNode(pool, NodeKind::ReturnType);
        attachChild(Parent, returnType);
        result = parseTypeDeclaration(returnType, Offset, Tokens,pool,Buffer,Bytes);
    }
    return result;
}

// interface_function: NAME function_declaration
bool parseInterfaceFunction(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Name){
        auto& ifunc = createNode(pool, NodeKind::InterfaceFunction);
        attachChild(Parent, ifunc);
        return parseName(ifunc, Offset, Tokens,pool,Buffer,Bytes) && parseFunctionDeclaration(ifunc,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;    
}

// interface: INTERFACE NAME LEFTBRACKET interface_function+ RIGHTBRACKET
bool parseInterface(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    // INTERFACE
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Interface){
        ++Offset;
        auto& interface = createNode(pool, NodeKind::Interface);
        attachChild(Parent, interface);
        // NAME LEFTBRACKET
        if (parseName(interface,Offset, Tokens,pool,Buffer,Bytes) &&
            parseLeftBracket(interface,Offset, Tokens,pool,Buffer,Bytes))
        {   
            // interface_function+
            while(parseInterfaceFunction(interface, Offset, Tokens,pool,Buffer,Bytes)){}
            // RIGHTBRACKET
            return parseRightBracket(interface,Offset, Tokens,pool,Buffer,Bytes);
        }
    }
    return false;
}

bool parseEnum(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    return false;
}

// use: USE (STRING | MATCH) ("," (STRING | MATCH))*
bool parseUse(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    // USE
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Use){
        ++Offset;
        auto& use = createNode(pool, NodeKind::Use);
        attachChild(Parent, use);        
        // (STRING | MATCH)
        if (parseString(use,Offset, Tokens,pool,Buffer,Bytes) == false &&
            parseMatch(use,Offset, Tokens, pool, Buffer, Bytes) == false){
            return false;
        }
        // ("," (STRING | MATCH))*
        while(Tokens.left[Offset] == SingleCharacterTerminals::Comma){
            if (parseString(use,Offset, Tokens,pool,Buffer,Bytes) == false &&
                parseMatch(use,Offset, Tokens, pool, Buffer, Bytes) == false){
                return false;
            }
        }
        return true;
    }
    return false;
}

// module: MODULE NAME
bool parseModule(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    if (Tokens.left[Offset] == MultipleCharacterTerminals::Module){
        auto& mod = createNode(pool, NodeKind::Module);
        attachChild(Parent, mod);
        ++Offset;
        return parseName(mod,Offset, Tokens,pool,Buffer,Bytes);
    }
    return false;
}

// block: [decorations] struct | function | constants | aliases | ast_decorator | use | operator | interface | enum
bool parseBlock(Node& Parent, size_t& Offset, TokenList& Tokens, vec<Node>& pool,uint8_t* Buffer, size_t Bytes){
    auto& block = createNode(pool, NodeKind::Block);
    attachChild(Parent, block);
    parseDecorations(block,Offset, Tokens,pool,Buffer,Bytes);
    if (parseStruct(block, Offset, Tokens,pool,Buffer,Bytes) || 
        parseFunction(block, Offset, Tokens,pool,Buffer,Bytes) || 
        parseConstants(block, Offset, Tokens,pool,Buffer,Bytes) ||
        parseAliases(block, Offset, Tokens,pool,Buffer,Bytes) ||
        parseAST_Decorator(block, Offset, Tokens,pool,Buffer,Bytes) ||
        parseUse(block, Offset, Tokens,pool,Buffer,Bytes) ||
        parseOperator(block, Offset, Tokens,pool,Buffer,Bytes) ||
        parseInterface(block, Offset, Tokens,pool,Buffer,Bytes) ||
        parseEnum(block, Offset, Tokens,pool,Buffer,Bytes)){
        return true;
    }
    return false;
}

template<class T>
T* parser(TokenList& Tokens, vec<T>& pool,uint8_t* Buffer, size_t Bytes){
    auto& root = createNode(pool, NodeKind::Unit);
    // unit_: module block*
    size_t offset = 0;
    parseModule(root,offset, Tokens,pool,Buffer,Bytes);
    while(parseBlock(root, offset,Tokens,pool,Buffer,Bytes)){};
    return &root;
}
void topDown(const Node* Node, TreeFunction Function, TokenList& Tokens,
             uint8_t* Buffer, size_t Bytes, size_t Depth){
    Function(Node, Tokens, Buffer, Bytes, Depth);
    if (Node->FirstChild){
        topDown(Node->FirstChild, Function, Tokens, Buffer, Bytes, Depth+1);
    }    
    if (Node->Next){
        topDown(Node->Next, Function, Tokens, Buffer, Bytes, Depth);
    }
}

void bottomUp(const Node* Node, TreeFunction Function, TokenList& Tokens,
              uint8_t* Buffer, size_t Bytes, size_t Depth){    
    if (Node->FirstChild){
        bottomUp(Node->FirstChild, Function, Tokens, Buffer, Bytes, Depth+1);
    }    
    Function(Node, Tokens, Buffer, Bytes, Depth);
    if (Node->Next){
        bottomUp(Node->Next, Function, Tokens, Buffer, Bytes, Depth);
    }
}

void printNode(const Node* Node, TokenList& Tokens, uint8_t* Buffer, size_t Bytes, size_t Depth){
    for(auto i = 0; i < Depth; ++i){
        print({"\t",1});
    }
    print({NodeKindString[Node->Kind],NodeKindStringSize[Node->Kind]});
    print({": ",2});
    //print(toString(Node->TokenIndex));
    if (static_cast<NodeKind>(Node->Kind) == NodeKind::Name ||
        static_cast<NodeKind>(Node->Kind) == NodeKind::String){
        strlit text = {reinterpret_cast<char*>(Buffer)+Tokens.right[Node->TokenIndex].Offset,Tokens.right[Node->TokenIndex].Bytes-1};
        print(text);
    }
    print({"\n",1});
}
#endif