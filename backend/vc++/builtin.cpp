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
    stackbuf[offset+1] = 0;
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

using TokenList = pair<vec<uint8_t>,vec<TokenLocationInfo>>;

TokenList lexer(uint8_t* Buffer, size_t Bytes, bool IgnoreSkipables, bool GenerateTokenInfos = false);

template<class T>
T* parser(TokenList& Tokens, vec<T>& pool,uint8_t* Buffer, size_t Bytes);

using TreeFunction = void (*)(Node*, TokenList&, uint8_t*, size_t, size_t);

void topDown(Node* Node, TreeFunction Function, TokenList& Tokens,
             uint8_t* Buffer, size_t Bytes, size_t Depth = 0);

void bottomUp(Node* Node, TreeFunction Function, TokenList& Tokens,
              uint8_t* Buffer, size_t Bytes, size_t Depth = 0);

void printNode(Node* Node, TokenList& Tokens, uint8_t* Buffer, size_t Bytes, size_t Depth);
#endif
#ifdef __POST
enum SingleCharacterTerminals:uint8_t{
    Colon=0,
    Questionmark,
    Star,
    Pipe,
    LeftParenthesis,
    RightParenthesis,
    LeftBracket,
    RightBracket,
    Comma,
    Plus,
    LeftSquareBracket,
    RightSquareBracket,
    DoubleQuotation,
    Percent,
    Minus,
    GreaterThan,
    Underscore,
    Space,
    Tab,
    NewLine,
    Slash,
    CarriageReturn,
    Dot,
    Backslash,
    EndOfStream
};

enum RangeTerminals : uint8_t{
    Letter = SingleCharacterTerminals::EndOfStream+1,
    Digit
};
enum MultipleCharacterTerminals: uint8_t{
    Ignore=RangeTerminals::Digit,
    Import,
    Alias,
    Name,
    Comment,
    String
};

