#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "unCap_Serialization.h"
#include "win32_Global.h"
#include "sqlite3.h"
#include "win32_Helpers.h"
#include "win32_button.h"
#include "LANGUAGE_MANAGER.h"
#include "win32_edit_oneline.h"
#include "win32_combobox.h"
#include "unCap_Math.h"
#include "win32_score.h"
#include "win32_static_oneline.h"
#include "win32_graph.h"
#include "win32_gridview.h"
#include "win32_searchbox.h"
#include "win32_new_msgs.h"

#include <string>
#include <array> //create_grid_2x2

//TODO(fran): db: table words: go back to using rowid and add an id member to the learnt_word struct
//TODO(fran): db: load the whole db in ram
//TODO(fran): db: store a version number on the db in order to be able to update the tables in case of changes with different versions
//TODO(fran): all controls: check for a valid brush and if it's invalid dont draw, that way we give the user the possibility to create transparent controls (gotta check that that works though)
//TODO(fran): all pages: it'd be nice to have a scrolling background with jp text going in all directions
//TODO(fran): all pages: hiragana text must always be rendered in the violet color I use in my notes, and the translation in my red, for kanji I dont yet know
//TODO(fran): all pages: can keyboard input be automatically changed to japanese when needed?
//TODO(fran): all pages: chrome style IME, the text is written directly on the control while you write
//TODO(fran): all pages: I dont know who should be in charge of converting from utf16 to utf8, the backend or front, Im now starting to lean towards the first one, that way we dont have conversion code all over the place, it's centralized in the functions themselves
//TODO(fran): all pages: any button that executes an operation, eg next practice, create word, etc, _must_ be somehow disabled after the first click, otherwise the user can spam clicks and possibly even crash the application
//TODO(fran): all pages: we need an extra control that acts as a page, and is the object that's shown and hidden, this way we can hide and show elements of the page which we currently cannot do since we sw_show each individual one (the control is very easy to implement, it should simply be a passthrough and have no logic at all other than redirect msgs)
//TODO(fran): all pages: color templates for set_brushes so I dont have to rewrite it each time I add a control
//TODO(fran): page landing: make it a 2x2 grid and add a stats button that redirect to a new stats page to put stuff that's in practice page, like "word count" and extra things like a list of last words added
//TODO(fran): page new_word: pressing enter should map to the add button
//TODO(fran): page new_word: check that no kanji is written to the hiragana box
//TODO(fran): page new_word: add edit box called "Notes" where the user can write anything eg make a clarification
//TODO(fran): page new_word: the add buton should offer the possibility to update (in the case where the word already exists), and show a separate (non editable) window with the current values of that word, idk whether I should modify from there or send the data to the show_word page and redirect them to there, in that case I really need to start using ROWID to predetermine which row has to be modified, otherwise the user can change everything and Im screwed
//TODO(fran): page practice: everything animated (including things like word_cnt going from 0 to N)
//TODO(fran): page practice: precalculate the entire array of practice leves from the start (avoids duplicates)
//TODO(fran): page practice_writing | win32_edit_oneline: page setfocus to the edit box (and add flag to the edit box for showing placeholder text while on focus)
//TODO(fran): page review_practice: alternative idea: cliking an element of the gridview redirects to the show_word page
//TODO(fran): new page practice_drawing: practice page for kanji via OCR, give the user translation or hiragana and ask them to draw kanji
//TODO(fran): page show_word: the center aligned editboxes _still_ render the caret in the wrong place when they have some text already set
//TODO(fran): BUG: page search: going to the previous page doesnt currently remove focus from whoever had it, therefore if you click on the searchbox, go to the prev page and press a key then the listbox will present the options and allow you to load the show_word page (this bug wasnt present before cause we handled searching in WM_COMMAND and check against our childs, therefore the landing page could not access controls from the search page, but now seach happens _inside_ the searchbox control and thus bypassing page checking), one quick and dirty way to solve this for now would be to check we're on the right page inside the seachbox_func_... functions, still other similar bugs could happen, best would probably be to steal focus when going back a page, or add a function that, given a page, sets focus to the desired control

//TODO(fran): IDEA: when opening practices for the review we could add new pages to the enum, this are like virtual pages, using the same controls, but now can have different layout or behaviour simply by adding them to the switch statements


//INFO: this wnd is divided into two, the UI side and the persistence/db interaction side, the first one operates on utf16 for input and output, and the second one in utf8 for input and utf8 for output (also utf16 but only if you manually handle it)
//INFO: this window is made up of many separate ones, as if they were in different tabs, in the sense only one is shown at any single time, and there isn't any relationship between them
//INFO: the hiragana aid on top of kanji is called furigana
//INFO: Similar applications/Possible Inspiration: anki, memrise, wanikani
//INFO: dates on the db are stored in GMT, REMEMBER to convert them for the UI


#define _SQL(x) (#x)
//IMPORTANT: careful when using this, by design it allows for macros to expand so you better not redefine sql keywords, eg microsoft defines NULL as 0 so you should write something like NuLL instead, same for DELETE use smth like DeLETE
#define SQL(x) _SQL(x)
//IMPORTANT: do not use "," in macros, instead use comma
#define comma ,


#define TIMER_next_practice_level 0x5


#define べんきょう_table_words "words"
#define _べんきょう_table_words words
#define べんきょう_table_words_structure \
	hiragana			TEXT PRIMARY KEY COLLATE NOCASE,\
	kanji				TEXT COLLATE NOCASE,\
	translation			TEXT NOT NuLL COLLATE NOCASE,\
	mnemonic			TEXT,\
	lexical_category	INTEGER,\
	creation_date		INTEGER DEFAULT(strftime('%s', 'now')),\
	last_shown_date		INTEGER DEFAULT 0,\
	times_shown			INTEGER DEFAULT 0,\
	times_right			INTEGER DEFAULT 0\

//INFO about べんきょう_table_words_structure:
//	hiragana: hiragana or katakana //NOTE: we'll find out if it was a good idea to switch to using this as the pk instead of rowid, I have many good reasons for both
//	general: user modifiable values first, application defined last
//	creation_date: in Unix time
//	last_shown_date: in Unix Time, used for sorting, TODO(fran): I should actually default to a negative value since 0 maps to 1970 but unix time can go much further back than that

//TODO(fran): hiragana, kanji and translation should actually be lists

;//INFO: ROWID: in sqlite a column ROWID is automatically created and serves the function of "id INTEGER PRIMARY KEY" and AUTOINCREMENT, _but_ AUTOINCREMENT as in MySQL or others (simply incrementing on every insert), on the other hand the keyword AUTOINCREMENT in sqlite incurrs an extra cost because it also checks that the value hasnt already been used for a deleted row (we dont care for this in this table)

#define べんきょう_table_user "user" 
#define _べんきょう_table_user user
#define べんきょう_table_user_structure \
	word_cnt		INTEGER DEFAULT 0,\
	times_practiced	INTEGER DEFAULT 0,\
	times_shown		INTEGER DEFAULT 0,\
	times_right		INTEGER DEFAULT 0\

//INFO about べんきょう_table_user:
//	a table with only one user, having to join user-word to enable multiuser is just a waste of performance, if we have multiple users we'll create a different folder and db for each, also prevents that corruption of one user affects another
//	word_cnt, times_shown, times_right are automatically calculated via triggers

#define _べんきょう_table_accuracy_timeline accuracy_timeline
#define べんきょう_table_accuracy_timeline_structure \
	creation_date		INTEGER DEFAULT(strftime('%s', 'now')),\
	accuracy			INTEGER NOT NuLL\

//INFO about _べんきょう_table_accuracy_timeline:
//	accuracy: value between [0,100]

//TODO(fran): add indexing (or default ordering, is that a thing?) by creation_date

#define べんきょう_table_version "version" /*TODO(fran): versioning system to be able to move at least forward, eg db is v2 and we are v5; for going backwards, eg db is v4 and we are v2, what we can do is avoid modifying any already existing columns of previous versions when we move to a new version, that way older versions simply dont use the columns/tables of the new ones*/

#define べんきょう_table_version_structure \
	"v					INTEGER"\

//INFO about べんきょう_table_version_structure:
//	v: stores the db/program version, to make it simpler versions are just a number, the db wont be changing too often

struct user_stats {
	i64 word_cnt;			//Count for words added
	i64 times_practiced;	//Count for completed practice runs
	i64 times_shown;		//Count for words shown in practice runs
	i64 times_right;		//Count for correct word guessings in practice runs
	ptr<u8> accuracy_timeline;	//Accuracy changes in some window of time, uses a separate query to obtain them, #free
	
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
enum lexical_category { //this value is stored on the db
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
//usage example: RS(lexical_category_str_lang_id(lexical_category::verb))
u32 lexical_category_str_lang_id(lexical_category cat) {
	return 200 + cat; //NOTE: dont_care should never be shown
}
void lexical_category_setup_combobox(HWND cb) {
	//INFO: the first element to add to a combobox _must_ be at index 0, it does not support starting at any index, conclusion: windows' combobox is terrible
	ACT(cb, lexical_category::noun, lexical_category_str_lang_id(lexical_category::noun));
	ACT(cb, lexical_category::verb, lexical_category_str_lang_id(lexical_category::verb));
	ACT(cb, lexical_category::adj_い, lexical_category_str_lang_id(lexical_category::adj_い));
	ACT(cb, lexical_category::adj_な, lexical_category_str_lang_id(lexical_category::adj_な));
	ACT(cb, lexical_category::adverb, lexical_category_str_lang_id(lexical_category::adverb));
	ACT(cb, lexical_category::conjunction, lexical_category_str_lang_id(lexical_category::conjunction));
	ACT(cb, lexical_category::pronoun, lexical_category_str_lang_id(lexical_category::pronoun));
	ACT(cb, lexical_category::counter, lexical_category_str_lang_id(lexical_category::counter));
}

union learnt_word { //contains utf16 when getting data from the UI, and utf8 when sending requests to the db
	using type = any_str;
	struct {
#define _foreach_learnt_word_member(op) \
		op(type,hiragana) \
		op(type,kanji) \
		op(type,translation) \
		op(type,mnemonic) \
		op(type,lexical_category) \

		_foreach_learnt_word_member(_generate_member_no_default_init);
		//IMPORTANT: members that map to primary keys must be first on the list (that way they are easily filtered out when updating their values), and UPDATE pk_count when you add a new primary key

	} attributes;
	type all[sizeof(attributes) / sizeof(type)];

