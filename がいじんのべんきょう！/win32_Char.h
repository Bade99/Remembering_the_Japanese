#pragma once
#include "win32_Platform.h"

#define between_inclusive(lower_bound,target,upper_bound) ((c)>=(lower_bound) && (c)<=(upper_bound))
#define between_exclusive(lower_bound,target,upper_bound) ((c)>(lower_bound) && (c)<(upper_bound))

bool is_hiragana(utf16 c) {
	bool res = between_inclusive(0x3041,c,0x30A0);
	return res;
}

bool is_katakana(utf16 c) {
	bool res = between_inclusive(0x30A0,c,0x30ff) || between_inclusive(0x31F0,c,0x31FF);
	//NOTE: there's also halfwidth katakana at U+FF61 and beyond though it doesnt seem to have much use today
	return res;
}

bool is_kanji(utf16 c) {//TODO(fran):Im pretty sure this has lots of korean too
	bool res = between_inclusive(0x2E80, c, 0x2FD5) || between_inclusive(0x3400, c, 0x4DB5) || between_inclusive(0x4E00, c, 0x9FCB) || between_inclusive(0xF900, c, 0xFA6A);
	//TODO(fran): im probably missing some stuff
	return res;
}