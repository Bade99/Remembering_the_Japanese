#pragma once
#include "win32_Platform.h"
#include "sqlite3.h"
#include "win32_Helpers.h"
#include "unCap_Math.h"
#include "unCap_Reflection.h"

#include <string>
//All interactions with the がいしんのべんきょう db are in here:

//-------------------------Types-----------------------------: //TODO(fran): should the types be inside the namespace?

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

template <typename type>
union _learnt_word { //contains utf16 when getting data from the UI, and utf8 when sending requests to the db
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

//UTF16 encoded data
typedef _learnt_word<utf16_str> learnt_word16;
//UTF8 encoded data
typedef _learnt_word<utf8_str> learnt_word8;

enum class learnt_word_elem : u32 {
	hiragana = (1 << 0),
	kanji = (1 << 1),
	meaning = (1 << 2),
};

template <typename type>
union _extra_word {
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

//UTF16 encoded data
typedef _extra_word<utf16_str> extra_word16;
//UTF8 encoded data
typedef _extra_word<utf8_str> extra_word8;

struct stored_word16 {
	learnt_word16 user_defined;
	extra_word16 application_defined;
};


//-------------------------Macros----------------------------:


#define _SQL(x) (#x)
//IMPORTANT: careful when using this, by design it allows for macros to expand so you better not redefine sql keywords, eg microsoft defines NULL as 0 so you should write something like NuLL instead, same for DELETE use smth like DeLETE
#define SQL(x) _SQL(x)
//IMPORTANT: do not use "," in macros, instead use comma
#define comma ,


#define _sqlite3_generate_columns(type,name,...) "" + #name + ","
//IMPORTANT: you'll have to remove the last comma since sql doesnt accept trailing commas

#define _sqlite3_generate_columns_array(type,name,...) #name,

#define _sqlite3_generate_values(type,name,...) "'" + word->attributes.name.str + "'"","
//IMPORTANT: you'll have to remove the last comma since sql doesnt accept trailing commas

#define _sqlite3_generate_values_array(type,name,...) std::string("'") + word->attributes.name.str + "'",


//----------------------DB Structure-------------------------:


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

//TODO(fran): I really dont know what to do with the primary key constraint, it's more annoying than anything else if applied to the hiragana column, but on the other hand we do need it for the type of practices we want, where we question the user based only on hiragana, maybe simply adding a list is for the best, at least for the 'meaning' column. That would be one kanji, one hiragana, multiple meanings. Check this actually applies to the language

//TODO(fran): hiragana, kanji and translation should actually be lists
//				https://stackoverflow.com/questions/9755741/vectors-lists-in-sqlite
//				https://www.sqlite.org/json1.html

;//INFO: ROWID: in sqlite a column called ROWID is automatically created and serves the function of "id INTEGER PRIMARY KEY" and AUTOINCREMENT, _but_ AUTOINCREMENT as in MySQL or others (simply incrementing on every insert), on the other hand the keyword AUTOINCREMENT in sqlite incurrs an extra cost because it also checks that the value hasnt already been used for a deleted row (we dont care for this in this table)

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

#define べんきょう_table_version version
#define べんきょう_table_version_structure \
	v					INTEGER PRIMARY KEY\

//INFO about べんきょう_table_version_structure:
//	v: stores the db/program version, to make it simpler versions are just a number, the db wont be changing too often


//-----------------------Functions--------------------------:


namespace べんきょう {

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


