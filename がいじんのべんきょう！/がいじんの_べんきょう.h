#pragma once
#include "windows_sdk.h"
#include "がいじんの_Platform.h"
#include "unCap_Serialization.h"
#include "sqlite3.h"
#include <string>
#include "がいじんの_Helpers.h"
#include "unCap_button.h"
#include "LANGUAGE_MANAGER.h"
#include "unCap_edit_oneline.h"
#include "unCap_combobox.h"

//TODO(fran): it'd be nice to have a scrolling background with jp text going in all directions
//TODO(fran): a back button to go one page back
//TODO(fran): pressing enter on new_word page should map to the add button
//TODO(fran): tabstop for comboboxes doesnt seem to work
//TODO(fran): there are still some maximize-restore problems, where controls disappear
//TODO(fran): check that no kanji is written to the hiragana box
//TODO(fran): fix new_word edit control not showing the caret in the right place! im sure I already did this for some other project

//TODO(fran): hiragana text must always be rendered in the violet color I use in my notes, and the translation in my red, for kanji I dont yet know

//IMPORTANT INFO: datetimes are stored in GMT in order to be generic and have the ability to convert to the user's timestamp whenever needed, the catch now is we gotta REMEMBER that, we must convert creation_date to "localtime" before showing it to the user

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

//TODO(fran): should I use the hiragana column as primary key?

constexpr char べんきょう_table_words[] = "words";
constexpr char べんきょう_table_words_structure[] = 
	"hiragana			TEXT UNIQUE NOT NULL COLLATE NOCASE," //hiragana or katakana
	"kanji				TEXT COLLATE NOCASE,"
	"translation		TEXT NOT NULL COLLATE NOCASE," //TODO(fran): this should actually be a list, so we need a separate table
	"mnemonic			TEXT,"
	"lexical_category	INTEGER," //noun,verb,... TODO(fran): could use an enum and reference to another table
	"creation_date		TEXT DEFAULT CURRENT_TIMESTAMP," //ISO8601 string ("YYYY-MM-DD HH:MM:SS.SSS")
	"last_shown_date	INTEGER DEFAULT 0" //Unix Time, we dont care to show this value, it's simply for sorting
;//INFO: a column ROWID is automatically created and serves the function of "id INTEGER PRIMARY KEY" and AUTOINCREMENT, _but_ AUTOINCREMENT as in MySQL or others (simply incrementing on every insert), on the other hand the keyword AUTOINCREMENT in sqlite incurrs an extra cost because it also checks that the value hasnt already been used for a deleted row (we dont care for this in this table)
//INFO: the last line cant have a "," REMEMBER to put it or take it off

enum lexical_category { //The value of this enums is what will be stored on the db, we'll map them to their correct language string
	dont_care = 0, //I tend to annotate the different adjectives, but not much else
	noun,
	verb,
	adj_い,
	adj_な,
	adverb,
	conjunction, //and, or, but, ...
	pronoun,
	counter,
};

str lexical_category_to_str(lexical_category cat) {
	return RS(200 + cat - 1); //NOTE: dont_care should never be shown
}
u32 lexical_category_str_id(lexical_category cat) {
	return 200 + cat - 1; //NOTE: dont_care should never be shown
}
void lexical_category_setup_combobox(HWND cb) {
	//IMPORTANT INFO: the first element to add to a combobox _must_ be at index 0, it does not support adding any index, amazingly enough, conclusion: the windows' combobox is terrible
	//So thanks to the huge limitation of comboboxes we have to subtract one from everything, and at the time of consulting the cb we'll need to add one, this is simply stupid //TODO(fran): find a fix
	ACT(cb, lexical_category::noun-1, lexical_category_str_id(lexical_category::noun));
	ACT(cb, lexical_category::verb-1, lexical_category_str_id(lexical_category::verb));
	ACT(cb, lexical_category::adj_い-1, lexical_category_str_id(lexical_category::adj_い));
	ACT(cb, lexical_category::adj_な-1, lexical_category_str_id(lexical_category::adj_な));
	ACT(cb, lexical_category::adverb-1, lexical_category_str_id(lexical_category::adverb));
	ACT(cb, lexical_category::conjunction-1, lexical_category_str_id(lexical_category::conjunction));
	ACT(cb, lexical_category::pronoun-1, lexical_category_str_id(lexical_category::pronoun));
	ACT(cb, lexical_category::counter-1, lexical_category_str_id(lexical_category::counter));
}
lexical_category retrieve_lexical_category_from_combobox(HWND cb) {
	lexical_category res;
	int sel = (int)SendMessage(cb, CB_GETCURSEL, 0, 0);
	res = (lexical_category)(sel + 1);//NOTE: thanks to the fact that when there's no selection sel=-1 we can map everything perfectly from dont_care onwards
	return res;
}
union learnt_word { //will contain utf16* when getting data from the UI, and utf8* to send requests to the db
	struct s { void* mem; size_t sz; };
	using type = s;
	struct {
#define _foreach_learnt_word_member(op) \
		op(type,hiragana) \
		op(type,kanji) \
		op(type,translation) \
		op(type,lexical_category) \
		op(type,mnemonic) \

