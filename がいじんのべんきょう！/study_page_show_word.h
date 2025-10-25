#pragma once
namespace べんきょう {
	namespace show_word {
		bool check_show_word(ProcState* state) {
			//TODO(fran): macro to check both new_word and show_word (show_word will now have some of its static edit controls changed to edit controls instead)
			auto& page = state->pages.show_word;
			HWND edit_required[] = { page.edit_meaning };
			for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
				int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
				if (!sz_char) {
					edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
					return false;
				}
			}
			//TODO(fran): avoid checks if it is a radical
			{
				HWND edit = page.edit_kanji;
				const auto& txt = edit_oneline::get_state(edit)->char_text;
				if (!txt.empty()) {
					if (!std::any_of(txt.begin(), txt.end(), [](utf16 c) {return is_kanji(c); }) //at least one kanji
						|| !std::all_of(txt.begin(), txt.end(), [](utf16 c) {return is_hiragana(c) || is_kanji(c); })//only kanji/hira
						) {
						edit_oneline::show_tip(edit, RCS(13), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
						return false;
					}
				}
			}
			return true;
		}

		bool modify_word(ProcState* state) {
			bool res = false;
			if (check_show_word(state)) {
				learnt_word16 w16; defer{ w16.free(); };
				auto& page = state->pages.show_word;

				_get_edit_str(page.static_id, w16.attributes.id);
				_get_edit_str(page.edit_hiragana, w16.attributes.hiragana);
				_get_edit_str(page.edit_kanji, w16.attributes.kanji);
				_get_edit_str(page.edit_meaning, w16.attributes.meaning);
				_get_edit_str(page.edit_mnemonic, w16.attributes.mnemonic);
				_get_edit_str(page.edit_notes, w16.attributes.notes);
				_get_edit_str(page.edit_example_sentence, w16.attributes.example_sentence);
				_get_combo_sel_idx_as_str(page.combo_lexical_category, w16.attributes.lexical_category);

				res = update_word(state->settings->db, w16);
			}
			return res;
		}

		learnt_word8 show_word_getPks(ProcState* state) {//TODO(fran): do not convert to utf8, let the db decide what to do with our utf16
			auto& page = state->pages.show_word;

			learnt_word16 word16;
			_get_edit_str(page.static_id, word16.attributes.id); defer{ free_any_str(word16.attributes.id); };
			static_assert(word16.pk_count == 1, "Additional primary keys need to be retrieved");

			learnt_word8 word8;
			for (int i = 0; i < word8.pk_count; i++) word8.all[i] = s16_to_s8(word16.all[i]);

			return word8;
		}

		bool remove_word(ProcState* state) {
			bool res = false;

			learnt_word8 word = show_word_getPks(state); defer{ word.free_pks(); };

			res = delete_word(state->settings->db, word);
			return res;
		}

		bool prioritize_word(ProcState* state) {
			//TODO(fran): should accept any word, not manually take it from the UI
			bool res = false;
			learnt_word8 word = show_word_getPks(state); defer{ word.free_pks(); };

			res = reset_word_priority(state->settings->db, word);

			return res;
		}


