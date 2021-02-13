#pragma once
#include <cstdint> //uint8_t
#include <string> //unfortunate

typedef uint8_t u8; //prepping for jai
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef char utf8;
typedef wchar_t utf16;
typedef char32_t utf32;

typedef std::wstring str;
typedef wchar_t cstr;

struct text { //A _non_ null terminated cstring
	cstr* str;
	size_t sz_chars; //TODO(fran): better is probably size in bytes
};


//TODO(fran): use this guys, if we use pointers utf8_str* we can cast easily to different types: (utf16_str*)&any_str
struct utf8_str {
	utf8* str;
	size_t sz;/*bytes*/
};

struct utf16_str {
	utf16* str;
	size_t sz;/*bytes*/

	//Includes null terminator
	size_t sz_char() { return sz / sizeof(*str); }//TODO(fran): it's probably better to not include the null terminator, or give two functions
};
//TODO(fran): try something like this
struct any_str {
	void* str;
	size_t sz;/*bytes*/
	operator utf16_str() const { return{ (utf16*)str,sz }; }//TODO(fran): this is terrible, this cast should be free
	operator utf8_str() const { return{ (utf8*)str,sz }; }//TODO(fran): this is terrible, this cast should be free
};
//NOTE: we want only one allocation and dealocation strategy that makes sense for the current project, in this case strings arent expected to ever be too large, therefore malloc and free is good enough
static any_str alloc_any_str(size_t sz) {
	any_str res{ malloc(sz), sz };
	return res;
}
static void free_any_str(void* str) { free(str); };

//TODO(fran):
//https://stackoverflow.com/questions/42293192/making-a-dynamic-array-that-accepts-any-type-in-c
//https://stackoverflow.com/questions/9804371/syntax-and-sample-usage-of-generic-in-c11