		_foreach_learnt_word_member(_generate_member_no_default_init);

	} attributes;
	type all[sizeof(attributes) / sizeof(type)];

	//learnt_word() { for (auto& s : all) s = str(L""); } //TODO(fran): why do I need to do this for the compiler to allow me to instantiate?
	//~learnt_word() { for (auto& s : all) s.~basic_string(); }
};

struct べんきょうProcState {
	HWND wnd;
	HWND nc_parent;
	べんきょうSettings* settings;

	struct {
		HBRUSH bk;
	} brushes;

	enum page { //This wnd will be managed in pages instead of launching separate windows for different tasks, like a browser
		landing,
		new_word,
		practice,
		search,
		show_word,
	} current_page;

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
				type edit_hiragana;//or katakana
				type edit_kanji;
				type edit_translation;
				type combo_lexical_category;//verb,noun,...
				type edit_mnemonic;//create a story/phrase around the word
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
				type combo_search;
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

namespace べんきょう { //INFO: Im trying namespaces to see if this is better than having every function with the name of the wndclass
	using ProcState = べんきょうProcState;

	constexpr cstr wndclass[] = L"がいじんの_wndclass_べんきょう";

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	//NOTE: any NULL HBRUSH remains unchanged
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	void startup(ProcState* state) {
		using namespace std::string_literals;
		std::string create_work_table = "CREATE TABLE IF NOT EXISTS "s + べんきょう_table_words + "("s + べんきょう_table_words_structure+ ");"s; //INFO: the param that requests this expects utf8
		char* create_errmsg;
		sqlite3_exec(state->settings->db, create_work_table.c_str(), 0, 0, &create_errmsg);
		sqlite_exec_runtime_assert(create_errmsg);
	}