TokenList lexer(uint8_t* Buffer, size_t Bytes, bool IgnoreSkipables, bool GenerateTokenInfos){
    TokenList result;
    result.left = initvec<uint8_t>();
    result.right = initvec<TokenLocationInfo>();
    Buffer[Bytes] = 0;    
    uint8_t terminals[SingleCharacterTerminals::EndOfStream+1] = {':', '?', '*', '|', '(', ')', '{', '}', ',', '+','[',']','\"', '%', '-', '>', '_', ' ', '\t', '\n', '/', '\r','.', '\\', 0};
    uint8_t skipables[] = {SingleCharacterTerminals::Space,
        SingleCharacterTerminals::Tab,SingleCharacterTerminals::CarriageReturn};

    //const char* TNames[] = {"COLON", "QUESTIONMARK", "START", "PIPE", "LPARA", "RPARA", "LBRACE", "RBRACE", "COMMA", "PLUS", "LBRACKET", "RBRACKET"};
    const char* keywords[3] = {"%ignore", "%import", "->"};
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
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o':
                case 'p':
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                    terminal = RangeTerminals::Letter;
                    break;
                // digit
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    terminal = RangeTerminals::Digit;
                    break;
            }
            // Go to the next byte even if the current one couldn't solved.
            c+=1; 
        }

        // Multi byte terminals.

        // Rule: comment
        if (terminal == 20){// "//" !("\n"|"\r")*
            if (Buffer[c] == '/'){
                auto lookAhead = 0;
                for(;;){
                    ++lookAhead;
                    if (Buffer[c+lookAhead] == '\n' || Buffer[c+lookAhead] == '\r'){
                        break;
                    }
                }
                c+=lookAhead;
                terminal = MultipleCharacterTerminals::Comment;
            }
        }

        // Rule: alias
        if (terminal == 14){// "->"
            if (Buffer[c] == '>'){
                terminal = MultipleCharacterTerminals::Alias;
                c+=1;
            }
        }
        // Rule: command
        if (terminal == 13){// "%" ("ignore"|"import")
            for (auto i = 0; i< 2; ++i){
                bool isSame = true;
                for (auto j = 0; j < 6; ++j){
                    if (Buffer[c+j] != keywords[i][j+1]){
                        isSame = false;
                        break;
                    }
                }
                if (isSame){
                    switch (i)
                    {
                    case 0:
                        terminal = MultipleCharacterTerminals::Ignore;
                        break;
                    case 1:
                        terminal = MultipleCharacterTerminals::Import;
                        break;
                    }                    
                    c+=6;
                    break;
                }
            }
        }
        // Rule: string
        if (terminal==12){// "\"" !("\n"|"\r"|"\"")* "\""
            auto lookAhead = 0;
            for(;;){
                ++lookAhead;
                if (Buffer[c+lookAhead] == '\n' || Buffer[c+lookAhead] == '\r' || Buffer[c+lookAhead] == '\"'){
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
        if (terminal==RangeTerminals::Letter || terminal==16){ // ("_" | LETTER) ("_" | LETTER | DIGIT)*
            auto lookAhead = 0;
            for(;;lookAhead++){
                switch(Buffer[c+lookAhead]){
                // letter
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o':
                case 'p':
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                // digit
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                // underscore
                case '_':
                // dot
                case '.':
                    continue;
                }
                break;
            }            
            c+=lookAhead;
            terminal = MultipleCharacterTerminals::Name;
        }

        bool skip = false;
        if (IgnoreSkipables){
            for (auto i = 0; i < 3; ++i){
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
            if (terminal == 19){
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
    Program=0,
    Template,
    TemplateParameters,
    Rule,
    RuleEmbedded,
    ExpressionList,
    Expression,
    ParentheseExpression,
    OccuranceModifier,
    Import,
    Ignore,
    Name,
    String,
    Alias,
    OptionalExpression,
    TemplateCall
};

const char* NodeKindString[]={"Program","Template","TemplateParameters","Rule", "RuleEmbedded",
                              "ExpressionList","Expression","ParentheseExpression", "OccuranceModifier",
                              "Import","Ignore","Name","String", "Alias", "OptionalExpression", "TemplateCall"};
const size_t NodeKindStringSize[]={7,8,18,4,12,14,10,20,17,6,6,4,6,5,18,12};

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

size_t expressionList(TokenList& Tokens, size_t Offset, Node& ParentNode, vec<Node>& Pool, uint8_t* Buffer, size_t Bytes){
    auto offset = 0;
    // exprlist: expr ("|" expr)*
    for(;;){
        auto& expr = createNode(Pool, NodeKind::Expression);
        attachChild(ParentNode, expr);
        // expr: (paren_expr | Name | String | occ_mod | template_call)* ["->" Name]
        for(;;){
            // paren_expr: "(" exrplist ")"
            if (Tokens.left[Offset+offset] == SingleCharacterTerminals::LeftParenthesis){
                offset++;
                auto& expressions = createNode(Pool, NodeKind::ParentheseExpression);            
                offset+=expressionList(Tokens, Offset+offset,expressions,Pool, Buffer, Bytes);
                if (Tokens.left[Offset+offset] == SingleCharacterTerminals::RightParenthesis){
                    offset++;
                }
                // occ_mod: (Name | paren_expr) "*"
                //        | (Name | paren_expr) "+"
                if (Tokens.left[Offset+offset] == SingleCharacterTerminals::Star ||
                    Tokens.left[Offset+offset] == SingleCharacterTerminals::Plus) {
                    auto& modifier = createNode(Pool, NodeKind::OccuranceModifier);
                    modifier.TokenIndex = Offset+offset;
                    attachChild(modifier, expressions);
                    attachChild(expr, modifier);
                    offset++;
                } else {
                    attachChild(expr, expressions);
                }
                continue;
            }
            // optional_expr: "[" exprlist "]"
            if (Tokens.left[Offset+offset] == SingleCharacterTerminals::LeftSquareBracket){
                offset++;
                auto& expressions = createNode(Pool, NodeKind::OptionalExpression);            
                offset+=expressionList(Tokens, Offset+offset,expressions,Pool, Buffer, Bytes);
                if (Tokens.left[Offset+offset] == SingleCharacterTerminals::RightSquareBracket){
                    offset++;
                }
                attachChild(expr, expressions);
                continue;
            }
            // template_call: Name "{" template_parameter "}"
            if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::Name &&
                Tokens.left[Offset+offset+1] == SingleCharacterTerminals::LeftBracket){
                auto& templateCall = createNode(Pool, NodeKind::TemplateCall);    
                attachChild(expr, templateCall);
                auto& name = createNode(Pool, NodeKind::Name);
                name.TokenIndex = Offset+offset;
                attachChild(templateCall, name);
                offset++;       
                auto& parameters = createNode(Pool, NodeKind::TemplateParameters);
                attachChild(templateCall, parameters);     
                // template_parameters: Name ("," Name)*
                do{
                    offset++;
                    if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::Name){
                        auto& name = createNode(Pool, NodeKind::Name);
                        name.TokenIndex = Offset+offset;
                        attachChild(parameters, name);
                        offset++;
                    }
                    if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::String){
                        auto& str = createNode(Pool, NodeKind::String);
                        str.TokenIndex = Offset+offset;
                        attachChild(parameters, str);
                        offset++;
                    }
                } while(Tokens.left[Offset+offset] == SingleCharacterTerminals::Comma);
                if (Tokens.left[Offset+offset] == SingleCharacterTerminals::RightBracket){
                    offset++;
                }
            }
            // Name
            if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::Name){
                auto& name = createNode(Pool, NodeKind::Name);
                name.TokenIndex = Offset+offset;
                offset++;                
                // occ_mod: (Name | paren_expr) "*"
                //        | (Name | paren_expr) "+"
                if (Tokens.left[Offset+offset] == SingleCharacterTerminals::Star ||
                    Tokens.left[Offset+offset] == SingleCharacterTerminals::Plus) {
                    auto& modifier = createNode(Pool, NodeKind::OccuranceModifier);
                    modifier.TokenIndex = Offset+offset;
                    attachChild(modifier, name);
                    attachChild(expr, modifier);
                    offset++;
                } else {
                    attachChild(expr, name);
                }
                continue;
            }
            if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::String){
                auto& str = createNode(Pool, NodeKind::String);
                str.TokenIndex = Offset+offset;
                attachChild(expr, str);
                offset++;
                continue;
            }
            break;
        }
        if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::Alias){
            offset++;
            auto& alias = createNode(Pool, NodeKind::Alias);
            attachChild(expr, alias);
            if (Tokens.left[Offset+offset] == MultipleCharacterTerminals::Name){
                auto& name = createNode(Pool, NodeKind::Name);
                name.TokenIndex = Offset+offset;
                attachChild(alias, name);
                offset++;
            }
        }
        if (Tokens.left[Offset+offset]== SingleCharacterTerminals::Pipe){
            offset++;
            continue;
        } else {
            break;
        }        
    } 
    return offset;    
}

