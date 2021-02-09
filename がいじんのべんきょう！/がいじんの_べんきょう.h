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
#include "unCap_Math.h"
#include "がいじんの_score.h"
#include "win32_static_oneline.h"
#include "win32_graph.h"


//INFO: this wnd is divided into two, the UI side and the persistence/db interaction side, the first one operates on utf16, and the second one in utf8 for input and utf8 for output (output could be changed to utf16)

//TODO(fran): it'd be nice to have a scrolling background with jp text going in all directions
//TODO(fran): pressing enter on new_word page should map to the add button
//TODO(fran): tabstop for comboboxes doesnt seem to work
//TODO(fran): there are still some maximize-restore problems, where controls disappear
//TODO(fran): check that no kanji is written to the hiragana box
//TODO(fran): fix new_word edit control not showing the caret in the right place! im sure I already did this for some other project
//TODO(fran): add extra control on new_word called "Notes" where the user can write anything eg make a clarification
//TODO(fran): hiragana text must always be rendered in the violet color I use in my notes, and the translation in my red, for kanji I dont yet know
//TODO(fran): pressing enter in the edit control of the searchbox should trigger searching
//TODO(fran): when querying for dates to show to the user format them to show up to the day, not including hours,min,sec
//TODO(fran): extra page "Stats" or "Your progress" so the user can see how they are doing, we can put there a total percentage of accuracy in a nice circular control that animates from 0% to whatever the user's got. also word count, accuracy, number of practices, ...
//TODO(fran): the add buton in new_word should offer the possibility to update (in the case where the word already exists), and show a separate (non editable) window with the current values of that word, idk whether I should modify from there or send the data to the show_word page and redirect them to there, in that case I really need to start using ROWID to predetermine which row has to be modified, otherwise the user can change everything and Im screwed
//TODO(fran): at the end of each practice I image a review page were not only do you get your score, but also a grid of buttons (green if you guessed correctly, red for incorrect) with each one having the hiragana of each word in the practice, and you being able to press that button like object and going to the show_word page
//TODO(fran): I dont know who should be in charge of converting from utf16 to utf8, the backend or front, Im now starting to lean towards the first one, that way we dont have conversion code all over the place, it's centralized in the functions themselves
//TODO(fran): we may want everything in the "practice" page to be animated, otherwise it feels like you're waiting for the score to fill, though maybe making that go a lot faster solves the issue, right now it's slow cause of performance
//TODO(fran): we may actually want to add a stats page to the landing page (and make it a 2x2 grid) in order to put stuff that's in practice, like "word count" and extra things like a list of last words added

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

constexpr char べんきょう_table_words[] = "words";
constexpr char べんきょう_table_words_structure[] = 
//NOTE: user modifiable values first, application defined after
	"hiragana			TEXT PRIMARY KEY COLLATE NOCASE," //hiragana or katakana //NOTE: we'll find out if it was a good idea to switch to using this as the pk instead of rowid, I have many good reasons for both
	"kanji				TEXT COLLATE NOCASE,"
	"translation		TEXT NOT NULL COLLATE NOCASE," //TODO(fran): this should actually be a list, so we need a separate table
	"mnemonic			TEXT,"
	"lexical_category	INTEGER," //noun,verb,... TODO(fran): could use an enum and reference to another table
	/*System generated*/
	"creation_date		TEXT DEFAULT CURRENT_TIMESTAMP," //ISO8601 string ("YYYY-MM-DD HH:MM:SS.SSS")
	"last_shown_date	INTEGER DEFAULT 0," //Unix Time, used for sorting, TODO(fran): I should actually put a negative value since 0 maps to 1970 but unix time can go much further back than that
	"times_shown		INTEGER DEFAULT 0,"
	"times_right		INTEGER DEFAULT 0" 
;//INFO: a column ROWID is automatically created and serves the function of "id INTEGER PRIMARY KEY" and AUTOINCREMENT, _but_ AUTOINCREMENT as in MySQL or others (simply incrementing on every insert), on the other hand the keyword AUTOINCREMENT in sqlite incurrs an extra cost because it also checks that the value hasnt already been used for a deleted row (we dont care for this in this table)
//INFO: the last line cant have a "," REMEMBER to put it or take it off

#define べんきょう_table_user "user" //This table will simply contain one user, having to join user-word to enable multiuser is just a waste of performance, if we have multiple users we'll create a different folder and db for each, also that will prevent that corruption of one user affects another
#define べんきょう_table_user_structure \
	"word_cnt			INTEGER DEFAULT 0,"\
	"times_practiced	INTEGER DEFAULT 0,"\
	"times_shown		INTEGER DEFAULT 0,"\
	"times_right		INTEGER DEFAULT 0"\

#define べんきょう_table_version "version" /*TODO(fran): versioning system to be able to move at least forward, eg db is v2 and we are v5; for going backwards, eg db is v4 and we are v2, what we can do is avoid modifying any already existing columns of previous versions when we move to a new version, that way older versions simply dont use the columns/tables of the new ones*/

#define べんきょう_table_version_structure \
	"v					INTEGER"\
//NOTE: lets keep it simple, versions are just a number, also the db wont be changing too often

union user_stats {//TODO(fran): some of this stuff is easier to update via a trigger, eg word count
	i64 word_cnt;			//Count for words added
	i64 times_practiced;	//Count for completed practice runs
	i64 times_shown;		//Count for words shown in practice runs
	i64 times_right;		//Count for correct word guessings in practice runs

	f32 accuracy() { // from 0 to 1
		f32 res = safe_ratio1((f32)times_right, (f32)times_shown);
		return res;
	}
	i64 times_wrong() {
		i64 res = times_shown - times_right;
		return res;
	}
};

//NOTE: Since comboboxes return -1 on no selection lexical_category maps perfectly from UI's combobox index to value
enum lexical_category { //The value of this enums is what will be stored on the db, we'll map them to their correct language string
	dont_care = -1, //I tend to annotate the different adjectives, but not much else
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
	return RS(200 + cat); //NOTE: dont_care should never be shown
}
u32 lexical_category_str_lang_id(lexical_category cat) {
	return 200 + cat; //NOTE: dont_care should never be shown
}
void lexical_category_setup_combobox(HWND cb) {
	//IMPORTANT INFO: the first element to add to a combobox _must_ be at index 0, it does not support adding any index, amazingly enough, conclusion: the windows' combobox is terrible
	//So thanks to the huge limitation of comboboxes we have to subtract one from everything, and at the time of consulting the cb we'll need to add one, this is simply stupid //TODO(fran): find a fix
	ACT(cb, lexical_category::noun, lexical_category_str_lang_id(lexical_category::noun));
	ACT(cb, lexical_category::verb, lexical_category_str_lang_id(lexical_category::verb));
	ACT(cb, lexical_category::adj_い, lexical_category_str_lang_id(lexical_category::adj_い));
	ACT(cb, lexical_category::adj_な, lexical_category_str_lang_id(lexical_category::adj_な));
	ACT(cb, lexical_category::adverb, lexical_category_str_lang_id(lexical_category::adverb));
	ACT(cb, lexical_category::conjunction, lexical_category_str_lang_id(lexical_category::conjunction));
	ACT(cb, lexical_category::pronoun, lexical_category_str_lang_id(lexical_category::pronoun));
	ACT(cb, lexical_category::counter, lexical_category_str_lang_id(lexical_category::counter));
}
union learnt_word { //will contain utf16* when getting data from the UI, and utf8* to send requests to the db
	using type = any_str;
	struct {
#define _foreach_learnt_word_member(op) \
		op(type,hiragana) \
		op(type,kanji) \
		op(type,translation) \
		op(type,mnemonic) \
		op(type,lexical_category) \

		_foreach_learnt_word_member(_generate_member_no_default_init);
		//NOTE: the ones that map to primary keys must be the first on the list, that way they are easily filtered out when updating their values

	} attributes;
	type all[sizeof(attributes) / sizeof(type)];

};
constexpr int learnt_word_pk_count = 1;//

union extra_word {
	using type = any_str;
	struct {
#define _foreach_extra_word_member(op) \
		op(type,creation_date) \
		op(type,last_shown_date) \
		op(type,times_shown) \
		op(type,times_right) \

		//NOTE: times_right is a count that goes up each time the user guessed the word correctly
		//TODO(fran): we may want to separate between times_right and times_shown for hiragana and the translation

