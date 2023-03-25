#ifdef __PRE
#include <cstdint>
#include <cmath>
#include <intrin.h>
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(__assume)

extern "C"
{
    uint32_t _tls_index{};
    __declspec(dllimport) void* GetStdHandle(int_fast32_t StdHandle);
    __declspec(dllimport) int_fast32_t WriteConsoleA(void* ConsoleOutput,const void* Buffer,uint_fast32_t NumberOfCharsToWrite,uint_fast32_t* NumberOfCharsWritten,void* Reserved);
    __declspec(dllimport) void* CreateThread(void* lpThreadAttributes, size_t dwStackSize, void* lpStartAddress, void* lpParameter, uint32_t dwCreationFlags, uint32_t* lpThreadId);
    __declspec(dllimport) void Sleep(int32_t ms);
    __declspec(dllimport) int32_t GetCurrentProcessorNumber();
    __declspec(dllimport) int32_t SetThreadAffinityMask(void* ThreadHandle, int32_t Mask);
    __declspec(dllimport) int32_t WaitForSingleObject(void* ThreadHandle, int32_t Ms);
    __declspec(dllimport) int32_t WaitForMultipleObjects(int32_t Count, void*const* ThreadHandle, bool WaitAll, int32_t Ms);
    __declspec(dllimport) void ExitThread(int32_t ExitCode);    
}

#include <atomic>

template<class T, uint32_t CNT>
class SPSCQueue
{
public:
    static_assert(CNT && !(CNT & (CNT - 1)), "CNT must be a power of 2");

    T* alloc() {
      if (write_idx - read_idx_cach == CNT) {
        _mm_mfence();
        read_idx_cach = read_idx;
        //read_idx_cach = ((std::atomic<uint32_t>*)&read_idx)->load(std::memory_order_consume);
        if (write_idx - read_idx_cach == CNT) { // no enough space
          __assume(0);
          return nullptr;
        }
      }
      return &data[write_idx % CNT];
    }

    void push() {
        _InterlockedIncrement((volatile long*)&write_idx);
      //((std::atomic<uint32_t>*)&write_idx)->store(write_idx + 1, std::memory_order_release);
    }

    template<typename Writer>
    bool tryPush(Writer writer) {
      T* p = alloc();
      if (!p) return false;
      writer(p);
      push();
      return true;
    }

    template<typename Writer>
    void blockPush(Writer writer) {
      while (!tryPush(writer))
        ;
    }

    T* front() {
        auto tmp = write_idx;
        _mm_mfence();
        if (read_idx == tmp){
        //if (read_idx == ((std::atomic<uint32_t>*)&write_idx)->load(std::memory_order_acquire)) {
          return nullptr;
        }
        return &data[read_idx % CNT];
    }

    void pop() {
      _InterlockedIncrement((volatile long*)&read_idx);
      //((std::atomic<uint32_t>*)&read_idx)->store(read_idx + 1, std::memory_order_release);
    }

    template<typename Reader>
    bool tryPop(Reader reader) {
      T* v = front();
      if (!v) return false;
      reader(v);
      pop();
      return true;
    }
    alignas(128)uint64_t bytes = 0;
  private:
    alignas(128) T data[CNT] = {};

    alignas(128) uint32_t write_idx = 0;
    uint32_t read_idx_cach = 0; // used only by writing thread
    
    alignas(128) uint32_t read_idx = 0;
};

