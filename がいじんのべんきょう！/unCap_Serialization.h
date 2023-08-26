#pragma once
#include "windows_sdk.h"
#include "unCap_Reflection.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include <string>
#include <Shlobj.h>//SHGetKnownFolderPath

//--------------------------------------------------------
//Defines serialization and deserialization for every type
//You can add your own complex structs via "reflection" or manually, preferably ones not expected to change
//--------------------------------------------------------

//There are 4 steps to serialization, explained easier by example:

//struct Example {

//1. Declare and generate your members

//#define foreach_Example_member(op) \
//		op(RECT, rc,200,200,600,800 ) \
//		op(int, a ) \
//		op(float, b, 4.f ) \
//
//	foreach_Example_member(_generate_member); //adds all the members to your struct
//
//	char* text; //you can still add members like usual though this ones wont be serialized
//

//2. Generate the serialization and deserialization functions

//	_generate_default_struct_serialize(foreach_Example_member); //creates serialize() function
//
//	_generate_default_struct_deserialize(foreach_Example_member); //creates deserialize() function
//
//};

//3. Add your new struct to the serialization namespace

//_add_struct_to_serialization_namespace(Example)

//4. The only thing left is to ask for a specific object to be serialized/deserialized, currently this is done only on startup. Go to main.cpp or the equivalent file on your project, look for _deserialize_struct and _serialize_struct and do the same as what the guys already there do

//INFO:

//Each serialize-deserialize function pair can decide the string format of the variable, eg SIZE might be encoded "{14,15}"

//If the string does not contain the proper encoding for the variable it is ignored, and nothing is modified, 
//therefore everything is expected to be preinitialized if you want to guarantee valid values

//for complex structs inside complex structs define them separately:
//struct smth {
//	struct orth {
//		int a;
//		RECT b;
//	} o;
//	SIZE s;
//};
//Instead do this:
//struct orth {
//	int a;
//	RECT b;
//};
//struct smth {
//	orth o;
//	SIZE s;
//};



//----------------------Implementation-------------------------

//TODO(fran): use n_tabs for deserialization also, it would help to know how many tabs should be before an identifier to do further filtering
static i32 n_tabs(i32 v = INT32_MAX) {
	static i32 n_tabs=0;
	if (v != INT32_MAX) {
		n_tabs = v;
	}
	return n_tabs;
}

#define _BeginSerialize() n_tabs(0)
#define _BeginDeserialize() n_tabs(0)
#define _AddTab() n_tabs(n_tabs()+1)
#define _RemoveTab() n_tabs(n_tabs()-1)
#define _keyvaluesepartor str(L"=")
#define _structbegin str(L"{")
#define _structend str(L"}")
#define _memberseparator str(L",")
#define _newline str(L"\n")
#define _tab L'\t'
#define _tabs str(n_tabs(),_tab)

//IMPORTANT: I'm using str because of the ease of use around the macros, but actually every separator-like macro MUST be only 1 character long, not all functions were made with more than 1 character in mind, though most are

//We use the Reflection standard
#define _serialize_member(type,name,...) + _tabs + str(L#name) + _keyvaluesepartor + userial::serialize(name) + _newline /*TODO:find a way to remove this last +*/
#define _serialize_struct(var) str(L#var) + _keyvaluesepartor + (var).serialize() + _newline
#define _deserialize_struct(var,serialized_content) (var).deserialize(str(L#var),serialized_content);
#define _deserialize_member(type,name,...) userial::deserialize(name,str(L#name),substr); /*NOTE: requires for the variable: str substr; to exist and contain the string for deserialization*/

#define _generate_default_struct_serialize(foreach_op) \
	str serialize() { \
		_AddTab(); \
		str res = _structbegin + _newline foreach_op(_serialize_member); \
		_RemoveTab(); \
		res += _tabs + _structend; \
		return res; \
	} \

#define _generate_default_struct_deserialize(foreach_op) \
	bool deserialize(str name, const str& content) { \
		str start = name + _keyvaluesepartor + _structbegin + _newline; \
		size_t s = find_identifier(content, 0, start); \
		size_t e = find_closing_str(content, s + start.size(), _structbegin, _structend); \
		if (str_found(s) && str_found(e)) { \
			s += start.size(); \
			str substr = content.substr(s, e - s); \
			foreach_op(_deserialize_member); \
			return true; \
		} \
		return false; \
	} \

#define _add_struct_to_serialization_namespace(struct_type) \
	namespace userial {  \
		static str serialize(struct_type var) { \
			return var.serialize(); \
		} \
		static bool deserialize(struct_type& var, str name, const str& content) { \
			return var.deserialize(name, content); \
		} \
	} \

//NOTE: offset corresponds to "s"
//the char behind an identifier should only be one of this: "begin of file", _tab, _newline, _memberseparator, _structbegin
static size_t find_identifier(str s, size_t offset, str compare) {
	for (size_t p; str_found(p = s.find(compare, offset));) {
		if (p == 0) return p;
		if (s[p - 1] == _tab) return p;
		if (s[p - 1] == _newline[0]) return p;
		if (s[p - 1] == _memberseparator[0]) return p;
		if (s[p - 1] == _structbegin[0]) return p;
		offset = p + 1;
	}
	return str::npos;
}

