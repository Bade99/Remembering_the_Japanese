#pragma once

#include "がいじんの_Platform.h"
#include "がいじんの_Helpers.h"
#include <filesystem>
#include <vector>
#include <map>
#include <sstream>

//Up to now we've been using the resource files (.rc) to store languages, this is annoying for a couple of reasons, that's why this file will define a new way to manage languages:
//there will be a couple of default languages that are stored with the .exe similar to how .rc does it, we'll manually set up some map/array whatever to write the strings we need and their mapping, and on startup those will be sent to disc on some specified folder, once that's done the language manager will read those files, and any other in the same folder, from disk, it'll either load only the current language and go to disk each time it needs to swap, or it'll load everything and just switch in code
//with this we get rid of the language enums and new languages can be very easily added after the fact

//IMPORTANT: Format of the file on disk: 
//	the file name will identify the language, meaning the user will be shown that same string, eg: c\user\appdata\app\lang\Español -> Español
//	file encoding must be UTF-8
//	inside the file there will be one language entry on each line
//		line format: number   tab/separator   string
//	additionally to support things like line breaks we parse the string to find the character representation of \n, \0, \t, ... and convert it to the single character it wants to represent, eg L"this is a \\n test" -> parser -> L"this is a \n test"

struct def_lang { //Identifies a default language that will be stored to disk by the application
	const utf16* name;
	const utf16* entries;
	const int entries_sz;//bytes
};

struct lang_mapper { //Identifies a usable language string mapping table
	std::map<u32, str> map;
};


#define lang_key_value_separator L" " /*IMPORTANT TODO(fran): japanese, and maybe other langs, map the space key to a different character "　" instead of " ", we gotta fix this somehow, option 1 provide a separate application "lang editor" for the user to create languages in, that takes care of this problems, option 2 have a better parser that takes this into account*/
#define lang_newline L"\n"

#define lang_make_utf16(quote) L##quote

#define lang_make_text(quote) lang_make_utf16(#quote)

#define lang_entry(id,txt) lang_make_text(id) lang_key_value_separator txt lang_newline

//TODO(fran): lang_日本語

constexpr static utf16 lang_english[] = L"English";
constexpr static utf16 lang_español[] = L"Español";

constexpr static utf16 lang_english_entries[] = 
	lang_entry(0,"Restore")
	lang_entry(1,"Minimize")
	lang_entry(2,"Maximize")
	lang_entry(3,"Close")

	lang_entry(10,"Invalid character, do not use:")
	lang_entry(11,"This information cannot be omitted")

	lang_entry(100,"New")
	lang_entry(101,"Practice")
	lang_entry(102,"Search")

	lang_entry(120,"Hiragana/Katakana")
	lang_entry(121,"Kanji")
	lang_entry(122,"Translation")
	lang_entry(123,"Type")
	lang_entry(124,"Add")
	lang_entry(125,"Mnemonic")

	lang_entry(200,"Noun")
	lang_entry(201,"Verb")
	lang_entry(202,"い-Adjective")
	lang_entry(203,"な-Adjective")
	lang_entry(204,"Adverb")
	lang_entry(205,"Conjunction")
	lang_entry(206,"Pronoun")
	lang_entry(207,"Counter")

	lang_entry(250,"Search Hiragana")

;

constexpr static utf16 lang_español_entries[] =
	lang_entry(0,"Restaurar")
	lang_entry(1,"Minimizar")
	lang_entry(2,"Maximizar")
	lang_entry(3,"Cerrar")
	lang_entry(10,"Caracter inválido, no usar:")
	lang_entry(100, "Nuevo")
	lang_entry(101, "Practicar")
	lang_entry(102, "Buscar")
;