	void add_controls(ProcState* state) {
		//---------------------Landing page----------------------:
		state->controls.landingpage.list.button_new = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.landingpage.list.button_new, 100);
		//TODO(fran): more brushes, fore_push,... , border_mouseover,...
		UNCAPBTN_set_brushes(state->controls.landingpage.list.button_new, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		state->controls.landingpage.list.button_practice = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.landingpage.list.button_practice, 101);
		//TODO(fran): more brushes, fore_push,... , border_mouseover,...
		UNCAPBTN_set_brushes(state->controls.landingpage.list.button_practice, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		state->controls.landingpage.list.button_search = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.landingpage.list.button_search, 102);
		//TODO(fran): more brushes, fore_push,... , border_mouseover,...
		UNCAPBTN_set_brushes(state->controls.landingpage.list.button_search, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		for (auto ctl : state->controls.landingpage.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);

		//---------------------New word----------------------:
		state->controls.new_word.list.edit_hiragana = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		EDITONELINE_set_brushes(state->controls.new_word.list.edit_hiragana, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
		SendMessage(state->controls.new_word.list.edit_hiragana, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(120));

		state->controls.new_word.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		lexical_category_setup_combobox(state->controls.new_word.list.combo_lexical_category);
		//SendMessage(state->controls.new_word.list.combo_lexical_category, CB_SETCURSEL, 0, 0);
		SetWindowSubclass(state->controls.new_word.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
		SendMessage(state->controls.new_word.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)unCap_bmps.dropdown, 0);
		ACC(state->controls.new_word.list.combo_lexical_category, 123);

		state->controls.new_word.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		EDITONELINE_set_brushes(state->controls.new_word.list.edit_kanji, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
		SendMessage(state->controls.new_word.list.edit_kanji, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(121));

		state->controls.new_word.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		EDITONELINE_set_brushes(state->controls.new_word.list.edit_translation, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
		SendMessage(state->controls.new_word.list.edit_translation, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(122));

		state->controls.new_word.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		EDITONELINE_set_brushes(state->controls.new_word.list.edit_mnemonic, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
		SendMessage(state->controls.new_word.list.edit_mnemonic, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(125));

		state->controls.new_word.list.button_save = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.new_word.list.button_save, 124);
		UNCAPBTN_set_brushes(state->controls.new_word.list.button_save, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		for (auto ctl : state->controls.new_word.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);

		//TODO(fran): create the other "tabs"

		//---------------------Search----------------------:
		state->controls.search.list.combo_search = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP /*TODO(fran):is CBS_HASSTRINGS necessary?*/
			, 0,0,0,0, state->wnd, 0, NULL, NULL);
		ACC(state->controls.search.list.combo_search, 250); //TODO(fran): allow from multiple searching, eg if kanji detected search by kanji, if neither hiragana nor kanji search translation
		SetWindowSubclass(state->controls.search.list.combo_search, TESTComboProc, 0, 0);
		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(state->controls.search.list.combo_search, &info);
		//SetWindowSubclass(info.hwndItem, PrintMsgProc, 0, (DWORD_PTR)"Combo Edit");
		SetWindowSubclass(info.hwndList, PrintMsgProc, 0, (DWORD_PTR)"Combo List");
		//cstr listclass[100];
		//GetClassNameW(info.hwndList, listclass, 100); //INFO: class is ComboLBox, one interesting class windows has an it is hidden

		for (auto ctl : state->controls.search.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
	}

	void resize_controls(ProcState* state) {
		//TODO(fran): should I resize everyone or just the ones from the current page, I feel like just resizing the necessary ones is better and when there's a page change we call resize there just in case
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTWIDTH(r);
		int h = RECTHEIGHT(r);
		int w_pad = (int)((float)w * .05f);
		int h_pad = (int)((float)h * .05f);
		
#define _MyMoveWindow(wnd,xywh,repaint) MoveWindow(wnd, xywh##_x, xywh##_y, xywh##_w, xywh##_h, repaint);

		switch (state->current_page) {
		case ProcState::page::landing: 
		{
			//One button on top of the other vertically, all buttons must be squares
			//TODO(fran): we could, at least to try how it looks, to check whether the wnd is longer on w or h and place the controls vertically or horizontally next to each other
			int wnd_cnt = ARRAYSIZE(state->controls.landingpage.all);
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control

			int max_h = h - pad_cnt * h_pad;
			int max_w = w - 2 * w_pad;
			int wnd_h = clamp(0, max_h / wnd_cnt, max_w);
			int wnd_w = wnd_h;
			int wnd_x = (w - wnd_w) / 2;
			int h_step = (h - (wnd_h*wnd_cnt)) / pad_cnt;

			int button_new_x = wnd_x;
			int button_new_y = h_step;
			int button_new_w = wnd_h;
			int button_new_h = wnd_h;

			int button_practice_x = wnd_x;
			int button_practice_y = button_new_y + button_new_h + h_step;
			int button_practice_w= wnd_h;
			int button_practice_h= wnd_h;

			int button_search_x = wnd_x;
			int button_search_y = button_practice_y + button_practice_h + h_step;
			int button_search_w= wnd_h;
			int button_search_h= wnd_h;

			_MyMoveWindow(state->controls.landingpage.list.button_new, button_new,FALSE);
			_MyMoveWindow(state->controls.landingpage.list.button_practice, button_practice,FALSE);
			_MyMoveWindow(state->controls.landingpage.list.button_search, button_search,FALSE);
		} break;
		case ProcState::page::new_word:
		{
			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned

			int wnd_cnt = ARRAYSIZE(state->controls.new_word.all);
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int start_y = (h - (pad_cnt-1 /*we only have 1 real pad for lex_categ*/) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): centering aint quite right

			int edit_hiragana_x = w_pad;
			int edit_hiragana_y = start_y;
			int edit_hiragana_h = wnd_h;
			int edit_hiragana_w = max_w; //TODO(fran): we may want to establish a max_w that's more fixed, as a clamp, instead of continually increasing as the wnd width does

			int cb_lex_categ_w = max_w / 3;
			int cb_lex_categ_x = w - w_pad - cb_lex_categ_w;
			int cb_lex_categ_y = edit_hiragana_y + edit_hiragana_h + h_pad/2;
			int cb_lex_categ_h = edit_hiragana_h;//TODO(fran): for some reason comboboxes are always a little smaller than you ask, find out how to correctly correct that
			

			int edit_kanji_x = edit_hiragana_x;
			int edit_kanji_y = cb_lex_categ_y + cb_lex_categ_h + h_pad/2;
			int edit_kanji_w = max_w;
			int edit_kanji_h = wnd_h;

			int edit_translation_x = edit_hiragana_x;
			int edit_translation_y = edit_kanji_y + edit_kanji_h + h_pad;
			int edit_translation_w = max_w;
			int edit_translation_h = wnd_h;

			int edit_mnemonic_x = edit_hiragana_x;
			int edit_mnemonic_y = edit_translation_y + edit_translation_h + h_pad;
			int edit_mnemonic_w = max_w;
			int edit_mnemonic_h = wnd_h;

			int btn_save_w = 70;
			int btn_save_h = wnd_h;
			int btn_save_y = edit_mnemonic_y + edit_mnemonic_h + h_pad;
			int btn_save_x = (w - btn_save_w) / 2;

			_MyMoveWindow(state->controls.new_word.list.edit_hiragana, edit_hiragana, FALSE);
			_MyMoveWindow(state->controls.new_word.list.combo_lexical_category, cb_lex_categ, FALSE);
			_MyMoveWindow(state->controls.new_word.list.edit_kanji, edit_kanji, FALSE);
			_MyMoveWindow(state->controls.new_word.list.edit_translation, edit_translation, FALSE);
			_MyMoveWindow(state->controls.new_word.list.edit_mnemonic, edit_mnemonic, FALSE);
			_MyMoveWindow(state->controls.new_word.list.button_save, btn_save, FALSE);

		} break;
		case ProcState::page::practice: 
		{
			state->controls.practice.all;
		} break;
		case ProcState::page::search: 
		{
			int max_w = w - w_pad * 2;
			int cb_search_x = w_pad;
			int cb_search_y = h_pad;
			int cb_search_h = 40;//TODO(fran): cbs dont respect this at all, they set a minimum h by the font size I think
			int cb_search_w = max_w;

			_MyMoveWindow(state->controls.search.list.combo_search, cb_search, FALSE);

		} break;
		case ProcState::page::show_word: 
		{
			state->controls.show_word.all;
		} break;
		}
	}

	void save_settings(ProcState* state) {
		RECT rc; GetWindowRect(state->wnd, &rc);
		state->settings->rc = rc;
	}

	void show_page(ProcState* state, ProcState::page p, u32 ShowWindow_cmd /*SW_SHOW,...*/) {
		switch (p) {
		case ProcState::page::landing: for (auto ctl : state->controls.landingpage.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::new_word: for (auto ctl : state->controls.new_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::practice: for (auto ctl : state->controls.practice.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::search: for (auto ctl : state->controls.search.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::show_word: for (auto ctl : state->controls.show_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		}
	}

	void set_current_page(ProcState* state, ProcState::page new_page) {
		show_page(state, state->current_page,SW_HIDE);
		state->current_page = new_page;
		resize_controls(state);
		show_page(state, state->current_page,SW_SHOW);
	}

	bool check_new_word(ProcState* state) {
		auto& page = state->controls.new_word;
		HWND edit_required[] = { page.list.edit_hiragana,page.list.edit_translation };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				EDITONELINE_show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, ETP::top);
				return false;
			}
		}
		return true;
	}

	bool insert_new_word(ProcState* state, const learnt_word& w) {
		bool res;
		//TODO(fran): specify all the columns for the insert, that will be our error barrier
		//TODO(fran): we are inserting everything with '' which is not right for numbers
		//NOTE: here I have an idea, if I store the desired type I can do type==number? string : 'string'

#define _sqlite3_generate_columns(type,name,...) "" + #name + ","
		std::string columns = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_columns);
		columns.pop_back(); //We need to remove the trailing comma

#define _sqlite3_generate_values(type,name,...) "'" + (utf8*)w.attributes.name.mem + "'"","
		std::string values = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_values); 
		values.pop_back(); //We need to remove the trailing comma

		std::string insert_word = std::string("INSERT INTO ") + べんきょう_table_words + "(" + columns + ")" + " VALUES(" + values + ");";

		char* insert_errmsg;
		res = sqlite3_exec(state->settings->db, insert_word.c_str(), 0, 0, &insert_errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(insert_errmsg);

		//TODO(fran): handle if the word already exists, maybe show the old word and ask if they want to override that content, NOTE: the handling code shouldnt be here, this function should be as isolated as possible, if we start heavily interacting with the user this will be ugly

		return res;
	}

	bool save_new_word(ProcState* state) {
		bool res = false;
		if (check_new_word(state)) {
			learnt_word w;
			auto& page = state->controls.new_word;
			//TODO(fran): macro
			int sz_char;
#define _get_edit_str(sz_char,edit,str) \
			sz_char = (int)SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0) + 1; \
			str.sz = sz_char * sizeof(utf16); \
			str.mem = malloc(str.sz); \
			SendMessageW(edit, WM_GETTEXT, sz_char, (WPARAM)str.mem);

			_get_edit_str(sz_char, page.list.edit_hiragana, w.attributes.hiragana);
			_get_edit_str(sz_char, page.list.edit_kanji, w.attributes.kanji);
			_get_edit_str(sz_char, page.list.edit_translation, w.attributes.translation);
			_get_edit_str(sz_char, page.list.edit_mnemonic, w.attributes.mnemonic);

			//NOTE: we convert the number to utf16 and then utf8 in order to make the conversion be able to iterate over all elements of the array
			int lex_categ = retrieve_lexical_category_from_combobox(page.list.combo_lexical_category);
			sz_char = _snwprintf(nullptr,0, L"%d", lex_categ) + 1;
			w.attributes.lexical_category.sz = sz_char * sizeof(utf16);
			w.attributes.lexical_category.mem = malloc(w.attributes.lexical_category.sz);
			_snwprintf((utf16*)w.attributes.lexical_category.mem, sz_char, L"%d", lex_categ);

			learnt_word w_utf8;
			for (int i = 0; i < ARRAYSIZE(w.all); i++) {
				auto res = convert_utf16_to_utf8((utf16*)w.all[i].mem, (int)w.all[i].sz);
				w_utf8.all[i].mem = res.mem;
				w_utf8.all[i].sz = res.sz;
				free(w.all[i].mem);//maybe set it to zero too
			}
			defer{ for (auto& _ : w_utf8.all)free_convert(_.mem); };
			//Now we can finally do the insert, TODO(fran): see if there's some way to go straight from utf16 to the db, and to send things like ints without having to convert them to strings
			res = insert_new_word(state, w_utf8); 
			//TODO(fran): maybe handle repeated words here
		}
		return res;
	}
	struct search_word_res { utf16** matches; int cnt; };
	void free_search_word(search_word_res* res) {
		for (int i = 0; i < res->cnt; i++) {
			free_convert(res->matches[i]);
		}
		free(res->matches);
		res->cnt = 0;//shouldnt be necessary
	}
	search_word_res/*what to return?*/ search_word(ProcState* state, utf16* match/*bytes*/, int max_cnt_results/*eg. I just want the top 5 matches*/) {
		//NOTE/TODO(fran): for now we'll simply retrieve the hiragana, it might be good to get the translation too, so the user can quick search, and if they really want to know everything about that word then the can click it and pass to the next stage
		search_word_res res; 
		res.matches = (decltype(res.matches))malloc(max_cnt_results * sizeof(*res.matches));
		res.cnt = 0;
		auto match_str = convert_utf16_to_utf8(match, (int)(cstr_len(match)+1)*sizeof(*match)); defer{ free_convert(match_str.mem); };
		char* select_errmsg;
		std::string select_matches = std::string("SELECT ") + "hiragana" /*TODO(fran): column names should be stored somewhere*/ + " FROM " + べんきょう_table_words + " WHERE " + "hiragana" " LIKE '" + (utf8*)match_str.mem + "%'" + " LIMIT " + std::to_string(max_cnt_results) + ";";

		auto parse_match_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			//NOTE: from what I understood this gets executed once for every resulting row
			//NOTE: unfortunately everything here comes in utf8 //TODO(fran): may be better to use the separate functions sqlite3_prepare, sqlite3_step, and to retrieve use sqlite3_column_[type] (this way I get utf16 at once)
			Assert(column_cnt == 1);
			search_word_res* res = (decltype(res))extra_param;

			//res->matches[res->cnt] = (decltype(*res->matches))convert_utf8_to_utf16(results[0], (int)strlen(results[0]) + 1).mem;//INFO: interesting, I know this is correct but compiler cant allow it, an explanation here https://stackoverflow.com/questions/3674456/why-this-is-causing-c2102-requires-l-value

			auto r = convert_utf8_to_utf16(results[0], (int)strlen(results[0]) + 1);
			res->matches[res->cnt] = (decltype(*res->matches))r.mem;

			res->cnt++;
			return 0;//if non-zero then the query stops and exec returns SQLITE_ABORT
		};

		sqlite3_exec(state->settings->db, select_matches.c_str(), parse_match_result, &res, &select_errmsg);
		sqlite_exec_runtime_check(select_errmsg);//TODO(fran): should I free the result and return an empty?
		return res;
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			decltype(state) st = (decltype(st))calloc(1, sizeof(decltype(*st)));
			Assert(st);
			st->nc_parent = GetParent(hwnd);
			st->wnd = hwnd;
			st->settings = ((べんきょうSettings*)create_nfo->lpCreateParams);
			st->current_page = ProcState::page::landing;
			set_state(hwnd, st);//TODO(fran): now I wonder why I append everything with the name of the wnd when I can simply use function overloading, even for get_state since all my functions do the same I can simple have one
			startup(st);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			add_controls(state);

			set_current_page(state, ProcState::page::landing);//TODO(fran): page:: would be nicer than ProcState::page::

			return 0;
		} break;
		case WM_SIZE:
		{
			LRESULT res = DefWindowProc(hwnd, msg, wparam, lparam);
			resize_controls(state);
			return res;
		} break;
		case WM_CLOSE:
		{
			return 0;
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				save_settings(state);
				free(state);
				state = nullptr;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_COMMAND:
		{
			HWND child = (HWND)lparam;
			if (child) {//Notifs from our childs
				switch (state->current_page) {
				case ProcState::page::landing:
				{
					auto& page = state->controls.landingpage;
					if (child == page.list.button_new) {
						set_current_page(state, ProcState::page::new_word);
					}
					else if (child == page.list.button_practice) {
						set_current_page(state, ProcState::page::practice);
					}
					else if (child == page.list.button_search) {
						set_current_page(state, ProcState::page::search);
					}
				} break;
				case ProcState::page::new_word:
				{
					auto& page = state->controls.new_word;
					if (child == page.list.button_save) {
						if (save_new_word(state)) {
							//If the new word was successfully saved then go back to the landing, TODO(fran): a better idea may be to reset the new_word page, so that the user can add multiple words faster
							set_current_page(state, ProcState::page::landing);
							//TODO(fran): clear new_word controls, now this one's annoying cause not everything is cleared the same way
						}
					}
				} break;
				case ProcState::page::practice:
				{
					auto& page = state->controls.practice;
				} break;
				case ProcState::page::search:
				{
					auto& page = state->controls.search;
					if (child == page.list.combo_search) {
						WORD notif = HIWORD(wparam);
						switch (notif) {
						case CBN_EDITUPDATE://The user has changed the content of the edit box, this msg is sent before the change is rendered in the control
						{
							//NOTE: there's also CBN_EDITCHANGE which does the same but after the modification has been rendered

							//Do a search for what they wrote and show the results in the listbox of the combobox
							//For now we'll get a max of 5 results, the best would be to set the max to reach the bottom of the parent window (aka us) so it can get to look like a full window instead of feeling so empty //TODO(fran): we could also append the "searchbar" in the landing page
							int sz_char = (int)SendMessage(page.list.combo_search, WM_GETTEXTLENGTH, 0, 0)+1;
							if (sz_char > 1) {
								utf16* search = (decltype(search))malloc(sz_char * sizeof(*search));
								SendMessage(page.list.combo_search, WM_GETTEXT, sz_char, (LPARAM)search);

								//TODO(fran): this might be an interesting case for multithreading, though idk how big the db has to be before this search is slower than the user writing, also for jp text the IME wont allow for the search to start til the user presses enter, that may be another <-TODO(fran): chrome-like handling for IME, text goes to the edit control at the same time as the IME

								auto search_res = search_word(state, search, 5);
								//TODO(fran): clear the listbox
								if (search_res.cnt) {
									for (int i = 0; i < search_res.cnt; i++) {
										//SendMessageW(page.list.combo_search, CB_INSERTSTRING, -1, (LPARAM)search_res.matches[i]);
									}
									//TODO(fran): for some reason the cb decides to set the selection once we insert some strings, if we try to set it to "no selection" it erases the user's string, terrible, we must fix this from the subclass

									//Show the listbox
									SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, TRUE, 0);
#if 0
									COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
									int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);//returns -1 on error, but who cares
									//NOTE: unless LBS_OWNERDRAWVARIABLE style is defined all items have the same height
									int list_h = max(SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt,10);
									RECT rw; GetWindowRect(page.list.combo_search, &rw);
									MoveWindow(nfo.hwndList, rw.left, rw.bottom, RECTWIDTH(rw), list_h, TRUE);
									AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
									SendMessage(nfo.hwndList, 0x1ae/*LBCB_STARTTRACK*/, 0, 0); //does start mouse tracking
									//NOTE: I think LBCB_ENDTRACK is handled by itself
#endif
									//TODO(fran): check listbox windowrect here
									//IDEA: bail out and have the first item on the list be exactly what the user just wrote(chrome does the same), serves as a helping hand for someone that wants to go back to what they wrote but dont know they can by pressing escape
								}
								else {
									//Hide the listbox
									SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
								}
							}
							else {
								//Hide the listbox
								SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
							}
						} break;
						}
					}
				} break;
				case ProcState::page::show_word:
				{
					auto& page = state->controls.show_word;
				} break;

				}
			}
			else {
				switch (LOWORD(wparam)) {//Menu notifications

				}
			}
			return 0;
		} break;
		case WM_NCCALCSIZE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SHOWWINDOW:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_GETTEXTLENGTH:
		{
			return 0;
		} break;
		case WM_IME_SETCONTEXT://sent the first time on SetFocus
		{
			return 0; //We dont want IME for the general wnd, the childs can decide
		} break;
		case WM_IME_NOTIFY://for some reason you still get this even when not doing WM_IME_SETCONTEXT
		{
			return 0;
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCPAINT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_ERASEBKGND:
		{
			LRESULT res;
			if (state->brushes.bk) {
				//TODO(fran): idk if WS_CLIPCHILDREN | WS_CLIPSIBLINGS automatically clip that regions when I draw or I have to do it manually to avoid flicker
				HDC dc = (HDC)wparam;
				RECT r; GetClientRect(state->wnd, &r);
				FillRect(dc, &r, state->brushes.bk);
				res = 1;//We erased the bk
			}
			else res = DefWindowProc(hwnd, msg, wparam, lparam);
			return res;
		} break;
		case WM_CTLCOLORLISTBOX: //for combobox list //TODO(fran): this has to go
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(unCap_colors.ControlTxt));

			return (INT_PTR)unCap_colors.ControlBk;
		} break;
		case WM_CTLCOLOREDIT: //for the combobox edit control (if I dont subclass it) //TODO(fran): this has to go
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(unCap_colors.ControlTxt));

