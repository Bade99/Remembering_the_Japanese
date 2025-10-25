#pragma once
namespace べんきょう {
	namespace practice {
		namespace drawing {
			void preload_page(ProcState* state, page_controls& controls, word* practice) {
				state->pagestate.practice_drawing.practice = practice;

				HBRUSH question_txt_br = brush_for(practice->question_type);

				SendMessageW(controls.static_question, WM_SETTEXT, 0, (LPARAM)practice->question_str);
				static_oneline::Theme static_theme;
				static_theme.brushes.foreground.normal = question_txt_br;
				static_oneline::set_theme(controls.static_question, &static_theme);

				button::Theme button_next_theme;
				button_next_theme.brushes.bk.normal = global::colors.ControlBk;
				button_next_theme.brushes.border.normal = global::colors.Img;
				button_next_theme.brushes.foreground.normal = global::colors.Img;
				button_next_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
				button_next_theme.brushes.bk.clicked = global::colors.ControlBkPush;
				button::set_theme(controls.button_next, &button_next_theme);

				SendMessageW(controls.static_correct_answer, WM_SETTEXT, 0, (LPARAM)practice->question.attributes.kanji.str /*TODO(fran): add answer_str*/);


				EnableWindow(controls.paint_answer, TRUE);
				EnableWindow(controls.button_show_word, FALSE);
				EnableWindow(controls.button_next, FALSE);

				embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->question);
				embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &practice->question);

				{
					HDC _dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,_dc); };
					HDC dc = CreateCompatibleDC(_dc); defer{ DeleteDC(dc); };//TODO(fran): use already existing dc
					int w = 100, h = 50;//TODO(fran): this size is pretty good, though idk how it'll look on different dpi
					HBITMAP paint_placeholder = CreateCompatibleBitmap(_dc, w, h); defer{ DeleteBitmap(paint_placeholder); };
					{
						auto oldbmp = SelectBitmap(dc, paint_placeholder); defer{ SelectBitmap(dc,oldbmp); };
						RECT r{ 0,0,w,h };
						FillRect(dc, &r, paint::get_state(controls.paint_answer)->brushes.bk/*HACK*/);
						utf16 _s[] = L"答え";
						utf16_str s{ _s, sizeof(_s) };
						urender::draw_text_max_coverage(dc, r, s, global::fonts.General, global::colors.ControlTxt_Disabled, urender::txt_align::center);
					}
					paint::set_placeholder(controls.paint_answer, paint_placeholder);
				}
			}

			void create_page(ProcState* state) {
				auto& controls = state->pages.practice_drawing;

				controls.page = create_empty_page(state, base_page_theme);

				controls.static_question = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				static_oneline::set_theme(controls.static_question, &base_static_theme);
				//NOTE: text color will be set according to the type of word being shown

				controls.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				button::set_theme(controls.button_next, &base_btn_theme);
				SendMessage(controls.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

				controls.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
					, 0, 0, 0, 0, controls.page, 0, 0, 0);
				button::set_theme(controls.button_show_word, &base_btn_theme);
				SendMessage(controls.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);
				button::set_user_extra(controls.button_show_word, state);
				button::set_function_on_click(controls.button_show_word,
					[](void* element, void* user_extra) {
						ProcState* state = (decltype(state))user_extra;
						auto& page = state->pages.practice_drawing;
						flip_visibility(page.embedded_show_word_reduced);
						if (IsWindowVisible(page.embedded_show_word_disambiguation))ShowWindow(page.embedded_show_word_disambiguation, SW_HIDE);
					}
				);

				controls.button_right = CreateWindowW(button::wndclass, NULL, style_button_txt
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				AWT(controls.button_right, 500);
				button::set_theme(controls.button_right, &base_btn_theme);//TODO(fran): maybe green bk

				controls.button_wrong = CreateWindowW(button::wndclass, NULL, style_button_txt
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				AWT(controls.button_wrong, 501);
				button::set_theme(controls.button_wrong, &base_btn_theme);//TODO(fran): maybe red bk

				controls.paint_answer = CreateWindow(paint::wndclass, 0, WS_CHILD | WS_VISIBLE //TODO(fran): rounded?
					, 0, 0, 0, 0, controls.page, 0, 0, 0);
				paint::set_brushes(controls.paint_answer, true, brush_for(learnt_word_elem::kanji), global::colors.ControlBk, brush_for(learnt_word_elem::kanji), global::colors.Img_Disabled);
				paint::set_dimensions(controls.paint_answer, 7);//TODO(fran): find good brush size

				controls.static_correct_answer = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				static_oneline::set_theme(controls.static_correct_answer, &kanji_static_theme);

				controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
					0, 0, 0, 0, controls.page, 0, 0, 0);//TODO(fran): must be shown on top of all the other wnds
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
						auto& page = state->pages.practice_drawing;
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
				auto& controls = state->pages.practice_drawing;

				int bigwnd_h = wnd_h * 4;
				int mediumwnd_h = wnd_h * 3;

				HFONT font = GetWindowFont(controls.button_wrong);
				SIZE layout_bounds = avg_str_dim(font, 100);
				layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

				hpsizer lhpad{};
				vpsizer lvpad{};

				ssizer static_question{ controls.static_question };
				ssizer paint_answer{ controls.paint_answer };

				ssizer button_show_disambiguation{ controls.button_show_disambiguation };
				ssizer button_show_word{ controls.button_show_word };
				ssizer button_next{ controls.button_next };
				hcsizer control_buttons{
					{&button_show_disambiguation, wnd_h * 16 / 9},
					{&lhpad,3},
					{&button_show_word, wnd_h * 16 / 9},
					{&lhpad,3},
					{&button_next, wnd_h} };

				ssizer static_correct_answer{ controls.static_correct_answer };

				ssizer button_wrong{ controls.button_wrong };
				ssizer button_right{ controls.button_right };
				hcsizer response_buttons{
					//TODO(fran): better check for the actual char cnt
					{&button_wrong, min(max_w / 2, avg_str_dim((HFONT)SendMessage(controls.button_wrong, WM_GETFONT, 0, 0), 20).cx)},
					{&lhpad,w_pad},
					{&button_right, min(max_w / 2, avg_str_dim((HFONT)SendMessage(controls.button_right, WM_GETFONT, 0, 0), 20).cx)} };

				vsizer layout{
					{&static_question, bigwnd_h},
					{&lvpad,h_pad},
					{&paint_answer, bigwnd_h * 2},
					{&lvpad,h_pad},
					{&control_buttons, wnd_h},
					{&lvpad,h_pad},
					{&static_correct_answer,mediumwnd_h },
					{&lvpad,h_pad},
					{&response_buttons, wnd_h}
				};

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
			}
		}
	}
}