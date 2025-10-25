#pragma once
namespace べんきょう {
	//NOTE: Since comboboxes return -1 on no selection lexical_category maps perfectly from UI's combobox index to value
	//TODO(fran): store lexical_category value together with it's string in the combobox, that way we dont depend on the order of the elements for mapping

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
		ACT(cb, lexical_category::particle, lexical_category_str_lang_id(lexical_category::particle));
		ACT(cb, lexical_category::prefix, lexical_category_str_lang_id(lexical_category::prefix));
		ACT(cb, lexical_category::radical, lexical_category_str_lang_id(lexical_category::radical));
		ACT(cb, lexical_category::numeric, lexical_category_str_lang_id(lexical_category::numeric));
		ACT(cb, lexical_category::phrase, lexical_category_str_lang_id(lexical_category::phrase));
		ACT(cb, lexical_category::suffix, lexical_category_str_lang_id(lexical_category::suffix));
	}

	lexical_category get_lexical_category(const utf16_str& str_with_int) {
		lexical_category res;
		if (str_with_int.str) {
			try { res = (lexical_category)std::stoi(str_with_int.str); }
			catch (...) { res = (lexical_category)-1; }
		}
		else res = (lexical_category)-1;
		return res;
	}
}