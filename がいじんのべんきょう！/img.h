#pragma once
#include "がいじんの_Platform.h"

//INFO: ways of embedding additional files at compile time http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1040r6.html

//INFO: interesting visual effect, scrolling the text while watching the "image" makes it easier to see it
//TODO(fran): auto exporter, .exe that checks every file in its current folder and tries, if it knows how, to convert it and generates .h files, 
//	eg maximize.bmp -> maximize.h and inside has a constexpr short maximize[16] = {...}; (if the bmp is 1bpp 16x16), another option is that it simply contains the content: 0b1111'1111'1111'1111 ... and you can then constexpr short maximize[16] = { #include"gen/maximize.h" };

//INFO: for 1bpp bmps in windows 0 = foreground , 1 = background, so the part you want to be visible has to have a 0

#define bswap16(x) ((x>>8) | (x<<8))

struct def_img {
	int bpp;
	int w;
	int h;
	void* mem;
};
#if 0
constexpr u16 empty[16] = //1bpp 16x16
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
#endif
constexpr u16 _close[16] =
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1110'1111'1111'0111),
	bswap16(0b1111'0111'1110'1111),
	bswap16(0b1111'1011'1101'1111),
	bswap16(0b1111'1101'1011'1111),
	bswap16(0b1111'1110'0111'1111),
	bswap16(0b1111'1110'0111'1111),
	bswap16(0b1111'1101'1011'1111),
	bswap16(0b1111'1011'1101'1111),
	bswap16(0b1111'0111'1110'1111),
	bswap16(0b1110'1111'1111'0111),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img close{ 1,16,16, (void*)_close };

constexpr u16 _maximize[16] =
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1100'0000'0000'0011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1101'1111'1111'1011),
	bswap16(0b1100'0000'0000'0011),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img maximize{ 1,16,16, (void*)_maximize };

constexpr u16 _minimize[16] =
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1100'0000'0000'0011),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img minimize{ 1,16,16, (void*)_minimize };

constexpr u16 _tick[16] =
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'0011),
	bswap16(0b1111'1111'1110'0111),
	bswap16(0b1111'1111'1100'1111),
	bswap16(0b1111'1111'1001'1111),
	bswap16(0b1100'1111'0011'1111),
	bswap16(0b1110'0110'0111'1111),
	bswap16(0b1111'0000'1111'1111),
	bswap16(0b1111'1001'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img tick{ 1,16,16, (void*)_tick };

constexpr u16 _arrow_right[16] =
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1101'1111'1111),
	bswap16(0b1111'1100'1111'1111),
	bswap16(0b1111'1100'0111'1111),
	bswap16(0b1111'1100'0011'1111),
	bswap16(0b1111'1100'0001'1111),
	bswap16(0b1111'1100'0001'1111),
	bswap16(0b1111'1100'0011'1111),
	bswap16(0b1111'1100'0111'1111),
	bswap16(0b1111'1100'1111'1111),
	bswap16(0b1111'1101'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img arrow_right{ 1,16,16, (void*)_arrow_right };

constexpr u16 _dropdown[16] = //TODO(fran): dropdown_inverted
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1001'1111'1111'1001),
	bswap16(0b1100'1111'1111'0011),
	bswap16(0b1110'0111'1110'0111),
	bswap16(0b1111'0011'1100'1111),
	bswap16(0b1111'1001'1001'1111),
	bswap16(0b1111'1100'0011'1111),
	bswap16(0b1111'1110'0111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img dropdown{ 1,16,16, (void*)_dropdown };

constexpr u16 _circle[16] =
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1110'0111'1111),
	bswap16(0b1111'1000'0001'1111),
	bswap16(0b1111'0000'0000'1111),
	bswap16(0b1111'0000'0000'1111),
	bswap16(0b1110'0000'0000'0111),
	bswap16(0b1110'0000'0000'0111),
	bswap16(0b1111'0000'0000'1111),
	bswap16(0b1111'0000'0000'1111),
	bswap16(0b1111'1000'0001'1111),
	bswap16(0b1111'1110'0111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
constexpr def_img circle{ 1,16,16, (void*)_circle };