		_foreach_extra_word_member(_generate_member_no_default_init);

	} attributes;
	type all[sizeof(attributes) / sizeof(type)];
};
struct stored_word {
	learnt_word user_defined;
	extra_word application_defined;
};

//Structures used by different practices
struct practice_writing_word {
	learnt_word word;
	enum class type {
		translate_hiragana_to_translation,
		translate_translation_to_hiragana,
		translate_kanji_to_hiragana,
		translate_kanji_to_translation,
		//NOTE: I'd add translate_translation_to_kanji but it's basically translating to hiragana and then letting the IME answer correclty
	}practice_type;
};

//---------------------Macros-------------------------:

#define _sqlite3_generate_columns(type,name,...) "" + #name + ","
//NOTE: you'll have to remove the last comma since sql doesnt accept trailing commas

#define _sqlite3_generate_columns_array(type,name,...) #name,

#define _sqlite3_generate_values(type,name,...) "'" + (utf8*)word->attributes.name.str + "'"","
//NOTE: you'll have to remove the last comma since sql doesnt accept trailing commas

#define _sqlite3_generate_values_array(type,name,...) std::string("'") + (utf8*)word->attributes.name.str + "'",


//Data retrieval from UI (UI string contents are always utf16)

/*NOTE: edit control or similar that uses WM_GETTEXT, eg static control, button, ...*/
#define _get_edit_str(edit,any_str) \
			{ \
				int _sz_char = (int)SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0) + 1; \
				any_str = alloc_any_str(_sz_char * sizeof(utf16)); \
				SendMessageW(edit, WM_GETTEXT, _sz_char, (WPARAM)any_str.str); \
			}