	//returns the basic user stats, this values dont need to be freed
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
	//retrieves the last cnt timepoints in order of oldest to newest and stores them inside the stats pointer
	void get_user_stats_accuracy_timeline(sqlite3* db, user_stats* stats, size_t cnt) {//TODO(fran): I feel like returning ptr<u8> is better than asking for a user_stats ptr but Im not sure
		using namespace std::string_literals;
		std::string timeline =
			SQL(SELECT accuracy FROM)
			+ "("s +
			SQL(SELECT accuracy comma creation_date FROM _べんきょう_table_accuracy_timeline ORDER BY creation_date DESC LIMIT) + " "s + std::to_string(cnt)
			+ ")" +
			SQL(ORDER BY creation_date ASC;);

		sqlite3_stmt* stmt;
		int errcode;
		errcode = sqlite3_prepare_v2(db, timeline.c_str(), (int)(timeline.size() + 1) * sizeof(decltype(timeline)::value_type), &stmt, nullptr);
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

	void user_stats_increment_times_practiced(sqlite3* db) {
		utf8 update_increment_times_practiced[] = SQL(
			UPDATE _べんきょう_table_user SET times_practiced = times_practiced + 1;
		);
		utf8* errmsg;
		sqlite3_exec(db, update_increment_times_practiced, 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}


	struct get_word_res { stored_word16 word; bool found; };
	void free_get_word(stored_word16& word) {
		//TODO(fran): I dont think I need the if, I should simply always allocate (and I think I already do)
		for (auto s : word.application_defined.all) if (s.str) free_any_str(s.str);
		for (auto s : word.user_defined.all) if (s.str) free_any_str(s.str);
	}
	get_word_res get_word(sqlite3* db, learnt_word_elem match_type, utf16* match) {
		using namespace std::string_literals;
		get_word_res res{ 0 };
		auto match_str = convert_utf16_to_utf8(match, (int)(cstr_len(match) + 1) * sizeof(*match)); defer{ free_any_str(match_str.str); };

		//TODO(fran): this should be a function that returns ptr to static strings of the columns
		const char* filter_col = 0;
		switch (match_type) {
		case decltype(match_type)::hiragana: filter_col = "hiragana"; break;
		case decltype(match_type)::kanji: filter_col = "kanji"; break;
		case decltype(match_type)::meaning: filter_col = "translation"; break;
		default: Assert(0);
		}

		std::string select_word = "SELECT "s
			+ _foreach_learnt_word_member(_sqlite3_generate_columns)
			+ _foreach_extra_word_member(_sqlite3_generate_columns);
		select_word.pop_back();//remove trailing comma
		select_word += " FROM "s + べんきょう_table_words +
			" WHERE " + filter_col + " LIKE '" + (utf8*)match_str.str + "'" ";";
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
				i64 unixtime = std::stoll(res->word.application_defined.attributes.last_shown_date.str, nullptr, 10);
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
					wcsftime(res->word.application_defined.attributes.last_shown_date.str, 30, L"%Y-%m-%d", time);
				}
			}
			catch (...) {}

			try {//Format created_date (this would be easier to do in the query but I'd have to remove the macros)
				i64 unixtime = std::stoll(res->word.application_defined.attributes.creation_date.str, nullptr, 10);
				free_any_str(res->word.application_defined.attributes.creation_date.str);

				std::time_t temp = unixtime;
				std::tm* time = std::localtime(&temp);//UNSAFE: not multithreading safe, returns a pointer to an internal unique object
				res->word.application_defined.attributes.creation_date = alloc_any_str(30 * sizeof(utf16));
				wcsftime(res->word.application_defined.attributes.creation_date.str, 30, L"%Y-%m-%d", time);
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

	//TODO(fran): I should handle gmt to localtime and viceversa conversions inside the db and leave the rest of the program to work in localtime
	//NOTE: returns an array of learnt_words that are contiguous in memory, which means that freeing the base pointer frees all the elements at once
	ptr<learnt_word16> get_learnt_word_by_date(sqlite3* db, time64 gmt_start, time64 gmt_end) {
		using namespace std::string_literals;
		ptr<learnt_word16> res;
		std::string columns = ""s + _foreach_learnt_word_member(_sqlite3_generate_columns); columns.pop_back();

		std::string filter = 
			" FROM "s + べんきょう_table_words +
			" WHERE " + "creation_date" + ">=" + std::to_string(gmt_start) +
			" AND " + "creation_date" + "<=" + std::to_string(gmt_end) + ";";

		std::string count_words = "SELECT "s + "COUNT(*)" + filter;
		std::string select_words = "SELECT "s + columns + filter;
		
		i64 cnt;
		{
			sqlite3_stmt* stmt;
			int errcode;
			errcode = sqlite3_prepare_v2(db, count_words.c_str(), (int)(count_words.length()+1)*sizeof(count_words[0]), &stmt, nullptr);

			if (errcode == SQLITE_OK) {
				errcode = sqlite3_step(stmt);
				if (errcode == SQLITE_ROW) cnt = sqlite3_column_int64(stmt, 0);
				else cnt = 0;
			}
			else cnt = 0;

			sqlite3_finalize(stmt);
		}

		res.alloc(cnt);//TODO(fran): I think allocating zero size works just as well but im not sure
		res.cnt = 0;

		auto parse_learnt_word_array = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			//NOTE: everything comes in utf8
			Assert(column_cnt == (0 + _foreach_learnt_word_member(_generate_count)));
			ptr<learnt_word16>* res = (decltype(res))extra_param;

			for (int i = 0; i < column_cnt; i++)
				res->mem[res->cnt].all[i] = convert_utf8_to_utf16(results[i], (int)strlen(results[i]) + 1);

			res->cnt++;
			return 0;//if non-zero then the query stops and exec returns SQLITE_ABORT
		};

		char* select_errmsg;
		sqlite3_exec(db, select_words.c_str(), parse_learnt_word_array, &res, &select_errmsg);
		sqlite_exec_runtime_check(select_errmsg);

		return res;
	}

	//returns GMT unixtime
	time64 get_latest_word_creation_date(sqlite3* db) {
		time64 res;

		utf8 select_last_creation_date[] = SQL(
			SELECT creation_date
			FROM _べんきょう_table_words
			ORDER BY creation_date DESC
			LIMIT 1;
		);

		{
			sqlite3_stmt* stmt;
			int errcode;
			errcode = sqlite3_prepare_v2(db, select_last_creation_date, sizeof(select_last_creation_date), &stmt, nullptr);

			if (errcode == SQLITE_OK) {
				errcode = sqlite3_step(stmt);
				if (errcode == SQLITE_ROW) res = sqlite3_column_int64(stmt, 0);
				else res = 0;
			}
			else res = 0;

			sqlite3_finalize(stmt);
		}

		return res;
		
	}

	struct unixtime_day_range { time64 start, end; };
	//returns the gmt timestamp for start and end of the day of the requested gmt unixtime converted to localtime
	unixtime_day_range day_range(time64 unixtime_gmt) {
		unixtime_day_range res;

		std::time_t temp = unixtime_gmt;
		std::tm* time = std::localtime(&temp);//UNSAFE: not multithreading safe, returns a pointer to an internal unique object
		time->tm_hour = 0;
		time->tm_min = 0;
		time->tm_sec = 0;

		//TODO(fran): make sure this is correct.
			//what im trying to do: I get a unixtime (in gmt), I convert it to localtime, I get start and end of that day in localtime, finally I convert those two values to unixtime (hopefully, I think, gmt)
			//examples: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/mkgmtime-mkgmtime32-mkgmtime64?view=msvc-160
		res.start = _mktime64(time);

		time->tm_hour = 23;
		time->tm_min = 59;
		time->tm_sec = 59;

		res.end = _mktime64(time);

		return res;
	}

	//returns sqlite error codes, SQLITE_OK,...
	int insert_word(sqlite3* db, const learnt_word8* word) {
		int res;
		//TODO(fran): specify all the columns for the insert, that will be our error barrier
		//TODO(fran): we are inserting everything with '' which is not right for numbers
		//NOTE: here I have an idea, if I store the desired type I can do type==number? string : 'string'

		std::string columns = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_columns);
		columns.pop_back(); //We need to remove the trailing comma

		std::string values = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_values);
		values.pop_back(); //We need to remove the trailing comma