static void save_to_file_lang(def_lang lang, utf16* folder) {
	str filename = str(folder) + L"\\" + lang.name;
	int sz = lang.entries_sz - (lang.entries[(lang.entries_sz / sizeof(*lang.entries)) - 1] ? 0 : sizeof(*lang.entries));//remove null terminator at the time of saving the file
	auto res = convert_utf16_to_utf8(lang.entries, sz); defer{ free_convert(res.mem); };
	write_entire_file(filename.c_str(), res.mem, (u32)res.sz);
}

static void save_to_file_langs(def_lang* langs, int cnt, utf16* folder) {
	CreateDirectoryW(folder, 0); //Create the folder if it doesnt already exist, TODO(fran): recursive folder creation
	for (int i = 0; i < cnt; i++)
		save_to_file_lang(langs[i], folder);
}

static std::vector<str> load_file_lang_names(utf16* folder) {
	std::vector<str> res;
	using namespace std::filesystem;
	for (auto& f : directory_iterator(folder)) { //TODO(fran): performance: do this with FindFirstFile and FindNextFile, problem is I have to manually check we dont enter into subdirs. Also this is only available on c++17
		if (f.is_regular_file()) {
			res.push_back(f.path().filename().wstring());
		}
	}
	std::sort(res.begin(), res.end());//Sort A to Z, TODO(fran): how to sort langs that dont use our alphabet?
	return res;//std::move?
}

static void _replace_all(str& s, const str& from, const str& to) { //Thanks https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
	size_t start_pos = 0;
	while ((start_pos = s.find(from, start_pos)) != str::npos) {
		s.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
}

static void parse_lang_entry(str& val) {
	//The string can contain backward slashes that must be parsed just like in the standard, eg L"\\n" (2 chars) -> L"\n" (1 char)
	//We need only parse a few of all the escape sequences since the rest can be put on the text in their single character form without problem
	//https://en.cppreference.com/w/cpp/language/escape

	// "\\n" -> "\n" : since each line contains a different entry we cannot allow the writer to separate the lines
	// "\\t" -> "\t" : I feel using \t is clearer than pressing tab since the size of the tabulated space will change depending on its placement
	// "\\0" -> "\0" : Normal text editors dont allow for this to be written
	_replace_all(val, L"\\n", L"\n");
	_replace_all(val, L"\\t", L"\t");
	_replace_all(val, L"\\0", L"\0");
	//TODO(fran): this is slower than looking for L"\\" first and then looking at the next char to see what to replace with, we are doing waay to many extra iterations
}

static std::map<u32, str> mappify_lang(cstr* s, cstr separator)//TODO(fran): multi character separator
{
	std::map<u32, str> m;

	str key, val;
	std::wistringstream iss(s);

	while (std::getline(std::getline(iss, key, separator) >> std::ws, val)) {
		try { //TODO(fran): find something other than stoi that doesnt use exceptions, strtol is close but returns 0 on failure, also stoi isnt very good https://stackoverflow.com/questions/11598990/is-stdstoi-actually-safe-to-use
			u32 id = std::stoul(key);
			parse_lang_entry(val);
			m[id] = val;
		}
		catch (...) {}
	}

	return std::move(m);//TODO(fran): not really sure if I need to do this, compiler might do it automatically
}

//In order to change languages the system will go look for the specific language from disc, this will not be as fast as loading all the languages on startup but will potentially reduce startup time by a lot since only one language needs to be entirely loaded and parsed; changing languages realtime will also be not as fast but the user isnt usually changing languages so that isnt a real concern
static lang_mapper load_file_lang(utf16* folder, utf16* lang_name) { //TODO(fran): this will probably be a slow and lenghty operation, best suited for a secondary thread to handle
	lang_mapper res;
	str path = str(folder) + L"\\" + lang_name;
	auto file_res = read_entire_file(path.c_str()); defer{ free_file_memory(file_res.mem); };
	auto convert_res = convert_utf8_to_utf16((utf8*)file_res.mem, file_res.sz);
	res.map = mappify_lang((cstr*)convert_res.mem, lang_key_value_separator[0]);
	return res;
}