	static constexpr int pk_count = 1;//number of primary keys, primary keys are the first members in the attributes list
};

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

//Structures for different practice levels
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

void languages_setup_combobox(HWND cb) {
	auto langs = LANGUAGE_MANAGER::Instance().GetAllLanguages();
	auto current_lang = LANGUAGE_MANAGER::Instance().GetCurrentLanguage();
	for (const auto& lang : *langs) {
		SendMessage(cb, CB_INSERTSTRING, -1, (LPARAM)lang.c_str());
		if(!lang.compare(current_lang)) SendMessage(cb, CB_SETCURSEL, (int)SendMessage(cb,CB_GETCOUNT,0,0)-1, 0);
	}
	InvalidateRect(cb, NULL, TRUE);
}

//---------------------Macros-------------------------:

#define _sqlite3_generate_columns(type,name,...) "" + #name + ","
//IMPORTANT: you'll have to remove the last comma since sql doesnt accept trailing commas

#define _sqlite3_generate_columns_array(type,name,...) #name,

#define _sqlite3_generate_values(type,name,...) "'" + (utf8*)word->attributes.name.str + "'"","
//IMPORTANT: you'll have to remove the last comma since sql doesnt accept trailing commas

#define _sqlite3_generate_values_array(type,name,...) std::string("'") + (utf8*)word->attributes.name.str + "'",


//-------------Data retrieval from UI-----------------: (UI strings are always utf16)

// use for controls whose value is obtained via WM_GETTEXT, eg static, button, edit, ...
//#define _get_edit_str(edit,any_str) \
//			{ \
//				int _sz_char = (int)SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0) + 1; \
//				any_str = alloc_any_str(_sz_char * sizeof(utf16)); \
//				SendMessageW(edit, WM_GETTEXT, _sz_char, (WPARAM)any_str.str); \
//			}

#define _get_combo_sel_idx_as_str(cb,any_str) \
			{ \
				int lex_categ = (int)SendMessageW(cb, CB_GETCURSEL, 0, 0); \
				int sz_char = _snwprintf(nullptr, 0, L"%d", lex_categ) + 1; \
				any_str = alloc_any_str(sz_char * sizeof(utf16)); \
				_snwprintf((utf16*)w.attributes.lexical_category.str, sz_char, L"%d", lex_categ); \
			}

#define _clear_combo_sel(cb) SendMessageW(cb, CB_SETCURSEL, -1, 0)

#define _clear_edit(edit) SendMessageW(edit, WM_SETTEXT, 0, 0)

#define _clear_static(st) SendMessageW(st, WM_SETTEXT, 0, 0)


struct べんきょうSettings {

#define foreach_べんきょうSettings_member(op) \
		op(RECT, rc,200,200,600,800 ) \

	foreach_べんきょうSettings_member(_generate_member);
	sqlite3* db;

	_generate_default_struct_serialize(foreach_べんきょうSettings_member);
	_generate_default_struct_deserialize(foreach_べんきょうSettings_member);

};
_add_struct_to_serialization_namespace(べんきょうSettings);

struct べんきょうProcState {
	HWND wnd;
	HWND nc_parent;
	べんきょうSettings* settings;

	struct {
		HBRUSH bk;
	} brushes;

	enum class page {
		landing,
		new_word,
		practice,
		practice_writing,
		practice_multiplechoice,//TODO(fran)
		review_practice,
		search,
		show_word,
	} current_page;

	struct prev_page_fifo_queue{
		decltype(current_page) pages[10];
		u32 cnt;
	}previous_pages;

	u32 practice_cnt;//counter for current completed stages/levels while on a pratice run, gets set to eg 10 and is decremented by -1 with each completed stage, when practice_cnt == 0  the practice ends

	struct {

		union landingpage_controls {
			using type = HWND;
			struct {
				type button_new;
				type button_practice;
				type button_search;
				type combo_languages;
			}list; //INFO: unfortunately in order to make 'all' auto-update we need to give a name to this struct
			type all[sizeof(list)/sizeof(type)]; //NOTE: make sure you understand structure padding before implementing this, also this should be re-tested if trying with different compilers or alignment
		} landingpage;

		union new_word_controls {
			using type = HWND;
			struct {
				type edit_hiragana;
				type edit_kanji;
				type edit_translation;
				type combo_lexical_category;
				type edit_mnemonic;//create a story/phrase around the word
				//TODO(fran): here you should be able to add more than one translation
				//TODO(fran): type edit_notes
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

				type static_accuracy_timeline_title;
				type graph_accuracy_timeline;

				type button_start;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice;

		union practice_writing_controls {
			using type = HWND;
			struct {
				type static_test_word;

				type edit_answer;

				type button_next;//TODO(fran): not the best name
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice_writing;

		union search_controls {
			using type = HWND;
			struct {
				//type combo_search;
				type searchbox_search;
				//and a list for the results
			}list;
			type all[sizeof(list) / sizeof(type)];
		} search;

		union review_practice_controls {
			using type = HWND;
			struct {
				type static_review;
				type gridview_practices;
				type button_continue;
			}list;
			type all[sizeof(list) / sizeof(type)];
		}review_practice;

		union show_word_controls {
			using type = HWND;
			struct {
				type static_hiragana;
				type edit_kanji;
				type edit_translation;
				type combo_lexical_category;
				type edit_mnemonic;
				//TODO(fran): here you should be able to add more than one translation

				type static_creation_date;
				type static_last_shown_date;
				type static_score; //eg Score: 4/5 - 80%

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

	struct practice_header {
		available_practices type;
	};
	struct practice_writing {
		practice_header header;
		practice_writing_word* practice;//#free
		const utf16_str* question;//points to some element inside practice.word
		utf16_str user_answer;//#free
		const utf16_str* correct_answer;//points to some element inside practice.word
		bool answered_correctly;//precalculated value so strcmp is used only once
	};

	struct {

		struct practice_review_state {

			std::vector<practice_header*> practices;

		}practice_review;

		struct practice_writing_state {
			practice_writing_word* practice;
		}practice_writing;//TODO(fran): if we already had the entire practices array from the start we could simplify this to a simple size_t idx and the multipage_mem.temp_practices[pagestate.practice_writing.idx]

	} pagestate;

	struct {
		std::vector<practice_header*> temp_practices;
	}multipagestate;
};

namespace べんきょう {
	using ProcState = べんきょうProcState;

	constexpr cstr wndclass[] = L"がいじんの_wndclass_べんきょう";

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	//NOTE: a null HBRUSH means dont change the current one
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	i64 get_table_rowcount(sqlite3* db, std::string table) {
		i64 res;
		std::string count = "SELECT COUNT(*) FROM " + table + ";";
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
		utf8 stats[] = SQL(
			SELECT word_cnt comma times_practiced comma times_shown comma times_right FROM _べんきょう_table_user;
		);

		sqlite3_stmt* stmt;
		int errcode;
		errcode = sqlite3_prepare_v2(db, stats, (int)(ARRAYSIZE(stats)) * sizeof(decltype(*stats)), &stmt, nullptr); //INFO: there's a prepare with utf16 that we can use!, everything utf16 would be a blessing
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

	//NOTE: afterwards you must .free() the accuracy_timeline member
	//retrieves the last cnt timepoints in order of oldest to newest
	void get_user_stats_accuracy_timeline(sqlite3* db, user_stats* stats, size_t cnt) {
		using namespace std::string_literals;
		std::string timeline =
			SQL(SELECT accuracy FROM) 
			+ "("s +
				SQL(SELECT accuracy comma creation_date FROM _べんきょう_table_accuracy_timeline ORDER BY creation_date DESC LIMIT) + " "s + std::to_string(cnt)
			+ ")" + 
			SQL(ORDER BY creation_date ASC;);

		sqlite3_stmt* stmt;
		int errcode;
		errcode = sqlite3_prepare_v2(db, timeline.c_str(), (int)(timeline.size()+1) * sizeof(decltype(timeline)::value_type), &stmt, nullptr);
		sqliteok_runtime_assert(errcode, db);

		stats->accuracy_timeline.alloc(cnt);

		int i = 0;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			stats->accuracy_timeline[i] = (decltype(stats->accuracy_timeline)::value_type)sqlite3_column_int(stmt, 0);
			i++;
		}
		stats->accuracy_timeline.cnt = i;

		sqlite3_finalize(stmt);
	}

