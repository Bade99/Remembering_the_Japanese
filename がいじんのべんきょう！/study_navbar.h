#pragma once
namespace study::study_navbar {
	//Page Search: Searchbox functions
	void searchbox_func_free_elements(ptr<void*> elements, void* user_extra) {
		//TODO(fran): right now Im allocating the whole array which means I only need to free the very first element, this is probably not the way to go for the future, for example if we wanted to do async search we wouldnt know which elements are the first in the array and therefore the only ones that need freeing
		//for(auto& e : elements) free_any_str(e);

		for (auto e : elements) ((learnt_word16*)e)->free();

		//TODO(fran): MEMLEAK, BUG: Im pretty sure I need to free the first element of 'elements' in order for the dynamic array to be freed, but it crashes right now, Im pretty sure there's a bug higher in the chain, and also the array idea is good from a performance standpoint but doesnt look too good for my needs here, there are never gonna be more than say 15 elements so speed isnt really an issue
		if (elements.cnt)free(elements[0]); //memleak solved, for now
		//elements.free(); //NOTE: again, all the elements are allocated continually in memory as an array, so we only need to deallocate the first one and the whole mem section is freed
	}

	ptr<void*> searchbox_func_retrieve_search_options(utf16_str user_input, void* user_extra) {
		ptr<void*> res{ 0 };
		ProcState* state = (decltype(state))user_extra;
		if (user_input.cnt()) {

			if (any_kanji(user_input)) state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::kanji;
			else if (any_hiragana_katakana(user_input)) state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::hiragana;
			else state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::meaning;

			auto search_res = search_word_matches(state->settings->db, state->pagestate.search.search_type, user_input, 8); //defer{ free(search_res.matches);/*free the dinamically allocated array*/ };
			//TODO(fran): search_word_matches should return a ptr
			res.alloc(search_res.cnt);//TODO(fran): am I ever freeing this?
			for (size_t i = 0; i < search_res.cnt; i++)
				//res[i] = search_res.matches[i];
				res[i] = &search_res[i];
		}
		return res;
	}

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra) {
		ProcState* state = (decltype(state))user_extra;

		learnt_word16 search{ 0 };

		stored_word16_res res; defer{ if (res.found) free_stored_word(res.word); };
		if (is_element) {
			//TODO(fran): at this point we already know which word to find on the db, the problem is we dont have its ID, we gotta search by string (hiragana) again, we need to retrieve and store IDs on the db. And add a function get_word(ID)
			search = *(decltype(&search))element;

			res = get_stored_word(state->settings->db, search);
		}
		else {
			//NOTE: here we dont know the "ID" so the search will simply take the first word it finds that matches the requirements //TODO(fran): we could present multiple results and ask the user which one to open
			learnt_word_elem search_type = state->pagestate.search.search_type;
			switch (search_type) {
			case decltype(search_type)::hiragana: search.attributes.hiragana = *((utf16_str*)element); break;
			case decltype(search_type)::kanji:    search.attributes.kanji = *((utf16_str*)element); break;
			case decltype(search_type)::meaning:  search.attributes.meaning = *((utf16_str*)element); break;
			default:Assert(0);//TODO
			}
			//search = ((utf16_str*)element)->str;

			res = get_word(state->settings->db, search_type, str_for(&search, search_type));
		}

		if (res.found) {
			preload_page(state, page_type::show_word, &res.word);//NOTE: considering we no longer have separate pages this might be a better idea than sending the struct at the time of creating the window
			store_previous_page(state, state->current_page);
			set_current_page(state, page_type::show_word);
		}
		else {
			HWND focuswnd = GetFocus();
			int ret = MessageBoxW(state->nc_parent, RCS(300), L"", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
			if (ret == IDYES) {
				//learnt_word2<utf16_str> new_word{ 0 };
				//new_word.attributes.hiragana = { search, (cstr_len(search)+1)*sizeof(*search) };
				//preload_page(state, ProcState::page::new_word, &new_word);
				preload_page(state, page_type::new_word, &search);
				store_previous_page(state, state->current_page);
				set_current_page(state, page_type::new_word);
			}
			else SetFocus(focuswnd);//Restore focus to the edit window since messagebox takes it away 
			//TODO(fran): a way to make this more streamlined would be to implement:
			//int MessageBoxW(...){ oldfocus=getfocus(); messagebox(); setfocus(oldfocus); }
		}
	}

	void searchbox_func_show_on_editbox(HWND editbox, void* element, void* user_extra) {
		learnt_word16* word = (decltype(word))element;
		ProcState* state = (decltype(state))user_extra;

		utf16_str txt = str_for(word, state->pagestate.search.search_type);

		SendMessage(editbox, WM_SETTEXT_NO_NOTIFY, 0, (LPARAM)txt.str);
	}

	void searchbox_func_listbox_render(HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
		int w = r.w, h = r.h;
		learnt_word16* txt = (decltype(txt))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk;
		if (flags.onSelected || flags.onMouseover)bk_br = global::colors.ControlBkMouseOver;
		if (flags.onClicked) bk_br = global::colors.ControlBkPush;

		RECT bk_rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		FillRect(dc, &bk_rc, bk_br);

		//Draw text
		i32 third_w = r.w / 3;
		rect_i32 tempr = r; tempr.w = third_w;

		RECT hira_rc = to_RECT(tempr);

		tempr.left += tempr.w;

		RECT kanji_rc = to_RECT(tempr);

		tempr.left += tempr.w;

		RECT meaning_rc = to_RECT(tempr);

		urender::draw_text(dc, hira_rc, txt->attributes.hiragana, global::fonts.General, brush_for(learnt_word_elem::hiragana), bk_br, urender::txt_align::left, 3);
		urender::draw_text(dc, kanji_rc, txt->attributes.kanji, global::fonts.General, brush_for(learnt_word_elem::kanji), bk_br, urender::txt_align::left, 3);
		urender::draw_text(dc, meaning_rc, txt->attributes.meaning, global::fonts.General, brush_for(learnt_word_elem::meaning), bk_br, urender::txt_align::left, 3);
	}


	void create(ProcState* state) {
		auto& navbar = state->pages.navbar;
		navbar = CreateWindowW(navbar::wndclass, NULL, WS_CHILD | WS_VISIBLE //TODO(fran): WS_CLIPCHILDREN?
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		navbar::set_theme(navbar, &nav_theme);

		HWND button_three_lines = CreateWindowW(button::wndclass, NULL, style_button_bmp | WS_VISIBLE
			, 0, 0, 0, 0, navbar, 0, NULL, NULL);
		//TODO(fran): try one pixel thick lines, and also try only two lines
		//AWT(controls.button_modify, 273);
		button::set_theme(button_three_lines, &navbar_img_btn_theme);
		SendMessage(button_three_lines, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.threeLines);
		button::set_user_extra(button_three_lines, state);
		button::set_function_on_click(button_three_lines,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				study_sidebar::animate(state);
			}
		);
		navbar::attach(navbar, button_three_lines, navbar::attach_point::left, -1);

		HWND button_new = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
			, 0, 0, 0, 0, navbar, 0, NULL, NULL);
		AWT(button_new, 100);
		button::set_theme(button_new, &navbar_btn_theme);
		button::set_user_extra(button_new, state);
		button::set_function_on_click(button_new, button_function_on_click_goto_page_new);
		navbar::attach(navbar, button_new, navbar::attach_point::left, -1);
		SendMessage(button_new, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

		HWND button_practice = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
			, 0, 0, 0, 0, navbar, 0, NULL, NULL);
		AWT(button_practice, 101);
		button::set_theme(button_practice, &navbar_btn_theme);
		button::set_user_extra(button_practice, state);
		button::set_function_on_click(button_practice, button_function_on_click_goto_page_practice);
		navbar::attach(navbar, button_practice, navbar::attach_point::left, -1);
		SendMessage(button_practice, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

		edit_oneline::Theme search_editoneline_theme = base_editoneline_theme;
		search_editoneline_theme.brushes.bk.normal = CreateSolidBrush(RGB(30, 31, 25));
		search_editoneline_theme.brushes.border = search_editoneline_theme.brushes.bk;

		HWND search = CreateWindowW(searchbox::wndclass, NULL, WS_CHILD | WS_TABSTOP | SRB_ROUNDRECT | WS_VISIBLE
			, 0, 0, 0, 0, navbar, 0, NULL, NULL);
		ACC(search, 251);
		searchbox::set_editbox_theme(search, &search_editoneline_theme);
		searchbox::set_user_extra(search, state);
		searchbox::set_function_free_elements(search, searchbox_func_free_elements);
		searchbox::set_function_retrieve_search_options(search, searchbox_func_retrieve_search_options);
		searchbox::set_function_perform_search(search, searchbox_func_perform_search);
		searchbox::set_function_show_element_on_editbox(search, searchbox_func_show_on_editbox);
		searchbox::set_function_render_listbox_element(search, searchbox_func_listbox_render);
		searchbox::maintain_placerholder_when_focussed(search, false);
		edit_oneline::set_IME_wnd(searchbox::get_controls(search).editbox, true);
		navbar::attach(navbar, search, navbar::attach_point::center, -1);
		SendMessage(search, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		//TODO(fran): searchbox: changing color of editbox text based on whether it has meaning,hiragana,kanji?
		//TODO(fran): searchbox: restore what the user wrote when they press the escape key
	}
}