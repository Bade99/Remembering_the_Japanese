#pragma once
#include "win32_Platform.h"

//TODO(fran): idk if we should really apply this checks, allowing the user the freedom to input whatever they want isnt a bad idea

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

bool any_kanji(utf16_str s) {
	for (const auto& c : s) if (is_kanji(c)) return true;
	return false;
}

bool any_hiragana_katakana(utf16_str s) {
	for (const auto& c : s) if (is_hiragana(c) || is_katakana(c)) return true;
	return false;
}

bool all_hiragana(utf16_str s) {
	//TODO(fran): surrogate pairs?
	//TODO(fran): what to do about whitespace?
	for (const auto& c : s) if (!is_hiragana(c)) return false;
	return true;
}

//TODO(fran): taking into account things like this with long and separate ranges it'd be good to add a character_check_function that can be injected into the edit control for doing this kinds of complex range checks (there's a little problem with this, that's japanese is generated via normal keyboard characters, therefore the standard abc will be present on the text if only for a brief period, this is why, once we implement the better chrome style IME, there wont be a way to implement this, it will be better to perform a final and single check once the user has finished inputting)