#define _get_combo_sel_str(cb,any_str) \
			{ \
				int lex_categ = (int)SendMessageW(cb, CB_GETCURSEL, 0, 0); \
				int sz_char = _snwprintf(nullptr, 0, L"%d", lex_categ) + 1; \
				any_str = alloc_any_str(sz_char * sizeof(utf16)); \
				_snwprintf((utf16*)w.attributes.lexical_category.str, sz_char, L"%d", lex_categ); \
			}


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
		practice_writing,//you're given a word in hiragana/kanji/translation and must write the response in hiragana/kanji/translation //TODO(fran)
		practice_multiplechoice,//TODO(fran)
		search,
		show_word,
	} current_page;

	struct prev_page_fifo_queue{
		decltype(current_page) pages[10];
		u32 cnt;
	}previous_pages;

	u32 practice_cnt;//counter for current completed practices while on a pratice run, use to stop the practice run once it gets to some threshold

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
				type static_word_cnt_title;
				type static_word_cnt;

				type static_practice_cnt_title;
				type static_practice_cnt;

				type static_accuracy_title;
				type score_accuracy;

				type static_accuracy_timeline_title; //TODO(fran): we may want to add this, or smth else
				type graph_accuracy_timeline;

				type button_start;
				//type edit_question;
				//type edit_answer;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice;

		union practice_writing_controls {
			using type = HWND;
			struct {
				type static_test_word;

				type edit_answer;//pressing enter on it will trigger checking

				type button_next;//TODO(fran): not the best name
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice_writing;

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
				type static_hiragana;//or katakana
				type edit_kanji;
				type edit_translation;
				type combo_lexical_category;//verb,noun,...
				type edit_mnemonic;
				//TODO(fran): here you should be able to add more than one translation

				type static_creation_date;
				type static_last_shown_date;
				type static_score; //eg Score: 4/5 - 80%
				//TODO(fran): store on the db: times_shown and times_correct

				type button_modify;
				type button_delete;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} show_word;

	}controls; //TODO(fran): should be called pages

	enum class available_practices {
		writing,
		multiplechoice,
	};

	struct {

		struct practice_writing_state {
			practice_writing_word practice;
		}practice_writing;

	} pagestate;
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

	i64 get_table_rowcount(sqlite3* db, std::string table) {
		i64 res;
		std::string count = "SELECT COUNT(*) FROM " + table;
		sqlite3_stmt* stmt;
		int errcode;
		errcode = sqlite3_prepare_v2(db, count.c_str(), (int)((count.length() + 1) * sizeof(decltype(count)::value_type)), &stmt, nullptr); //INFO: there's a prepare with utf16 that we can use!, everything utf16 would be a blessing
		sqliteok_runtime_assert(errcode, db);

		errcode = sqlite3_step(stmt);
		sqlite_runtime_assert(errcode == SQLITE_ROW, db);

		res = sqlite3_column_int64(stmt, 0);

		sqlite3_finalize(stmt);

		return res;
	}

	user_stats get_user_stats(sqlite3* db) {
		user_stats res;
		std::string stats = " SELECT " " word_cnt, times_practiced, times_shown, times_right " " FROM " べんきょう_table_user;

		sqlite3_stmt* stmt;
		int errcode;
		errcode = sqlite3_prepare_v2(db, stats.c_str(), (int)((stats.length() + 1) * sizeof(decltype(stats)::value_type)), &stmt, nullptr); //INFO: there's a prepare with utf16 that we can use!, everything utf16 would be a blessing
		sqliteok_runtime_assert(errcode, db);

		errcode = sqlite3_step(stmt);
		sqlite_runtime_assert(errcode == SQLITE_ROW, db);

		res.word_cnt = sqlite3_column_int64(stmt, 0);
		res.times_practiced = sqlite3_column_int64(stmt, 1);
		res.times_shown = sqlite3_column_int64(stmt, 2);
		res.times_right = sqlite3_column_int64(stmt, 3);

		sqlite3_finalize(stmt);

		return res;
	}

	void startup(sqlite3* db) {
		using namespace std::string_literals;
		std::string create_word_table = "CREATE TABLE IF NOT EXISTS "s + べんきょう_table_words + "("s + べんきょう_table_words_structure+ ") WITHOUT ROWID;"s; //INFO: the param that requests this expects utf8
		char* create_errmsg;
		sqlite3_exec(db, create_word_table.c_str(), 0, 0, &create_errmsg);
		sqlite_exec_runtime_assert(create_errmsg);
		//TODO(fran): we should also implement update table for future versions that might need new columns, the other idea is to make separate tables join by foreign key, I dont know if it has any bennefit, probably not since the queries will become bigger and slower cause of the joins, what I do have to be careful is that downgrading doesnt destroy anything

		std::string create_user_table = "CREATE TABLE IF NOT EXISTS " べんきょう_table_user "(" べんきょう_table_user_structure ");"; //INFO: the param that requests this expects utf8
		sqlite3_exec(db, create_user_table.c_str(), 0, 0, &create_errmsg);
		sqlite_exec_runtime_assert(create_errmsg);

		if (get_table_rowcount(db, べんきょう_table_user) > 0) {
			//The entry is already there, here we can set new values on future updates for example
		}
		else {
			//Entry isnt there, create it
			std::string insert_user = " INSERT INTO "s + べんきょう_table_user + " DEFAULT VALUES "s;
			sqlite3_exec(db, insert_user.c_str(), 0, 0, &create_errmsg);
			sqlite_exec_runtime_assert(create_errmsg);

		}
	}

	void add_controls(ProcState* state) {
		//---------------------Landing page----------------------:
		{
			auto& controls = state->controls.landingpage;

			controls.list.button_new = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_new, 100);
			//TODO(fran): more brushes, fore_push,... , border_mouseover,...
			UNCAPBTN_set_brushes(controls.list.button_new, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

			controls.list.button_practice = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_practice, 101);
			//TODO(fran): more brushes, fore_push,... , border_mouseover,...
			UNCAPBTN_set_brushes(controls.list.button_practice, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

			controls.list.button_search = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_search, 102);
			//TODO(fran): more brushes, fore_push,... , border_mouseover,...
			UNCAPBTN_set_brushes(controls.list.button_search, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
		}
		//---------------------New word----------------------:
		{
			auto& controls = state->controls.new_word;

			controls.list.edit_hiragana = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_hiragana, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_hiragana, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(120));//TODO(fran): lang mgr

			controls.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.list.combo_lexical_category);
			//SendMessage(controls.list.combo_lexical_category, CB_SETCURSEL, 0, 0);
			SetWindowSubclass(controls.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)unCap_bmps.dropdown, 0);
			ACC(controls.list.combo_lexical_category, 123);

			controls.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_kanji, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_kanji, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(121));

			controls.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_translation, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_translation, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(122));

			controls.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_mnemonic, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_mnemonic, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(125));

			controls.list.button_save = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_save, 124);
			UNCAPBTN_set_brushes(controls.list.button_save, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
		}
		//TODO(fran): create the other "tabs"

		//---------------------Search----------------------:
		{
			auto& controls = state->controls.search;

			controls.list.combo_search = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP /*TODO(fran):is CBS_HASSTRINGS necessary?*/
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			ACC(controls.list.combo_search, 250); //TODO(fran): allow from multiple searching, eg if kanji detected search by kanji, if neither hiragana nor kanji search translation
			SetWindowSubclass(controls.list.combo_search, TESTComboProc, 0, 0);
#if 0
			COMBOBOXINFO info = { sizeof(info) };
			GetComboBoxInfo(controls.list.combo_search, &info);
			//SetWindowSubclass(info.hwndItem, PrintMsgProc, 0, (DWORD_PTR)"Combo Edit");
			SetWindowSubclass(info.hwndList, PrintMsgProc, 0, (DWORD_PTR)"Combo List");
#endif
			//cstr listclass[100];
			//GetClassNameW(info.hwndList, listclass, 100); //INFO: class is ComboLBox, one interesting class windows has an it is hidden

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
		}
		//---------------------Show word----------------------:
		{
			auto& controls = state->controls.show_word;

			controls.list.static_hiragana = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);

			controls.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_kanji, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_kanji, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(121));

			controls.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_translation, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_translation, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(122));

			controls.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.list.combo_lexical_category);
			//SendMessage(controls.list.combo_lexical_category, CB_SETCURSEL, 0, 0);
			SetWindowSubclass(controls.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)unCap_bmps.dropdown, 0);
			ACC(controls.list.combo_lexical_category, 123);

			controls.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_mnemonic, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			SendMessage(controls.list.edit_mnemonic, WM_SETDEFAULTTEXT, 0, (LPARAM)RCS(125));

			controls.list.static_creation_date = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);

			controls.list.static_last_shown_date = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);

			controls.list.static_score = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);

			controls.list.button_modify = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_modify, 273);
			UNCAPBTN_set_brushes(controls.list.button_modify, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

			controls.list.button_delete = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP | BS_BITMAP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_modify, 273);
			UNCAPBTN_set_brushes(controls.list.button_delete, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
			SendMessage(controls.list.button_delete, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)unCap_bmps.bin);
			//TODO(fran): use an img of a trash can for this one, so it's smaller and square and we can put it to the side of the modify button which is the important one and should remain centered

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
		}

		//---------------------Practice----------------------:
		{
			auto& controls = state->controls.practice;

			controls.list.static_word_cnt_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_word_cnt_title, 351);

			controls.list.static_word_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_word_cnt, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, 0, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, 0);
			
			controls.list.static_practice_cnt_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_practice_cnt_title, 352);

			controls.list.static_practice_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_practice_cnt, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, 0, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, 0);//TODO(fran): add border colors
			
			controls.list.static_accuracy_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_accuracy_title, 353);

			controls.list.score_accuracy = CreateWindowW(score::wndclass, NULL, WS_CHILD 
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			score::set_brushes(controls.list.score_accuracy, FALSE, unCap_colors.ControlBk, unCap_colors.Score_RingBk, unCap_colors.Score_RingFull, unCap_colors.Score_RingEmpty, unCap_colors.Score_InnerCircle);

			controls.list.button_start = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_start, 350);
			UNCAPBTN_set_brushes(controls.list.button_start, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
			
			controls.list.static_accuracy_timeline_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_accuracy_timeline_title, 354);

			controls.list.graph_accuracy_timeline = CreateWindowW(graph::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			graph::set_brushes(controls.list.graph_accuracy_timeline, FALSE, unCap_colors.Graph_Line, unCap_colors.Graph_BkUnderLine, unCap_colors.Graph_Bk, unCap_colors.Graph_Border);

			//TODO(fran): we may want to add smth else like a total number of words practiced


			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
		}

		//---------------------Practice writing----------------------:
		{
			auto& controls = state->controls.practice_writing;

			controls.list.static_test_word = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_test_word, TRUE, 0, unCap_colors.ControlBk, 0, 0, unCap_colors.ControlBk_Disabled, 0);
			//NOTE: text color will be set according to the type of word being shown

			controls.list.edit_answer = CreateWindowW(edit_oneline::wndclass, 0, WS_CHILD | ES_CENTER | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			EDITONELINE_set_brushes(controls.list.edit_answer, TRUE, 0, unCap_colors.ControlBk, unCap_colors.Img, 0, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
			//NOTE: text color and default text will be set according to the type of word that has to be written

			controls.list.button_next = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			UNCAPBTN_set_brushes(controls.list.button_next, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
			SendMessage(controls.list.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)unCap_bmps.arrowSimple_right);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);
		}
	}

	void resize_controls(ProcState* state) {
		//TODO(fran): should I resize everyone or just the ones from the current page, I feel like just resizing the necessary ones is better and when there's a page change we call resize there just in case
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTWIDTH(r);
		int h = RECTHEIGHT(r);
		int w_pad = (int)((float)w * .05f);
		int h_pad = (int)((float)h * .05f);
		
#define _MyMoveWindow(wnd,xywh,repaint) MoveWindow(wnd, xywh##_x, xywh##_y, xywh##_w, xywh##_h, repaint)

#define _MyMoveWindow2(wnd,xywh,repaint) MoveWindow(wnd, xywh.x, xywh.y + y_offset, xywh.w, xywh.h, repaint)

		switch (state->current_page) {
		case ProcState::page::landing: 
		{
			auto& controls = state->controls.landingpage;
			//One button on top of the other vertically, all buttons must be squares
			//TODO(fran): we could, at least to try how it looks, to check whether the wnd is longer on w or h and place the controls vertically or horizontally next to each other
			int wnd_cnt = ARRAYSIZE(controls.all);
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

			_MyMoveWindow(controls.list.button_new, button_new,FALSE);
			_MyMoveWindow(controls.list.button_practice, button_practice,FALSE);
			_MyMoveWindow(controls.list.button_search, button_search,FALSE);
		} break;
		case ProcState::page::new_word:
		{
			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned
			auto& controls = state->controls.new_word;

			int wnd_cnt = ARRAYSIZE(controls.all);
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

			_MyMoveWindow(controls.list.edit_hiragana, edit_hiragana, FALSE);
			_MyMoveWindow(controls.list.combo_lexical_category, cb_lex_categ, FALSE);
			_MyMoveWindow(controls.list.edit_kanji, edit_kanji, FALSE);
			_MyMoveWindow(controls.list.edit_translation, edit_translation, FALSE);
			_MyMoveWindow(controls.list.edit_mnemonic, edit_mnemonic, FALSE);
			_MyMoveWindow(controls.list.button_save, btn_save, FALSE);

		} break;
		case ProcState::page::practice: 
		{
			auto& controls = state->controls.practice;

			//Stats first, then start button
			//For stats I imagine a 2x2 "grid":	first row has "word count" and "times practiced"
			//									second row has "score" and a chart of accuracy for last 30 days for example


			int wnd_cnt = ARRAYSIZE(controls.all);
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
#if 0
			int start_y = (h - (pad_cnt) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): not good, what we want is the total height of all windows combined, an use that to center correctly, eg start_y = (h-total_h)/2; and we offset everything by start_y at the end
#else
			int start_y = 0;//We start from 0 and offset once we know the sizes and positions for everything
#endif

			int grid_h = wnd_h * 4;
			int grid_w = grid_h * 16 / 9;
			int grid_start_y = start_y;
			rect_i32 grid[2][2];
			for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) { grid[i][j].w = grid_w; grid[i][j].h = grid_h; }
			int grid_w_pad = w_pad / 2;
			int grid_h_pad = h_pad / 2;
			bool two_by_two = (grid_w * 2 + grid_w_pad) <= max_w;

			if (two_by_two) {
				grid[0][0].left = w/2 - grid_w_pad/2 - grid[0][0].w;	grid[0][0].top = grid_start_y;
				grid[0][1].left = w/2 + grid_w_pad/2;					grid[0][1].top = grid[0][0].top;

				grid[1][0].left = grid[0][0].left;						grid[1][0].top = grid[0][0].bottom() + grid_h_pad;
				grid[1][1].left = grid[0][1].left;						grid[1][1].top = grid[1][0].top;
			}
			else {
				for (int next_y = grid_start_y, i = 0; i < 2; i++) 
					for (int j = 0; j < 2; j++) { 
						grid[i][j].left = (w - grid[i][j].w) / 2; 
						grid[i][j].top = next_y; 
						next_y = grid[i][j].bottom() + grid_h_pad; 
					}
			}
			//NOTE: we dont handle the case when there's not enough height since it'll look too long if all the things are next to each other and you probably wont have the space anyway

			rect_i32 cell;

			//First cell
			cell = grid[0][0];
			rect_i32 static_word_cnt_title;
			static_word_cnt_title.w = cell.w;
			static_word_cnt_title.h = min(wnd_h,cell.h);
			static_word_cnt_title.x = cell.center_x() - static_word_cnt_title.w/2;
			static_word_cnt_title.y = cell.top;

			//NOTE: the values should use a much bigger font
			rect_i32 static_word_cnt;
			static_word_cnt.w = cell.w;
			static_word_cnt.x = cell.center_x() - static_word_cnt.w/2;
			static_word_cnt.h = distance(cell.bottom(), static_word_cnt_title.bottom());
			static_word_cnt.y = cell.bottom() - static_word_cnt.h;

			//Second cell
			cell = grid[0][1];
			rect_i32 static_practice_cnt_title;
			static_practice_cnt_title.w = cell.w;
			static_practice_cnt_title.h = min(wnd_h, cell.h);
			static_practice_cnt_title.x = cell.center_x() - static_practice_cnt_title.w / 2;
			static_practice_cnt_title.y = cell.top;

			rect_i32 static_practice_cnt;
			static_practice_cnt.w = cell.w;
			static_practice_cnt.x = cell.center_x() - static_practice_cnt.w / 2;
			static_practice_cnt.h = distance(cell.bottom(), static_practice_cnt_title.bottom());
			static_practice_cnt.y = cell.bottom() - static_practice_cnt.h;

			//3rd cell
			cell = grid[1][0];
			rect_i32 static_accuracy_title;
			static_accuracy_title.w = cell.w;
			static_accuracy_title.h = min(wnd_h, cell.h);
			static_accuracy_title.x = cell.center_x() - static_accuracy_title.w / 2;
			static_accuracy_title.y = cell.top;

			rect_i32 score_accuracy;
			score_accuracy.h = min(cell.w, distance(cell.bottom(), static_accuracy_title.bottom()));
			score_accuracy.w = score_accuracy.h;
			score_accuracy.x = cell.center_x() - score_accuracy.w / 2;
			score_accuracy.y = cell.bottom() - score_accuracy.h;

			//4th cell
			cell = grid[1][1];
			rect_i32 static_accuracy_timeline_title;
			static_accuracy_timeline_title.w = cell.w;
			static_accuracy_timeline_title.h = min(wnd_h, cell.h);
			static_accuracy_timeline_title.x = cell.center_x() - static_accuracy_timeline_title.w / 2;
			static_accuracy_timeline_title.y = cell.top;

			rect_i32 graph_accuracy_timeline;
			graph_accuracy_timeline.h = min(cell.w, distance(cell.bottom(), static_accuracy_timeline_title.bottom()));
			graph_accuracy_timeline.w = min(graph_accuracy_timeline.h * 16 / 9,cell.w);
			graph_accuracy_timeline.x = cell.center_x() - graph_accuracy_timeline.w / 2;
			graph_accuracy_timeline.y = cell.bottom() - graph_accuracy_timeline.h;


			rect_i32 last_stat = graph_accuracy_timeline;

			rect_i32 button_start;
			button_start.y = last_stat.bottom() + h_pad;
			button_start.w = 70;
			button_start.h = wnd_h;
			button_start.x = (w - button_start.w)/2;

			rect_i32 bottom_most_control = button_start;

			int used_h = bottom_most_control.bottom();// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			
			_MyMoveWindow2(controls.list.static_word_cnt_title, static_word_cnt_title, FALSE);
			_MyMoveWindow2(controls.list.static_word_cnt, static_word_cnt, FALSE);
			_MyMoveWindow2(controls.list.static_practice_cnt_title, static_practice_cnt_title, FALSE);
			_MyMoveWindow2(controls.list.static_practice_cnt, static_practice_cnt, FALSE);
			_MyMoveWindow2(controls.list.static_accuracy_title, static_accuracy_title, FALSE);
			_MyMoveWindow2(controls.list.score_accuracy, score_accuracy, FALSE);
			_MyMoveWindow2(controls.list.static_accuracy_timeline_title, static_accuracy_timeline_title, FALSE);
			_MyMoveWindow2(controls.list.graph_accuracy_timeline, graph_accuracy_timeline, FALSE);
			_MyMoveWindow2(controls.list.button_start, button_start, FALSE);

		} break;
		case ProcState::page::practice_writing:
		{
			auto& controls = state->controls.practice_writing;

			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int start_y = 0;
			int bigwnd_h = wnd_h * 4;

			rect_i32 static_test_word;
			static_test_word.y = start_y;
			static_test_word.h = bigwnd_h;
			static_test_word.w = w;
			static_test_word.x = (w - static_test_word.w)/2;

			rect_i32 edit_answer;
			edit_answer.y = static_test_word.bottom() + h_pad;
			edit_answer.h = wnd_h;
			edit_answer.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_answer, WM_GETFONT, 0, 0), 20).cx);
			edit_answer.x = (w - edit_answer.w) / 2;

			rect_i32 button_next;
			button_next.h = edit_answer.h;
			button_next.w = button_next.h;
			button_next.y = edit_answer.y;
			button_next.x = edit_answer.right() - button_next.w;

			rect_i32 bottom_most_control = button_next;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			_MyMoveWindow2(controls.list.static_test_word, static_test_word, FALSE);
			_MyMoveWindow2(controls.list.edit_answer, edit_answer, FALSE);
			_MyMoveWindow2(controls.list.button_next, button_next, FALSE);

		} break;
		case ProcState::page::search: 
		{
			auto& controls = state->controls.search;

			int max_w = w - w_pad * 2;
			int cb_search_x = w_pad;
			int cb_search_y = h_pad;
			int cb_search_h = 40;//TODO(fran): cbs dont respect this at all, they set a minimum h by the font size I think
			int cb_search_w = max_w;

			_MyMoveWindow(controls.list.combo_search, cb_search, FALSE);

		} break;
		case ProcState::page::show_word: 
		{
			auto& controls = state->controls.show_word;

			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned, non user editable controls go below the rest

			int wnd_cnt = ARRAYSIZE(controls.all) - 2;//subtract the controls that go next to one another
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int start_y = (h - (pad_cnt - 1 /*we only have 1 real pad for lex_categ*/) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): centering aint quite right

			int static_hiragana_x = w_pad;
			int static_hiragana_y = start_y;
			int static_hiragana_h = wnd_h;
			int static_hiragana_w = max_w; //TODO(fran): we may want to establish a max_w that's more fixed, as a clamp, instead of continually increasing as the wnd width does

			int cb_lex_categ_w = max_w / 3;
			int cb_lex_categ_x = w - w_pad - cb_lex_categ_w;
			int cb_lex_categ_y = static_hiragana_y + static_hiragana_h + h_pad / 2;
			int cb_lex_categ_h = static_hiragana_h;//TODO(fran): for some reason comboboxes are always a little smaller than you ask, find out how to correctly correct that


			int edit_kanji_x = static_hiragana_x;
			int edit_kanji_y = cb_lex_categ_y + cb_lex_categ_h + h_pad / 2;
			int edit_kanji_w = max_w;
			int edit_kanji_h = wnd_h;

			int edit_translation_x = static_hiragana_x;
			int edit_translation_y = edit_kanji_y + edit_kanji_h + h_pad;
			int edit_translation_w = max_w;
			int edit_translation_h = wnd_h;

			int edit_mnemonic_x = static_hiragana_x;
			int edit_mnemonic_y = edit_translation_y + edit_translation_h + h_pad;
			int edit_mnemonic_w = max_w;
			int edit_mnemonic_h = wnd_h;

			int last_user_x = edit_mnemonic_x;
			int last_user_y = edit_mnemonic_y;
			int last_user_w = edit_mnemonic_w;
			int last_user_h = edit_mnemonic_h;

			int dual_w = (max_w-w_pad)/2;

			int static_creation_date_x = w_pad;
			int static_creation_date_y = last_user_y + last_user_h + h_pad;
			int static_creation_date_w = dual_w;
			int static_creation_date_h = wnd_h;

			int static_last_shown_date_x = static_creation_date_x + static_creation_date_w + w_pad;
			int static_last_shown_date_y = static_creation_date_y;
			int static_last_shown_date_w = dual_w;
			int static_last_shown_date_h = wnd_h;

			int static_score_x = w_pad;
			int static_score_y = static_last_shown_date_y + static_last_shown_date_h + h_pad;
			int static_score_w = max_w;
			int static_score_h = wnd_h;

			int btn_modify_w = 70;
			int btn_modify_h = wnd_h;
			int btn_modify_y = static_score_y + static_score_h + h_pad;
			int btn_modify_x = (w - btn_modify_w) / 2;

			int btn_delete_h = wnd_h;
			int btn_delete_w = btn_delete_h;
#if 0 //TODO(fran): which one to go with?
			int btn_delete_x = btn_modify_x + btn_modify_w + w_pad*;/*TODO(fran): clamp to not go beyond w*/
			int btn_delete_y = btn_modify_y;
#else
			int btn_delete_x = (w - btn_delete_w)/2;
			int btn_delete_y = btn_modify_y + btn_modify_h + h_pad;
#endif

			_MyMoveWindow(controls.list.static_hiragana, static_hiragana, FALSE);
			_MyMoveWindow(controls.list.combo_lexical_category, cb_lex_categ, FALSE);
			_MyMoveWindow(controls.list.edit_kanji, edit_kanji, FALSE);
			_MyMoveWindow(controls.list.edit_translation, edit_translation, FALSE);
			_MyMoveWindow(controls.list.edit_mnemonic, edit_mnemonic, FALSE);

			//Dates one next to the other, the same for times_... related objs
			_MyMoveWindow(controls.list.static_creation_date, static_creation_date, FALSE);
			_MyMoveWindow(controls.list.static_last_shown_date, static_last_shown_date, FALSE);
			_MyMoveWindow(controls.list.static_score, static_score, FALSE);
			_MyMoveWindow(controls.list.button_modify, btn_modify, FALSE);
			_MyMoveWindow(controls.list.button_delete, btn_delete, FALSE);

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
		default:Assert(0);
		}
	}

	void set_current_page(ProcState* state, ProcState::page new_page) {
		show_page(state, state->current_page,SW_HIDE);
		state->current_page = new_page;
		resize_controls(state);
		show_page(state, state->current_page,SW_SHOW);
	}

	void goto_previous_page(ProcState* state) {
		if (state->previous_pages.cnt>0) {
			set_current_page(state, state->previous_pages.pages[--state->previous_pages.cnt]);
			if(state->previous_pages.cnt==0)PostMessage(state->nc_parent, WM_SHOWBACKBTN, FALSE, 0);//hide the back button
		}
	}

	//TODO(fran): maybe set_current_page should store the page it's replacing into the queue, problem there would be with goto_previous_page, which will cause a store that we dont want, but maybe some better defined functions with the goto_previous distinction in mind could work well
	void store_previous_page(ProcState* state, ProcState::page prev_page) {
		if (state->previous_pages.cnt == ARRAYSIZE(state->previous_pages.pages)) {
			//cnt stays the same
			//we move all the entries one position down and place the new one on top
			//decltype(state->previous_pages.pages) temp;//IMPORTANT INFO: you can create arrays in ways similar to this without the need to put [...] _after_ the name
			memcpy(state->previous_pages.pages, &state->previous_pages.pages[1], (ARRAYSIZE(state->previous_pages.pages) - 1) * sizeof(*state->previous_pages.pages));
			state->previous_pages.pages[ARRAYSIZE(state->previous_pages.pages) - 1] = prev_page;
		}
		else {
			state->previous_pages.pages[state->previous_pages.cnt++] = prev_page;
		}

		PostMessage(state->nc_parent, WM_SHOWBACKBTN, TRUE, 0);//show the back button
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

	bool check_show_word(ProcState* state) {
		auto& page = state->controls.show_word;
		HWND edit_required[] = { page.list.edit_translation };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				EDITONELINE_show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, ETP::top);
				return false;
			}
		}
		return true;
	}

	bool insert_word(sqlite3* db, const learnt_word* word) {
		bool res;
		//TODO(fran): specify all the columns for the insert, that will be our error barrier
		//TODO(fran): we are inserting everything with '' which is not right for numbers
		//NOTE: here I have an idea, if I store the desired type I can do type==number? string : 'string'

		std::string columns = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_columns);
		columns.pop_back(); //We need to remove the trailing comma

		std::string values = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_values); 
		values.pop_back(); //We need to remove the trailing comma

		std::string insert_word = std::string(" INSERT INTO ") + べんきょう_table_words + "(" + columns + ")" + " VALUES(" + values + ");";

		char* insert_errmsg;
		res = sqlite3_exec(db, insert_word.c_str(), 0, 0, &insert_errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(insert_errmsg);

		//TODO(fran): handle if the word already exists, maybe show the old word and ask if they want to override that content, NOTE: the handling code shouldnt be here, this function should be as isolated as possible, if we start heavily interacting with the user this will be ugly

		return res;
	}

	bool save_new_word(ProcState* state) {
		bool res = false;
		if (check_new_word(state)) {
			learnt_word w;
			auto& page = state->controls.new_word;

			_get_edit_str(page.list.edit_hiragana, w.attributes.hiragana);
			_get_edit_str(page.list.edit_kanji, w.attributes.kanji);
			_get_edit_str(page.list.edit_translation, w.attributes.translation);
			_get_edit_str(page.list.edit_mnemonic, w.attributes.mnemonic);

			_get_combo_sel_str(page.list.combo_lexical_category, w.attributes.lexical_category);

			learnt_word w_utf8;
			for (int i = 0; i < ARRAYSIZE(w.all); i++) {
				auto res = convert_utf16_to_utf8((utf16*)w.all[i].str, (int)w.all[i].sz);
				w_utf8.all[i] = res;
				free_any_str(w.all[i].str);//maybe set it to zero too
			}
			defer{ for (auto& _ : w_utf8.all)free_any_str(_.str); };
			//Now we can finally do the insert, TODO(fran): see if there's some way to go straight from utf16 to the db, and to send things like ints without having to convert them to strings
			res = insert_word(state->settings->db, &w_utf8); 
			//TODO(fran): maybe handle repeated words here
		}
		return res;
	}
	struct search_word_res { utf16** matches; int cnt; };
	void free_search_word(search_word_res res) {
		for (int i = 0; i < res.cnt; i++) {
			free_any_str(res.matches[i]);
		}
		free(res.matches);
	}
	search_word_res search_word_matches(sqlite3* db, utf16* match/*bytes*/, int max_cnt_results/*eg. I just want the top 5 matches*/) {
		//NOTE/TODO(fran): for now we'll simply retrieve the hiragana, it might be good to get the translation too, so the user can quick search, and if they really want to know everything about that word then the can click it and pass to the next stage
		search_word_res res; 
		res.matches = (decltype(res.matches))malloc(max_cnt_results * sizeof(*res.matches));
		res.cnt = 0;
		auto match_str = convert_utf16_to_utf8(match, (int)(cstr_len(match)+1)*sizeof(*match)); defer{ free_any_str(match_str.str); };
		char* select_errmsg;
		std::string select_matches = std::string(" SELECT ") + "hiragana" /*TODO(fran): column names should be stored somewhere*/ + " FROM " + べんきょう_table_words + " WHERE " + "hiragana" " LIKE '" + (utf8*)match_str.str + "%'" + " LIMIT " + std::to_string(max_cnt_results) + ";";

		auto parse_match_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			//NOTE: from what I understood this gets executed once for every resulting row
			//NOTE: unfortunately everything here comes in utf8 //TODO(fran): may be better to use the separate functions sqlite3_prepare, sqlite3_step, and to retrieve use sqlite3_column_[type] (this way I get utf16 at once)
			Assert(column_cnt == 1);
			search_word_res* res = (decltype(res))extra_param;

			//res->matches[res->cnt] = (decltype(*res->matches))convert_utf8_to_utf16(results[0], (int)strlen(results[0]) + 1).mem;//INFO: interesting, I know this is correct but compiler cant allow it, an explanation here https://stackoverflow.com/questions/3674456/why-this-is-causing-c2102-requires-l-value

			auto r = convert_utf8_to_utf16(results[0], (int)strlen(results[0]) + 1);
			res->matches[res->cnt] = (decltype(*res->matches))r.str;

			res->cnt++;
			return 0;//if non-zero then the query stops and exec returns SQLITE_ABORT
		};

		sqlite3_exec(db, select_matches.c_str(), parse_match_result, &res, &select_errmsg);
		sqlite_exec_runtime_check(select_errmsg);//TODO(fran): should I free the result and return an empty?
		return res;
	}
	struct get_word_res {
		stored_word word;
		bool found;
	};
	void free_get_word(get_word_res res) {
		//TODO(fran): I dont think I need the if, I should simply always allocate (and I think I already do)
		for(auto s : res.word.application_defined.all) if(s.str) free_any_str(s.str);
		for(auto s : res.word.user_defined.all) if (s.str) free_any_str(s.str);
	}
	get_word_res get_word(sqlite3* db, utf16* word_hiragana) {
		get_word_res res{0};
		auto match_str = convert_utf16_to_utf8(word_hiragana, (int)(cstr_len(word_hiragana) + 1) * sizeof(*word_hiragana)); defer{ free_any_str(match_str.str); };
		std::string select_word = std::string("SELECT ")
			+ _foreach_learnt_word_member(_sqlite3_generate_columns)
			+ _foreach_extra_word_member(_sqlite3_generate_columns); 
		select_word.pop_back();//remove trailing comma
		select_word += std::string(" FROM ") + べんきょう_table_words + " WHERE " + "hiragana" " LIKE '" + (utf8*)match_str.str + "'";
		//NOTE: here I'd like to check last_shown_date and do an if last_shown_date == 0 -> 'Never' else last_shown_data, but I cant cause of the macros, it's pretty hard to do operations on specific columns if you have to autogen it with macros

		auto parse_select_word_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			get_word_res* res = (decltype(res))extra_param;
			res->found = true;
			//here we should already know the order of the columns since they should all be specified in their correct order in the select
			//NOTE: we should always get only one result since we search by hiragana

			//TODO(fran): the conversion im doing here from "convert_res" to "s" should get standardized in one type eg "any_text" or "any_str" where struct any_str{void* mem,size_t sz;/*bytes*/}
#define load_learnt_word_member(type,name,...) if(results) res->word.user_defined.attributes.name = convert_utf8_to_utf16(*results, (int)strlen(*results) + 1); results++;
#define load_extra_word_member(type,name,...) if(results) res->word.application_defined.attributes.name = convert_utf8_to_utf16(*results, (int)strlen(*results) + 1); results++;
			_foreach_learnt_word_member(load_learnt_word_member);
			_foreach_extra_word_member(load_extra_word_member);
			
			try {//Format last_shown_date (this would be easier to do in the query but I'd have to remove the macros)
				i64 unixtime = std::stoll((utf16*)res->word.application_defined.attributes.last_shown_date.str, nullptr, 10);
				free_any_str(res->word.application_defined.attributes.last_shown_date.str);
				if (unixtime == 0) {
					//this word has never been shown in a practice run
					str never = RS(274);
					int sz_bytes = (int)(never.length() + 1) * sizeof(str::value_type);
					res->word.application_defined.attributes.last_shown_date = alloc_any_str(sz_bytes);
					memcpy(res->word.application_defined.attributes.last_shown_date.str, never.c_str(), sz_bytes);
				}
				else {
					std::time_t temp = unixtime;
					std::tm* time = std::localtime(&temp);//IMPORTANT: not multithreading safe, returns a pointer to an internal unique object
					res->word.application_defined.attributes.last_shown_date = alloc_any_str(30 * sizeof(utf16));
					wcsftime((utf16*)res->word.application_defined.attributes.last_shown_date.str, 30, L"%Y-%m-%d", time);
				}
			}
			catch (...) {}

			//TODO(fran): once again we want to change something for the UI, the created_date should only show "y m d", that may mean that we want macros for the user defined stuff, but not for application defined, there are 2 out of 4 things we want to change there

			return 0;
		};
		//TODO(fran):in the sql query convert the datetime values to the user's localtime

		char* select_errmsg;
		sqlite3_exec(db, select_word.c_str(), parse_select_word_result, &res, &select_errmsg);
		sqlite_exec_runtime_check(select_errmsg);
		return std::move(res);
	}

	//Sets the items in the corresponding page to the values on *data
	void preload_page(ProcState* state, ProcState::page page, void* data) {
		//TODO(fran): we probably want to clear the whole page before we start adding stuff
		switch (page) {
		case ProcState::page::new_word:
		{
			learnt_word* new_word = (decltype(new_word))data;//NOTE: since we are in UI we expect utf16 strings
			auto controls = state->controls.new_word;
			//NOTE: any of the values in new_word can be invalid, we gotta check before using
			//TODO(fran): if the controls only had name and no identifier eg "edit" for "edit_hiragana" we could directly map everything with a foreach by having the same name in the word and controls structs
			//NOTE: settext already has null checking
			SendMessageW(controls.list.edit_hiragana, WM_SETTEXT, 0, (LPARAM)new_word->attributes.hiragana.str);
			SendMessageW(controls.list.edit_kanji, WM_SETTEXT, 0, (LPARAM)new_word->attributes.kanji.str);
			SendMessageW(controls.list.edit_translation, WM_SETTEXT, 0, (LPARAM)new_word->attributes.translation.str);
			SendMessageW(controls.list.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)new_word->attributes.mnemonic.str);
			int lex_categ_sel;
			if (new_word->attributes.lexical_category.str) {
				try { lex_categ_sel = std::stoi((utf16*)new_word->attributes.lexical_category.str); }
				catch (...) { lex_categ_sel = -1; }
			} else lex_categ_sel = -1;
			SendMessageW(controls.list.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

		} break;
		case ProcState::page::show_word:
		{
			stored_word* word_to_show = (decltype(word_to_show))data;
			auto controls = state->controls.show_word;
			//IDEA: in this page we could reuse the controls from new_word, that way we first call preload_page(new_word) with word_to_show.user_defined and then do our thing (this idea doesnt quite work)

			SendMessageW(controls.list.static_hiragana, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.hiragana.str);
			SendMessageW(controls.list.edit_kanji, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.kanji.str);
			SendMessageW(controls.list.edit_translation, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.translation.str);
			SendMessageW(controls.list.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.mnemonic.str);

			int lex_categ_sel;
			if (word_to_show->user_defined.attributes.lexical_category.str) {
				try { lex_categ_sel = std::stoi((utf16*)word_to_show->user_defined.attributes.lexical_category.str); }
				catch (...) { lex_categ_sel = -1; }
			}
			else lex_categ_sel = -1;
			SendMessageW(controls.list.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

			if(word_to_show->application_defined.attributes.creation_date.str)
				SendMessageW(controls.list.static_creation_date, WM_SETTEXT, 0, (LPARAM)(RS(270) + L" " + (utf16*)word_to_show->application_defined.attributes.creation_date.str).c_str());

			if (word_to_show->application_defined.attributes.last_shown_date.str)
				SendMessageW(controls.list.static_last_shown_date, WM_SETTEXT, 0, (LPARAM)(RS(271) + L" " + (utf16*)word_to_show->application_defined.attributes.last_shown_date.str).c_str());

			if (word_to_show->application_defined.attributes.times_right.str && word_to_show->application_defined.attributes.times_shown.str) {
				std::wstring score = (RS(272) + L" " + (utf16*)word_to_show->application_defined.attributes.times_right.str + L" / " + (utf16*)word_to_show->application_defined.attributes.times_shown.str);
				try {
					float numerator = std::stof((utf16*)word_to_show->application_defined.attributes.times_right.str);
					float denominator = std::stof((utf16*)word_to_show->application_defined.attributes.times_shown.str);
					if (denominator != 0.f) {
						int percentage = (int)((numerator/denominator) * 100.f);
						score += L" - " + std::to_wstring(percentage) + L"%";
					}
				}
				catch (...) {}
				SendMessageW(controls.list.static_score, WM_SETTEXT, 0, (LPARAM)score.c_str());
			}

		} break;
		case ProcState::page::practice:
		{
			user_stats* stats = (decltype(stats))data;
			auto controls = state->controls.practice;
#define PRACTICE_TEST_STATS
#ifndef PRACTICE_TEST_STATS
			SendMessage(controls.list.score_accuracy, SC_SETSCORE, f32_to_WPARAM(stats->accuracy()), 0);
			SendMessage(controls.list.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->word_cnt).c_str());
			SendMessage(controls.list.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->times_practiced).c_str());
			//TODO(fran): timeline, we'll probably need to store that as blob or text in the db, this is were mongodb would be nice, just throw a js obj for each timepoint
#else
			SendMessage(controls.list.score_accuracy, SC_SETSCORE, f32_to_WPARAM(.6f), 0);
			SendMessage(controls.list.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(1452).c_str());
			SendMessage(controls.list.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(559).c_str());
			i32 accu[]{ 77,56,32,12,48,95,65,32,54,67,79,88,100 };
			graph::set_points(controls.list.graph_accuracy_timeline, accu, ARRAYSIZE(accu));
			graph::set_top_point(controls.list.graph_accuracy_timeline, 100);
			graph::set_bottom_point(controls.list.graph_accuracy_timeline, 0);
			graph::set_viewable_points_range(controls.list.graph_accuracy_timeline, 0, ARRAYSIZE(accu));
#endif
			//BOOL can_practice = stats->word_cnt > 0;
			//EnableWindow(controls.list.button_start, can_practice);
		} break;
		}
	}

	bool update_word(sqlite3* db, learnt_word* word) {
		bool res;
		//TODO(fran): specify all the columns for the insert, that will be our error barrier
		//TODO(fran): we are inserting everything with '' which is not right for numbers
		//NOTE: here I have an idea, if I store the desired type I can do type==number? string : 'string'

		std::string columns[] = { _foreach_learnt_word_member(_sqlite3_generate_columns_array) };

		std::string values[] = { _foreach_learnt_word_member(_sqlite3_generate_values_array) };

		std::string update_word = std::string(" UPDATE ") + べんきょう_table_words + " SET ";
		for (int i = learnt_word_pk_count/*dont update pk columns*/; i < ARRAYSIZE(columns); i++)
			update_word += columns[i] + "=" + values[i] + ",";
		update_word.pop_back();//you probably need to remove the trailing comma as always
		update_word += " WHERE ";
		for (int i = 0/*match by pk columns*/; i < learnt_word_pk_count; i++)
			update_word += columns[i] + " LIKE " + values[i] + (((i+1) != learnt_word_pk_count)? "AND" : "");
		//TODO(fran): should I use "like" or "="  or "==" ?

		char* update_errmsg;
		res = sqlite3_exec(db, update_word.c_str(), 0, 0, &update_errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(update_errmsg);

		return res;
	}

	bool modify_word(ProcState* state) {
		bool res = false;
		if (check_show_word(state)) {
			learnt_word w;
			auto& page = state->controls.show_word;

			_get_edit_str(page.list.static_hiragana, w.attributes.hiragana);
			_get_edit_str(page.list.edit_kanji, w.attributes.kanji);
			_get_edit_str(page.list.edit_translation, w.attributes.translation);
			_get_edit_str(page.list.edit_mnemonic, w.attributes.mnemonic);
			_get_combo_sel_str(page.list.combo_lexical_category, w.attributes.lexical_category);

			learnt_word w_utf8; defer{ for (auto& _ : w_utf8.all)free_any_str(_.str); };
			for (int i = 0; i < ARRAYSIZE(w.all); i++) {
				w_utf8.all[i] = convert_utf16_to_utf8((utf16*)w.all[i].str, (int)w.all[i].sz);
				free_any_str(w.all[i].str);//maybe set it to zero too
			}

			res = update_word(state->settings->db, &w_utf8);
		}
		return res;
	}

	bool delete_word(sqlite3* db, learnt_word* word) {
		bool res = false;

		//To delete you find the word by its primary keys
		std::string delete_word = std::string(" DELETE FROM ") + べんきょう_table_words + " WHERE " + "hiragana" + " = " + "'" + (utf8*)word->attributes.hiragana.str + "'" + ";";//TODO(fran): parametric filtering by all primary keys


		char* delete_errmsg;
		res = sqlite3_exec(db, delete_word.c_str(), 0, 0, &delete_errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(delete_errmsg);
		return res;
	}

	bool remove_word(ProcState* state) {
		bool res = false;
		learnt_word word_to_delete{ 0 };

		auto& page = state->controls.show_word;

		_get_edit_str(page.list.static_hiragana, word_to_delete.attributes.hiragana); //TODO(fran): should check for valid values? I feel it's pointless in this case
		defer{ free_any_str(word_to_delete.attributes.hiragana.str); };

		learnt_word word_to_delete_utf8; //TODO(fran): maybe converting the attributes in place would be nicer, also we could iterate over all the members and check for valid pointers and only convert those

		word_to_delete_utf8.attributes.hiragana = convert_utf16_to_utf8((utf16*)word_to_delete.attributes.hiragana.str, (int)word_to_delete.attributes.hiragana.sz);
		defer{ free_any_str(word_to_delete_utf8.attributes.hiragana.str); };

		res = delete_word(state->settings->db, &word_to_delete_utf8);
		return res;
	}

	learnt_word get_practice_word(sqlite3* db) {
		learnt_word res{0};
		//TODO(fran)
		return res;
	}

	struct practice_data {
		ProcState::page page;
		void* data;
	};
	practice_data prepare_practice(ProcState* state) {
		practice_data res;
		ProcState::available_practices practices[]{
			ProcState::available_practices::writing,
			//ProcState::available_practices::multiplechoice
		};

		ProcState::available_practices practice = practices[random_between(0,ARRAYSIZE(practices)-1)];
		learnt_word practice_word = get_practice_word(state->settings->db);//we always get a target word
		//TODO(fran): the corresponding practice page will take care of freeing the contents of this

		switch (practice) {
		case ProcState::available_practices::writing:
		{
			//NOTE: writing itself has many different practices
			practice_writing_word* data = (decltype(data))malloc(sizeof(data));//TODO(fran): practice_writing page will take care of freeing this once the page completes its practice
			data->word = std::move(practice_word);

			practice_writing_word::type practice_types[]{
				practice_writing_word::type::translate_hiragana_to_translation,
				practice_writing_word::type::translate_translation_to_hiragana,
				practice_writing_word::type::translate_kanji_to_hiragana,
				practice_writing_word::type::translate_kanji_to_translation,
			};

			data->practice_type = practice_types[random_between(0, ARRAYSIZE(practice_types) - 1)];
			res.page = ProcState::page::practice_writing;
			res.data = data;
		} break;
		default: Assert(0);
		}
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
			startup(st->settings->db);
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
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::new_word);
					}
					else if (child == page.list.button_practice) {
						store_previous_page(state, state->current_page);
						user_stats stats = get_user_stats(state->settings->db);
						preload_page(state, ProcState::page::practice, &stats);
						set_current_page(state, ProcState::page::practice);
					}
					else if (child == page.list.button_search) {
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::search);
					}
				} break;
				case ProcState::page::new_word:
				{
					auto& page = state->controls.new_word;
					if (child == page.list.button_save) {
						if (save_new_word(state)) {
							//If the new word was successfully saved then go back to the landing, TODO(fran): a better idea may be to reset the new_word page, so that the user can add multiple words faster
							store_previous_page(state, state->current_page);
							set_current_page(state, ProcState::page::landing);
							//TODO(fran): clear new_word controls, now this one's annoying cause not everything is cleared the same way
						}
					}
				} break;
				case ProcState::page::practice:
				{
					auto& page = state->controls.practice;
					if (child == page.list.button_start) {
						
						i64 word_cnt = get_user_stats(state->settings->db).word_cnt; //TODO(fran): I dont know if this is the best way to do it but it is the most accurate
						if (word_cnt > 0) {
							store_previous_page(state, state->current_page);//TODO(fran): any time the user presses the back button while on a practice they'll be prompted to confirm and then sent to the main practice page
							//TODO(fran): randomly select the type of practice (eg hiragana to translation, etc) and retrieve the first candidate word
							state->practice_cnt = 0;//reset the practice counter
							practice_data practice = prepare_practice(state);//get a random practice
							preload_page(state, practice.page, practice.data);//load the practice
							set_current_page(state, practice.page);//go practice!
						}
						else MessageBoxW(state->wnd, RCS(360), 0, MB_OK, MBP::center);
					}
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

							//DWORD sel_start, sel_end; SendMessage(page.list.combo_search, CB_GETEDITSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);

							COMBOBOX_clear_list_items(page.list.combo_search);

							int sz_char = (int)SendMessage(page.list.combo_search, WM_GETTEXTLENGTH, 0, 0)+1;
							if (sz_char > 1) {
								utf16* search = (decltype(search))malloc(sz_char * sizeof(*search));
								SendMessage(page.list.combo_search, WM_GETTEXT, sz_char, (LPARAM)search);

								//TODO(fran): this might be an interesting case for multithreading, though idk how big the db has to be before this search is slower than the user writing, also for jp text the IME wont allow for the search to start til the user presses enter, that may be another <-TODO(fran): chrome-like handling for IME, text goes to the edit control at the same time as the IME

								auto search_res = search_word_matches(state->settings->db, search, 5); defer{ free_search_word(search_res); };
								//TODO(fran): clear the listbox

								//Semi HACK: set first item of the listbox to what the user just wrote, to avoid the stupid combobox from selecting something else
								//Show the listbox
								SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, TRUE, 0);
								SendMessageW(page.list.combo_search, CB_INSERTSTRING, 0, (LPARAM)search);

								if (search_res.cnt) {
									for (int i = 0; i < search_res.cnt; i++) 
										SendMessageW(page.list.combo_search, CB_INSERTSTRING, -1, (LPARAM)search_res.matches[i]);
									
									//TODO(fran): for some reason the cb decides to set the selection once we insert some strings, if we try to set it to "no selection" it erases the user's string, terrible, we must fix this from the subclass

#if 1
									//TODO(fran): why the hell does this make the mouse disappear, just terrible, I really need to make my own one
									//TODO(fran): not all the items are shown if they are too many, I think there's a msg that increases the number of items to show

									//NOTE: ok now we have even more garbage, the cb sets the selection to the whole word, which means that the next char the user writes _overrides_ the previous one, I really cant comprehend how this is so awful to use, I must be doing something wrong
									//SendMessage(page.list.combo_search, CB_SETEDITSEL, 0, MAKELONG(sel_start,sel_end));//This is also stupid, you go from DWORD on CB_GETEDITSEL to WORD here, you loose a whole byte of info

									//IMPORTANT INFO: I think that the "autoselection" problem only happens when you call CB_SHOWDROPDOWN, if we called it _before_ adding elements to the list we may be ok, the answer is yes and no, my idea works _but_ the listbox doesnt update its size to accomodate for the items added after showing it, gotta do that manually

									//COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
									//int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);
									//int list_h = max((int)SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt,2);
									//RECT combo_rw; GetWindowRect(page.list.combo_search, &combo_rw);
									//MoveWindow(nfo.hwndList, combo_rw.left, combo_rw.bottom, RECTWIDTH(combo_rw), list_h, TRUE); //TODO(fran): handle showing it on top of the control if there's no space below
									//AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
#else
									COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
									int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);//returns -1 on error, but who cares
									//NOTE: unless LBS_OWNERDRAWVARIABLE style is defined all items have the same height
									int list_h = max(SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt,2);
									RECT rw; GetWindowRect(page.list.combo_search, &rw);
									MoveWindow(nfo.hwndList, rw.left, rw.bottom, RECTWIDTH(rw), list_h, TRUE);
									SetWindowPos(nfo.hwndList, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);//INFO: here was the problem, the window was being occluded
									//ShowWindow(nfo.hwndList, SW_SHOWNA);
									AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
									SendMessage(nfo.hwndList, 0x1ae/*LBCB_STARTTRACK*/, 0, 0); //does start mouse tracking
									//NOTE: I think LBCB_ENDTRACK is handled by itself
#endif
									//TODO(fran): add something to the default list item or add extra info to the db results to differentiate the default first listbox item from the results
									//TODO(fran): now we have semi success with the cb, but still the first character comes out a little too late, it feels bad, we probably cant escape creating our own combobox (I assume that happens because animatewindow is happening?)
									//TODO(fran): actually we can implement this in the subclass, we can add a msg for a non stupid way of showing the listbox, we can probaly handle everything well from there
								}
								else {
									//Hide the listbox, well... maybe not since we now show the search string on the first item
									//SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
								}
								//Update listbox size
								COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
								int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);
								int list_h = max((int)SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt, 2);
								RECT combo_rw; GetWindowRect(page.list.combo_search, &combo_rw);
								MoveWindow(nfo.hwndList, combo_rw.left, combo_rw.bottom, RECTWIDTH(combo_rw), list_h, TRUE); //TODO(fran): handle showing it on top of the control if there's no space below
								AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
							}
							else {
								//Hide the listbox
								SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
							}
						} break;
						case CBN_CLOSEUP:
						{
							//SendMessage(page.list.combo_search, CB_RESETCONTENT, 0, 0);//Terrible again, what if you just want to clear the listbox alone? we need the poor man's solution:
							COMBOBOX_clear_list_items(page.list.combo_search);
						} break;
						case CBN_SELENDOK://User has selected an item from the list
						{
							//TODO(fran): lets hope this gets sent _before_ hiding the list, otherwise I'd have already deleted the elements by this point
							int sel = (int)SendMessageW(page.list.combo_search, CB_GETCURSEL, 0, 0);
							if (sel != CB_ERR) {
								int char_sz = (int)SendMessageW(page.list.combo_search, CB_GETLBTEXTLEN, sel, 0)+1;
								any_str word = alloc_any_str(char_sz * sizeof(utf16)); defer{ free_any_str(word.str); };
								SendMessageW(page.list.combo_search, CB_GETLBTEXT, sel, (LPARAM)word.str);
								//We got the word, it may or may not be valid, so there are two paths, show error or move to the next page "show_word"
								get_word_res res = get_word(state->settings->db, (utf16*)word.str); defer{ free_get_word(res); };
								if (res.found) {
									preload_page(state, ProcState::page::show_word, &res.word);//NOTE: considering we no longer have separate pages this might be a better idea than sending the struct at the time of creating the window
									store_previous_page(state, state->current_page);
									set_current_page(state, ProcState::page::show_word);
								}
								else {
									int ret = MessageBoxW(state->nc_parent, RCS(300), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL,MBP::center);
									if (ret == IDYES) {
										learnt_word new_word{0};
										new_word.attributes.hiragana = word;
										preload_page(state, ProcState::page::new_word, &new_word);
										store_previous_page(state, state->current_page);
										set_current_page(state, ProcState::page::new_word);
									}
								}
							}
						} break;
						}
					}
				} break;
				case ProcState::page::show_word:
				{
					auto& page = state->controls.show_word;
					if (child == page.list.button_modify) {
						if (modify_word(state)) {
							goto_previous_page(state);
							//set_current_page(state, ProcState::page::search);//TODO(fran): this should be a "go back" once we have the back button and a "go back" queue(or similar) set up
						}
					}
					if (child == page.list.button_delete) {

						int ret = MessageBoxW(state->nc_parent, RCS(280), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL);
						if (ret == IDYES) {
							if (remove_word(state)) {
								goto_previous_page(state);
								//set_current_page(state, ProcState::page::search);//TODO(fran): this should also be go back, we can get to show_word from new_word
							}
						}
}
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
		case WM_CTLCOLORSTATIC: //for the static controls //TODO(fran): this has to go, aka make my own
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(unCap_colors.ControlTxt));//TODO(fran): maybe a lighter color to signalize non-editable?

			return (INT_PTR)unCap_colors.ControlBk;
		} break;
		case WM_BACK:
		{
			goto_previous_page(state);
			return 0;
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
		case WM_DELETEITEM://Sent by the cb when I delete its items (which I need to do one by one since there's no msg to send it that can do that)
		{
			return TRUE;
		} break;
		case WM_INPUTLANGCHANGE:
		{
			return 1;
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
