#pragma once
namespace べんきょう {
	namespace practice {
		namespace writing {
			void preload_page(ProcState* state, page_controls& controls, word* practice) {
				//TODO(fran): shouldnt preload_page also call clear_page?
				//store data for future proof checking
				state->pagestate.practice_writing.practice = practice;

				utf16* test_word{ 0 };//NOTE: compiler cant know that these guys will always be initialized so I gotta zero them
				HBRUSH test_word_br{ 0 };
				str answer_placeholder;//NOTE: using string so the object doesnt get destroyed inside the switch statement
				HBRUSH answer_br{ 0 };

				str answer_hiragana{ L"こたえ" };

				bool show_ime_suggestions = true;

				switch (practice->practice_type) {
				case decltype(practice->practice_type)::hiragana_to_meaning:
				{
					test_word = (utf16*)practice->word.attributes.hiragana.str;
					test_word_br = brush_for(learnt_word_elem::hiragana);
					//TODO(fran): idk if I should put "meaning" or "answer", and for hiragana "hiragana" or "こたえ" (meaning answer), and "kanji" or "答え"
					answer_placeholder = RS(380);
					answer_br = brush_for(learnt_word_elem::meaning);
				} break;
				case decltype(practice->practice_type)::kanji_to_hiragana:
				{
					test_word = (utf16*)practice->word.attributes.kanji.str;
					test_word_br = brush_for(learnt_word_elem::kanji);

					answer_placeholder = answer_hiragana;
					answer_br = brush_for(learnt_word_elem::hiragana);

					show_ime_suggestions = false;
				} break;
				case decltype(practice->practice_type)::kanji_to_meaning:
				{
					test_word = (utf16*)practice->word.attributes.kanji.str;
					test_word_br = brush_for(learnt_word_elem::kanji);

					answer_placeholder = RS(380);
					answer_br = brush_for(learnt_word_elem::meaning);
				} break;
				case decltype(practice->practice_type)::meaning_to_hiragana:
				{
					test_word = (utf16*)practice->word.attributes.meaning.str;
					test_word_br = brush_for(learnt_word_elem::meaning);

					answer_placeholder = answer_hiragana;
					answer_br = brush_for(learnt_word_elem::hiragana);

					show_ime_suggestions = false;
				} break;
				default:Assert(0);
				}

				SendMessageW(controls.static_test_word, WM_SETTEXT, 0, (LPARAM)test_word);
				static_oneline::Theme static_theme;
				static_theme.brushes.foreground.normal = test_word_br;
				static_oneline::set_theme(controls.static_test_word, &static_theme);

				edit_oneline::Theme editoneline_theme;
				editoneline_theme.dimensions.border_thickness = 1;
				editoneline_theme.brushes.foreground.normal = answer_br;
				editoneline_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
				editoneline_theme.brushes.bk.normal = global::colors.ControlBk;
				editoneline_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
				editoneline_theme.brushes.border.normal = global::colors.Img;
				editoneline_theme.brushes.border.disabled = global::colors.Img_Disabled;
				editoneline_theme.brushes.selection.normal = global::colors.Selection;
				editoneline_theme.brushes.selection.disabled = global::colors.Selection_Disabled;

				edit_oneline::set_theme(controls.edit_answer, &editoneline_theme);

				SendMessageW(controls.edit_answer, WM_SETDEFAULTTEXT, 0, (LPARAM)answer_placeholder.c_str());
				edit_oneline::maintain_placerholder_when_focussed(controls.edit_answer, true);
				edit_oneline::set_IME_wnd(controls.edit_answer, !show_ime_suggestions);

				button::Theme btn_theme;
				btn_theme.brushes.bk.normal = global::colors.ControlBk;
				btn_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
				btn_theme.brushes.bk.clicked = global::colors.ControlBkPush;
				btn_theme.brushes.border.normal = global::colors.ControlBk;
				btn_theme.brushes.foreground.normal = global::colors.Img;
				button::set_theme(controls.button_next, &btn_theme);

				EnableWindow(controls.button_show_word, FALSE);

				embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->word);
				embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &practice->word);
			}

			void create_page(ProcState* state) {
				auto& controls = state->pages.practice_writing;

				controls.page = create_empty_page(state, base_page_theme);

				controls.static_test_word = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				static_oneline::set_theme(controls.static_test_word, &base_static_theme);
				//NOTE: text color will be set according to the type of word being shown

				controls.edit_answer = CreateWindowW(edit_oneline::wndclass, 0, WS_CHILD | ES_CENTER | WS_TABSTOP | WS_CLIPCHILDREN | ES_ROUNDRECT
					, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
				//NOTE: text color and default text will be set according to the type of word that has to be written

				controls.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
					, 0, 0, 0, 0, controls.edit_answer, 0, NULL, NULL);
				SendMessage(controls.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

				controls.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
					, 0, 0, 0, 0, controls.page, 0, 0, 0);
				button::set_theme(controls.button_show_word, &base_btn_theme);
				SendMessage(controls.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);
				button::set_user_extra(controls.button_show_word, state);
				button::set_function_on_click(controls.button_show_word,
					[](void* element, void* user_extra) {
						ProcState* state = (decltype(state))user_extra;
						auto& page = state->pages.practice_writing;
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
						auto& page = state->pages.practice_writing;
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
				auto& controls = state->pages.practice_writing;

				int bigwnd_h = wnd_h * 4;
				int smallwnd_h = (i32)(wnd_h * .8f);

				HFONT font = GetWindowFont(controls.edit_answer);
				SIZE layout_bounds = avg_str_dim(font, 100);
				layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

				hpsizer lhpad{};
				vpsizer lvpad{};

				ssizer static_test_word{ controls.static_test_word };
				ssizer _edit_answer{ controls.edit_answer };
				hcsizer edit_answer{ {&_edit_answer,min(max_w, avg_str_dim(GetWindowFont(controls.edit_answer), 20).cx)} };

				ssizer button_disambiguation{ controls.button_show_disambiguation };
				ssizer button_show_word{ controls.button_show_word };
				hcsizer helpers{
					{&button_disambiguation, smallwnd_h * 16 / 9},
					{&lhpad,3},
					{&button_show_word, smallwnd_h * 16 / 9},
				};

				vsizer practice_column{
					{&static_test_word,bigwnd_h},
					{&edit_answer, wnd_h},
					{&lvpad, 3},
					{&helpers,smallwnd_h} };

				hsizer layout{
					{&practice_column,layout_bounds.cx} };

				rect_i32 layout_rc;
				layout_rc.w = layout_bounds.cx;
				layout_rc.y = 0;
				layout_rc.h = h;
				layout_rc.x = (w - layout_rc.w) / 2;
				layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

				page_scroll(controls.page, w, page_space_h, layout_rc.h);

				layout.resize(layout_rc);

				//TODO(fran): resizing for embedded controls via ssizer,...
				rect_i32 button_next;//child inside edit_answer
				RECT _edit_answer_rc;  GetClientRect(controls.edit_answer, &_edit_answer_rc);
				auto edit_answer_rc = to_rect_i32(_edit_answer_rc);
				button_next.y = 1; //the button is inside the edit box (and past the border) //TODO(fran): we should ask the parent for its border size
				button_next.h = edit_answer_rc.h - 2;
				button_next.w = min(button_next.h, max(0, edit_answer_rc.w - 4/*avoid covering rounded borders*/));
				button_next.x = edit_answer_rc.w - button_next.w - 2;//TODO(fran): if the style of the edit box parent is  ES_ROUNDRECT we gotta subtract one more, in this case we went from -1 to -2
				MyMoveWindow(controls.button_next, button_next, FALSE);

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