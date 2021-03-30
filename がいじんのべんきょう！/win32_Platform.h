#pragma once
#include <cstdint> //uint8_t
#include <string> //unfortunate

typedef uint8_t u8; //prepping for jai
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

//TODO(fran): constexpr?
#define U8MAX  ((u8)0xff)
#define U16MAX ((u16)0xffff)
#define U32MAX ((u32)0xffffffff)
#define U64MAX ((u64)0xffffffffffffffff)

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define I8MIN  (-127i8 - 1)
#define I8MAX  127i8
#define I16MIN (-32767i16 - 1)
#define I16MAX 32767i16
#define I32MIN (-2147483647i32 - 1)
#define I32MAX 2147483647i32
#define I64MIN (-9223372036854775807i64 - 1)
#define I64MAX 9223372036854775807i64

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

	utf16& operator[](size_t i) { return str[i]; }

	utf16* begin() { return this->sz ? &(*this)[0] : nullptr; }

	utf16* end() { return this->sz ? (utf16*)((u8*)&(*this)[0] + this->sz) : nullptr; /*ptr one past the last element*/ }
};

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


template<typename T>
struct ptr {//TODO(fran): maybe 'arr' or 'array' is a better name
	T* mem{0};
	size_t cnt{ 0 };//NOTE: you can change the cnt in case you alloc more elems than necessary
	//TODO(fran): differentiate between alloc-ed cnt and used cnt
	static constexpr size_t type_sz = sizeof(*mem);//TODO(fran): this may be a stupid idea, also it seems to be fully supported as of c++17 before there were some stupid surprises https://stackoverflow.com/questions/22867654/enum-vs-constexpr-for-actual-static-constants-inside-classes

	using value_type = T;

	size_t sz() const { return cnt * type_sz; }

	void free() { if (mem) { ::free(mem); cnt = 0; } }
	//NOTE: also frees any previous allocation
	void alloc(size_t cnt) { free(); this->cnt = cnt; mem = (T*)malloc(cnt * type_sz); }//TODO(fran): dont know how good the free idea is, say the user mallocs a ptr<> then mem and sz will be !=0 but not valid values and we crash

	//void operator[](size_t i) const { return mem[i]; }

	T& operator[](size_t i) { return mem[i]; }

	T* begin() {
		return this->cnt ? &(*this)[0] : nullptr;
	}

	T* end() {
		return this->cnt ? &(*this)[0] + this->cnt : nullptr; //ptr one past the last element
	}
};
//TODO(fran): it may be easier to read if we asked for the pointer directly, eg ptr<i32*> may be nicer than ptr<i32> as is now



union brush_group {
	struct {
		HBRUSH normal, disabled, mouseover, clicked;
	};
	HBRUSH all[4]{ 0 };
private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array to the correct element count!"); }
};

//Returns true if any modification was made to 'dest'
bool copy_brush_group(brush_group* dest, const brush_group* src) {
	bool copied = false;
	for (auto i = 0; i < ARRAYSIZE(src->all); i++) if (src->all[i]) { dest->all[i] = src->all[i]; copied = true; }
	return copied;
}