	void startup(sqlite3* db) {
		utf8* errmsg;
		{
			utf8 create_word_table[] = SQL(
				CREATE TABLE IF NOT EXISTS _べんきょう_table_words(べんきょう_table_words_structure) WITHOUT ROWID;
			);
			sqlite3_exec(db, create_word_table, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
		//TODO(fran): we should also implement UPDATE TABLE for future versions that might need new columns, the other idea is to make separate tables join by foreign key, I dont know if it has any benefit, probably not since the queries will become bigger and slower cause of the joins, what I do have to be careful is that downgrading doesnt destroy anything
		{
			utf8 create_user_table[] = SQL(
				CREATE TABLE IF NOT EXISTS _べんきょう_table_user(べんきょう_table_user_structure);
			);
			sqlite3_exec(db, create_user_table, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
		{
			if (get_table_rowcount(db, べんきょう_table_user) > 0) {
				//The entry is already there, here we can set new values on future updates for example
				//TODO(fran)
			}
			else {
				//Entry isnt there, create it
				utf8 insert_user[] = SQL(
					INSERT INTO _べんきょう_table_user DEFAULT VALUES;
				);
				sqlite3_exec(db, insert_user, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
				//TODO(fran): we should check for existing words and compute the values of the user table, idk if this case can happen
			}
		}
		{
			utf8 create_accuracy_timeline_table[] = SQL(
				CREATE TABLE IF NOT EXISTS _べんきょう_table_accuracy_timeline(べんきょう_table_accuracy_timeline_structure);
			);
			sqlite3_exec(db, create_accuracy_timeline_table, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}

		//Triggers 
		//TODO(fran): here the versioning system comes in handy, if the db version == ours then we dont create the triggers since they already exist, otherwise we drop any existing trigger and override it with the new one, _emphasis_ on "==" the triggers work only with what we know in this current version, the versioning system will have to take care of fixing anything future triggers might need already done with the entries that have already been loaded (this is obvious for older versions but it also establishes an invariant for future versions, this allows each version's triggers to work independently of each other), I do think this is the better choice, triggers are independent but that doesnt mean the should break something they know a previous version will need, my concern would be of the need for a trigger to stop an operation, and lets say it expects for a value that v5 gives but v4 doesnt, that means the user can no longer downgrade from v5. Following this sense it'd mean we should probably simply create temp triggers and save the extra hussle of having to check
		{
			//TODO(fran): remove the IF NOT EXISTS once we have db version checking
			utf8 create_trigger_increment_word_cnt[] = SQL(
				CREATE TRIGGER IF NOT EXISTS increment_word_cnt AFTER INSERT ON _べんきょう_table_words
				BEGIN
					UPDATE _べんきょう_table_user SET word_cnt = word_cnt + 1;
				END;
			);
			sqlite3_exec(db, create_trigger_increment_word_cnt, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
		{
			//TODO(fran): remove the IF NOT EXISTS once we have db version checking
			utf8 create_trigger_decrement_word_cnt[] = SQL(
				CREATE TRIGGER IF NOT EXISTS decrement_word_cnt AFTER DeLETE ON _べんきょう_table_words
				BEGIN
					UPDATE _べんきょう_table_user SET word_cnt = word_cnt - 1;
				END;
			);
			sqlite3_exec(db, create_trigger_decrement_word_cnt, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
		{
			//TODO(fran): remove the IF NOT EXISTS once we have db version checking
			utf8 create_trigger_increment_times_shown[] = SQL(
				CREATE TRIGGER IF NOT EXISTS increment_times_shown
				AFTER UPDATE OF times_shown ON _べんきょう_table_words
				BEGIN
					UPDATE _べんきょう_table_user SET times_shown = times_shown + NEW.times_shown - OLD.times_shown;
				END;
			);
			sqlite3_exec(db, create_trigger_increment_times_shown, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
		{
			//TODO(fran): remove the IF NOT EXISTS once we have db version checking
			utf8 create_trigger_increment_times_right[] = SQL(
				CREATE TRIGGER IF NOT EXISTS increment_times_right
				AFTER UPDATE OF times_right ON _べんきょう_table_words
				BEGIN
					UPDATE _べんきょう_table_user SET times_right = times_right + NEW.times_right - OLD.times_right;
				END;
			);
			sqlite3_exec(db, create_trigger_increment_times_right, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
		//TODO(fran): remove the IF NOT EXISTS once we have db version checking
		/*
		utf8 create_trigger_insert_accuracy_timepoint[] = SQL(
			CREATE TRIGGER IF NOT EXISTS insert_accuracy_timepoint
			AFTER UPDATE OF times_shown comma times_right ON _べんきょう_table_user
			WHEN (ABS(strftime('%s', 'now') - COALESCE((SELECT MAX(creation_date) FROM _べんきょう_table_accuracy_timeline),0)) > 2)
			BEGIN
			INSERT INTO _べんきょう_table_accuracy_timeline(accuracy) 
				VALUES((NEW.times_right / COALESCE(NULLIF(NEW.times_shown, 0), NULLIF(NEW.times_right, 0), 1))*100);
			END;
		);
		utf8 create_trigger_update_accuracy_timepoint[] = SQL(
			CREATE TRIGGER IF NOT EXISTS update_accuracy_timepoint
			AFTER UPDATE OF times_shown comma times_right ON _べんきょう_table_user
			WHEN(ABS(strftime('%s', 'now') - COALESCE((SELECT MAX(creation_date) FROM _べんきょう_table_accuracy_timeline), 0)) <= 2)
			BEGIN
			UPDATE _べんきょう_table_accuracy_timeline
				SET accuracy = (NEW.times_right / COALESCE(NULLIF(NEW.times_shown, 0), NULLIF(NEW.times_right, 0), 1))*100
			WHERE creation_date = COALESCE((SELECT MAX(creation_date) FROM _べんきょう_table_accuracy_timeline), 0);
			END;
		);
		*/
		{
			//IMPORTANT: this depends on times_shown being updated _before_ times_right
			utf8 create_trigger_insert_accuracy_timepoint[] = SQL(
				CREATE TRIGGER IF NOT EXISTS insert_accuracy_timepoint
				AFTER UPDATE OF times_shown ON _べんきょう_table_user
				BEGIN
					INSERT INTO _べんきょう_table_accuracy_timeline(accuracy)
					VALUES(CAST((CAST(NEW.times_right AS REAL) / CAST(NEW.times_shown AS REAL) * 100.0) AS INTEGER));
				END;
			);

			utf8 create_trigger_update_accuracy_timepoint[] = SQL(
				CREATE TRIGGER IF NOT EXISTS update_accuracy_timepoint
				AFTER UPDATE OF times_right ON _べんきょう_table_user
				BEGIN
					UPDATE _べんきょう_table_accuracy_timeline
					SET accuracy = CAST(((CAST(NEW.times_right AS REAL) / CAST(NEW.times_shown AS REAL)) * 100.0) AS INTEGER)
					WHERE creation_date = (SELECT MAX(creation_date) FROM _べんきょう_table_accuracy_timeline);
				END;
			);

			//IDEA: another way is to always insert on times_shown and update on times_rigth
			//TODO(fran): this two triggers are really dumb, and we depend on the operation happening in less than N seconds (timinig dependency), if we could make this be one trigger at least but sqlite doenst have if-else on triggers
			//TODO(fran): make sure that update of triggers for any of the columns names and not only if both are SET in a single query
			//IMPORTANT INFO: since times_shown and times_right can be updated at different times we gotta be a little more clever at the time of adding a new timepoint
			sqlite3_exec(db, create_trigger_insert_accuracy_timepoint, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);

			sqlite3_exec(db, create_trigger_update_accuracy_timepoint, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}

//#define TEST_WORDS
#if defined(TEST_WORDS) && defined(_DEBUG)
		{
			utf8 test_insert_words[] = SQL(
				INSERT INTO _べんきょう_table_words(hiragana,kanji,translation,mnemonic,lexical_category)
				VALUES
					('ha','ha','ha','a',-1)comma
					('he','he','he','a',-1)comma
					('hi','hi','hi','a',-1)comma
					('ho','ho','ho','a',-1)comma
					('hu','hu','hu','a',-1);
			);
			sqlite3_exec(db, test_insert_words, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
#endif

	}

	RECT to_rect(const rect_i32& r) {//TODO(fran): get this out of here
		RECT res;
		res.left = r.left;
		res.top = r.top;
		res.right = r.right();
		res.bottom = r.bottom();
		return res;
	}

	//TODO(fran): there are two things I view as possibly necessary extra params: HWND wnd (of the gridview), void* user_extra
	void gridview_practices_renderfunc(HDC dc, rect_i32 r, void* element) {
		ProcState::practice_header* header = (decltype(header))element;
		//TODO(fran): actually the drawing code could be done at the end, and use the switches to get the string to render and the color of the border
		switch (header->type) {
		case decltype(header->type)::writing:
		{
			ProcState::practice_writing* data = (decltype(data))header;
			int w = r.w, h = r.h;
			RECT rc = to_rect(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT

			//Draw border
			HBRUSH border_br = data->answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
			int thickness = 3;
#if 0
			FillRectBorder(dc, rc, thickness, border_br,BORDERALL);
#elif 0
			{
				int roundedness = (int)ceilf(min((f32)w * .1f, (f32)h * .1f));
				//NOTE: border == pen, bk == brush
				HPEN border_pen = CreatePen(PS_SOLID, thickness, ColorFromBrush(border_br)); defer{ DeletePen(border_pen); };
				HPEN oldpen = SelectPen(dc, border_pen); defer{ SelectPen(dc,oldpen); };
				HBRUSH oldbr = SelectBrush(dc, GetStockBrush(HOLLOW_BRUSH)); defer{ SelectBrush(dc,oldbr); };
				RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, roundedness, roundedness);
			}
#else
			u16 degrees = 20;
			//TODO(fran): I dont quite love how this looks
			urender::RoundRectangleFill(dc, border_br, rc, degrees);
#endif
			InflateRect(&rc, -thickness, -thickness);

			//Draw text
			//TODO(fran): idk whether I want to show the question or what the user answered
#if 0
			utf16_str txt = *data->question;
#else
			utf16_str txt = data->user_answer;//the user will remember better what they wrote rather than what they saw
#endif
			urender::draw_text_max_coverage(dc, rc, txt, global::fonts.General, global::colors.ControlTxt, urender::txt_align::center);

		} break;
		default: Assert(0);
		}
	}

	//Page Search: Searchbox functions
	void searchbox_free_elements_func(ptr<void*> elements, void* user_extra){
		for(size_t i=0; i <elements.cnt;i++) free_any_str(elements[i]);
	}

	ptr<void*> searchbox_retrieve_search_options_func(utf16_str user_input, void* user_extra);

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra);

	void listbox_search_renderfunc(HDC dc, rect_i32 r, listbox::listbox_func_renderflags flags, void* element, void* user_extra) {
		int w = r.w, h = r.h;
		RECT rc = to_rect(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		utf16* txt = (decltype(txt))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk;
		if (flags.onSelected)bk_br = global::colors.ControlBkMouseOver;
		if(flags.onClicked) bk_br = global::colors.ControlBkPush;

		FillRect(dc, &rc, bk_br);

		//Draw text
		urender::draw_text(dc, rc, to_utf16_str(txt), global::fonts.General, global::colors.ControlTxt, bk_br, urender::txt_align::left, 3);
	}

	void searchbox_func_show_on_editbox(HWND editbox, void* element, void* user_extra) {
		utf16* txt = (decltype(txt))element;
		SendMessage(editbox, WM_SETTEXT_NO_NOTIFY, 0, (LPARAM)txt);
	}

	void add_controls(ProcState* state) {
		DWORD style_button_txt = WS_CHILD | WS_TABSTOP | BS_ROUNDRECT;
		DWORD style_button_bmp = WS_CHILD | WS_TABSTOP | BS_ROUNDRECT | BS_BITMAP;
		//---------------------Landing page----------------------:
		{
			auto& controls = state->controls.landingpage;

			controls.list.button_new = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_new, 100);
			//TODO(fran): more brushes, fore_push,... , border_mouseover,...
			button::set_brushes(controls.list.button_new, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);

			controls.list.button_practice = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_practice, 101);
			button::set_brushes(controls.list.button_practice, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);

			controls.list.button_search = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_search, 102);
			button::set_brushes(controls.list.button_search, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);

			controls.list.combo_languages = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			languages_setup_combobox(controls.list.combo_languages);
			SetWindowSubclass(controls.list.combo_languages, ComboProc, 0, 0);
			SendMessage(controls.list.combo_languages, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			//TODO(fran): more subdued colors for the lang combo, also no border, possibly also bold text, and right aligned

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------New word----------------------:
		{
			auto& controls = state->controls.new_word;

			controls.list.edit_hiragana = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_hiragana, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_hiragana, 120);

			controls.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.list.combo_lexical_category);
			//SendMessage(controls.list.combo_lexical_category, CB_SETCURSEL, 0, 0);
			SetWindowSubclass(controls.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.list.combo_lexical_category, 123);

			controls.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_kanji, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_kanji, 121);

			controls.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_translation, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_translation, 122);

			controls.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_mnemonic, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_mnemonic, 125);

			controls.list.button_save = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_save, 124);
			button::set_brushes(controls.list.button_save, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------Search----------------------:
		{
			auto& controls = state->controls.search;

			//controls.list.combo_search = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_ROUNDRECT /*TODO(fran):is CBS_HASSTRINGS necessary?*/
			//	, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			//ACC(controls.list.combo_search, 250); //TODO(fran): allow from multiple searching, eg if kanji detected search by kanji, if neither hiragana nor kanji search translation
			//SetWindowSubclass(controls.list.combo_search, TESTComboProc, 0, 0);
#if 0
			COMBOBOXINFO info = { sizeof(info) };
			GetComboBoxInfo(controls.list.combo_search, &info);
			//SetWindowSubclass(info.hwndItem, PrintMsgProc, 0, (DWORD_PTR)"Combo Edit");
			SetWindowSubclass(info.hwndList, PrintMsgProc, 0, (DWORD_PTR)"Combo List");
#endif
			//cstr listclass[100];
			//GetClassNameW(info.hwndList, listclass, 100); //INFO: class is ComboLBox, one interesting class windows has and it is hidden

			controls.list.searchbox_search = CreateWindowW(searchbox::wndclass, NULL, WS_CHILD | WS_TABSTOP | SRB_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			ACC(controls.list.searchbox_search, 251);
			searchbox::set_brushes(controls.list.searchbox_search, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled, global::colors.Img_Disabled);
			searchbox::set_user_extra(controls.list.searchbox_search, state->wnd);
			searchbox::set_function_free_elements(controls.list.searchbox_search, searchbox_free_elements_func);
			searchbox::set_function_retrieve_search_options(controls.list.searchbox_search, searchbox_retrieve_search_options_func);
			searchbox::set_function_perform_search(controls.list.searchbox_search, searchbox_func_perform_search);
			searchbox::set_function_show_element_on_editbox(controls.list.searchbox_search,searchbox_func_show_on_editbox);//TODO(fran)
			searchbox::set_function_render_listbox_element(controls.list.searchbox_search, listbox_search_renderfunc);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------Show word----------------------:
		{
			auto& controls = state->controls.show_word;

			controls.list.static_hiragana = CreateWindowW( static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_hiragana, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_kanji, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_kanji, 121);

			controls.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_translation, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_translation, 122);

			controls.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.list.combo_lexical_category);
			//SendMessage(controls.list.combo_lexical_category, CB_SETCURSEL, 0, 0);
			SetWindowSubclass(controls.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.list.combo_lexical_category, 123);

			controls.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_mnemonic, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			AWDT(controls.list.edit_mnemonic, 125);

			controls.list.static_creation_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_creation_date, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.static_last_shown_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_last_shown_date, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.static_score = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_score, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.button_modify = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_modify, 273);
			button::set_brushes(controls.list.button_modify, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);

			controls.list.button_delete = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_modify, 273);
			button::set_brushes(controls.list.button_delete, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.Img, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);
			SendMessage(controls.list.button_delete, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.bin);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice----------------------:
		{
			auto& controls = state->controls.practice;

			controls.list.static_word_cnt_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_word_cnt_title, 351);

			controls.list.static_word_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_word_cnt, TRUE, global::colors.ControlTxt, global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);
			
			controls.list.static_practice_cnt_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_practice_cnt_title, 352);

			controls.list.static_practice_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_practice_cnt, TRUE, global::colors.ControlTxt, global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);//TODO(fran): add border colors
			
			controls.list.static_accuracy_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_accuracy_title, 353);

			controls.list.score_accuracy = CreateWindowW(score::wndclass, NULL, WS_CHILD 
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			score::set_brushes(controls.list.score_accuracy, FALSE, global::colors.ControlBk, global::colors.Score_RingBk, global::colors.Score_RingFull, global::colors.Score_RingEmpty, global::colors.Score_InnerCircle);

			controls.list.button_start = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_start, 350);
			button::set_brushes(controls.list.button_start, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);
			
			controls.list.static_accuracy_timeline_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_accuracy_timeline_title, 354);

			controls.list.graph_accuracy_timeline = CreateWindowW(graph::wndclass, NULL, WS_CHILD | GP_CURVE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			graph::set_brushes(controls.list.graph_accuracy_timeline, FALSE, global::colors.Graph_Line, global::colors.Graph_BkUnderLine, global::colors.Graph_Bk, global::colors.Graph_Border);

			//TODO(fran): we may want to add smth else like a total number of words practiced


			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice writing----------------------:
		{
			auto& controls = state->controls.practice_writing;

			controls.list.static_test_word = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_test_word, TRUE, 0, global::colors.ControlBk, 0, 0, global::colors.ControlBk_Disabled, 0);
			//NOTE: text color will be set according to the type of word being shown

			controls.list.edit_answer = CreateWindowW(edit_oneline::wndclass, 0, WS_CHILD | ES_CENTER | WS_TABSTOP | WS_CLIPCHILDREN | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_brushes(controls.list.edit_answer, TRUE, 0, global::colors.ControlBk, global::colors.Img, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.Img_Disabled);
			//NOTE: text color and default text will be set according to the type of word that has to be written

			controls.list.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.list.edit_answer, 0, NULL, NULL);
			button::set_brushes(controls.list.button_next, TRUE, global::colors.ControlBk, global::colors.ControlBk, global::colors.Img, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);
			SendMessage(controls.list.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Review practice----------------------:
		{
			auto& controls = state->controls.review_practice;

			controls.list.static_review= CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_review, TRUE, global::colors.ControlTxt, global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);//TODO(fran): add border colors
			AWT(controls.list.static_review, 450);

			controls.list.gridview_practices = CreateWindowW(gridview::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
//#define TEST_GRIDVIEW
#ifndef TEST_GRIDVIEW
			gridview::set_brushes(controls.list.gridview_practices, TRUE, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);//TODO(fran): add border brushes
#else
			gridview::set_brushes(controls.list.gridview_practices, TRUE, global::colors.CaptionBk, 0, global::colors.CaptionBk_Inactive, 0);
#endif
			gridview::set_render_function(controls.list.gridview_practices, gridview_practices_renderfunc);

			controls.list.button_continue = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_continue, 451);
			button::set_brushes(controls.list.button_continue, TRUE, global::colors.Img, global::colors.ControlBk, global::colors.ControlTxt, global::colors.ControlBkPush, global::colors.ControlBkMouseOver);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
	}

	//returns a 2x2 configuration or 1x4 if there isnt enough max_w
	//w is used for horizontal centering only
	//grid_h and grid_w identify the size of one cell //TODO(fran): why dont I call it cell_w and _h?
	//TODO(fran): maybe creating a struct with 4 rect_i32 is less compiler expensive and less c++isy
	//NOTE: we dont handle the case when there's not enough height since it'll look too long if all the things are next to each other and you probably wont have the space anyway
	std::array<std::array<rect_i32,2>,2> create_grid_2x2(i32 grid_h, i32 grid_w, i32 grid_y, i32 grid_w_pad, i32 grid_h_pad, i32 max_w, i32 w) {
		std::array<std::array<rect_i32, 2>, 2> res;
		for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) { res[i][j].w = grid_w; res[i][j].h = grid_h; }
		bool two_by_two = (grid_w * 2 + grid_w_pad) <= max_w;

		if (two_by_two) {
			res[0][0].left = w / 2 - grid_w_pad / 2 - res[0][0].w;	res[0][0].top = grid_y;
			res[0][1].left = w / 2 + grid_w_pad / 2;					res[0][1].top = res[0][0].top;

			res[1][0].left = res[0][0].left;						res[1][0].top = res[0][0].bottom() + grid_h_pad;
			res[1][1].left = res[0][1].left;						res[1][1].top = res[1][0].top;
		}
		else {
			for (int next_y = grid_y, i = 0; i < 2; i++)
				for (int j = 0; j < 2; j++) {
					res[i][j].left = (w - res[i][j].w) / 2;
					res[i][j].top = next_y;
					next_y = res[i][j].bottom() + grid_h_pad;
				}
		}
		return res;
	}

	void resize_controls(ProcState* state) {
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTWIDTH(r);
		int h = RECTHEIGHT(r);
		int w_pad = (int)((float)w * .05f);//TODO(fran): hard limit for max padding
		int h_pad = (int)((float)h * .05f);
		
//#define _MyMoveWindow(wnd,xywh,repaint) MoveWindow(wnd, xywh##_x, xywh##_y, xywh##_w, xywh##_h, repaint)
#define MyMoveWindow(wnd,xywh,repaint) MoveWindow(wnd, xywh.x, xywh.y, xywh.w, xywh.h, repaint)

#define _MyMoveWindow2(wnd,xywh,repaint) MoveWindow(wnd, xywh.x, xywh.y + y_offset, xywh.w, xywh.h, repaint)
#define MyMoveWindow_offset _MyMoveWindow2

		switch (state->current_page) {
		case ProcState::page::landing: 
		{
			auto& controls = state->controls.landingpage;
			//One button on top of the other vertically, all buttons must be squares
			//TODO(fran): we could, at least to try how it looks, to check whether the wnd is longer on w or h and place the controls vertically or horizontally next to each other
			int wnd_cnt = ARRAYSIZE(controls.all) -1 /*subtract the lang combobox*/;
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control

			int max_h = h - pad_cnt * h_pad;
			int max_w = w - 2 * w_pad;
			int wnd_h = clamp(0, max_h / wnd_cnt, max_w);
			int wnd_w = wnd_h;
			int wnd_x = (w - wnd_w) / 2;
			int h_step = (h - (wnd_h*wnd_cnt)) / pad_cnt;

			rect_i32 button_new;
			button_new.x = wnd_x;
			button_new.y = h_step;
			button_new.w = wnd_h;
			button_new.h = wnd_h;

			rect_i32 button_practice;
			button_practice.x = wnd_x;
			button_practice.y = button_new.bottom() + h_step;
			button_practice.w= wnd_h;
			button_practice.h= wnd_h;

			rect_i32 button_search;
			button_search.x = wnd_x;
			button_search.y = button_practice.bottom() + h_step;
			button_search.w= wnd_h;
			button_search.h= wnd_h;

			rect_i32 combo_languages;
			combo_languages.h = wnd_h;
			combo_languages.y = h_pad / 2;
			combo_languages.w = min(distance(w,button_new.right()+w_pad/2), avg_str_dim((HFONT)SendMessage(controls.list.combo_languages, WM_GETFONT, 0, 0), 20).cx);
			combo_languages.x = w - combo_languages.w - w_pad / 2;

			MyMoveWindow(controls.list.button_new, button_new,FALSE);
			MyMoveWindow(controls.list.button_practice, button_practice,FALSE);
			MyMoveWindow(controls.list.button_search, button_search,FALSE);
			MyMoveWindow(controls.list.combo_languages, combo_languages,FALSE);
		} break;
		case ProcState::page::new_word:
		{
			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned
			auto& controls = state->controls.new_word;

			//int wnd_cnt = ARRAYSIZE(controls.all);
			//int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
#if 0
			int start_y = (h - (pad_cnt-1 /*we only have 1 real pad for lex_categ*/) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): centering aint quite right
#else
			int start_y = 0;
#endif

			rect_i32 edit_hiragana;
			edit_hiragana.y = start_y;
			edit_hiragana.h = wnd_h;
			edit_hiragana.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_hiragana, WM_GETFONT, 0, 0), 30).cx);
			edit_hiragana.x = (w- edit_hiragana.w)/2;

			rect_i32 cb_lex_categ;
			cb_lex_categ.w = (i32)((f32)edit_hiragana.w * .7f);
			cb_lex_categ.x = (w- cb_lex_categ.w)/2;//TODO(fran): placement doesnt look too natural nor good; maybe if I centered the text of the cb?
			cb_lex_categ.y = edit_hiragana.bottom() + h_pad/2;
			cb_lex_categ.h = edit_hiragana.h;//TODO(fran): for some reason comboboxes are always a little smaller than you ask, find out how to correctly correct that
			
			rect_i32 edit_kanji;
			edit_kanji.y = cb_lex_categ.bottom() + h_pad/2;
			edit_kanji.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_kanji, WM_GETFONT, 0, 0), 30).cx);
			edit_kanji.h = wnd_h;
			edit_kanji.x = (w- edit_kanji.w)/2;

			rect_i32 edit_translation;
			edit_translation.y = edit_kanji.bottom() + h_pad;
			edit_translation.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_translation, WM_GETFONT, 0, 0), 30).cx);
			edit_translation.h = wnd_h;
			edit_translation.x = (w- edit_translation.w)/2;

			rect_i32 edit_mnemonic;
			edit_mnemonic.y = edit_translation.bottom() + h_pad;
			edit_mnemonic.w = max_w;
			edit_mnemonic.h = wnd_h;
			edit_mnemonic.x = (w- edit_mnemonic.w)/2;

			rect_i32 btn_save;
			btn_save.w = 70;
			btn_save.h = wnd_h;
			btn_save.y = edit_mnemonic.bottom() + h_pad;
			btn_save.x = (w - btn_save.w) / 2;

			rect_i32 bottom_most_control = btn_save;

			int used_h = bottom_most_control.bottom();// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.list.edit_hiragana, edit_hiragana, FALSE);
			MyMoveWindow_offset(controls.list.combo_lexical_category, cb_lex_categ, FALSE);
			MyMoveWindow_offset(controls.list.edit_kanji, edit_kanji, FALSE);
			MyMoveWindow_offset(controls.list.edit_translation, edit_translation, FALSE);
			MyMoveWindow_offset(controls.list.edit_mnemonic, edit_mnemonic, FALSE);
			MyMoveWindow_offset(controls.list.button_save, btn_save, FALSE);

		} break;
		case ProcState::page::practice: 
		{
			auto& controls = state->controls.practice;

			//Stats first, then start button
			//For stats I imagine a 2x2 "grid":	first row has "word count" and "times practiced"
			//									second row has "score" and a chart of accuracy for last 30 days for example


			//int wnd_cnt = ARRAYSIZE(controls.all);
			//int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
#if 0
			int start_y = (h - (pad_cnt) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): not good, what we want is the total height of all windows combined, an use that to center correctly, eg start_y = (h-total_h)/2; and we offset everything by start_y at the end
#else
			int start_y = 0;//We start from 0 and offset once we know the sizes and positions for everything
#endif

			int grid_h = wnd_h * 4;
			int grid_w = grid_h * 16 / 9;
			auto grid = create_grid_2x2(grid_h, grid_w, start_y, w_pad / 2, h_pad / 2, max_w, w);
			
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
			
			MyMoveWindow_offset(controls.list.static_word_cnt_title, static_word_cnt_title, FALSE);
			MyMoveWindow_offset(controls.list.static_word_cnt, static_word_cnt, FALSE);
			MyMoveWindow_offset(controls.list.static_practice_cnt_title, static_practice_cnt_title, FALSE);
			MyMoveWindow_offset(controls.list.static_practice_cnt, static_practice_cnt, FALSE);
			MyMoveWindow_offset(controls.list.static_accuracy_title, static_accuracy_title, FALSE);
			MyMoveWindow_offset(controls.list.score_accuracy, score_accuracy, FALSE);
			MyMoveWindow_offset(controls.list.static_accuracy_timeline_title, static_accuracy_timeline_title, FALSE);
			MyMoveWindow_offset(controls.list.graph_accuracy_timeline, graph_accuracy_timeline, FALSE);
			MyMoveWindow_offset(controls.list.button_start, button_start, FALSE);

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

			rect_i32 button_next;//child inside edit_answer
			button_next.y = 1; //the button is inside the edit box (and past the border) //TODO(fran): we should ask the parent for its border size
			button_next.h = edit_answer.h - 2;
			button_next.w = min(button_next.h, max(0, edit_answer.w-4/*avoid covering rounded borders*/));
			button_next.x = edit_answer.w - button_next.w - 2;//TODO(fran): if the style of the edit box parent is  ES_ROUNDRECT we gotta subtract one more, in this case we went from -1 to -2

			rect_i32 bottom_most_control = edit_answer;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			MyMoveWindow_offset(controls.list.static_test_word, static_test_word, FALSE);
			MyMoveWindow_offset(controls.list.edit_answer, edit_answer, FALSE);
			MyMoveWindow(controls.list.button_next, button_next, FALSE);

		} break;
		case ProcState::page::review_practice:
		{
			auto& controls = state->controls.review_practice;

			int wnd_h = 30;
			int bigwnd_h = wnd_h * 3;
			int max_w = w - w_pad * 2;
			int start_y = 0;

			rect_i32 static_review;
			static_review.y = start_y;
			static_review.h = wnd_h*2;
			static_review.w = max_w;
			static_review.x = (w - static_review.w) / 2;

			rect_i32 gridview_practices;
			gridview_practices.y = static_review.bottom() + h_pad;

			gridview::element_dimensions gridview_practices_dims;
			gridview_practices_dims.border_pad_y = 3;
			gridview_practices_dims.inbetween_pad = { 5,5 };
			gridview_practices_dims.element_dim = { bigwnd_h,bigwnd_h };
			gridview::set_dimensions(controls.list.gridview_practices, gridview_practices_dims);

			SIZE gridview_practices_wh;
			{
				//TODO(fran): this is the worst code I've written in quite a while, this and the gridview code that was needed need a revision
				const size_t elems_per_row = 5;
				size_t curr_elem_cnt = gridview::get_elem_cnt(controls.list.gridview_practices);
				i32 full_w = gridview::get_dim_for_elemcnt_elemperrow(controls.list.gridview_practices, curr_elem_cnt, elems_per_row).cx;

				i32 real_w = min(w - w_pad * 2, full_w);

				i32 max_h = gridview::get_dim_for_elemcnt_elemperrow(controls.list.gridview_practices, elems_per_row*4, elems_per_row).cy;//4 rows

				i32 full_h = gridview::get_dim_for_elemcnt_w(controls.list.gridview_practices,curr_elem_cnt, real_w).cy;

				//SIZE full_dim = gridview::get_dim_for(controls.list.gridview_practices, row_cnt, elems_per_row);
				gridview_practices_wh = { real_w,min(full_h,max_h)};
			}

			gridview_practices.h = gridview_practices_wh.cy;//TODO(fran): should get smaller if the controls below it cant fit, as small as to only allow 1 row to be visible
			gridview_practices.w = gridview_practices_wh.cx;
			gridview_practices.x = (w - gridview_practices.w) / 2;

			rect_i32 button_continue;
			button_continue.h = wnd_h;
			button_continue.w = 70;
			button_continue.x = (w - button_continue.w) / 2;
			button_continue.y = gridview_practices.bottom() + h_pad;

			rect_i32 bottom_most_control = button_continue;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.list.static_review, static_review, FALSE);
			MyMoveWindow_offset(controls.list.gridview_practices, gridview_practices, FALSE);
			MyMoveWindow_offset(controls.list.button_continue, button_continue, FALSE);

		} break;
		case ProcState::page::search: 
		{
			auto& controls = state->controls.search;
			int wnd_h = 30;
			int max_w = w - w_pad * 2;

			//rect_i32 cb_search;
			//cb_search.x = w_pad;
			//cb_search.y = h_pad;
			//cb_search.h = 40;//TODO(fran): cbs dont respect this at all, they set a minimum h by the font size I think
			//cb_search.w = max_w;

			rect_i32 searchbox_search;
			searchbox_search.w = max_w;
			searchbox_search.x = (w- searchbox_search.w) /2;
			searchbox_search.h = wnd_h;
			searchbox_search.y = h_pad;

			//MyMoveWindow(controls.list.combo_search, cb_search, FALSE);
			MyMoveWindow(controls.list.searchbox_search, searchbox_search, FALSE);
			searchbox::set_listbox_dimensions(controls.list.searchbox_search, listbox::dimensions().set_border_thickness(1).set_element_h(wnd_h));

		} break;
		case ProcState::page::show_word: 
		{
			auto& controls = state->controls.show_word;

			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned, non user editable controls go below the rest

			//int wnd_cnt = ARRAYSIZE(controls.all) - 2;//subtract the controls that go next to one another
			//int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int bigwnd_h = wnd_h * 4;
#if 0
			int start_y = (h - (pad_cnt - 1 /*we only have 1 real pad for lex_categ*/) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): centering aint quite right
#else
			int start_y = 0;
#endif

			rect_i32 static_hiragana;
			static_hiragana.x = w_pad;
			static_hiragana.y = start_y;
			static_hiragana.h = bigwnd_h;
			static_hiragana.w = max_w; //TODO(fran): we may want to establish a max_w that's more fixed, as a clamp, instead of continually increasing as the wnd width does

			rect_i32 cb_lex_categ;
			cb_lex_categ.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.combo_lexical_category, WM_GETFONT, 0, 0), 20).cx);
#if 0
			cb_lex_categ.x = min( w - w_pad - cb_lex_categ.w, w/2 + cb_lex_categ.w);//TODO(fran): doesnt look good, maybe just center it
#else
			cb_lex_categ.x = (w - cb_lex_categ.w) / 2;
#endif
			cb_lex_categ.y = static_hiragana.bottom() + h_pad / 2;
			cb_lex_categ.h = wnd_h;//TODO(fran): for some reason comboboxes are always a little smaller than you ask, find out how to correctly correct that