template<class T>
T* parser(TokenList& Tokens, vec<T>& pool,uint8_t* Buffer, size_t Bytes){
    auto& root = createNode(pool, NodeKind::Program);
    // program: (rule | import | ignore | template)*
    for (auto t = 0; t < Tokens.left.elements;){
        // ignore: Ignore Name
        if (Tokens.left[t] == MultipleCharacterTerminals::Ignore){
            if (Tokens.left[t+1] == MultipleCharacterTerminals::Name){
                auto& node = createNode(pool, NodeKind::Ignore);
                auto& name = createNode(pool, NodeKind::Name);
                name.TokenIndex = t+1;
                attachChild(node, name);                
                attachChild(root, node);
                t+=2;
                continue;
            }
        }
        // import: Import Name
        if (Tokens.left[t] == MultipleCharacterTerminals::Import){
            if (Tokens.left[t+1] == MultipleCharacterTerminals::Name){
                auto& node = createNode(pool, NodeKind::Import);
                auto& name = createNode(pool, NodeKind::Name);
                name.TokenIndex = t+1;
                attachChild(node, name);
                attachChild(root, node);
                t+=2;
                continue;
            }
        }        
        if (Tokens.left[t] == MultipleCharacterTerminals::Name){
            // template: Name "{" template_parameters "}" ":" exprlist
            if (Tokens.left[t+1] == SingleCharacterTerminals::LeftBracket){
                auto& node = createNode(pool, NodeKind::Template);
                auto& name = createNode(pool, NodeKind::Name);
                name.TokenIndex = t;
                attachChild(node, name);
                auto& parameters = createNode(pool, NodeKind::TemplateParameters);
                attachChild(node, parameters);
                auto& expressions = createNode(pool, NodeKind::ExpressionList);
                attachChild(node, expressions);
                attachChild(root, node);
                auto offset = 1;
                // template_parameters: Name ("," Name)*
                do{
                    offset++;
                    if (Tokens.left[t+offset] == MultipleCharacterTerminals::Name){
                        auto& name = createNode(pool, NodeKind::Name);
                        name.TokenIndex = t+offset;
                        attachChild(parameters, name);
                        offset++;
                    }
                } while(Tokens.left[t+offset] == SingleCharacterTerminals::Comma);

                if(Tokens.left[t+offset] == SingleCharacterTerminals::RightBracket){
                    offset++;
                }

                if (Tokens.left[t+offset] == SingleCharacterTerminals::Colon){
                    offset++;
                    offset+=expressionList(Tokens, t+offset, expressions, pool, Buffer, Bytes);
                }
                t+=offset;
                continue;
            }
            // rule: Name ":" exprlist
            if (Tokens.left[t+1] == SingleCharacterTerminals::Colon){
                auto& node = createNode(pool, NodeKind::Rule);
                auto& name = createNode(pool, NodeKind::Name);
                auto& expressions = createNode(pool, NodeKind::ExpressionList);
                name.TokenIndex = t;
                attachChild(node, name);                
                attachChild(node, expressions);
                attachChild(root, node);
                t+=2;
                t+=expressionList(Tokens, t,expressions, pool, Buffer, Bytes);
                // | expr
                for(;;){
                    if (Tokens.left[t] == SingleCharacterTerminals::NewLine){
                        if (Tokens.left[t+1] == SingleCharacterTerminals::Pipe){
                            t+=2;
                            t+=expressionList(Tokens, t, expressions, pool, Buffer, Bytes);
                            continue;
                        }
                    }
                    break;
                }
                continue;
            }
        }
        // rule_embedded: "?" Name ":" exprlist
        if (Tokens.left[t] == SingleCharacterTerminals::Questionmark){
            auto& rule = createNode(pool, NodeKind::RuleEmbedded);
            attachChild(root, rule);
            t++;
            if (Tokens.left[t] == MultipleCharacterTerminals::Name){
                auto& name = createNode(pool, NodeKind::Name);
                auto& expressions = createNode(pool, NodeKind::ExpressionList);
                name.TokenIndex = t;
                attachChild(rule, name);
                attachChild(rule, expressions);
                t++;
                if (Tokens.left[t] == SingleCharacterTerminals::Colon){
                    t++;
                    t+=expressionList(Tokens, t, expressions, pool, Buffer, Bytes);
                }
                // | expr
                for(;;){
                    if (Tokens.left[t] == SingleCharacterTerminals::NewLine){
                        if (Tokens.left[t+1] == SingleCharacterTerminals::Pipe){
                            t+=2;
                            t+=expressionList(Tokens, t, expressions, pool, Buffer, Bytes);
                            continue;
                        }
                    }
                    break;
                }
            }
            continue;
        }
        // newline: "\n"
        if (Tokens.left[t] == SingleCharacterTerminals::NewLine){
            t++;
            continue;
        }
        t++;
    }        
    return &root;
}
void topDown(Node* Node, TreeFunction Function, TokenList& Tokens,
             uint8_t* Buffer, size_t Bytes, size_t Depth){
    Function(Node, Tokens, Buffer, Bytes, Depth);
    if (Node->FirstChild){
        topDown(Node->FirstChild, Function, Tokens, Buffer, Bytes, Depth+1);
    }    
    if (Node->Next){
        topDown(Node->Next, Function, Tokens, Buffer, Bytes, Depth);
    }
}