		void preload_page(ProcState* state, page_controls& controls, stored_word16* word_to_show) {
			//IDEA: in this page we could reuse the controls from new_word, that way we first call preload_page(new_word) with word_to_show.user_defined and then do our thing (this idea doesnt quite work)

			SendMessageW(controls.static_id, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.id.str);
			SendMessageW(controls.edit_hiragana, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.hiragana.str);
			SendMessageW(controls.edit_kanji, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.kanji.str);
			SendMessageW(controls.edit_meaning, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.meaning.str);
			SendMessageW(controls.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.mnemonic.str);
			SendMessageW(controls.edit_notes, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.notes.str);
			SendMessageW(controls.edit_example_sentence, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.example_sentence.str);

			int lex_categ_sel = get_lexical_category(word_to_show->user_defined.attributes.lexical_category);
			SendMessageW(controls.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

			if (word_to_show->application_defined.attributes.creation_date.str)
				SendMessageW(controls.static_creation_date, WM_SETTEXT, 0, (LPARAM)(RS(270) + L" " + (utf16*)word_to_show->application_defined.attributes.creation_date.str).c_str());

			if (word_to_show->application_defined.attributes.last_practiced_date.str)
				SendMessageW(controls.static_last_practiced_date, WM_SETTEXT, 0, (LPARAM)(RS(271) + L" " + (utf16*)word_to_show->application_defined.attributes.last_practiced_date.str).c_str());

			if (word_to_show->application_defined.attributes.times_right.str && word_to_show->application_defined.attributes.times_practiced.str) {
				std::wstring score = (RS(272) + L" " + (utf16*)word_to_show->application_defined.attributes.times_right.str + L" / " + (utf16*)word_to_show->application_defined.attributes.times_practiced.str);
				try {
					float numerator = std::stof((utf16*)word_to_show->application_defined.attributes.times_right.str);
					float denominator = std::stof((utf16*)word_to_show->application_defined.attributes.times_practiced.str);
					if (denominator != 0.f) {
						int percentage = (int)((numerator / denominator) * 100.f);
						score += L" - " + std::to_wstring(percentage) + L"%";
					}
				}
				catch (...) {}
				SendMessageW(controls.static_score, WM_SETTEXT, 0, (LPARAM)score.c_str());
			}

			button::Theme accent_btn_theme;
			accent_btn_theme.brushes.foreground.normal = global::colors.Accent;
			accent_btn_theme.brushes.border.normal = global::colors.Accent;
			button::set_theme(controls.button_remember, &accent_btn_theme);
		}

		void create_page(ProcState* state) {
			auto& controls = state->pages.show_word;

			controls.page = create_empty_page(state, base_page_theme);

			controls.static_id = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);

			controls.edit_hiragana = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWDT(controls.edit_hiragana, 120);
			edit_oneline::set_theme(controls.edit_hiragana, &hiragana_editoneline_theme);

			controls.edit_kanji = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_kanji, &kanji_editoneline_theme);
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
			AWDT(controls.edit_meaning, 122);

			controls.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_mnemonic, &base_editoneline_theme);
			AWDT(controls.edit_mnemonic, 125);