			rect_i32 edit_kanji;
			edit_kanji.y = cb_lex_categ.bottom() + h_pad / 2;
			edit_kanji.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_kanji, WM_GETFONT, 0, 0), 30).cx);
			edit_kanji.x = (w- edit_kanji.w)/2;
			edit_kanji.h = wnd_h;

			rect_i32 edit_translation;
			edit_translation.y = edit_kanji.bottom() + h_pad;
			edit_translation.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_translation, WM_GETFONT, 0, 0), 30).cx);
			edit_translation.x = (w - edit_translation.w) / 2;
			edit_translation.h = wnd_h;

			rect_i32 edit_mnemonic;
			edit_mnemonic.w = max_w;
			edit_mnemonic.x = (w- edit_mnemonic.w)/2;
			edit_mnemonic.y = edit_translation.bottom() + h_pad;
			edit_mnemonic.h = wnd_h;

			rect_i32 last_user = edit_mnemonic;

			auto stats_grid = create_grid_2x2(
				wnd_h, avg_str_dim((HFONT)SendMessage(controls.list.static_creation_date, WM_GETFONT, 0, 0), 25).cx
				, last_user.bottom() + h_pad, w_pad/2, h_pad/2, max_w, w);

			rect_i32 static_creation_date = stats_grid[0][0];