			return (INT_PTR)unCap_colors.ControlBk;
		} break;
		case WM_PAINT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCHITTEST:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETCURSOR:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEMOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEACTIVATE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PARENTNOTIFY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_KILLFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PRINTCLIENT://For some reason the base combobox (not subclassed) sends this msg, it makes no sense to me why the cb would need my client rendering
		{
			return 0;
		} break;
		default:
#ifdef _DEBUG
			Assert(0);
#else 
			return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
		}
		return 0;
	}

	void init_wndclass(HINSTANCE inst) { //INFO: now that we use pre_post_main we cant depend on anything that isnt calculated at compile time
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = Proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(ProcState*);
		wcex.hInstance = inst;
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = 0; //TODO(fran): unCap_colors.ControlBk hasnt been deserialized by the point pre_post_main gets executed!
		wcex.lpszMenuName = 0;
		wcex.lpszClassName = wndclass;
		wcex.hIconSm = 0;

		ATOM class_atom = RegisterClassExW(&wcex);
		Assert(class_atom);
	}

	struct pre_post_main {
		pre_post_main() {
			init_wndclass(GetModuleHandleW(NULL));
		}
		~pre_post_main() { //INFO: you can also use the atexit function
			//Classes are de-registered automatically by the os
		}
	};
	static const pre_post_main PREMAIN_POSTMAIN;
}