			controls.edit_example_sentence = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_example_sentence, &base_editoneline_theme);
			AWDT(controls.edit_example_sentence, 127);

			controls.edit_notes = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_notes, &base_editoneline_theme);
			AWDT(controls.edit_notes, 126);

			controls.static_creation_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_creation_date, &base_static_theme);

			controls.static_last_practiced_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_last_practiced_date, &base_static_theme);

			controls.static_score = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_score, &base_static_theme);

			controls.button_modify = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_modify, 273);
			button::set_theme(controls.button_modify, &base_btn_theme);
			button::set_user_extra(controls.button_modify, state);
			button::set_function_on_click(controls.button_modify,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					if (modify_word(state)) {
						goto_previous_page(state);//TODO(fran): dont go back, simply notify the user of the successful modification; or first notify and only later go back
					}
				}
			);

			controls.button_delete = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			//AWT(controls.button_modify, 273);
			button::set_theme(controls.button_delete, &img_btn_theme);
			SendMessage(controls.button_delete, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.bin);
			button::set_user_extra(controls.button_delete, state);
			button::set_function_on_click(controls.button_delete,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					int ret = MessageBoxW(state->nc_parent, RCS(280), L"", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
					if (ret == IDYES) {
						if (remove_word(state)) {
							goto_previous_page(state);
						}
					}
				}
			);

			controls.button_remember = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_remember, 275);
			AWTT(controls.button_remember, 276);
			button::set_theme(controls.button_remember, &accent_btn_theme);
			button::set_user_extra(controls.button_remember, state);
			button::set_function_on_click(controls.button_remember,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					Assert(state->current_page == ProcState::page::show_word);
					auto& page = state->pages.show_word;
					//TODO(fran): change button bk and mouseover color to green
					if (prioritize_word(state)) {
						button::Theme accent_btn_theme;
						accent_btn_theme.brushes.foreground.normal = global::colors.Bk_right_answer;
						accent_btn_theme.brushes.border.normal = global::colors.Bk_right_answer;
						button::set_theme(page.button_remember, &accent_btn_theme);
					}
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
			auto& controls = state->pages.show_word;

			HFONT font = GetWindowFont(controls.edit_meaning);
			int bigwnd_h = wnd_h * 2;
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer edit_hiragana{ controls.edit_hiragana };
			ssizer edit_kanji{ controls.edit_kanji };
			vsizer jp_sizer{
				{&edit_hiragana,bigwnd_h},
				{&lvpad,half_wnd_h},
				{&edit_kanji,bigwnd_h}, //TODO(fran): text editor needs to adapt to font size changing
			};

			ssizer combo_lexical_category{ controls.combo_lexical_category };
			hsizer lexical_category{ {&combo_lexical_category,GetWindowDesiredSize(combo_lexical_category.wnd, { 0,wnd_h }, { (int)(.55f * layout_bounds.cx),wnd_h}).max.cx} };
			ssizer edit_meaning{ controls.edit_meaning };
			ssizer edit_mnemonic{ controls.edit_mnemonic };
			ssizer edit_notes{ controls.edit_notes };
			ssizer edit_example_sentence{ controls.edit_example_sentence };

			vsizer meaning_sizer{
				{&lexical_category,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_meaning,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_mnemonic,GetWindowDesiredSize(edit_mnemonic.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
				{&lvpad,half_wnd_h},
				{&edit_example_sentence,GetWindowDesiredSize(edit_example_sentence.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
				{&lvpad,half_wnd_h},
				{&edit_notes,GetWindowDesiredSize(edit_notes.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
			};

			hsizer word_info{
				{(sizer*)&jp_sizer,(int)(.4f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{(sizer*)&meaning_sizer,(int)(.55f * (f32)layout_bounds.cx)}
			};

			ssizer static_creation_date{ controls.static_creation_date };
			ssizer static_last_practiced_date{ controls.static_last_practiced_date };
			vsizer left_stats{
				{&static_creation_date,wnd_h},
				{&lvpad,half_wnd_h},
				{&static_last_practiced_date,wnd_h},
			};//TODO(fran): vcsizer

			ssizer static_score{ controls.static_score };
			vsizer right_stats{ {&static_score,wnd_h} };

			hsizer word_stats{ //TODO(fran): switch from 2x2 to 1x4 grid depending on width
				{&left_stats,(int)(.45f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{&right_stats,(int)(.45f * (f32)layout_bounds.cx)},
			};

			ssizer button_delete{ controls.button_delete };
			ssizer button_remember{ controls.button_remember };
			ssizer button_modify{ controls.button_modify };

			hrsizer buttons{
				{&button_modify,GetWindowDesiredSize(button_modify.wnd,{200,200},{200,200}).max.cx},
				{&lhpad,half_wnd_h},
				{&button_remember,GetWindowDesiredSize(button_remember.wnd,{200,200},{200,200}).max.cx},
				{&lhpad,half_wnd_h},
				{&button_delete,GetWindowDesiredSize(button_delete.wnd,{200,200},{200,200}).max.cx},
			};//TODO(fran): idk whether I want to reverse the order for hrsizer, so that the first wnd added is leftmost

			vsizer layout{
				{&word_info,word_info.get_bottom({0,0,layout_bounds.cx,h}).y},
				{&lhpad,wnd_h},
				{&word_stats,wnd_h * 3},
				{&lhpad,wnd_h},
				{&buttons,wnd_h},
			};

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			page_scroll(controls.page, w, page_space_h, layout_rc.h);

			layout.resize(layout_rc);
		}
	}
}