		std::string insert_word = std::string(" INSERT INTO ") + べんきょう_table_words + "(" + columns + ")" + " VALUES(" + values + ");";

		//char* insert_errmsg;
		res = sqlite3_exec(db, insert_word.c_str(), 0, 0, 0/*&insert_errmsg*/) /*== SQLITE_OK*/;
		//sqlite_exec_runtime_check(insert_errmsg);

		//TODO(fran): handle if the word already exists, maybe show the old word and ask if they want to override that content, NOTE: the handling code shouldnt be here, this function should be as isolated as possible, if we start heavily interacting with the user this will be ugly

		return res;
	}


	bool update_word(sqlite3* db, learnt_word8* word) {
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
			update_word += columns[i] + " LIKE " + values[i] + (((i + 1) != word->pk_count) ? "AND" : "");
		//TODO(fran): should I use "like" or "="  or "==" ?

		char* update_errmsg;
		res = sqlite3_exec(db, update_word.c_str(), 0, 0, &update_errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(update_errmsg);

		return res;
	}


	//NOTE word should be in utf16
	void word_update_last_shown_date(sqlite3* db, const learnt_word16& word) {
		using namespace std::string_literals;
		utf8_str hiragana = (utf8_str)convert_utf16_to_utf8(word.attributes.hiragana.str, (int)word.attributes.hiragana.sz); defer{ free_any_str(hiragana.str); };

		std::string update_last_shown_date = SQL(
			UPDATE _べんきょう_table_words SET last_shown_date = strftime('%s', 'now') WHERE hiragana =
		) + "'"s + std::string((utf8*)hiragana.str) + "'" ";";

		utf8* errmsg;
		sqlite3_exec(db, update_last_shown_date.c_str(), 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	//IMPORTANT: do not use directly, only through word_increment_times_shown__times_right
	void word_increment_times_shown(sqlite3* db, const learnt_word16& word) {
		using namespace std::string_literals;
		//TODO(fran): having to convert the hiragana is pretty annoying and pointless, rowid or similar looks much better
		utf8_str hiragana = (utf8_str)convert_utf16_to_utf8(word.attributes.hiragana.str, (int)word.attributes.hiragana.sz); defer{ free_any_str(hiragana.str); };

		std::string increment_times_shown = SQL(
			UPDATE _べんきょう_table_words SET times_shown = times_shown + 1 WHERE hiragana =
		) + "'"s + std::string((utf8*)hiragana.str) + "'" ";";

		utf8* errmsg;
		sqlite3_exec(db, increment_times_shown.c_str(), 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	//IMPORTANT: do not use directly, only through word_increment_times_shown__times_right
	void word_increment_times_right(sqlite3* db, const learnt_word16& word) {
		using namespace std::string_literals;
		//TODO(fran): having to convert the hiragana is pretty annoying and pointless, rowid or similar looks much better
		utf8_str hiragana = (utf8_str)convert_utf16_to_utf8(word.attributes.hiragana.str, (int)word.attributes.hiragana.sz); defer{ free_any_str(hiragana.str); };

		std::string increment_times_right = SQL(
			UPDATE _べんきょう_table_words SET times_right = times_right + 1 WHERE hiragana =
		) + "'"s + std::string((utf8*)hiragana.str) + "'" ";";

		utf8* errmsg;
		sqlite3_exec(db, increment_times_right.c_str(), 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	//NOTE word should be in utf16
	void word_increment_times_shown__times_right(sqlite3* db, const learnt_word16& word, bool inc_times_right) {
		//INFO: times_shown _must_ be updated before times_right in order for the triggers to function correctly
		word_increment_times_shown(db, word);
		if (inc_times_right)word_increment_times_right(db, word);
	}


	bool delete_word(sqlite3* db, learnt_word8* word) {//TODO(fran): it'd be better to receive a learnt_word16 and only here convert it to learnt_word8
		using namespace std::string_literals;
		bool res = false;

		//To delete you find the word by its primary keys
		std::string delete_word = SQL(
			DELeTE FROM _べんきょう_table_words WHERE hiragana =
		) + "'"s + word->attributes.hiragana.str + "'" ";";//TODO(fran): parametric filtering by all primary keys


		char* delete_errmsg;
		res = sqlite3_exec(db, delete_word.c_str(), 0, 0, &delete_errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(delete_errmsg);
		return res;
	}


	//struct search_word_res { utf16** matches; int cnt; };
	//void free_search_word(search_word_res res) {
	//	for (int i = 0; i < res.cnt; i++) {
	//		free_any_str(res.matches[i]);
	//	}
	//	free(res.matches);
	//}

	//NOTE: returns an array of learnt_words that are contiguous in memory, which means that freeing the base pointer frees all the elements at once
	ptr<learnt_word16> search_word_matches(sqlite3* db, learnt_word_elem match_type, utf16* match/*bytes*/, int max_cnt_results/*eg. I just want the top 5 matches*/) {
		//NOTE/TODO(fran): for now we'll simply retrieve the hiragana, it might be good to get the translation too, so the user can quick search, and if they really want to know everything about that word then the can click it and pass to the next stage
		ptr<learnt_word16> res;
		res.alloc(max_cnt_results);
		//res.matches = (decltype(res.matches))malloc(max_cnt_results * sizeof(*res.matches));
		res.cnt = 0;
		auto match_str = convert_utf16_to_utf8(match, (int)(cstr_len(match) + 1) * sizeof(*match)); defer{ free_any_str(match_str.str); };
		const char* filter_col = 0;
		switch (match_type) {
		case decltype(match_type)::hiragana: filter_col = "hiragana"; break;
		case decltype(match_type)::kanji: filter_col = "kanji"; break;
		case decltype(match_type)::meaning: filter_col = "translation"; break;
		default: Assert(0);
		}

		std::string learnt_word_cols = std::string("") + _foreach_learnt_word_member(_sqlite3_generate_columns); learnt_word_cols.pop_back();//remove trailing comma
		char* select_errmsg;
		std::string select_matches =
			std::string(" SELECT ") + learnt_word_cols + /*TODO(fran): column names should be stored somewhere*/
			" FROM " + べんきょう_table_words +
			" WHERE " + filter_col + " LIKE '" + (utf8*)match_str.str + "%'" +
			" LIMIT " + std::to_string(max_cnt_results) + ";";

		auto parse_match_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			//NOTE: everything comes in utf8
			Assert(column_cnt == (0 + _foreach_learnt_word_member(_generate_count)));
			//search_word_res* res = (decltype(res))extra_param;
			ptr<learnt_word16>* res = (decltype(res))extra_param;

			//res->matches[res->cnt] = (decltype(*res->matches))convert_utf8_to_utf16(results[0], (int)strlen(results[0]) + 1).mem;//INFO: interesting, I know this is correct but compiler cant allow it, an explanation here https://stackoverflow.com/questions/3674456/why-this-is-causing-c2102-requires-l-value

			//res->matches[res->cnt] = (decltype(*res->matches))r.str;
			for (int i = 0; i < column_cnt; i++)
				res->mem[res->cnt].all[i] = convert_utf8_to_utf16(results[i], (int)strlen(results[i]) + 1);

			res->cnt++;
			return 0;//if non-zero then the query stops and exec returns SQLITE_ABORT
		};

		sqlite3_exec(db, select_matches.c_str(), parse_match_result, &res, &select_errmsg);
		sqlite_exec_runtime_check(select_errmsg);//TODO(fran): should I free the result and return an empty?
		return res;
	}


	bool reset_word_priority(sqlite3* db, learnt_word8* word) {
		using namespace std::string_literals;
		bool res = false;
		//HACK: TODO(fran): store on the db two different values for each of the prioritizing columns so we can maintain untouched the one we show to the user but change/reset the real values the system uses when choosing practice words

		//TODO(fran): can I SET to default values?
		std::string reset_priority = SQL(
			UPDATE _べんきょう_table_words
			SET last_shown_date = 0 comma
			times_shown = 0
			WHERE hiragana =
		) + "'"s + word->attributes.hiragana.str + "'" ";";

		utf8* errmsg;
		res = sqlite3_exec(db, reset_priority.c_str(), 0, 0, &errmsg) == SQLITE_OK;
		sqlite_exec_runtime_check(errmsg);
		return res;
	}

	bool reset_word_priority(sqlite3* db, learnt_word16* word16) {
		learnt_word8 word8{ 0 }/*we actually only need to clear what can be used by the real function*/;
		//TODO(fran): here we'll only convert what's strictly necessary for the real function to succeed
		word8.attributes.hiragana = convert_utf16_to_utf8(word16->attributes.hiragana.str, (int)word16->attributes.hiragana.sz); defer{ free_any_str(word8.attributes.hiragana.str); };
		return reset_word_priority(db, &word8);
	}


	learnt_word16 get_practice_word(sqlite3* db, bool hiragana_req = false, bool kanji_req = false, bool meaning_req = false) {
		learnt_word16 res{ 0 };

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

		std::string where_req("");
		if (hiragana_req || kanji_req || meaning_req) {
			where_req = " WHERE ";
			bool first = true;
			auto do_first_and_others = [&]() { if (first) first = !first; else where_req += " AND "; };
			if (hiragana_req) { do_first_and_others(); where_req += "hiragana" " <> " "''"; }
			if (kanji_req) { do_first_and_others(); where_req += "kanji" " <> " "''"; }
			if (meaning_req) { do_first_and_others(); where_req += "translation" " <> " "''"; }
		}

		std::string select_practice_word =
			" SELECT * FROM "
			"("
			//1. least shown: use times_shown and pick the 30 of lowest value, then reduce that at random down to 10.
			" SELECT * FROM " //IMPORTANT INFO: UNION is a stupid operator, if you also want to _previously_ "order by" your selects you have to "hide" them inside another select and parenthesis
			"("
			" SELECT * FROM "
			"(" " SELECT " + columns + " FROM " べんきょう_table_words + where_req + " ORDER BY times_shown ASC LIMIT 30" ")"
			"ORDER BY RANDOM() LIMIT 10" //TODO(fran): I dont know how random this really is, probably not good enough
			")"
			" UNION " //TODO(fran): UNION ALL is faster cause it doesnt bother to remove duplicates
			//2. least recently shown: use last_shown_date and pick the 30 oldest dates (lowest numbers), then reduce that at random down to 5
			" SELECT * FROM "
			"("
			" SELECT * FROM "
			"(" " SELECT " + columns + " FROM " べんきょう_table_words + where_req + " ORDER BY last_shown_date ASC LIMIT 30"  ")"
			"ORDER BY RANDOM() LIMIT 5"
			")"
			")"
			"ORDER BY RANDOM() LIMIT 1;"
			;//Now that's why sql is a piece of garbage, look at the size of that query!! for such a stupid two select + union operation, if they had given you the obvious option of storing the select on a variable, then store the other select on a variable and then union both this would be so much more readable, comprehensible and easier to write

		auto parse_select_practice_word_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			learnt_word16* res = (decltype(res))extra_param;
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


	//NOTE: can return less than 'cnt' if not enough elements exist in the db
	ptr<utf16*> get_word_choices(sqlite3* db, learnt_word_elem request, u32 cnt, const learnt_word16* filter) {
		ptr<utf16*> res; res.alloc(cnt); res.cnt = 0;

		utf8* filter_elem; defer{ free_any_str(filter_elem); };
		std::string req_column;
		switch (request) {
		case decltype(request)::hiragana:
			filter_elem = (utf8*)convert_utf16_to_utf8(filter->attributes.hiragana.str, (int)filter->attributes.hiragana.sz).str;
			req_column = "hiragana";
			break;
		case decltype(request)::kanji:
			filter_elem = (utf8*)convert_utf16_to_utf8(filter->attributes.kanji.str, (int)filter->attributes.kanji.sz).str;
			req_column = "kanji";
			break;
		case decltype(request)::meaning:
			filter_elem = (utf8*)convert_utf16_to_utf8(filter->attributes.translation.str, (int)filter->attributes.translation.sz).str;
			req_column = "translation";
			break;
		default: Assert(0);
		}
		//TODO(fran): prioritize choosing words of the same lexical category as 'filter'
		std::string select_word_choices =
			" SELECT " + req_column + " FROM " べんきょう_table_words
			" WHERE " + req_column + " <> " "''" " AND " + req_column + "<>" "'" + filter_elem + "'"
			" ORDER BY RANDOM() LIMIT " + std::to_string(cnt) + ";" //TODO(fran): I dont know how random this really is, probably not good enough
			;//TODO(fran): idk whether to store on the db the empty string '' (as we do currently) or straight null, storing nulls would mean lots of extra checks in other sections of the code, what we _do_ need is a standard, either one or the other but _not_ both

		auto parse_select_word_choices_result = [](void* extra_param, int column_cnt, char** results, char** column_names) -> int {
			ptr<utf16*>* res = (decltype(res))extra_param;
			Assert(column_cnt == 1);
			Assert(results);

			(*res)[res->cnt++] = (std::remove_reference<decltype(*res)>::type::value_type)convert_utf8_to_utf16(*results, (int)strlen(*results) + 1).str;

			return 0;
		};

		char* errmsg;
		sqlite3_exec(db, select_word_choices.c_str(), parse_select_word_choices_result, &res, &errmsg);
		sqlite_exec_runtime_check(errmsg);

		return res;
	}


	//TODO(fran): it may be less annoying to use a tuple/similar of the form tuple<f_apply,f_rollback>
	struct db_version {
		//NOTEs:
		//-Newer db versions never destroy older db tables nor alter already existing column names/types, they can though add new tables and add new columns to old tables while they are _not_ primary keys
		//-All triggers are created as temporary, triggers from older versions can not be dropped
		//-By this logic once a table is set by one version it can no longer be set back to a previous version, this allows for all the data to remain on the database even if the current version running doesnt know how to use it. On the other hand triggers are the opposite, only the required ones are running, this allows for example to apply automated checks via triggers that only execute on the correct version and dont cause problems for other versions were their existence is not handled
		virtual void apply_tables(sqlite3* db) = 0;//Executed only the first time the db needs to be updated to this version
		virtual void apply_triggers(sqlite3* db) = 0;//Executed each time the db is opened and the version number is greater than or equal to this one
	};

	//TODO(fran): make sure all requests to the db ask only for the columns they need, otherwise if new columns are added we could get garbage data or override/corrupt other memory
	struct v1 : db_version {
		void apply_tables(sqlite3* db) override {
			utf8* errmsg;

			//TODO(fran): idk if checking for "if not exists" is still necessary

			//PRAGMA //TODO(fran): add separate apply_pragmas() function?
			{
				utf8 pragma_encoding[] = SQL(PRAGMA encoding = 'UTF-8';);//default encoding for db must be utf8
				sqlite3_exec(db, pragma_encoding, 0, 0, &errmsg);
				sqlite_exec_runtime_check(errmsg);
			}


			//CREATE
			{
				utf8 create_version_table[] = SQL(
					CREATE TABLE べんきょう_table_version(べんきょう_table_version_structure) WITHOUT ROWID;
				);
				sqlite3_exec(db, create_version_table, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
			}
			{
				utf8 create_word_table[] = SQL(
					CREATE TABLE _べんきょう_table_words(べんきょう_table_words_structure) WITHOUT ROWID;
				);
				sqlite3_exec(db, create_word_table, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
			}
			{
				utf8 create_user_table[] = SQL(
					CREATE TABLE _べんきょう_table_user(べんきょう_table_user_structure);
				);
				sqlite3_exec(db, create_user_table, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
			}
			{
				utf8 create_accuracy_timeline_table[] = SQL(
					CREATE TABLE _べんきょう_table_accuracy_timeline(べんきょう_table_accuracy_timeline_structure);
				);
				sqlite3_exec(db, create_accuracy_timeline_table, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
			}

			//INSERT
			{
				//if (get_table_rowcount(db, べんきょう_table_user) > 0) {
				//	//The entry is already there, here we can set new values on future updates for example
				//	//TODO(fran)
				//}
				//else {
					//Entry isnt there, create it
				utf8 insert_user[] = SQL(
					INSERT INTO _べんきょう_table_user DEFAULT VALUES;
				);
				sqlite3_exec(db, insert_user, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
				//TODO(fran): we should check for existing words and compute the values of the user table, idk if this case can happen
			//}
			}

			{
				//if (get_table_rowcount(db, べんきょう_table_user) > 0) {
				//	//The entry is already there, here we can set new values on future updates for example
				//	//TODO(fran)
				//}
				//else {
					//Entry isnt there, create it
				utf8 insert_version[] = SQL(
					INSERT INTO べんきょう_table_version(v) VALUES(1);
				);
				sqlite3_exec(db, insert_version, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
				//NOTE: this will be an UPDATE on future versions
			//}
			}
		}

		void apply_triggers(sqlite3* db) override {
			utf8* errmsg;

			//CREATE TEMPORARY TRIGGER
			{
				//TODO(fran): HACK: Im still using IF NOT EXISTS cause of multiple "tabs", we need a better solution for multi tabs that doesnt do unnecessary setup
				utf8 create_trigger_increment_word_cnt[] = SQL(
					CREATE TEMPORARY TRIGGER IF NOT EXISTS increment_word_cnt AFTER INSERT ON _べんきょう_table_words
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
					CREATE TEMPORARY TRIGGER IF NOT EXISTS decrement_word_cnt AFTER DeLETE ON _べんきょう_table_words
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
					CREATE TEMPORARY TRIGGER IF NOT EXISTS increment_times_shown
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
					CREATE TEMPORARY TRIGGER IF NOT EXISTS increment_times_right
					AFTER UPDATE OF times_right ON _べんきょう_table_words
					BEGIN
					UPDATE _べんきょう_table_user SET times_right = times_right + NEW.times_right - OLD.times_right;
				END;
				);
				sqlite3_exec(db, create_trigger_increment_times_right, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
			}
			{
				//IMPORTANT: this depends on times_shown being updated _before_ times_right
				utf8 create_trigger_insert_accuracy_timepoint[] = SQL(
					CREATE TEMPORARY TRIGGER IF NOT EXISTS insert_accuracy_timepoint
					AFTER UPDATE OF times_shown ON _べんきょう_table_user
					BEGIN
					INSERT INTO _べんきょう_table_accuracy_timeline(accuracy)
					VALUES(CAST((CAST(NEW.times_right AS REAL) / CAST(NEW.times_shown AS REAL) * 100.0) AS INTEGER));
				END;
				);

				utf8 create_trigger_update_accuracy_timepoint[] = SQL(
					CREATE TEMPORARY TRIGGER IF NOT EXISTS update_accuracy_timepoint
					AFTER UPDATE OF times_right ON _べんきょう_table_user
					BEGIN
					UPDATE _べんきょう_table_accuracy_timeline
					SET accuracy = CAST(((CAST(NEW.times_right AS REAL) / CAST(NEW.times_shown AS REAL)) * 100.0) AS INTEGER)
					WHERE creation_date = (SELECT MAX(creation_date) FROM _べんきょう_table_accuracy_timeline);
				END;
				);

				sqlite3_exec(db, create_trigger_insert_accuracy_timepoint, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);

				sqlite3_exec(db, create_trigger_update_accuracy_timepoint, 0, 0, &errmsg);
				sqlite_exec_runtime_assert(errmsg);
			}
		}
	};

	void startup(sqlite3* db) {
		i32 version;
		{
			utf8 get_version[] = SQL(
				SELECT v FROM べんきょう_table_version;
			);
			sqlite3_stmt* stmt;
			int errcode;
			errcode = sqlite3_prepare_v2(db, get_version, sizeof(get_version), &stmt, nullptr);

			if (errcode == SQLITE_OK) {
				errcode = sqlite3_step(stmt);
				if (errcode == SQLITE_ROW) version = (i32)sqlite3_column_int64(stmt, 0);
				else version = 0;
			}
			else version = 0;

			sqlite3_finalize(stmt);
		}
		runtime_assert(version >= 0, L"Invalid database version");

		db_version* versions[]{ new v1() }; defer{ for (auto& v : versions)delete v; };//TODO(fran): having to allocate and deallocate objs in this pointless fashion is garbage, throw away obj oriented idea

		i32 setup_tables_idx = version;//this and higher indexes will be executed

		i32 setup_triggers_idx = ARRAYSIZE(versions);//indexes lower than this one will be executed

		for (i32 i = setup_tables_idx; i < ARRAYSIZE(versions); i++)versions[i]->apply_tables(db);

		for (i32 i = 0; i < setup_triggers_idx; i++)versions[i]->apply_triggers(db);

		//NOTE on Triggers: here the versioning system comes in handy, if the db version == ours then we dont create the triggers since they already exist, otherwise we drop any existing trigger and override it with the new one, _emphasis_ on "==" the triggers work only with what we know in this current version, the versioning system will have to take care of fixing anything future triggers might need already done with the entries that have already been loaded (this is obvious for older versions but it also establishes an invariant for future versions, this allows each version's triggers to work independently of each other), I do think this is the better choice, triggers are independent but that doesnt mean they should break something they know a previous version will need, my concern would be of the need for a trigger to stop an operation, and lets say it expects for a value that v5 gives but v4 doesnt, that means the user can no longer downgrade from v5. Following this sense it'd mean we should probably simply create temp triggers and save the extra hussle of having to check

		//TODO(fran) BUG:
		//Newer versions will need a "ponerse al día" module, in case they need to add data an old version didnt, an example would be the word count, assuming v1 didnt have it when v2 comes the user has potentially already added lots of words but their counter would be 0, here we'll need the module to to count the current number of already existing words on the db and update the counter(this type of operations could get very expensive, the good thing is they'll only execute once when we go from a lower db version to the higher one)
		//ADDED PROBLEM: if the user goes up some versions, then down, then adds words and then goes back up the module wont execute since the db version always stays at the highest one, SOLUTION: one way to fix this is to add one extra information, last_db_version_execution (records the db version that was actually used during execution, which can be lower than the effective db_version of the tables), checking against this we can always know when there's a period of version change which means we have to "ponerlo al día".

//#define TEST_WORDS
#if defined(TEST_WORDS) && defined(_DEBUG)
		{
			utf8* errmsg;
			utf8 test_insert_words[] = SQL(
				INSERT INTO _べんきょう_table_words(hiragana, kanji, translation, mnemonic, lexical_category)
				VALUES
				('はな', '話', 'Flower', 'Flowernote', -1)comma
				('なに', '何', 'What', 'Nani?!', -1)comma
				('ひと', '人', 'Person', 'Note...', -1)comma
				('べんきょう', '勉強', 'Study', 'Emm...', -1)comma
				('おかしい', '', 'Strange', 'ちょっとおかしくないですか', -1);
			);
			sqlite3_exec(db, test_insert_words, 0, 0, &errmsg);
			sqlite_exec_runtime_assert(errmsg);
		}
#endif

	}

}