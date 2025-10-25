#pragma once
namespace べんきょう {
	namespace wordbook_all {
		word_filters get_filters(ProcState* state) {
			word_filters res;
			const auto& controls = state->pages.wordbook_all;
			apply_word_order_element((int)SendMessageW(controls.combo_orderby, CB_GETCURSEL, 0, 0), &res.order);
			apply_word_filter_element((int)SendMessageW(controls.combo_filterby, CB_GETCURSEL, 0, 0), &res.filter);
			return res;
		}

		void update_wordlist(ProcState* state) {
			auto& controls = state->pages.wordbook_all;

			word_filters filters = wordbook_all::get_filters(state);
			ptr<learnt_word16> words = get_all_learnt_words(state->settings->db, filters);
			//TODO(fran): new struct reduced_word (or smth like that) that only contains hira,kanji,meaning

			ptr<void*> elems{ 0 }; elems.alloc(words.cnt); defer{ elems.free(); };
			for (size_t i = 0; i < words.cnt; i++) elems[i] = &words[i];

			{//Free previous elements
				ptr<void*> elements = listbox::get_all_elements(controls.listbox_words);//HACK
				for (auto e : elements) ((decltype(words.mem))e)->free();
				if (elements.cnt)free(elements[0]);
			}

			listbox::set_elements(controls.listbox_words, elems.mem, elems.cnt);
		}


		void set_current_page(ProcState* state) {
			update_wordlist(state);
		}

		void create_page(ProcState* state) {
			auto& controls = state->pages.wordbook_all;

			controls.page = create_empty_page(state, base_page_theme);

			auto page = controls.page;

			//controls.static_orderby = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_LEFT | WS_VISIBLE
			//	, 0, 0, 0, 0, page, 0, NULL, NULL);
			//AWT(controls.static_orderby, 900);
			//static_oneline::set_theme(controls.static_orderby, &base_static_theme);

			//controls.static_filterby = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_LEFT | WS_VISIBLE
			//	, 0, 0, 0, 0, page, 0, NULL, NULL);
			//AWT(controls.static_filterby, 901);
			//static_oneline::set_theme(controls.static_filterby, &base_static_theme);

			//TODO(fran): custom rendering, they shouldnt have a border
			controls.combo_orderby = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			word_order_setup_combobox(controls.combo_orderby);
			SetWindowSubclass(controls.combo_orderby, ComboProc, 0, 0);
			SendMessage(controls.combo_orderby, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.combo_orderby, 900);

			controls.combo_filterby = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			word_filter_setup_combobox(controls.combo_filterby);
			SetWindowSubclass(controls.combo_filterby, ComboProc, 0, 0);
			SendMessage(controls.combo_filterby, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.combo_filterby, 901);

			controls.listbox_words = CreateWindowW(listbox::wndclass, 0, WS_CHILD | WS_VISIBLE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_function_render(controls.listbox_words, listbox_recents_func_render);
			listbox::set_user_extra(controls.listbox_words, state);
			listbox::set_function_on_click(controls.listbox_words, [](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				learnt_word16* txt = (decltype(txt))element;

				stored_word16_res res = get_stored_word(state->settings->db, *txt/*TODO(fran): make sure this isnt a copy*/);  defer{ if (res.found) free_stored_word(res.word); };
				if (res.found) {
					preload_page(state, ProcState::page::show_word, &res.word);
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::show_word);
				}
				//TODO(fran): else {notify user of error finding the word}, we need to get good error info from the db functions
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
			auto& controls = state->pages.wordbook_all;

			listbox::set_dimensions(controls.listbox_words, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			HFONT font = GetWindowFont(controls.listbox_words);//TODO(fran): listboxes dont store fonts, change to a different control
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer combo_orderby{ controls.combo_orderby };
			ssizer combo_filterby{ controls.combo_filterby };
			hrsizer filters{ {&combo_orderby,avg_str_dim(font, 20).cx}, {&lhpad,2},{&combo_filterby,avg_str_dim(font, 20).cx} };//TODO(fran): request desired size

			ssizer listbox_words{ controls.listbox_words };
			vsizer layout{
				{&lvpad,wnd_h},
				{&filters,wnd_h},
				{&lvpad,wnd_h},
				{&listbox_words, wnd_h * (int)listbox::get_element_cnt(controls.listbox_words)},
			};

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.h = layout.get_bottom(layout_rc).y;

			page_scroll(controls.page, w, page_space_h, layout_rc.h);

			layout.resize(layout_rc);
		}
	}
}