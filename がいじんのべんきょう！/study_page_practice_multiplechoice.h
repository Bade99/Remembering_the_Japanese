#pragma once
namespace study::practice::multiplechoice {
	void preload_page(ProcState* state, page_controls& controls, word* practice) {
		state->pagestate.practice_multiplechoice.practice = practice;

		HBRUSH question_txt_br = brush_for(practice->question_type);
		HBRUSH choice_txt_br = brush_for(practice->choices_type);


		SendMessageW(controls.static_question, WM_SETTEXT, 0, (LPARAM)practice->question_str);
		static_oneline::Theme static_theme;
		static_theme.brushes.foreground.normal = question_txt_br;
		static_oneline::set_theme(controls.static_question, &static_theme);

		multibutton::set_buttons(controls.multibutton_choices, practice->choices);
		button::Theme multibutton_button;
		multibutton_button.brushes.foreground.normal = choice_txt_br;
		multibutton_button.brushes.bk.normal = global::colors.ControlBk;
		multibutton_button.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
		multibutton_button.brushes.bk.clicked = global::colors.ControlBkPush;
		multibutton_button.brushes.border.normal = global::colors.Img;//TODO(fran): = choice_txt_br ?
		multibutton::set_button_theme(controls.multibutton_choices, &multibutton_button);


		button::Theme button_next_theme;
		button_next_theme.brushes.bk.normal = global::colors.ControlBk;
		button_next_theme.brushes.border.normal = global::colors.Img;
		button_next_theme.brushes.foreground.normal = global::colors.Img;
		button_next_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
		button_next_theme.brushes.bk.clicked = global::colors.ControlBkPush;
		button::set_theme(controls.button_next, &button_next_theme);


		EnableWindow(controls.button_show_word, FALSE);


		embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->question);
		embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &practice->question);
	}

	void create_page(ProcState* state) {
		auto& controls = state->pages.practice_multiplechoice;

		controls.page = create_empty_page(state, base_page_theme);

		controls.static_question = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		static_oneline::set_theme(controls.static_question, &base_static_theme);
		//NOTE: text color will be set according to the type of word being shown

		//TODO(fran): should I simply use a gridview instead?
		controls.multibutton_choices = CreateWindowW(multibutton::wndclass, 0, WS_CHILD | WS_CLIPCHILDREN | multibutton::style::roundrect
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		multibutton::Theme multibutton_choices_theme;
		multibutton_choices_theme.dimensions.border_thickness = 1;
		multibutton_choices_theme.brushes.bk.normal = global::colors.ControlBk;//TODO(fran): try with a different color to make it destacar
		multibutton_choices_theme.brushes.border.normal = global::colors.Img;
		multibutton::set_theme(controls.multibutton_choices, &multibutton_choices_theme);
		//NOTE: buttons' colors will be set according to the type of word that has to be written

		controls.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		SendMessage(controls.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

		controls.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
			, 0, 0, 0, 0, controls.page, 0, 0, 0);
		button::set_theme(controls.button_show_word, &base_btn_theme);
		SendMessage(controls.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);
		button::set_user_extra(controls.button_show_word, state);
		button::set_function_on_click(controls.button_show_word,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				auto& page = state->pages.practice_multiplechoice;
				flip_visibility(page.embedded_show_word_reduced);
				if (IsWindowVisible(page.embedded_show_word_disambiguation))ShowWindow(page.embedded_show_word_disambiguation, SW_HIDE);
			}
		);

		controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
			0, 0, 0, 0, controls.page, 0, 0, 0);
		embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

		controls.button_show_disambiguation = CreateWindowW(button::wndclass, NULL, style_button_bmp
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		button::set_theme(controls.button_show_disambiguation, &base_btn_theme);
		SendMessage(controls.button_show_disambiguation, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.disambiguation);
		AWTT(controls.button_show_disambiguation, 700);
		button::set_user_extra(controls.button_show_disambiguation, state);
		button::set_function_on_click(controls.button_show_disambiguation,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				auto& page = state->pages.practice_multiplechoice;
				flip_visibility(page.embedded_show_word_disambiguation);
				if (IsWindowVisible(page.embedded_show_word_reduced))ShowWindow(page.embedded_show_word_reduced, SW_HIDE);
			}
		);

		controls.embedded_show_word_disambiguation = CreateWindow(embedded::show_word_disambiguation::wndclass, NULL, WS_CHILD | embedded::show_word_disambiguation::style::roundrect,
			0, 0, 0, 0, controls.page, 0, 0, 0);
		embedded::show_word_disambiguation::set_theme(controls.embedded_show_word_disambiguation, &eswd_theme);

		for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
	}

	void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
		auto& controls = state->pages.practice_multiplechoice;

		int start_y = 0;
		int bigwnd_h = wnd_h * 4;

		HFONT font = GetWindowFont(controls.multibutton_choices);
		SIZE layout_bounds = avg_str_dim(font, 100);
		layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

		hpsizer lhpad{};
		vpsizer lvpad{};

		ssizer static_question{ controls.static_question };
		ssizer multibutton_choices{ controls.multibutton_choices };

		ssizer button_show_disambiguation{ controls.button_show_disambiguation };
		ssizer button_show_word{ controls.button_show_word };
		ssizer button_next{ controls.button_next };
		hcsizer control_buttons{
			{&button_show_disambiguation, wnd_h * 16 / 9},
			{&lhpad,3},
			{&button_show_word, wnd_h * 16 / 9},
			{&lhpad,3},
			{&button_next, wnd_h} };

		vsizer layout{
			{&static_question,bigwnd_h},
			{&lvpad, h_pad},
			{&multibutton_choices,bigwnd_h},
			{&lvpad, h_pad},
			{&control_buttons,wnd_h} };

		rect_i32 layout_rc;
		layout_rc.w = layout_bounds.cx;
		layout_rc.y = 0;
		layout_rc.h = h;
		layout_rc.x = (w - layout_rc.w) / 2;
		layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

		page_scroll(controls.page, w, page_space_h, layout_rc.h);

		layout.resize(layout_rc);

		rect_i32 embedded_show_word_reduced;
		RECT _button_show_word_rc;  GetWindowRect(controls.button_show_word, &_button_show_word_rc); MapWindowRect(0, controls.page, &_button_show_word_rc);
		auto button_show_word_rc = to_rect_i32(_button_show_word_rc);
		embedded_show_word_reduced.w = layout_bounds.cx;
		embedded_show_word_reduced.h = wnd_h * 3;
		embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
		embedded_show_word_reduced.y = button_show_word_rc.bottom() + 3;

		MyMoveWindow(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);

		rect_i32 embedded_show_word_disambiguation = embedded_show_word_reduced;

		MyMoveWindow(controls.embedded_show_word_disambiguation, embedded_show_word_disambiguation, FALSE);


		multibutton::Theme multibutton_choices_theme;
		multibutton_choices_theme.dimensions.btn = { avg_str_dim((HFONT)SendMessage(controls.multibutton_choices, WM_GETFONT, 0, 0), 15).cx,wnd_h };
		multibutton_choices_theme.dimensions.inbetween_pad = { 3,3 };
		multibutton::set_theme(controls.multibutton_choices, &multibutton_choices_theme);
	}

	void handle_event(ProcState* state, HWND event_wnd, u64 event_data) {
		auto& page = state->pages.practice_multiplechoice;
		auto& pagestate = state->pagestate.practice_multiplechoice;
		bool already_answered = IsWindowEnabled(page.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
		if (event_wnd == page.multibutton_choices) {
			//The user has selected a choice
			if (!already_answered) {
				size_t user_answer_idx = event_data;
				bool answered_correctly = pagestate.practice->idx_answer == user_answer_idx;


				HBRUSH bk = answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
				HBRUSH bk_mouseover = answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
				HBRUSH bk_push = answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;
				button::Theme multibtn_btn_theme;
				multibtn_btn_theme.brushes.bk.normal = bk;
				multibtn_btn_theme.brushes.bk.mouseover = bk_mouseover;
				multibtn_btn_theme.brushes.bk.clicked = bk_push;
				multibtn_btn_theme.brushes.border.normal = bk;
				multibtn_btn_theme.brushes.border.mouseover = bk_mouseover;
				multibtn_btn_theme.brushes.border.clicked = bk_push;
				multibtn_btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
				multibutton::set_button_theme(page.multibutton_choices, &multibtn_btn_theme, user_answer_idx);
				//TODO(fran): when the user answers incorrectly we want to visually show the correct answer too, maybe with a lower brightness global::colors.Bk_right_answer or a yellow

				button::Theme btn_theme;
				btn_theme.brushes.bk.normal = bk;
				btn_theme.brushes.bk.mouseover = bk_mouseover;
				btn_theme.brushes.bk.clicked = bk_push;
				btn_theme.brushes.border.normal = bk;
				btn_theme.brushes.border.mouseover = bk_mouseover;
				btn_theme.brushes.border.clicked = bk_push;
				btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
				button::set_theme(page.button_next, &btn_theme);

				//Update word stats
				word_update_last_practiced_date(state->settings->db, pagestate.practice->question);
				word_increment_times_practiced__times_right(state->settings->db, pagestate.practice->question, answered_correctly);

				//Add this practice to the list of current completed ones
				practice::practice_multiplechoice* p = (decltype(p))malloc(sizeof(*p));
				p->header.type = decltype(p->header.type)::multiplechoice;
				p->practice = pagestate.practice;
				pagestate.practice = nullptr;//clear the pointer just in case
				p->answered_correctly = answered_correctly;
				p->user_answer_idx = user_answer_idx;

				state->multipagestate.temp_practices.push_back((practice::practice_header*)p);

				EnableWindow(page.button_show_word, TRUE);
				if (answered_correctly) practice::next_practice_level(state);
			}
		}
		else if (event_wnd == page.button_next) {
			if (already_answered) {
				practice::next_practice_level(state, false);
			}
		}
		/*else if (child == page.button_show_word) {
			flip_visibility(page.embedded_show_word_reduced);
		}*/
		else
		{
			printf("FIX ERROR\n");
			//NOTE: we're getting an EN_KILLFOCUS from the edit control in practice_writing, TODO(fran): do like windows and add a msg to specify which notifications you want to receive from a specific control
		}
	}

	void show_page(ProcState* state, u32 ShowWindow_cmd) {
		auto& controls = state->pages.practice_multiplechoice;
		for (auto ctl : controls.all) ShowWindow(ctl, ShowWindow_cmd);
		ShowWindow(controls.embedded_show_word_reduced, SW_HIDE);
		ShowWindow(controls.embedded_show_word_disambiguation, SW_HIDE);
	}
}