			rect_i32 static_last_shown_date = stats_grid[0][1];

			rect_i32 static_score = stats_grid[1][0];

			rect_i32 last_cell = static_score;//TODO(fran): we still got one more space at [1][1]

			rect_i32 btn_modify;
			btn_modify.w = 70;
			btn_modify.h = wnd_h;
			btn_modify.y = last_cell.bottom() + h_pad;
			btn_modify.x = (w - btn_modify.w) / 2;

			rect_i32 btn_delete;
			btn_delete.h = wnd_h;
			btn_delete.w = btn_delete.h;
#if 0 //TODO(fran): which one to go with?
			btn_delete.x = btn_modify.right() + w_pad*;/*TODO(fran): clamp to not go beyond w*/
			btn_delete.y = btn_modify_y;
#else
			btn_delete.x = (w - btn_delete.w)/2;
			btn_delete.y = btn_modify.bottom() + h_pad;
#endif

			rect_i32 bottom_most_control = btn_delete;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.list.static_hiragana, static_hiragana, FALSE);
			MyMoveWindow_offset(controls.list.combo_lexical_category, cb_lex_categ, FALSE);
			MyMoveWindow_offset(controls.list.edit_kanji, edit_kanji, FALSE);
			MyMoveWindow_offset(controls.list.edit_translation, edit_translation, FALSE);
			MyMoveWindow_offset(controls.list.edit_mnemonic, edit_mnemonic, FALSE);

			//Dates one next to the other, the same for times_... related objs
			MyMoveWindow_offset(controls.list.static_creation_date, static_creation_date, FALSE);
			MyMoveWindow_offset(controls.list.static_last_shown_date, static_last_shown_date, FALSE);
			MyMoveWindow_offset(controls.list.static_score, static_score, FALSE);
			MyMoveWindow_offset(controls.list.button_modify, btn_modify, FALSE);
			MyMoveWindow_offset(controls.list.button_delete, btn_delete, FALSE);

		} break;
		default:Assert(0);
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
		case ProcState::page::practice_writing: for (auto ctl : state->controls.practice_writing.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::search: for (auto ctl : state->controls.search.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::show_word: for (auto ctl : state->controls.show_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::review_practice: for (auto ctl : state->controls.review_practice.all) ShowWindow(ctl, ShowWindow_cmd); break;
		default:Assert(0);
		}
	}

	void set_current_page(ProcState* state, ProcState::page new_page) {
		show_page(state, state->current_page,SW_HIDE);
		state->current_page = new_page;
		resize_controls(state);
		show_page(state, state->current_page,SW_SHOW);

		switch (new_page) {
		case decltype(new_page)::practice_writing:
		{
			//TODO(fran): idk where to put this, I'd put it in preload but preload doesnt guarantee we're switching pages, even though it's mostly true currently
			auto& controls = state->controls.practice_writing;
			//SetFocus(controls.list.edit_answer);//TODO(fran): problem is now we cant see in which lang we have to write, we have two options, either give the edit control an extra flag of show placeholder even if cursor is visible; option 2 setfocus to us (main wnd) wait for the user to press something, redirect the msg to the edit box and setfocus. INFO: wanikani shows the placeholder and the caret at the same time, lets do that
		} break;
		}
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
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
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
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
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

			_get_combo_sel_idx_as_str(page.list.combo_lexical_category, w.attributes.lexical_category);

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

	ptr<void*> searchbox_retrieve_search_options_func(utf16_str user_input, void* user_extra) {
		ptr<void*> res{ 0 };
		ProcState* state = get_state((HWND)user_extra);
		if (user_input.sz && state) {
			auto search_res = search_word_matches(state->settings->db, user_input.str, 8); defer{ free(search_res.matches);/*free the dinamically allocated array*/ };
			//TODO(fran): search_word_matches should return a ptr
			res.alloc(search_res.cnt);
			for (size_t i = 0; i < search_res.cnt; i++)
				res[i] = search_res.matches[i];
		}
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
					std::tm* time = std::localtime(&temp);//UNSAFE: not multithreading safe, returns a pointer to an internal unique object
					res->word.application_defined.attributes.last_shown_date = alloc_any_str(30 * sizeof(utf16));
					wcsftime((utf16*)res->word.application_defined.attributes.last_shown_date.str, 30, L"%Y-%m-%d", time);
				}
			}
			catch (...) {}

			try {//Format created_date (this would be easier to do in the query but I'd have to remove the macros)
				i64 unixtime = std::stoll((utf16*)res->word.application_defined.attributes.creation_date.str, nullptr, 10);
				free_any_str(res->word.application_defined.attributes.creation_date.str);
				
			   	std::time_t temp = unixtime;
			   	std::tm* time = std::localtime(&temp);//UNSAFE: not multithreading safe, returns a pointer to aninternal unique object
			   	res->word.application_defined.attributes.creation_date = alloc_any_str(30 * sizeof(utf16));
			   	wcsftime((utf16*)res->word.application_defined.attributes.creation_date.str, 30, L"%Y-%m-%d", time);
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

	void clear_practices_vector(decltype(decltype(ProcState::pagestate)::practice_review_state::practices)& practices) {
		for (auto p : practices) {
			switch (p->type) {
			case decltype(p->type)::writing:
			{
				ProcState::practice_writing* data = (decltype(data))p;
				if (data->user_answer.str) { free_any_str(data->user_answer.str); data->user_answer.sz = 0; }
				for (auto& _ : data->practice->word.all)free_any_str(_.str);
				free(data->practice);
			} break;
			default: Assert(0);
			}
			free(p);//the object, no matter what type, can always be freed the same way since p is the ptr returned by the allocator
		}
		practices.clear();
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
//#define PRACTICE_TEST_STATS
#ifndef PRACTICE_TEST_STATS
			SendMessage(controls.list.score_accuracy, SC_SETSCORE, f32_to_WPARAM(stats->accuracy()), 0);
			SendMessage(controls.list.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->word_cnt).c_str());
			SendMessage(controls.list.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->times_practiced).c_str());
			//TODO(fran): timeline, we'll probably need to store that as blob or text in the db, this is were mongodb would be nice, just throw a js obj for each timepoint
			//TODO(fran): if the timeline is empty we should simply put the current accuracy, or leave it empty idk
			graph::set_points(controls.list.graph_accuracy_timeline, stats->accuracy_timeline.mem, stats->accuracy_timeline.cnt);
			graph::graph_dimensions grid_dims;
			grid_dims.set_top_point(100);
			grid_dims.set_bottom_point(0);
			grid_dims.set_viewable_points_range(0, stats->accuracy_timeline.cnt);
			graph::set_dimensions(controls.list.graph_accuracy_timeline, grid_dims);
			//graph::set_top_point(controls.list.graph_accuracy_timeline, 100);
			//graph::set_bottom_point(controls.list.graph_accuracy_timeline, 0);
			//graph::set_viewable_points_range(controls.list.graph_accuracy_timeline, 0, stats->accuracy_timeline.cnt);
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
		case ProcState::page::practice_writing:
		{
			//TODO(fran): shouldnt preload_page also call clear_page?
			practice_writing_word* practice = (decltype(practice))data;
			auto controls = state->controls.practice_writing;
			//store data for future proof checking
			state->pagestate.practice_writing.practice = practice;//TODO(fran): free the object this points to and the word inside the object

			utf16* test_word{0};//NOTE: compiler cant know that these guys will always be initialized so I gotta zero them
			HBRUSH test_word_br{0};
			str answer_placeholder;//NOTE: using string so the object doesnt get destroyed inside the switch statement
			HBRUSH answer_br{0};

			str answer_hiragana = str(L"こたえ");

			switch (practice->practice_type) {
			case decltype(practice->practice_type)::translate_hiragana_to_translation:
			{
				test_word = (utf16*)practice->word.attributes.hiragana.str;
				test_word_br = global::colors.hiragana;
				//TODO(fran): idk if I should put "translation" or "answer", and for hiragana "hiragana" or "こたえ" (meaning answer), and "kanji" or "答え"
				answer_placeholder = RS(380);
				answer_br = global::colors.translation;
			} break;
			case decltype(practice->practice_type)::translate_kanji_to_hiragana:
			{
				test_word = (utf16*)practice->word.attributes.kanji.str;
				test_word_br = global::colors.kanji;

				answer_placeholder = answer_hiragana;
				answer_br = global::colors.hiragana;
			} break;
			case decltype(practice->practice_type)::translate_kanji_to_translation:
			{
				test_word = (utf16*)practice->word.attributes.kanji.str;
				test_word_br = global::colors.kanji;

				answer_placeholder = RS(380);
				answer_br = global::colors.translation;
			} break;
			case decltype(practice->practice_type)::translate_translation_to_hiragana:
			{
				test_word = (utf16*)practice->word.attributes.translation.str;
				test_word_br = global::colors.translation;

				answer_placeholder = answer_hiragana;
				answer_br = global::colors.hiragana;
			} break;
			default:Assert(0);
			}

			//TODO(fran): setup control colors, text and default text, etc
			SendMessageW(controls.list.static_test_word, WM_SETTEXT, 0, (LPARAM)test_word);
			static_oneline::set_brushes(controls.list.static_test_word, TRUE, test_word_br, 0, 0, 0, 0, 0);
			
			edit_oneline::set_brushes(controls.list.edit_answer, TRUE, answer_br, 0, 0, 0, 0, 0);
			SendMessageW(controls.list.edit_answer, WM_SETDEFAULTTEXT, 0, (LPARAM)answer_placeholder.c_str());

		} break;
		case ProcState::page::review_practice:
		{
			std::vector<ProcState::practice_header*>* practices = (decltype(practices))data;
			auto controls = state->controls.review_practice;

			//free current elements before switching, I think this is the best place to do it
			clear_practices_vector(state->pagestate.practice_review.practices);

			state->pagestate.practice_review.practices = std::move(*practices);
			size_t practices_cnt = state->pagestate.practice_review.practices.size(); Assert(practices_cnt != 0);
			void** practices_data = (void**)malloc(sizeof(void*) * practices_cnt); defer{ free(practices_data); };
			for (size_t i = 0; i < practices_cnt; i++) practices_data[i] = state->pagestate.practice_review.practices[i];
			gridview::set_elements(controls.list.gridview_practices, practices_data, practices_cnt);

		} break;
		//TODO(fran): for kanji practice it'd be nice to add a drawing feature, I give you the translation and you draw the kanji
		default: Assert(0);
		}
	}

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra) {
		ProcState* state = get_state((HWND)user_extra);
		if (state) {
			//TODO(fran): differentiate implementation for is_element true or false once *element isnt always a utf16*
			utf16* search;
			
			if (is_element) search = (decltype(search))element;
			else search = ((utf16_str*)element)->str;

			get_word_res res = get_word(state->settings->db, search); defer{ free_get_word(res); };
			if (res.found) {
				preload_page(state, ProcState::page::show_word, &res.word);//NOTE: considering we no longer have separate pages this might be a better idea than sending the struct at the time of creating the window
				store_previous_page(state, state->current_page);
				set_current_page(state, ProcState::page::show_word);
			}
			else {
				int ret = MessageBoxW(state->nc_parent, RCS(300), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
				if (ret == IDYES) {
					learnt_word new_word{ 0 };
					new_word.attributes.hiragana = { search, (cstr_len(search)+1)*sizeof(*search) };
					preload_page(state, ProcState::page::new_word, &new_word);
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::new_word);
				}
			}
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
		for (int i = word->pk_count/*dont update pk columns*/; i < ARRAYSIZE(columns); i++)
			update_word += columns[i] + "=" + values[i] + ",";
		update_word.pop_back();//you probably need to remove the trailing comma as always
		update_word += " WHERE ";
		for (int i = 0/*match by pk columns*/; i < word->pk_count; i++)
			update_word += columns[i] + " LIKE " + values[i] + (((i+1) != word->pk_count)? "AND" : "");
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
			_get_combo_sel_idx_as_str(page.list.combo_lexical_category, w.attributes.lexical_category);

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

//Algorithms to decide which words to show:
//Baseline:
//1. least shown: use times_shown and pick the 30 of lowest value, then reduce that at random down to 10.
//2. least recently shown: use last_shown_date and pick the 30 oldest dates (lowest numbers), then reduce that at random down to 5
//TODO(fran): When the user asks explicitly for a word to be remembered we reset the times_shown count to 0 and maybe also set last_shown_date to the oldest possible date, and do the same for words the user gets wrong on the practices.
//Advantages: assuming a 5 words a day workflow the user will see the new words off and on for a week, old words are guaranteed to be shown from time to time and in case the user fails at one of them it will go to the front of our priority queue, not so in the case they get it right, then it'll probably dissapear for quite a while, the obvious problem with this system is the lack of use of words that land in the middle of the two filters, we may want to maintain new words appearing for more than a week, closer to a month, one solution would be to pick 60 or 90 words
//Extra: we could allow for specific practices, for example, replacing the "note besides the bed" in the sense that the user can tells us I want to practice specifically what I added yesterday or the day I last added something new
//Super extra: when remembering a word this feature should also discriminate between each possible translation, say one jp word has a noun and an adjective registered, then both should work as separate entities, aka the user can ask for one specific translation to be put on the priority queue, we got a "one to many" situation


		std::string columns = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_columns);
		columns.pop_back(); //We need to remove the trailing comma

		//TODO(fran): we probably want to speed up this searches via adding indexes to times_shown and last_shown_date

		std::string select_practice_word =
			" SELECT * FROM "
			"("
				//1. least shown: use times_shown and pick the 30 of lowest value, then reduce that at random down to 10.
				" SELECT * FROM " //IMPORTANT INFO: UNION is a stupid operator, if you also want to _previously_ "order by" your selects you have to "hide" them inside another select and parenthesis
				"("
					" SELECT * FROM "
						"(" " SELECT " + columns + " FROM " べんきょう_table_words " ORDER BY times_shown ASC LIMIT 30" ")"
					"ORDER BY RANDOM() LIMIT 10" //TODO(fran): I dont know how random this really is, probably not good enough
				")"
				" UNION " //TODO(fran): UNION ALL is faster cause it doesnt bother to remove duplicates
				//2. least recently shown: use last_shown_date and pick the 30 oldest dates (lowest numbers), then reduce that at random down to 5
				" SELECT * FROM "
				"("
					" SELECT * FROM "
						"(" " SELECT " + columns + " FROM " べんきょう_table_words " ORDER BY last_shown_date ASC LIMIT 30"  ")"
					"ORDER BY RANDOM() LIMIT 5"
				")"
			")"
			"ORDER BY RANDOM() LIMIT 1;"
		;//Now that's why sql is a piece of garbage, look at the size of that query!! for such a stupid two select + union operation, if they had given you the obvious option of storing the select on a variable, then store the other select on a variable and then union both this would be so much more readable, comprehensible and easier to write

		auto parse_select_practice_word_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			learnt_word* res = (decltype(res))extra_param;
			//NOTE: we should already know the order of the columns since they should all be specified in their correct order in the select
			//NOTE: we should always get only one result

			#define load_practice_word_member(type,name,...) if(results) res->attributes.name = convert_utf8_to_utf16(*results, (int)strlen(*results) + 1); results++;
			_foreach_learnt_word_member(load_practice_word_member);

			return 0;
		};

		char* errmsg;
		sqlite3_exec(db, select_practice_word.c_str(), parse_select_practice_word_result, &res, &errmsg);
		sqlite_exec_runtime_check(errmsg);

		return res;
	}

	bool has_hiragana(const learnt_word* word) {
		bool res;
		res = word && word->attributes.hiragana.str && *((utf16*)word->attributes.hiragana.str);//TODO(fran): learnt_word should have a defined str type, we cant be guessing here
		return res;
	}

	bool has_translation(const learnt_word* word) {
		bool res;
		res = word && word->attributes.translation.str && *((utf16*)word->attributes.translation.str);//TODO(fran): learnt_word should have a defined str type, we cant be guessing here
		return res;
	}

	bool has_kanji(const learnt_word* word) {
		bool res;
		res = word && word->attributes.kanji.str && *((utf16*)word->attributes.kanji.str);//TODO(fran): learnt_word should have a defined str type, we cant be guessing here
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
			practice_writing_word* data = (decltype(data))malloc(sizeof(*data));//TODO(fran): MEMLEAK practice_writing page will take care of freeing this once the page completes its practice
			data->word = std::move(practice_word);

			std::vector<practice_writing_word::type> practice_types;
			bool hiragana = has_hiragana(&data->word), translation = has_translation(&data->word), kanji = has_kanji(&data->word);
			practice_types.push_back(practice_writing_word::type::translate_hiragana_to_translation);//we always add a default to make sure there can never be no items
			if (hiragana && translation) practice_types.push_back(practice_writing_word::type::translate_translation_to_hiragana);
			if (hiragana && kanji) practice_types.push_back(practice_writing_word::type::translate_kanji_to_hiragana);
			if (kanji && translation) practice_types.push_back(practice_writing_word::type::translate_kanji_to_translation);

			data->practice_type = practice_types[random_between(0, (i32)(practice_types.size() - 1))];
			res.page = ProcState::page::practice_writing;
			res.data = data;
		} break;
		default: Assert(0);
		}
		return res;
	}

	void init_cpp_objects(ProcState* state) {
		state->pagestate.practice_review.practices = decltype(state->pagestate.practice_review.practices)();
		//NOTE: since I know from the start the number of practices I could also allocate and array, but there's really no point, simpler is this since it's not at all performance critical code
	}

	//IMPORTANT: handling the review page, we'll have two vectors, one floating in space to which each practice level will add to each time it completes, once the whole practice is complete we'll preload() review_practice with std::move(this vector), this page has it's own vector which at this point will be emptied and its contents freed and replaced with the new vector (the floating in space one). This guarantees the review page is always on a valid state. //TODO(fran): idk if std::move works when I actually need to send the vector trough a pointer, it may be better to allocate an array and send it together with its size (which makes the vector pointless all together), I do think allocating arrays is beter, we simply alloc when the start button is pressed on the practice page and at the end of the practice send that same pointer to review. The single annoying problem is the back button, solution: we'll go simple for now, hide the back button until the practice is complete, the review page will have the back button active and that will map back to the practice page (at least at the time Im writing this). This way we guarantee valid states for everything, the only semi problem would be if the user decides to close the application, in that case we could, if we wanted, ask for confirmation depending on the page, but once the user says yes we care no more about memory, yes there will be some mem leaks but at that point it no longer matters since that memory will be removed automatically by the OS

	//TODO(fran): in practice_writing: I like the idea of putting a bar in the middle between the question and answer and that it lights up when the user responds, it either goes green and it's text says "Correct!" or red and text "Incorrect: right answer was [...]". The good thing about this is that I dont need to add extra controls for the review page, I can simply reconstruct the exact same page and there's already a spot to indicate the correction, would be the same with a multiple choice, eg 3 words user clicks wrong answer and it lights up red and the correct one lights up green, the thing with this is that I'd need the text colors to be fixed in order to not be opaqued by the new red/green light, that's kind of annoying but I dont see an easy solution. Sidenode: actually wanikani doesnt have a separate bar, it chages the color of the edit box


	void reset_page(ProcState* state, ProcState::page page) {
		//NOTE: one solution here would be to destroy all the controls remove_controls(page) and then call add_controls(page)
		switch (page) {
		case decltype(page)::new_word:
		{
			//NOTE: the problem here is that its not enough to settext to null to everyone, that can be done fairly easily implementing some WM_RESET msg, but there are some controls that shouldnt be reset like the buttons, we'd need to implement a .reseteable() or somehow via union magic or smth that gives us only the controls that should be reset
			auto& controls = state->controls.new_word;
			_clear_combo_sel(controls.list.combo_lexical_category);
			_clear_edit(controls.list.edit_hiragana);
			_clear_edit(controls.list.edit_kanji);
			_clear_edit(controls.list.edit_mnemonic);
			_clear_edit(controls.list.edit_translation);
		} break;
		case decltype(page)::practice_writing:
		{
			auto& controls = state->controls.practice_writing;

			_clear_static(controls.list.static_test_word);
			_clear_edit(controls.list.edit_answer);
			//TODO(fran): I really need to destroy the controls and call add_controls, I cant be restoring colors here when that's already done there
		} break;
		default:Assert(0);
		}
	}

	void user_stats_increment_times_practiced(sqlite3* db) {
		utf8 update_increment_times_practiced[] = SQL(
			UPDATE _べんきょう_table_user SET times_practiced = times_practiced + 1;
		);
		utf8* errmsg;
		sqlite3_exec(db, update_increment_times_practiced, 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	void _next_practice_level(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
		ProcState* state = get_state(hwnd);
		KillTimer(state->wnd, anim_id);

		state->practice_cnt--;
		if (state->practice_cnt > 0) {
			practice_data practice = prepare_practice(state);//get a random practice
			reset_page(state, practice.page);
			preload_page(state, practice.page, practice.data);//load the practice
			set_current_page(state, practice.page);//go practice!
		}
		else {
			//TODO(fran): increase the practice counter on the db
			user_stats_increment_times_practiced(state->settings->db);
			preload_page(state, ProcState::page::review_practice,&state->multipagestate.temp_practices);
			set_current_page(state, ProcState::page::review_practice);
		}
	}

	//decreases the practice counter and loads/sets a new practice level or goes to the review page if the practice is over
	void next_practice_level(ProcState* state, u32 ms_delay = USER_TIMER_MAXIMUM) {
		u32 delay = ms_delay != USER_TIMER_MAXIMUM ? ms_delay : 500;
		SetTimer(state->wnd, TIMER_next_practice_level, delay, _next_practice_level);
	}

	//NOTE word should be in utf16
	void word_update_last_shown_date(sqlite3* db,const learnt_word& word) {
		using namespace std::string_literals;
		utf8_str hiragana = (utf8_str)convert_utf16_to_utf8((utf16*)word.attributes.hiragana.str, (int)word.attributes.hiragana.sz); defer{ free_any_str(hiragana.str); };

		std::string update_last_shown_date = SQL(
			UPDATE _べんきょう_table_words SET last_shown_date = strftime('%s', 'now') WHERE hiragana =
		) + "'"s + std::string((utf8*)hiragana.str) + "'" ";";

		utf8* errmsg;
		sqlite3_exec(db, update_last_shown_date.c_str(), 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	//NOTE word should be in utf16
	void word_increment_times_shown(sqlite3* db, const learnt_word& word) {
		using namespace std::string_literals;
		//TODO(fran): having to convert the hiragana is pretty annoying and pointless, rowid or similar looks much better
		utf8_str hiragana = (utf8_str)convert_utf16_to_utf8((utf16*)word.attributes.hiragana.str, (int)word.attributes.hiragana.sz); defer{ free_any_str(hiragana.str); };

		std::string increment_times_shown = SQL(
			UPDATE _べんきょう_table_words SET times_shown = times_shown + 1 WHERE hiragana =
		) + "'"s + std::string((utf8*)hiragana.str) + "'" ";";

		utf8* errmsg;
		sqlite3_exec(db, increment_times_shown.c_str(), 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	void word_increment_times_right(sqlite3* db, const learnt_word& word) {
		using namespace std::string_literals;
		//TODO(fran): having to convert the hiragana is pretty annoying and pointless, rowid or similar looks much better
		utf8_str hiragana = (utf8_str)convert_utf16_to_utf8((utf16*)word.attributes.hiragana.str, (int)word.attributes.hiragana.sz); defer{ free_any_str(hiragana.str); };

		std::string increment_times_right = SQL(
			UPDATE _べんきょう_table_words SET times_right = times_right + 1 WHERE hiragana =
		) + "'"s + std::string((utf8*)hiragana.str) + "'" ";";

		utf8* errmsg;
		sqlite3_exec(db, increment_times_right.c_str(), 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
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
			init_cpp_objects(st);
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

				clear_practices_vector(state->pagestate.practice_review.practices);
				state->pagestate.practice_review.practices.~vector();
				
				//#free? state->pagestate.practice_writing.practice;

				clear_practices_vector(state->multipagestate.temp_practices);
				state->multipagestate.temp_practices.~vector();

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
						reset_page(state, ProcState::page::new_word);
						set_current_page(state, ProcState::page::new_word);
					}
					else if (child == page.list.button_practice) {
						store_previous_page(state, state->current_page);
						user_stats stats = get_user_stats(state->settings->db);
						get_user_stats_accuracy_timeline(state->settings->db, &stats, 30); defer{ stats.accuracy_timeline.free(); };
						preload_page(state, ProcState::page::practice, &stats);
						set_current_page(state, ProcState::page::practice);
					}
					else if (child == page.list.button_search) {
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::search);
					}
					else if (child == page.list.combo_languages) {
						WORD notif = HIWORD(wparam);
						switch (notif) {
						case CBN_SELENDOK:
						{
							//user requested a language change
							//TODO(fran): pretty sure I can reduce all this to _get_edit_str() instead of going through the listbox
							i32 cur_sel = (i32)SendMessage(child, CB_GETCURSEL, 0, 0);
							if (cur_sel != CB_ERR) {
								utf16_str lang = alloc_any_str(sizeof(*lang.str) * (SendMessage(child, CB_GETLBTEXTLEN, cur_sel, 0) + 1)); defer{ if(lang.sz) free_any_str(lang.str); };
								if (lang.sz) {
									SendMessage(child, CB_GETLBTEXT, cur_sel, (LPARAM)lang.str);
									LANGUAGE_MANAGER::Instance().ChangeLanguage(lang.str);
								}
							}

						} break;
						}
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
#define TEST_REVIEWPAGE
#ifndef TEST_REVIEWPAGE
							constexpr int practice_cnt = 15 + 1;//TODO(fran): I dont like this +1
#else
							constexpr int practice_cnt = 2 + 1;
#endif
							state->practice_cnt = (u32)min(word_cnt, practice_cnt);//set the practice counter (and if there arent enough words reduce the practice size, not sure how useful this is)
							store_previous_page(state, state->current_page);

							//Clear previous practice data:
							clear_practices_vector(state->multipagestate.temp_practices);

							next_practice_level(state, USER_TIMER_MINIMUM);
						}
						else MessageBoxW(state->wnd, RCS(360), 0, MB_OK, MBP::center);
					}
				} break;
				case ProcState::page::search:
				{
					auto& page = state->controls.search;
#if 0
					if (child == page.list.combo_search) {
						WORD notif = HIWORD(wparam);
						switch (notif) {
						case CBN_EDITCHANGE://The user has changed the content of the edit box, this msg is sent after the change is rendered in the control (if we use CBN_EDITUPDATE whick is sent before re-rendering it slows down user input and feels bad to type)
						{

							//For now we'll get a max of 5 results, the best would be to set the max to reach the bottom of the parent window (aka us) so it can get to look like a full window instead of feeling so empty //TODO(fran): we could also append the "searchbar" in the landing page

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
								//else {
									//Hide the listbox, well... maybe not since we now show the search string on the first item
									//SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
								//}
								//Update listbox size
								COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
								int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);
								int list_h = max((int)SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt, 2);
								RECT combo_rw; GetWindowRect(page.list.combo_search, &combo_rw);
								MoveWindow(nfo.hwndList, combo_rw.left, combo_rw.bottom, RECTWIDTH(combo_rw), list_h, TRUE); //TODO(fran): handle showing it on top of the control if there's no space below
#if 0
								AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
#else
								//ShowWindow(nfo.hwndList, SW_SHOW);//NOTE: dont call AnimateWindow, for starters SW_SHOW appears to already animate, unfortunately we dont want animation, we want the results as fast as possible, also the animation seems to be performed by the same thread which slows down user input
#endif
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
#endif
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

						int ret = MessageBoxW(state->nc_parent, RCS(280), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL,MBP::center);
						if (ret == IDYES) {
							if (remove_word(state)) {
								goto_previous_page(state);
								//set_current_page(state, ProcState::page::search);//TODO(fran): this should also be go back, we can get to show_word from new_word
							}
						}
}
				} break;
				case ProcState::page::practice_writing:
				{
					auto& page = state->controls.practice_writing;
					auto& pagestate = state->pagestate.practice_writing;
					WORD notif = HIWORD(wparam);
					if (child == page.list.button_next || (child == page.list.edit_answer && notif == EN_ENTER)) {
						utf16_str user_answer; _get_edit_str(page.list.edit_answer, user_answer);
						if (user_answer.sz_char() != 1) {
							const utf16_str* correct_answer{ 0 };
							const utf16_str* question{ 0 };//TODO(fran); this should already come calculated at this point
							switch (pagestate.practice->practice_type) {
							case decltype(pagestate.practice->practice_type)::translate_hiragana_to_translation:
							{
								correct_answer = (utf16_str*)&pagestate.practice->word.attributes.translation;
								question = (utf16_str*)&pagestate.practice->word.attributes.hiragana;
							} break;
							case decltype(pagestate.practice->practice_type)::translate_kanji_to_translation:
							{
								correct_answer = (utf16_str*)&pagestate.practice->word.attributes.translation;
								question = (utf16_str*)&pagestate.practice->word.attributes.kanji;
							} break;
							case decltype(pagestate.practice->practice_type)::translate_kanji_to_hiragana:
							{
								correct_answer = (utf16_str*)&pagestate.practice->word.attributes.hiragana;
								question = (utf16_str*)&pagestate.practice->word.attributes.kanji;

							} break;
							case decltype(pagestate.practice->practice_type)::translate_translation_to_hiragana:
							{
								correct_answer = (utf16_str*)&pagestate.practice->word.attributes.hiragana;
								question = (utf16_str*)&pagestate.practice->word.attributes.translation;
							} break;
							default:Assert(0);
							}

							bool success = !StrCmpIW(correct_answer->str, user_answer.str);

							//NOTE: we can also change text color here, if we set it to def text color then we can change the bk without fear of the text not being discernible from the bk
							HBRUSH bk = success ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
							edit_oneline::set_brushes(page.list.edit_answer, TRUE, global::colors.ControlTxt, bk, bk, 0, 0, 0);
							button::set_brushes(page.list.button_next, TRUE, bk, bk, global::colors.ControlTxt, 0, 0);

							//TODO(fran): block input to the edit and btn controls, we dont want the user to be inputting new values or pressing next multiple times

							//Update word stats
							//TODO(fran): if we knew had the old values in the word object we could simply update those and batch update the whole word
							word_update_last_shown_date(state->settings->db,pagestate.practice->word);
							word_increment_times_shown(state->settings->db,pagestate.practice->word);
							if(success) word_increment_times_right(state->settings->db,pagestate.practice->word);

							//Add this practice to the list of current completed ones
							ProcState::practice_writing* p = (decltype(p))malloc(sizeof(*p));//TODO(fran): free once there's a new review
							p->header.type = decltype(p->header.type)::writing;
							p->practice = state->pagestate.practice_writing.practice;
							state->pagestate.practice_writing.practice = nullptr;//clear the pointer just in case
							p->user_answer = user_answer;
							p->correct_answer = correct_answer;
							p->answered_correctly = success;
							p->question = question;

							state->multipagestate.temp_practices.push_back((ProcState::practice_header*)p);

							//TODO(fran): I think we actually want to show the correct answer right here, so the user can make a direct connection with it and not forget, because of this I'd either make the timer longer or simply wait for the user to click next so they have all the time they want to look at the answer, what we can also do is implement this only when the user fails, on success just wait a moment and follow to the next level

							next_practice_level(state);
						}
						else {
							free_any_str(user_answer.str);
						}
					}
				} break;
				case ProcState::page::review_practice:
				{
					auto& page = state->controls.review_practice;
					if (child == page.list.button_continue) {
						store_previous_page(state, state->current_page);
						//user_stats stats = get_user_stats(state->settings->db);
						//get_user_stats_accuracy_timeline(state->settings->db, &stats, 30); defer{ stats.accuracy_timeline.free(); };//TODO(fran): this things should be joined into a function, I've already got to do this same code twice
						//preload_page(state, ProcState::page::practice, &stats);
						//set_current_page(state, ProcState::page::practice);
						set_current_page(state, ProcState::page::landing);//TODO(fran): this isnt best, it'd be nice if we went back to the practice page but from here it'd require a goto_previous_page and the fact that we know prev page is practice and that we should preload cause it's values have changed
					} 
					else if (child == page.list.gridview_practices) {
						//The user clicked an element
						void* data = (void*)wparam;
						ProcState::practice_header* header = (decltype(header))data;
						switch (header->type) {
						case decltype(header->type)::writing:
						{
							ProcState::practice_writing* pagedata;
							Assert(0);
							//TODO(fran): set up and switch pages

							//IDEA: implement ocarina of time's concept of a scene flag, the same page can be loaded in different scenes, eg on a practice page we'd have the default scene where we'd setup default colors, then the wrong_answer scene where we show a button or smth to allow the user to see the correct answer, also here we no longer accept new keyboard input beyond WM_ENTER, we also have the right_answer scene where we simply change colors and disable all user input since a timer will automatically change page. 
							//IMPORTANT IDEA: we want individual/independent pages in the case we do reviews and similar things where we simply want to show the data but dont care about user input, eg the user presses an element in the gridview and we create a new wnd, create the corresponding controls, set them up (colors, disabled, etc) and show that window as a separate entity, that's what we want, to call add_controls on a different window and simply give it to the user, whenever the user's done they can close the window, meanwhile they can still interact with the main window, I think this is really good, the user could for example open all the words they got wrong at the same time, instead of having to open one, go back to the grid, open another; the one problem with this is where to show the new window, say we have 5 windows open, how do we choose where to show the sixth

						} break;
						}
					}
				} break;

				}
			}
			else {
				switch (LOWORD(wparam)) {//Menu notifications
				default:Assert(0);
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
			SetBkColor(listboxDC, ColorFromBrush(global::colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(global::colors.ControlTxt));

			return (INT_PTR)global::colors.ControlBk;
		} break;
		case WM_CTLCOLOREDIT: //for the combobox edit control (if I dont subclass it) //TODO(fran): this has to go
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(global::colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(global::colors.ControlTxt));

			return (INT_PTR)global::colors.ControlBk;
		} break;
		case WM_CTLCOLORSTATIC: //for the static controls //TODO(fran): this has to go, aka make my own
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(global::colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(global::colors.ControlTxt));//TODO(fran): maybe a lighter color to signalize non-editable?

			return (INT_PTR)global::colors.ControlBk;
		} break;
		case WM_BACK:
		{
			goto_previous_page(state);
			//TODO(fran): going back is a bit of a problem, eg the user finishes a practice and presses back, then they'll see outdated values, we could live with that saying well yeah you went back, or we could do preloading, for specific pages or have a flag that says for each page if it needs preloading
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
		//case WM_KEYUP://HACK: because of using Sleep some msgs seem to go to the wrong place
		//{
		//	return 0;
		//} break;
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
		wcex.hbrBackground = 0; //TODO(fran): global::colors.ControlBk hasnt been deserialized by the point pre_post_main gets executed!
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
