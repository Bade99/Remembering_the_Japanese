#pragma once
#include "windows_sdk.h"
#include "がいじんの_Platform.h"
#include "unCap_Serialization.h"
#include "sqlite3.h"
#include <string>
#include "がいじんの_Helpers.h"
#include "unCap_button.h"
#include "LANGUAGE_MANAGER.h"

constexpr cstr がいじんの_wndclass_べんきょう[] = L"がいじんの_wndclass_べんきょう";

struct べんきょうSettings {

#define foreach_べんきょうSettings_member(op) \
		op(RECT, rc,200,200,600,800 ) \

	foreach_べんきょうSettings_member(_generate_member);

	sqlite3* db;

	_generate_default_struct_serialize(foreach_べんきょうSettings_member);

	_generate_default_struct_deserialize(foreach_べんきょうSettings_member);

};
_add_struct_to_serialization_namespace(べんきょうSettings)

//This window will comprise of 4 main separate ones, as if they were in a tab control, in the sense only one will be shown at any single time, and there wont be any relationships between them

constexpr char べんきょう_table_words[] = "words";
constexpr char べんきょう_table_words_structure[] = 
	"hiragana			TEXT UNIQUE NOT NULL COLLATE NOCASE," //hiragana or katakana
	"kanji				TEXT COLLATE NOCASE,"
	"translation		TEXT NOT NULL COLLATE NOCASE" //TODO(fran): this should actually be a list, so we need a separate table
	"type				TEXT NOT NULL COLLATE NOCASE," //noun,verb,... TODO(fran): could use an enum and reference to another table
	"creation_date		TEXT" //ISO8601 string ("YYYY-MM-DD HH:MM:SS.SSS")
	"last_shown_date	INTEGER" //Unix Time, we dont care to show this value, it's simply for sorting
	;//INFO: a column ROWID is automatically created and serves the function of "id INTEGER PRIMARY KEY" and AUTOINCREMENT, _but_ AUTOINCREMENT as in MySQL or others (simply incrementing on every insert), on the other hand the keyword AUTOINCREMENT in sqlite incurrs an extra cost because it also checks that the value hasnt already been used for a deleted row (we dont care for this in this table)

struct べんきょうProcState {
	HWND wnd;
	HWND nc_parent;
	べんきょうSettings* settings;

	struct {

		union landingpage_controls {
			using type = HWND;
			struct {
				type button_new;
				type button_practice;
				type button_search;
			}list; //INFO: unfortunately in order to make 'all' auto-update we need to give a name to this struct
			type all[sizeof(list)/sizeof(type)]; //NOTE: make sure you understand structure padding before implementing this, also this should be re-tested if trying with different compilers or alignment
			//static_assert(sizeof(((landingpage_controls*)0)->all) == sizeof(s), "Element count for 'all' does not match, update the number!");
		} landingpage;

		union new_word_controls {
			using type = HWND;
			struct {
				type edit_kanji;
				type edit_hiragana;//or katakana
				type edit_translation;
				type combo_type;//verb,noun,...
				//TODO(fran): here you should be able to add more than one translation
				type button_save;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} new_word;

		union practice_controls {
			using type = HWND;
			struct {
				type edit_question;
				type edit_answer;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice;

		union search_controls {
			using type = HWND;
			struct {
				type edit_search;
				//and a list for the results
			}list;
			type all[sizeof(list) / sizeof(type)];
		} search;

		union show_word_controls {
			using type = HWND;
			struct {
				type edit_kanji;
				type edit_hiragana;//or katakana
				type edit_translation;
				type combo_type;//verb,noun,...
				//TODO(fran): here you should be able to add more than one translation
				type button_modify;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} show_word;

	}controls;
};

べんきょうProcState* べんきょう_get_state(HWND wnd) {
	べんきょうProcState* state = (べんきょうProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	return state;
}

void べんきょう_set_state(HWND wnd, べんきょうProcState* state) {
	SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
}

void べんきょう_startup(べんきょうProcState* state) {
	using namespace std::string_literals;
	std::string create_work_table = "CREATE TABLE IF NOT EXISTS"s + べんきょう_table_words + "("s + べんきょう_table_words_structure+ ");"s; //INFO: the param that requests this expects utf8
	char* create_errmsg;
	sqlite3_exec(state->settings->db, create_work_table.c_str(), 0, 0, &create_errmsg);
	sqlite_exec_runtime_assert(create_errmsg);
}

void べんきょう_add_controls(べんきょうProcState* state) {
	//---------------------Landing page----------------------:
	state->controls.landingpage.list.button_new = CreateWindowW(unCap_wndclass_button, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP
		, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
	AWT(state->controls.landingpage.list.button_new, 0);
	UNCAPBTN_set_brushes(state->controls.landingpage.list.button_new, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
}

LRESULT CALLBACK べんきょうProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	べんきょうProcState* state = べんきょう_get_state(hwnd);
	switch (msg) {
	case WM_NCCREATE:
	{
		CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
		decltype(state) st = (decltype(st))calloc(1, sizeof(decltype(*st)));
		Assert(st);
		st->nc_parent = GetParent(hwnd);
		st->wnd = hwnd;
		st->settings = ((べんきょうSettings*)create_nfo->lpCreateParams);
		べんきょう_set_state(hwnd, st);//TODO(fran): now I wonder why I append everything with the name of the wnd when I can simply use function overloading, even for get_state since all my functions do the same I can simple have one
		べんきょう_startup(st);
		return TRUE;
	} break;
	case WM_CREATE:
	{
		CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

		べんきょう_add_controls(state);

		return 0;
	} break;
	}
}