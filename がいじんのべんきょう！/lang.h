#pragma once

#include "がいじんの_Platform.h"
#include "がいじんの_Helpers.h"
#include <string>

//Up to now we've been using the resource files (.rc) to store languages, this is annoying for a couple of reasons, that's why this file will define a new way to manage languages:
//there will be a couple of default languages that are stored with the .exe similar to how .rc does it, we'll manually set up some map/array whatever to write the strings we need and their mapping, and on startup those will be sent to disc on some specified folder, once that's done the language manager will read those files, and any other in the same folder, from disk, it'll either load only the current language and go to disk each time it needs to swap, or it'll load everything and just switch in code
//with this we get rid of the language enums and new languages can be very easily added after the fact

//IMPORTANT: Format of the file on disk: 
//	the filename will identify the language, meaning the user will be shown that same string 
//	file encoding must always be UTF-8, one standard encoding saves a lot of hustles
//	inside the file there will be one language entry on each line
//		line format: number   tab/separator   string
//	additionally to support things like line breaks we parse the string to find the character representation of \n, \t, ... and convert it to the single character it wants to represent, eg L"this is a \\n test" -> lang_mgr parser -> L"this is a \n test"

struct lang { //Identifies a language that will be stored to disk by the application
	const utf16* name;
	const utf16* entries;
	const int entries_sz;//bytes
};

void save_to_file_lang(lang lang, utf16* folder) {
	str filename = str(folder) + lang.name;
	auto res = convert_utf16_to_utf8(lang.entries, lang.entries_sz); defer{ free_convert(res.mem); };
	write_entire_file(filename.c_str(), res.mem, (u32)res.sz);
}

void save_to_file_langs(lang* langs, int cnt, utf16* folder) {
	CreateDirectoryW(folder, 0); //Create the folder if it doesnt already exist, TODO(fran): recursive folder creation
	for (int i = 0; i < cnt; i++)
		save_to_file_lang(langs[i],folder);
}

#define lang_key_value_separator L" "
#define lang_newline L"\n"

#define lang_make_utf16(quote) L##quote

#define lang_make_text(quote) lang_make_utf16(#quote)

#define lang_entry(id,txt) lang_make_text(id) lang_key_value_separator txt lang_newline

//TODO(fran): lang_日本語

constexpr static utf16 lang_english[] = L"English";
constexpr static utf16 lang_español[] = L"Español";

constexpr static utf16 lang_english_entries[] = 
	lang_entry(0,"New")
	lang_entry(1,"Practice")
	lang_entry(2,"Search")
;

constexpr static utf16 lang_español_entries[] =
	lang_entry(0, "Nuevo")
	lang_entry(1, "Practicar")
	lang_entry(2, "Buscar")
;