static str load_file_serialized(std::wstring folder, std::wstring filename = L"\\serialized.txt") {
	std::wstring file = folder + filename;

	str res;
	HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER sz;
		if (GetFileSizeEx(hFile, &sz)) {
			u32 sz32 = (u32)sz.QuadPart;
			TCHAR* buf = (TCHAR*)malloc(sz32);
			if (buf) {
				DWORD bytes_read;
				if (ReadFile(hFile, buf, sz32, &bytes_read, 0) && sz32 == bytes_read) {
					res = buf;//TODO(fran): write straight to str, we are doing two copy unnecessarily
				}
				free(buf);
			}
		}
		CloseHandle(hFile);
	}
	return res;
}

static void save_to_file_serialized(str content, std::wstring folder, std::wstring filename = L"\\serialized.txt") {
	std::wstring dir = folder;
	std::wstring path = dir + filename;

	//SUPERTODO(fran): gotta create the folder first, if the folder isnt there the function fails
	CreateDirectoryW(dir.c_str(), 0);//Create the folder where info will be stored, since windows wont do it

	HANDLE file_handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle != INVALID_HANDLE_VALUE) {
		DWORD bytes_written;
		BOOL write_res = WriteFile(file_handle, (TCHAR*)content.c_str(), (DWORD)content.size() * sizeof(TCHAR), &bytes_written, NULL);
		CloseHandle(file_handle);
	}
}

//Sending everyone into a namespace avoids having to include a multitude of files here since you can re-open the namespace from other files
namespace userial {

	static str serialize(str var) {
		str res = _t("`") + var + _t("´");
		return res;
	}

	static str serialize(bool var) {
		str res = var ? _t("true") : _t("false");
		return res;
	}

	static str serialize(i32 var) {//Basic types are done by hand since they are the building blocks
		str res = to_str(var);
		return res;
	}
	static str serialize(f32 var) {
		str res = to_str(var);
		return res;
	}

	static str serialize(RECT var) {//Also simple structs that we know wont change
		str res = _structbegin + str(L"left") + _keyvaluesepartor + serialize((i32)var.left) + _memberseparator + str(L"top") + _keyvaluesepartor + serialize((i32)var.top) + _memberseparator + str(L"right") + _keyvaluesepartor + serialize((i32)var.right) + _memberseparator + str(L"bottom") + _keyvaluesepartor + serialize((i32)var.bottom) + _structend;
		return res;
	}

	static str serialize(HBRUSH var) {//You can store not the pointer but some of its contents
		COLORREF col = ColorFromBrush(var);
		str res = _structbegin + str(L"R") + _keyvaluesepartor + serialize(GetRValue(col)) + _memberseparator + str(L"G") + _keyvaluesepartor + serialize(GetGValue(col)) + _memberseparator + str(L"B") + _keyvaluesepartor + serialize(GetBValue(col)) + _structend;
		return res;
	}

	static bool deserialize(str& var, str name, const str& content) {
		str start = name + _keyvaluesepartor + _t("`");
		size_t s = find_identifier(content, 0, start);
		size_t e = find_closing_str(content, s + start.size(), _t("`"), _t("´"));
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			var = substr;
			return true;
		}
		return false;
	}

	static bool deserialize(bool& var, str name, const str& content) {
		str start = name + _keyvaluesepartor;
		size_t s = find_identifier(content, 0, start);
		if (str_found(s)) {
			s += start.size();
			str substr = content.substr(s, 5/*false is 5 chars*/);
			if (!substr.compare(_t("true"))) { var = true; return true; }
			else if (!substr.compare(_t("false"))) { var = false; return true; }
		}
		return false;
	}

	static bool deserialize(i32& var, str name, const str& content) {//We use references in deserialization to simplify the macros
		str start = name + _keyvaluesepartor;
		str numeric_i = str(L"1234567890.-");
		size_t s = find_identifier(content, 0, start);
		size_t e = find_till_no_match(content, s + start.size(), numeric_i);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			try {
				i32 temp = std::stoi(substr);
				var = temp;
				return true;
			}
			catch (...) {}
		}
		return false;
	}

	static bool deserialize(f32& var, str name, const str& content) {
		str start = name + _keyvaluesepartor;
		str numeric_f = str(L"1234567890.-");
		size_t s = find_identifier(content, 0, start);
		size_t e = find_till_no_match(content, s + start.size(), numeric_f);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			try {
				f32 temp = std::stof(substr);//TODO: check whether stof is locale dependent
				var = temp;
				return true;
			}
			catch (...) {}
		}
		return false;
	}

	static bool deserialize(HBRUSH& var, str name, const str& content) {
		str start = name + _keyvaluesepartor + _structbegin;
		size_t s = find_identifier(content, 0, start);
		size_t e = find_closing_str(content, s + start.size(), _structbegin, _structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			i32 R, G, B;
			bool r1 = _deserialize_member(0, R, 0);
			bool r2 = _deserialize_member(0, G, 0);
			bool r3 = _deserialize_member(0, B, 0);
			if (r1 && r2 && r3) {
				COLORREF col = RGB(R, G, B);
				var = CreateSolidBrush(col);
				return true;
			}
		}
		return false;
	}

	static bool deserialize(RECT& var, str name, const str& content) {
		str start = name + _keyvaluesepartor + _structbegin;
		size_t s = find_identifier(content, 0, start);
		size_t e = find_closing_str(content, s + start.size(), _structbegin, _structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			i32 left, top, right, bottom;
			bool r1 = _deserialize_member(0, left, 0);
			bool r2 = _deserialize_member(0, top, 0);
			bool r3 = _deserialize_member(0, right, 0);
			bool r4 = _deserialize_member(0, bottom, 0);
			if (r1 && r2 && r3 && r4) {
				var = { left,top,right,bottom };
				return true;
			}
		}
		return false;
	}

}