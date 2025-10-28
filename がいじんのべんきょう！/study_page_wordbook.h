#pragma once
namespace study::wordbook {
	void set_current_page(ProcState* state) {
		auto& controls = state->pages.wordbook;

		time64 upper_bound = I64MAX;
		for (int i = 0; i < ARRAYSIZE(controls.listbox_last_days_words); i++) {
			const auto [start, end] = day_range(get_latest_word_creation_date(state->settings->db, upper_bound));
			upper_bound = start - 1;

			//TODO(fran): get and set words in different threads
			ptr<learnt_word16> words = get_learnt_word_by_date(state->settings->db, start, end);

			ptr<void*> elems{ 0 }; elems.alloc(words.cnt); defer{ elems.free(); };
			for (size_t i = 0; i < words.cnt; i++) elems[i] = &words[i];

			HWND listbox = controls.listbox_last_days_words[i];
			{//Free previous elements
				ptr<void*> elements = listbox::get_all_elements(listbox);//HACK
				for (auto e : elements) ((decltype(words.mem))e)->free();
				if (elements.cnt)free(elements[0]);
			}

			listbox::set_elements(listbox, elems.mem, elems.cnt);
		}
	}

	void create_page(ProcState* state) {
		auto& controls = state->pages.wordbook;

		controls.page = create_empty_page(state, base_page_theme);

		auto page = controls.page;

		controls.button_all_words = CreateWindowW(button::wndclass, NULL, style_button_txt
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		AWT(controls.button_all_words, 800);
		button::set_theme(controls.button_all_words, &base_btn_theme);
		button::set_user_extra(controls.button_all_words, state);
		button::set_function_on_click(controls.button_all_words,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				store_previous_page(state, state->current_page);
				set_current_page(state, page_type::wordbook_all);
			}
		);

		for (auto& listbox : controls.listbox_last_days_words) {
			listbox = CreateWindowW(listbox::wndclass, 0, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_function_render(listbox, listbox_recents_func_render);
			listbox::set_user_extra(listbox, state);
			listbox::set_function_on_click(listbox, [](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				learnt_word16* txt = (decltype(txt))element;

				stored_word16_res res = get_stored_word(state->settings->db, *txt/*TODO(fran): make sure this isnt a copy*/);  defer{ if (res.found) free_stored_word(res.word); };
				if (res.found) {
					preload_page(state, page_type::show_word, &res.word);
					store_previous_page(state, state->current_page);
					set_current_page(state, page_type::show_word);
				}
				//TODO(fran): else {notify user of error finding the word}, we need to get good error info from the db functions
				}
			);
		}

		for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
	}

	void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
		auto& controls = state->pages.wordbook;

		for (const auto& listbox : controls.listbox_last_days_words)
			listbox::set_dimensions(listbox, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

		HFONT font = GetWindowFont(controls.button_all_words);//TODO(fran): listboxes dont store fonts, change to a different control
		SIZE layout_bounds = avg_str_dim(font, 100);
		layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

		hpsizer lhpad{};
		vpsizer lvpad{};

		ssizer _button_all_words{ controls.button_all_words };
		hcsizer button_all_words{ {&_button_all_words, GetWindowDesiredSize(_button_all_words.wnd,{200,200},{200,200}).max.cx} };

		ssizer listbox_last_days_words[ARRAYSIZE(controls.listbox_last_days_words)]{ controls.listbox_last_days_words[0],controls.listbox_last_days_words[1],controls.listbox_last_days_words[2],controls.listbox_last_days_words[3] };
		static_assert(ARRAYSIZE(listbox_last_days_words) == 4);//TODO(fran): make parametric somehow

		vsizer lists_left{
			{&listbox_last_days_words[0],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[0].wnd)},
			{&lvpad,half_wnd_h},
			{&listbox_last_days_words[2],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[2].wnd)} };

		vsizer lists_right{
			{&listbox_last_days_words[1],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[1].wnd)},
			{&lvpad,half_wnd_h},
			{&listbox_last_days_words[3],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[3].wnd)} };

		hsizer lists{
			{&lists_left,(int)(.475f * (f32)layout_bounds.cx)},
			{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
			{&lists_right,(int)(.475f * (f32)layout_bounds.cx)},
		};

		vsizer layout{
			{&lvpad,wnd_h},
			{&button_all_words, wnd_h},//TODO(fran): standard layout logic tells me the button should go last of all, bottom-most
			{&lvpad,wnd_h},
			{&lists,lists.get_bottom({ 0,0,0,0 }).y},
		};

		normal_layout(layout, controls.page, layout_bounds.cx, w, h, h_pad, page_space_h);
	}

	void show_page(ProcState* state, u32 ShowWindow_cmd) {
		auto& controls = state->pages.wordbook;
		for (auto ctl : controls.all) ShowWindow(ctl, ShowWindow_cmd);
	}
}