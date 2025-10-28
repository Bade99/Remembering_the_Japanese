#pragma once
namespace study::new_word {
	//returns true if the input is valid and usable, returns false otherwise
	bool check_new_word(ProcState* state) {
		auto& page = state->pages.new_word;
		HWND edit_required[] = { page.edit_hiragana,page.edit_meaning };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
				return false;
			}
		}

		//TODO(fran): we could build smth strange like an array of tuples in order to be able to repeat the same code but use different function calls and string notifs in each case
		utf16_str lex_cat; defer{ free_any_str(lex_cat.str); };
		_get_combo_sel_idx_as_str(page.combo_lexical_category, lex_cat);

		if (get_lexical_category(lex_cat) != lexical_category::radical)
		{
			{
				HWND edit = page.edit_hiragana;
				const auto& txt = edit_oneline::get_state(edit)->char_text;//quick HACK
				if (!std::all_of(txt.begin(), txt.end(), [](utf16 c) {return is_hiragana(c) || is_katakana(c); })) {
					edit_oneline::show_tip(edit, RCS(12), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
					return false;
				}
			}

			{
				HWND edit = page.edit_kanji;
				const auto& txt = edit_oneline::get_state(edit)->char_text;
				if (!txt.empty()) {
					if (!std::any_of(txt.begin(), txt.end(), [](utf16 c) {return is_kanji(c); })//must have at least one kanji
						|| !std::all_of(txt.begin(), txt.end(), [](utf16 c) {return is_hiragana(c) || is_kanji(c); })
						) {
						edit_oneline::show_tip(edit, RCS(13), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
						return false;
					}
				}
			}
		}

		return true;
	}

	bool save_new_word(ProcState* state) {
		bool res = false;
		if (check_new_word(state)) {
			learnt_word16 w16; defer{ w16.free_non_pks(); };

			auto& page = state->pages.new_word;
			_get_edit_str(page.edit_hiragana, w16.attributes.hiragana);
			_get_edit_str(page.edit_kanji, w16.attributes.kanji);
			_get_edit_str(page.edit_meaning, w16.attributes.meaning);
			_get_edit_str(page.edit_mnemonic, w16.attributes.mnemonic);
			_get_edit_str(page.edit_notes, w16.attributes.notes);
			_get_edit_str(page.edit_example_sentence, w16.attributes.example_sentence);
			_get_combo_sel_idx_as_str(page.combo_lexical_category, w16.attributes.lexical_category);

			//TODO(fran): check for similar words, and consult the user whether they want to create it as a new word or cancel cause the same word already exists

			//Now we can finally do the insert
			//TODO(fran): see if there's some way to go straight from utf16 to the db, and to send things like ints without having to convert them to strings. we could show a list, like we do on the landing page, with the similar words, and only if they then click on an element of the list we open a separate window to allow them the see it in full / edit it
			int insert_res = insert_word(state->settings->db, w16);

			//Error handling
			switch (insert_res) {
			case SQLITE_OK: { res = true; } break;
			case SQLITE_CONSTRAINT:
			{
				Assert(0); break;//TODO(fran): check for duplicate words manually, this codepath is not being triggered now
				//TODO(fran): this should actually be a more specific check for word.hiragana but for now we know that's the only constraint check there is

				//The user tried to add a word that already exists, we must notify them about it and ask wether to override the previous word or cancel
				//INFO: we mustnt allow the user to edit the word while the msgbox is active since we dont retrieve the data, we use the one we already have
				//On a separate window we show the previously existing word so the user can compare and or copy it

				{//Open separate page with the currently stored word
					//TODO(fran): streamline this process
					Settings* study_settings = (decltype(study_settings))malloc(sizeof(Settings));//TODO(fran): MEMLEAK: maybe we can say that non primary windows have to release this memory but it's pretty hacky
					RECT study_wnd_rc; GetWindowRect(state->nc_parent, &study_wnd_rc);
					int w = RECTW(study_wnd_rc);
					study_wnd_rc.left = study_wnd_rc.right;
					study_wnd_rc.right += w;
					//TODO(fran): place new window on the left if no space is available on the right
					study_settings->db = state->settings->db;
					study_settings->is_primary_wnd = false;

					unCapNcLpParam study_wnd_param;
					study_wnd_param.client_class_name = study::wndclass;
					study_wnd_param.client_lp_param = study_settings;
					//TODO(fran): tell the window which pages we want it to create, otherwise window creation takes a couple of seconds, hanging the whole application with it

					HWND study_wnd = CreateWindowEx(WS_EX_CONTROLPARENT, nonclient::wndclass, global::app_name, WS_VISIBLE | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						study_wnd_rc.left, study_wnd_rc.top, RECTWIDTH(study_wnd_rc), RECTHEIGHT(study_wnd_rc), nullptr, nullptr, GetModuleHandleW(NULL), &study_wnd_param);
					Assert(study_wnd);

					study::set_brushes(nonclient::get_state(study_wnd)->client, TRUE, global::colors.ControlBk);
					study::set_current_page(study::get_state(nonclient::get_state(study_wnd)->client), page_type::show_word);

					if (stored_word16_res old_word = get_stored_word(state->settings->db, w16); old_word.found) {
						defer{ free_stored_word(old_word.word); };
						study::preload_page(study::get_state(nonclient::get_state(study_wnd)->client), page_type::show_word, &old_word.word);
					}
					UpdateWindow(study_wnd);
				}

				int ret = MessageBoxW(state->nc_parent, RCS(170), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
				if (ret == IDYES) {
					Assert(0); break;//TODO(fran): again, manually check for repeated words, we could offer this option of updating an already existing word with the new contents
					//res = update_word(state->settings->db, &w8);
				}

			} break;
			default: { sqlite_runtime_check(false, state->settings->db); } break;
			}
			//TODO(fran): maybe handle repeated words here
		}
		return res;
	}


	void preload_page(ProcState* state, page_controls& controls, learnt_word16* new_word) {
		//NOTE: any of the values in new_word can be invalid, we gotta check before using
		//TODO(fran): if the controls only had name and no identifier eg "edit" for "edit_hiragana" we could directly map everything with a foreach by having the same name in the word and controls structs
		//NOTE: settext already has null checking
		SendMessageW(controls.edit_hiragana, WM_SETTEXT, 0, (LPARAM)new_word->attributes.hiragana.str);
		SendMessageW(controls.edit_kanji, WM_SETTEXT, 0, (LPARAM)new_word->attributes.kanji.str);
		SendMessageW(controls.edit_meaning, WM_SETTEXT, 0, (LPARAM)new_word->attributes.meaning.str);
		SendMessageW(controls.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)new_word->attributes.mnemonic.str);
		SendMessageW(controls.edit_notes, WM_SETTEXT, 0, (LPARAM)new_word->attributes.notes.str);
		SendMessageW(controls.edit_example_sentence, WM_SETTEXT, 0, (LPARAM)new_word->attributes.example_sentence.str);
		int lex_categ_sel = get_lexical_category(new_word->attributes.lexical_category);
		SendMessageW(controls.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);
	}

	void create_page(ProcState* state){
		auto& controls = state->pages.new_word;

		controls.page = create_empty_page(state, base_page_theme);

		//TODO(fran): hide primary IME window, the one that shows the composition string, we no longer need it now we show the string straight into the editbox
		controls.edit_hiragana = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		edit_oneline::set_theme(controls.edit_hiragana, &hiragana_editoneline_theme);
		edit_oneline::maintain_placerholder_when_focussed(controls.edit_hiragana, true);
		AWDT(controls.edit_hiragana, 120);

		controls.edit_kanji = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		edit_oneline::set_theme(controls.edit_kanji, &kanji_editoneline_theme);
		edit_oneline::maintain_placerholder_when_focussed(controls.edit_kanji, true);
		AWDT(controls.edit_kanji, 121);

		controls.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		SetWindowSubclass(controls.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
		lexical_category_setup_combobox(controls.combo_lexical_category);
		SendMessage(controls.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
		ACC(controls.combo_lexical_category, 123);

		controls.edit_meaning = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		edit_oneline::set_theme(controls.edit_meaning, &meaning_editoneline_theme);
		edit_oneline::maintain_placerholder_when_focussed(controls.edit_meaning, true);
		AWDT(controls.edit_meaning, 122);

		controls.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		edit_oneline::set_theme(controls.edit_mnemonic, &base_editoneline_theme);
		edit_oneline::maintain_placerholder_when_focussed(controls.edit_mnemonic, true);
		AWDT(controls.edit_mnemonic, 125);

		controls.edit_example_sentence = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		edit_oneline::set_theme(controls.edit_example_sentence, &base_editoneline_theme);
		edit_oneline::maintain_placerholder_when_focussed(controls.edit_example_sentence, true);
		AWDT(controls.edit_example_sentence, 127);

		controls.edit_notes = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		edit_oneline::set_theme(controls.edit_notes, &base_editoneline_theme);
		edit_oneline::maintain_placerholder_when_focussed(controls.edit_notes, true);
		AWDT(controls.edit_notes, 126);
		//NOTE: remember that the window switching order because of tabstop is the same as the window creation order

		controls.button_save = CreateWindowW(button::wndclass, NULL, style_button_txt
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		AWT(controls.button_save, 124);
		button::set_theme(controls.button_save, &base_btn_theme);
		button::set_user_extra(controls.button_save, state);
		button::set_function_on_click(controls.button_save,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				if (save_new_word(state)) {
					learnt_word16 empty{ 0 };
					preload_page(state, state->current_page, &empty);//TODO(fran): function restart_page()

					notify(state, notification_relevance::success, RS(600).c_str());
				}
				else notify(state, notification_relevance::error, RS(601).c_str());
			}
		);

		controls.static_notify = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_RIGHT
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		static_oneline::set_theme(controls.static_notify, &base_static_theme);
		SetWindowSubclass(controls.static_notify, NotifyProc, 0, 0);
		Notify_SetTextDuration(controls.static_notify, 2000);


		for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
	}

	void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
		//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned
		auto& controls = state->pages.new_word;

		HFONT font = GetWindowFont(controls.edit_hiragana);
		SIZE layout_bounds = avg_str_dim(font, 100);
		layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

		//IMPORTANT: fran: language feature request, scope unnamed variables in some way (maybe giving names to a scope so you can tell it in which one to live) so we can do the following:
		//	jp_sizer{ {&ssizer(edit_hiragana),wnd_h}, {&ssizer(edit_kanji),wnd_h} };
		//The only way I know to be able to do this is to allocate the variables:
		//	jp_sizer{ {new ssizer(edit_hiragana),wnd_h}, { new ssizer(edit_kanji),wnd_h} };
		//why do we have to pay for memory allocation when it's completely unnecesary and simply a sintax limitation, you can simply declare everything beforehand:
		//	ssizer edit_hiragana{ controls.edit_hiragana };
		//	ssizer edit_kanji{ controls.edit_kanji };
		//	vsizer jp_sizer{ {&edit_hiragana,wnd_h}, {&edit_kanji,wnd_h} };
		//but now you need to give everything a name and your code becomes more bloated and confusing
		//it'd much rather do:
		//	jp_sizer{ {&(scope new_word)ssizer(edit_hiragana),wnd_h}, {&(scope new_word)ssizer(edit_kanji),wnd_h} };

		hpsizer lhpad{};
		vpsizer lvpad{};

		ssizer edit_hiragana{ controls.edit_hiragana };
		ssizer edit_kanji{ controls.edit_kanji };
		vsizer jp_sizer{
			{&edit_hiragana,wnd_h},
			{&lvpad,half_wnd_h},
			{&edit_kanji,wnd_h} };

		ssizer combo_lexical_category{ controls.combo_lexical_category };
		hsizer lexical_category{ {&combo_lexical_category,GetWindowDesiredSize(combo_lexical_category.wnd, { 0,wnd_h }, { (int)(.55f * layout_bounds.cx),wnd_h}).max.cx} };
		ssizer edit_meaning{ controls.edit_meaning };
		ssizer edit_mnemonic{ controls.edit_mnemonic };
		ssizer edit_notes{ controls.edit_notes };
		ssizer edit_example_sentence{ controls.edit_example_sentence };
		ssizer btn_save{ controls.button_save };
		ssizer static_notify{ controls.static_notify };
		hrsizer save{ {&btn_save,avg_str_dim(font, 10).cx}, {&static_notify,layout_bounds.cx / 2} };
		vsizer meaning_sizer{
			{&lexical_category,wnd_h},
			{&lvpad,half_wnd_h},
			{&edit_meaning,wnd_h},
			{&lvpad,half_wnd_h},
			{&edit_mnemonic,GetWindowDesiredSize(edit_mnemonic.wnd, { 0,wnd_h }, { 0,wnd_h * 5 }).max.cy},
			{&lvpad,half_wnd_h},
			{&edit_example_sentence,GetWindowDesiredSize(edit_example_sentence.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
			{&lvpad,half_wnd_h},
			{&edit_notes,GetWindowDesiredSize(edit_notes.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
			//TODO(fran): BUG: text editor's sizing is retained after pressing Add (ie if it occupied 3 lines it still does), it should restart the page correctly
			{&lvpad,half_wnd_h},
			{&save,wnd_h} };

		hsizer layout{
			{&jp_sizer,(int)(.4f * (f32)layout_bounds.cx)},
			{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
			{&meaning_sizer,(int)(.55f * (f32)layout_bounds.cx)} };

		centered_layout(layout, controls.page, layout_bounds.cx, w, h, h_pad, page_space_h);
	}

	void reset_page(ProcState* state) {
		auto& controls = state->pages.new_word;
		_clear_combo_sel(controls.combo_lexical_category);
		_clear_edit(controls.edit_hiragana);
		_clear_edit(controls.edit_kanji);
		_clear_edit(controls.edit_mnemonic);
		_clear_edit(controls.edit_notes);
		_clear_edit(controls.edit_example_sentence);
		_clear_edit(controls.edit_meaning);
	}

	void show_page(ProcState* state, u32 ShowWindow_cmd) {
		auto& controls = state->pages.new_word;
		for (auto ctl : controls.all) ShowWindow(ctl, ShowWindow_cmd);
		ShowWindow(controls.static_notify, SW_HIDE);
	}

	void set_default_focus(ProcState* state) {
		SetFocus(state->pages.new_word.edit_hiragana);
	}
}