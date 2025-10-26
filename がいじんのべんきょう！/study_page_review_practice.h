#pragma once
namespace べんきょう::practice::review {
	//TODO(fran): there are two things I view as possibly necessary extra params: HWND wnd (of the gridview), void* user_extra
	void gridview_practices_renderfunc(HDC dc, rect_i32 r, gridview::render_flags flags, void* element, void* user_extra) {
		practice::practice_header* header = (decltype(header))element;

		//------Render Setup------:
		HBRUSH border_br{ 0 };
		utf16_str txt{ 0 };

		//TODO(fran): we can bake this switch into the header by adding the params answered_correctly and question_str there
		bool answered_correctly = false;
		switch (header->type) {
		case decltype(header->type)::writing:
		{
			practice::practice_writing* data = (decltype(data))header;
			answered_correctly = data->answered_correctly;
#if 1 //TODO(fran): idk which is better
			txt = *data->question;
#else
			txt = data->user_answer;//the user will remember better what they wrote rather than what they saw
#endif
		} break;
		case decltype(header->type)::multiplechoice:
		{
			practice::practice_multiplechoice* data = (decltype(data))header;
			answered_correctly = data->answered_correctly;
#if 1
			txt.str = data->practice->question_str;
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
#else
			txt.str = data->practice->choices[data->user_answer_idx];
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
#endif
		} break;
		case decltype(header->type)::drawing:
		{
			practice::practice_drawing* data = (decltype(data))header;
			answered_correctly = data->answered_correctly;
			txt.str = data->practice->question_str;
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
		} break;
		default: Assert(0);
		}

		if (flags.onMouseover)
			border_br = answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
		else if (flags.onClicked)
			border_br = answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;
		else
			border_br = answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;

		//------Rendering------:
		int w = r.w, h = r.h;
		RECT rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT

		//Draw border
		int thickness = 3;
#if 0
		FillRectBorder(dc, rc, thickness, border_br, BORDERALL);
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
		urender::draw_text_max_coverage(dc, rc, txt, global::fonts.General, global::colors.ControlTxt, urender::txt_align::center);

	}


	void preload_page(ProcState* state, page_controls& controls, decltype(page_state::practices)* practices) {
		//free current elements before switching, I think this is the best place to do it
		practice::clear_practices_vector(state->pagestate.practice_review.practices);

		state->pagestate.practice_review.practices = std::move(*practices);
		size_t practices_cnt = state->pagestate.practice_review.practices.size(); Assert(practices_cnt != 0);
		void** practices_data = (void**)malloc(sizeof(void*) * practices_cnt); defer{ free(practices_data); };
		for (size_t i = 0; i < practices_cnt; i++) practices_data[i] = state->pagestate.practice_review.practices[i];
		gridview::set_elements(controls.gridview_practices, practices_data, practices_cnt);

	}

	void create_page(ProcState* state) {
		auto& controls = state->pages.review_practice;

		controls.page = create_empty_page(state, base_page_theme);

		controls.static_review = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		static_oneline::set_theme(controls.static_review, &base_static_theme);
		AWT(controls.static_review, 450);

		controls.gridview_practices = CreateWindowW(gridview::wndclass, NULL, WS_CHILD
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		//#define TEST_GRIDVIEW
#ifndef TEST_GRIDVIEW
		gridview::set_brushes(controls.gridview_practices, TRUE, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);//TODO(fran): add border brushes
#else
		gridview::set_brushes(controls.gridview_practices, TRUE, global::colors.CaptionBk, 0, global::colors.CaptionBk_Inactive, 0);
#endif
		gridview::set_user_extra(controls.gridview_practices, state);
		gridview::set_function_render_element(controls.gridview_practices, gridview_practices_renderfunc);
		gridview::set_function_on_click_element(controls.gridview_practices,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				void* data = element;
				practice::practice_header* header = (decltype(header))data;
				page_type new_review_page;
				switch (header->type) {
				case decltype(header->type)::writing: new_review_page = decltype	(new_review_page)::review_practice_writing; break;
				case decltype(header->type)::multiplechoice: new_review_page = decltype (new_review_page)::review_practice_multiplechoice; break;
				case decltype(header->type)::drawing: new_review_page = decltype	(new_review_page)::review_practice_drawing; break;
				default: Assert(0);
				}
				preload_page(state, new_review_page, element/*pagedata*/);
				store_previous_page(state, state->current_page);
				set_current_page(state, new_review_page);
			}
		);

		controls.button_continue = CreateWindowW(button::wndclass, NULL, style_button_txt
			, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
		AWT(controls.button_continue, 451);
		button::set_theme(controls.button_continue, &base_btn_theme);
		button::set_user_extra(controls.button_continue, state);
		button::set_function_on_click(controls.button_continue,
			[](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				store_previous_page(state, state->current_page);
				set_current_page(state, page_type::landing);//TODO(fran): this isnt best, it'd be nice if we went back to the practice page but from here it'd require a goto_previous_page and the fact that we know prev page is practice and that we should preload cause it's values have changed
			}
		);

		for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
	}

	void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
		auto& controls = state->pages.review_practice;

		int bigwnd_h = wnd_h * 3;
		int start_y = 0;

		rect_i32 static_review;
		static_review.y = start_y;
		static_review.h = wnd_h * 2;
		static_review.w = max_w;
		static_review.x = (w - static_review.w) / 2;

		rect_i32 gridview_practices;
		gridview_practices.y = static_review.bottom() + h_pad;

		gridview::element_dimensions gridview_practices_dims;
		gridview_practices_dims.border_pad_y = 3;
		gridview_practices_dims.inbetween_pad = { 5,5 };
		gridview_practices_dims.element_dim = { bigwnd_h,bigwnd_h };
		gridview::set_dimensions(controls.gridview_practices, gridview_practices_dims);

		SIZE gridview_practices_wh;
		{
			//TODO(fran): this is the worst code I've written in quite a while, this and the gridview code that was needed need a revision
			const size_t elems_per_row = 5;
			size_t curr_elem_cnt = gridview::get_elem_cnt(controls.gridview_practices);
			i32 full_w = gridview::get_dim_for_elemcnt_elemperrow(controls.gridview_practices, curr_elem_cnt, elems_per_row).cx;

			i32 real_w = min(w - w_pad * 2, full_w);

			i32 max_h = gridview::get_dim_for_elemcnt_elemperrow(controls.gridview_practices, elems_per_row * 4, elems_per_row).cy;//4 rows

			i32 full_h = gridview::get_dim_for_elemcnt_w(controls.gridview_practices, curr_elem_cnt, real_w).cy;

			//SIZE full_dim = gridview::get_dim_for(controls.gridview_practices, row_cnt, elems_per_row);
			gridview_practices_wh = { real_w,min(full_h,max_h) };
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

		page_scroll(controls.page, w, page_space_h, used_h);

		MyMoveWindow_offset(controls.static_review, static_review, FALSE);
		MyMoveWindow_offset(controls.gridview_practices, gridview_practices, FALSE);
		MyMoveWindow_offset(controls.button_continue, button_continue, FALSE);
	}

	void show_page(ProcState* state, u32 ShowWindow_cmd) {
		auto& controls = state->pages.review_practice;
		for (auto ctl : controls.all) ShowWindow(ctl, ShowWindow_cmd);
	}
}