#pragma function(memset)
void *memset(void *dest, int c, size_t count)
{
    char *bytes = (char *)dest;
    while (count--)
    {
        *bytes++ = (char)c;
    }
    return dest;
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

    const T& operator[](int Index)const{
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
struct Job;

using TokenList = pair<vec<uint8_t>,vec<TokenLocationInfo>>;

struct Bucket{
    uint8_t Entries;
    uint8_t Values[15];
};

struct Task{
    uint8_t* Buffer;
    uint8_t* Tokens;
    size_t Bytes;
};

SPSCQueue<Task,4> ThreadJobs[11];

struct LexerContext{
    vec<uint8_t> KeywordLookup;// [0-65535]
    vec<Bucket> Bucket;// [0-LastBucketIndex]
    uint8_t  LastBucketIndex;
    uint8_t RoundRobin;
    size_t ThreadCount;
    SPSCQueue<Task,4>* ThreadJobs;
    void** ThreadHandles;
};

LexerContext initLexerContext(){
    return LexerContext{};
}

void prepareLexer(LexerContext& lexerContext);
void freeLexer(LexerContext& lexerContext);
uint64_t lexerbytes(LexerContext& lexerContext);

TokenList lexer(const LexerContext& Context, uint8_t* Buffer, size_t Bytes, bool IgnoreSkipables, bool GenerateTokenInfos = false);

template<class T>
T* parser(TokenList& Tokens, vec<T>& pool,uint8_t* Buffer, size_t Bytes);

using TreeFunction = void (*)(const Node*, TokenList&, uint8_t*, size_t, size_t);

void topDown(const Node* Node, TreeFunction Function, TokenList& Tokens,
             uint8_t* Buffer, size_t Bytes, size_t Depth = 0);

void bottomUp(const Node* Node, TreeFunction Function, TokenList& Tokens,
              uint8_t* Buffer, size_t Bytes, size_t Depth = 0);

void printNode(const Node* Node, TokenList& Tokens, uint8_t* Buffer, size_t Bytes, size_t Depth);
void LexerCFG_SIMD_MP(LexerContext& Context, uint8_t* Buffer, size_t Bytes, uint8_t* Tokens);
void LexerCFG_Regular_Expression(uint8_t* Buffer, size_t Bytes, uint8_t* Tokens);
#endif
#ifdef __POST
#define NOPE
#ifdef NOPE
#include "tokenizer.ipp"
bool memoryCompare(const void* Source, size_t SourceOffset, size_t SourceBytes, 
                   const void* Target, size_t TargetOffset, size_t TargetBytes,
                   size_t CompareBytes){
    if ((SourceBytes - SourceOffset) >= CompareBytes &&
        (TargetBytes - TargetOffset) >= CompareBytes){
        for (auto i = 0; i < CompareBytes; ++i){
            if (reinterpret_cast<const uint8_t*>(Source)[SourceOffset+i]!=reinterpret_cast<const uint8_t*>(Target)[TargetOffset+i]){
                return false;
            }
        }
        return true;
    }
    return false;
}

uint32_t starting_workers = 0;
bool running = false;
constexpr uint8_t skipables[5]={0,1,2,3,64};

int32_t WorkerRunner(void* userdata){
    LexerContext* context = static_cast<LexerContext*>(userdata);
    auto threadIndex = GetCurrentProcessorNumber();
    uint64_t hotLoop = 0;
    mem tokens=initmem(4096);
    while(running == 1 || context->ThreadJobs[threadIndex].front()){
        if (context->ThreadJobs[threadIndex].front()){
            Task* job = context->ThreadJobs[threadIndex].front();
            LexerContextFreeGrammar(job->Buffer, job->Bytes, tokens.data);
            context->ThreadJobs[threadIndex].bytes += job->Bytes;
            context->ThreadJobs[threadIndex].pop();
            hotLoop=0;
        } else {
            for (auto j = 0; j<(1024*(1<<hotLoop)); j++){}
            hotLoop=(hotLoop+1)&7;
            //::Sleep(1);
        }        
    }
    return 0;
}

void LexerCFG_Regular_Expression(uint8_t* Buffer, size_t Bytes, uint8_t* Tokens) {
	__m128i* p = reinterpret_cast<__m128i*>(Buffer);
	__m128i* tokens = reinterpret_cast<__m128i*>(Tokens);
	__m128i b,b2,a,a2;
	uint32_t start = 0;
	uint8_t state = 0,end =0;
    a = _mm_load_si128(tokens);
	b = _mm_load_si128(tokens + 1);
	for (size_t i = 0; i < (Bytes / 32); ++i) {		
        tokens += 2;        
        a2 = _mm_load_si128(tokens);
        // letters mask
        //[v,-,-,-][-,-,-,-]
        // digits mask
        //[-,v,-,-][-,-,-,-]
        // hexes mask
        //[-,-,v,-][-,-,-,-]
        // terminals mask
        //[-,-,-,v][-,-,-,-]
		b2 = _mm_load_si128(tokens + 1);
        // keyword needle mask
        //[-,-,-,-][v,v,-,-]
		// expr needle masks
		//[-,-,-,-][-,-,v,v]
		auto leading = static_cast<uint64_t>(b.m128i_u32[2])<<32 | static_cast<uint64_t>(b2.m128i_u32[2]);
		auto trailing = static_cast<uint64_t>(b.m128i_u32[3])<<32 | static_cast<uint64_t>(b2.m128i_u32[3]);
		auto candidates = leading & (trailing >> 1);
		auto offset = __lzcnt64(candidates);
		while(offset < 32) {
			auto index = 31-offset;
			if (Buffer[i * 32 + index] == '/' && Buffer[i * 32 + index +1] == '*' && state == 0) {
				// comment start
				start = i * 32 + index;
				state = 1;
			}
			if (Buffer[i * 32 + index] == '"' && Buffer[i * 32 + index +1] == '>' && state == 0) {
				// string start
				start = i * 32 + index;
				state = 2;
			}
			if (Buffer[i * 32 + index] == '0' && Buffer[i * 32 + index +1] == 'x' && state == 0) {
				// hexa decimal number
				//start = i * 32 + offset;
				//state = 3;
			}
			if (Buffer[i * 32 + index] == '*' && Buffer[i * 32 + index + 1] == '/' && state == 1) {
				// comment end
				end = i * 32 + index +1;
				state = 0;
			}
			if (Buffer[i * 32 + index] == '<' && Buffer[i * 32 + index + 1] == '"' && state == 2) {
				// string end
				end = i * 32 + index + 1;
				state = 0;
			}
			candidates = candidates & ~(1ull << (63-offset));
			offset = __lzcnt64(candidates);
		}
		b = b2;
        a = a2;
	}
}

void LexerCFG_SIMD_MP(LexerContext& Context, uint8_t* Buffer, size_t Bytes, uint8_t* Tokens) {
    auto chunks = ((Bytes-1)/4096)+1;
    for (auto i = 0; i< chunks; ++i){
        uint64_t hotLoop = 1;
        auto tind = Context.RoundRobin % 11;
        auto* job = Context.ThreadJobs[tind].alloc();
        while(job == nullptr){
            //::Sleep(1);
            for (auto j = 0; j<(1024*(1<<hotLoop)); j++){}
            job = Context.ThreadJobs[tind].alloc();
            hotLoop=(hotLoop+1)&7;
        }
        job->Buffer = Buffer+(i*4096);
        job->Tokens = Tokens+(i*4096);
        job->Bytes = i < (chunks-1) ? 4096 : Bytes & 4095;
        Context.ThreadJobs[tind].push();
        Context.RoundRobin++;
    }
}

int32_t Startup(void* userdata){
    LexerContext* context = static_cast<LexerContext*>(userdata);
    _InterlockedIncrement((volatile long*)&starting_workers);
    while(running == false){
        Sleep(1);
    }
    _InterlockedDecrement((volatile long*)&starting_workers);
    return WorkerRunner(userdata);
}

uint64_t lexerbytes(LexerContext& lexerContext){
    uint64_t result = 0;
    for (auto i = 0; i< 11; ++i){
        result+=lexerContext.ThreadJobs[i].bytes;
    }
    return result;
}

void prepareLexer(LexerContext& lexerContext){
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    lexerContext.ThreadJobs = ThreadJobs;
    lexerContext.ThreadCount = info.NumberOfProcessors-1;
    lexerContext.ThreadHandles = (void**)HeapAlloc(GetProcessHeap(), 0, 4096);
    for (auto t = 0; t < lexerContext.ThreadCount; ++t){
        auto threadHandle = ::CreateThread(nullptr, 0, Startup, &lexerContext, 0, nullptr);
        SetThreadAffinityMask(threadHandle,2<<t);    
        lexerContext.ThreadHandles[t] = threadHandle;
        lexerContext.ThreadJobs[t].bytes = 0;
    }

    // init lookup
    lexerContext.KeywordLookup = initvec<uint8_t>();
    resize(lexerContext.KeywordLookup,65535);
    for (auto i = 0; i < 65535; ++i){
        lexerContext.KeywordLookup[i] = 0;
    }
    // sort data
    lexerContext.Bucket = initvec<Bucket>();
    // add empty bucket
    auto& e = add(lexerContext.Bucket);
    e.Entries = 0;
    lexerContext.LastBucketIndex++;
    // add sorted buckets
    for (auto i = 0; i < KEYWORDS_COUNT; ++i){
        uint16_t key = (static_cast<uint16_t>(KEYWORDS[i][0])<<8)+KEYWORDS[i][1];
        auto bucketIndex = lexerContext.KeywordLookup[key];
        if (bucketIndex == 0){
            auto& e = add(lexerContext.Bucket);
            e.Entries = 0;
            lexerContext.KeywordLookup[key] = lexerContext.LastBucketIndex;
            bucketIndex = lexerContext.LastBucketIndex;
            lexerContext.LastBucketIndex++;
        }        
        lexerContext.Bucket[bucketIndex].Values[lexerContext.Bucket[bucketIndex].Entries] = i;
        lexerContext.Bucket[bucketIndex].Entries++;
    }
    while(starting_workers!=lexerContext.ThreadCount){
        Sleep(0);
    }
    running = true;
    while(starting_workers!=0){
        Sleep(0);
    }
}

void freeLexer(LexerContext& lexerContext) {
    running = false;
    WaitForMultipleObjects(lexerContext.ThreadCount,lexerContext.ThreadHandles,true, 0xffffffff);
    HeapFree(GetProcessHeap(), 0, lexerContext.ThreadHandles);
}
/*
TokenList lexer(const LexerContext& Context, uint8_t* Buffer, size_t Bytes, bool IgnoreSkipables, bool GenerateTokenInfos){
    TokenList result;
    result.left = initvec<uint8_t>();
    resize(result.left,1000);
    result.right = initvec<TokenLocationInfo>();
    resize(result.right,1000);
    // token infos
    uint64_t col = 1;
    uint64_t line = 1;
    const size_t SkipablesCount = 5;

    for (auto c = 0; c < Bytes;){
        uint8_t terminal = 255;
        auto start = c;
        uint64_t processedLines = 0;
        uint64_t processedLineColumns = 0;
        // Check expressions.
        for (auto i = 0; i < REGULAR_EXPRESSION_COUNT; ++i){
            switch (i)
            {
            case 0:// Comment
                if (Buffer[start] == '/' && Buffer[start+1] == '*'){
                    c=start+2;
                    while(Buffer[c] != '*' && Buffer[c+1] != '/'){
                        if (Buffer[c] == '\n'){
                            processedLines++;
                            processedLineColumns = 0;
                        }
                        c++;
                        processedLineColumns++;
                    }
                    c+=2;
                    terminal = SINGLE_CHARACTER_TERMINALS_COUNT+KEYWORDS_COUNT+i;
                }
                break;
            case 1: // StringLiteral
                if (Buffer[start] == '"' && Buffer[start+1] == '>'){
                    c=start+2;
                    while(Buffer[c] != '<' && Buffer[c+1] != '"'){
                        if (Buffer[c] == '\n'){
                            processedLines++;
                            processedLineColumns = 0;
                        }
                        c++;
                        processedLineColumns++;
                    }
                    c+=2;
                    terminal = SINGLE_CHARACTER_TERMINALS_COUNT+KEYWORDS_COUNT+i;
                }
                break;
            case 2: // HEX_NUMBER
                if (Buffer[start] == '0' && (Buffer[start+1] == 'x' || Buffer[start+1] == 'X')){
                    auto lookAhead = 0;
                    for(;;lookAhead++){
                        switch(Buffer[start+lookAhead]){
                        // hex-letter
                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                        // digit
                        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                            continue;
                        }
                        break;
                    }            
                    if (lookAhead > 0){
                        c+=lookAhead;
                        terminal = SINGLE_CHARACTER_TERMINALS_COUNT+KEYWORDS_COUNT+i;
                    }                    
                }
                break;
            case 3: // DEC_NUMBER
                {
                    auto lookAhead = 0;
                    for(;;lookAhead++){
                        switch(Buffer[start+lookAhead]){
                        // digit
                        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                            continue;
                        }
                        break;
                    }            
                    if (lookAhead > 0){
                        c+=lookAhead;
                        terminal = SINGLE_CHARACTER_TERMINALS_COUNT+KEYWORDS_COUNT+i;
                    }    
                }
                break;
            case 4: // Name
                auto lookAhead = 0;
                for(;;lookAhead++){
                    switch(Buffer[start+lookAhead]){
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
                if (lookAhead > 0){
                    c+=lookAhead;
                    terminal = SINGLE_CHARACTER_TERMINALS_COUNT+KEYWORDS_COUNT+i;
                }       
                break;
            }
            if (terminal!=255){
                break;
            }
        }
        if (terminal==255){
            // Check keywords.
            uint16_t key = (static_cast<uint16_t>(Buffer[start])<<8)+Buffer[start+1];
            auto bucketIndex = Context.KeywordLookup[key];
            if (bucketIndex <= Context.LastBucketIndex){
                auto& bucket = Context.Bucket[bucketIndex];
                for (auto i = 0; i< bucket.Entries; i++){
                    if (memoryCompare(Buffer,start,Bytes,KEYWORDS[bucket.Values[i]],0,KEYWORDS_SIZE[bucket.Values[i]],KEYWORDS_SIZE[bucket.Values[i]])){
                        terminal = SINGLE_CHARACTER_TERMINALS_COUNT+bucket.Values[i];
                        c=start+KEYWORDS_SIZE[i];
                        break;
                    }                    
                }
            }
        }
        if (terminal==255){
            // Single byte terminals.
            for (auto i = 0; i < SINGLE_CHARACTER_TERMINALS_COUNT; ++i){
                if (Buffer[c] == SINGLE_CHARACTER_TERMINALS[i]){
                    terminal = i;
                    c+=1;
                    break;
                }
            }
        }
        // The lexer can't process the current token.
        if (terminal == 255){
            c++;
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
            if (terminal == (uint8_t)SingleCharacterTerminals::NEW_LINE){
                col = 1;
                line++;
            } else {
                col+=c-start;
                if (processedLines != 0){
                    line+=processedLines;
                    col = processedLineColumns;
                }
            }
        }
    }
    return result;
}*/
#else
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
#endif

#ifdef NOPE
#else
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

struct NFATransition{
    uint8_t Input;
    NFATransition* Target;
    NFATransition* ParallelTarget;
};

void patchTransition(NFATransition* Self, uint8_t Input, NFATransition* Target = nullptr, 
                     NFATransition* ParallelTarget = nullptr){
    Self->Input = Input;
    Self->Target = Target;
    Self->ParallelTarget = ParallelTarget;
}

struct NFA{
    NFATransition* Start;
    NFATransition* End;
};

const uint8_t Empty = 255;

NFA ConvertPostfixExpressionToNFA(vec<uint8_t>& PostfixExpression, vec<NFATransition>& TransitionPool){
    NFATransition* endTransition = &add(TransitionPool);
    patchTransition(endTransition,4); // 4 is ASCII End of Transmission

    NFA result={nullptr, nullptr};
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
                ruleStack[ruleStackIndex].Start=&add(TransitionPool);

                ruleStack[ruleStackIndex].Start->Input = PostfixExpression[i]-RegularExpressionOperator::Union-1;
                ruleStack[ruleStackIndex].Start->Target = ruleStack[ruleStackIndex].End;
                ruleStack[ruleStackIndex].Start->
                //ruleStack[ruleStackIndex].Start=&add(NodePool);
                //ruleStack[ruleStackIndex].End=&add(NodePool);
                //ruleStack[ruleStackIndex].Start->FirstTransition=&add(TransitionPool);
                //ruleStack[ruleStackIndex].Start->FirstTransition->Input = PostfixExpression[i]-RegularExpressionOperator::Union-1;
                //ruleStack[ruleStackIndex].Start->FirstTransition->Target = ruleStack[ruleStackIndex].End;
                //ruleStack[ruleStackIndex].Start->FirstTransition->Next = nullptr;
                //ruleStack[ruleStackIndex].End->FirstTransition = nullptr;
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
#endif