void bottomUp(Node* Node, TreeFunction Function, TokenList& Tokens,
              uint8_t* Buffer, size_t Bytes, size_t Depth){    
    if (Node->FirstChild){
        bottomUp(Node->FirstChild, Function, Tokens, Buffer, Bytes, Depth+1);
    }    
    Function(Node, Tokens, Buffer, Bytes, Depth);
    if (Node->Next){
        bottomUp(Node->Next, Function, Tokens, Buffer, Bytes, Depth);
    }
}

void printNode(Node* Node, TokenList& Tokens, uint8_t* Buffer, size_t Bytes, size_t Depth){
    for(auto i = 0; i < Depth; ++i){
        print({"\t",1});
    }
    print({NodeKindString[Node->Kind],NodeKindStringSize[Node->Kind]});
    print({": ",2});
    //print(toString(Node->TokenIndex));
    if (static_cast<NodeKind>(Node->Kind) == NodeKind::Name ||
        static_cast<NodeKind>(Node->Kind) == NodeKind::OccuranceModifier ||
        static_cast<NodeKind>(Node->Kind) == NodeKind::String){
        strlit text = {reinterpret_cast<char*>(Buffer)+Tokens.right[Node->TokenIndex].Offset,Tokens.right[Node->TokenIndex].Bytes-1};
        print(text);
    }
    print({"\n",1});
}
#endif