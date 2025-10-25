#pragma once
namespace べんきょう {
	namespace practice {
		str GetStringPracticeType(available_practices practice) {
			return RS(1201 + get_bit_set_position((u32)practice));
		}

		str GetStringPracticeWritingVariant(writing::variant practiceType) {
			return RS(1250 + get_bit_set_position((u32)practiceType));
		}

		str GetStringPracticeMultiplechoiceVariant(multiplechoice::variant practiceType) {
			return RS(1260 + get_bit_set_position((u32)practiceType));
		}

		str GetStringPracticeDrawingVariant(drawing::variant practiceType) {
			return RS(1270 + get_bit_set_position((u32)practiceType));
		}


		void set_current_page(ProcState* state) {
			auto& controls = state->pages.practice;

			//TODO(fran): only ask for new values if we know something changed
			//TODO(fran): I dont think we're freeing this if the app is closed, not that it matters in that case though
			ptr<practiced_word16> practiced = get_previously_practiced_words(state->settings->db, 15);

			ptr<void*> elems{ 0 }; elems.alloc(practiced.cnt); defer{ elems.free(); };
			for (size_t i = 0; i < practiced.cnt; i++) elems[i] = &practiced[i];

			{//Free previous elements
				ptr<void*> elements = listbox::get_all_elements(controls.listbox_words_practiced);//HACK
				for (auto e : elements) ((decltype(practiced.mem))e)->word.free();
				if (elements.cnt)free(elements[0]);
			}

			listbox::set_elements(controls.listbox_words_practiced, elems.mem, elems.cnt);
		}

		void create_page(ProcState* state) {
			auto& controls = state->pages.practice;
			auto& pagestate = state->pagestate.practice;

			controls.page = create_empty_page(state, base_page_theme);

			auto page = controls.page;

			controls.listbox_words_practiced = CreateWindowW(listbox::wndclass, 0, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_user_extra(controls.listbox_words_practiced, state);
			listbox::set_function_render(controls.listbox_words_practiced,
				[](HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
					int w = r.w, h = r.h;
					practiced_word16* practiced = (decltype(practiced))element;

					//Draw bk
					HBRUSH bk_br = practiced->answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
					if (flags.onSelected || flags.onMouseover)
						bk_br = practiced->answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
					if (flags.onClicked)
						bk_br = practiced->answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;

					HBRUSH hira_br = global::colors.ControlTxt;
					HBRUSH kanji_br = global::colors.ControlTxt;
					HBRUSH meaning_br = global::colors.ControlTxt;

					render_hiragana_kanji_meaning(dc, r, bk_br, hira_br, kanji_br, meaning_br, &practiced->word);
				}
			);
			listbox::set_function_on_click(controls.listbox_words_practiced,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					practiced_word16* practiced = (decltype(practiced))element;

					stored_word16_res res = get_stored_word(state->settings->db, practiced->word);  defer{ if (res.found) free_stored_word(res.word); };
					if (res.found) {
						preload_page(state, ProcState::page::show_word, &res.word);
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::show_word);
					}
					else {
						MessageBoxW(state->wnd, RCS(401), 0, MB_OK, MBP::center);
					}
				}
			);

			controls.button_words_practiced = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.button_words_practiced, 400);
			button::set_theme(controls.button_words_practiced, &dark_nonclickable_btn_theme);
			button::set_user_extra(controls.button_words_practiced, state);
			button::set_function_render(controls.button_words_practiced, button_recents_func_render);
			button::set_function_on_click(controls.button_words_practiced,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					//do nothing, we're here just for looks
				}
			);

			controls.settings_listbox_practices = CreateWindowW(listbox::wndclass, 0, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_user_extra(controls.settings_listbox_practices, state);
			static const auto drawListboxPracticeSettingsItem = [](HDC dc, rect_i32 r, listbox::renderflags flags, treeview_practice_data element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;


				bool selected;
				std::wstring txt;
				if (element.type_header == treeview_practice_type_root) {
					available_practices practiceType = (available_practices)element.practice_type;
					auto& practices_to_perform = state->settings->practices;
					selected = practices_to_perform & (i32)practiceType;
					txt = GetStringPracticeType(practiceType);
				}
				else if (element.type_header == treeview_practice_type_writing_variant) {
					writing::variant practiceType = (writing::variant)element.practice_type;
					auto& practice_variants_to_perform = state->settings->practice_writing_variants;
					selected = practice_variants_to_perform & (i32)practiceType;
					txt = GetStringPracticeWritingVariant(practiceType);
				}
				else if (element.type_header == treeview_practice_type_multiplechoice_variant) {
					multiplechoice::variant practiceType = (multiplechoice::variant)element.practice_type;
					auto& practice_variants_to_perform = state->settings->practice_multiplechoice_variants;
					selected = practice_variants_to_perform & (i32)practiceType;
					txt = GetStringPracticeMultiplechoiceVariant(practiceType);
				}
				else if (element.type_header == treeview_practice_type_drawing_variant) {
				drawing:: variant practiceType = (drawing::variant)element.practice_type;
					auto& practice_variants_to_perform = state->settings->practice_drawing_variants;
					selected = practice_variants_to_perform & (i32)practiceType;
					txt = GetStringPracticeDrawingVariant(practiceType);
				}
				else {
					Assert(0);
				}

				i32 w = r.w, h = r.h;

				//Draw bk
				HBRUSH bk_br = global::colors.CaptionBk;
				if (flags.onSelected || flags.onMouseover)
					bk_br = global::colors.ControlBkMouseOver;
				if (flags.onClicked)
					bk_br = global::colors.ControlBkPush;

				RECT bk_rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
				FillRect(dc, &bk_rc, bk_br);

				HBRUSH txt_br = global::colors.ControlTxt;

				//Draw Checkbox

				f32 box_dim = (f32)minimum(w, h);
				f32 box_pad_percentage = .2f;
				f32 box_w = box_dim * (1 - box_pad_percentage);
				i32 box_pad = (i32)((box_dim * box_pad_percentage) / 2);

				rect_i32 boxr = r;
				boxr.left += box_pad;
				boxr.top += box_pad;
				boxr.w = (i32)box_w;
				boxr.h = (i32)box_w;
				int box_roundedness = maximum(1, (i32)roundf((f32)box_w * .2f));
				int box_thickness = 1;
				{
					HPEN pen = CreatePen(PS_SOLID, box_thickness, ColorFromBrush(txt_br)); defer{ DeletePen(pen); };
					HPEN oldpen = SelectPen(dc, pen); defer{ SelectObject(dc, oldpen); };
					HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
					RoundRect(dc, boxr.left, boxr.top, boxr.right(), boxr.bottom(), box_roundedness, box_roundedness);

					if (selected)
					{
						SelectBrush(dc, txt_br);
						RECT check_rc = to_RECT(boxr);
						i32 delta_dim = -maximum(1, (i32)roundf((f32)box_w * .15f));
						InflateRect(&check_rc, delta_dim, delta_dim);
						RoundRect(dc, check_rc.left, check_rc.top, check_rc.right, check_rc.bottom, box_roundedness, box_roundedness);
					}
				}
				//urender::RoundRectangleBorder_smooth(dc, txt_br, to_RECT(boxr), 10, box_thickness);

				//Draw Text

				HFONT font = global::fonts.General; //TODO(fran): get font from listbox
				RECT txt_rc = to_RECT(r);
				txt_rc.left += (i32)box_dim;

				urender::draw_text(dc, txt_rc, to_utf16_str(txt), font, txt_br, bk_br, urender::txt_align::left, (int)avg_str_dim(font, 1).cx);
				};
			auto drawTreeviewPracticeSettingsItem = [](HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
				treeview_practice* treeview = (treeview_practice*)element;
				const int rem = avg_str_dim(global::fonts.General, 1).cx;
				const int treeview_w = 4 * rem;
				auto treeview_r = r;
				auto treeview_displacement = treeview_w * treeview->level;
				treeview_r.left += treeview_displacement;
				treeview_r.w = treeview_w;

				RECT bk_rc = to_RECT(r);
				FillRect(dc, &bk_rc, global::colors.CaptionBk);
				if (treeview->children.cnt)
					if (treeview->open)
						draw_bitmap_1bpp(global::bmps.dropdown, dc, treeview_r, 1 * rem);
					else
						draw_bitmap_1bpp(global::bmps.arrow_right, dc, treeview_r, 1 * rem);
				//else 
					//draw_bitmap_1bpp(global::bmps.minimize, dc, treeview_r, 1 * rem);

				r.cut_left(treeview_displacement + treeview_w);
				drawListboxPracticeSettingsItem(dc, r, flags, treeview->data, user_extra);
				};
			listbox::set_function_render(controls.settings_listbox_practices, drawTreeviewPracticeSettingsItem);

			static const auto listbox_practices_set_elements = [](HWND control, treeview_element<bool, treeview_practice, countAvailablePractices>& root) {
				std::vector<void*> elements;
				root.build_treeview(elements);
				void* a = (void*)&elements;
				std::vector<treeview_practice*> b = *(std::vector<treeview_practice*>*)a;
				listbox::set_elements(control, elements.data(), elements.size());
				};

			static auto onClickListboxPracticeSettingsItem = [](treeview_practice_data element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;


				switch (element.type_header) {
				case treeview_practice_type_root: {
					auto& practices_to_perform = state->settings->practices;
					available_practices practiceType = (available_practices)element.practice_type;
					practices_to_perform ^= (int)practiceType;
				} break;
				case treeview_practice_type_writing_variant: {
					auto& practice_variants_to_perform = state->settings->practice_writing_variants;
					writing::variant variant = (writing::variant)element.practice_type;
					practice_variants_to_perform ^= (int)variant;
				} break;
				case treeview_practice_type_multiplechoice_variant: {
					auto& practice_variants_to_perform = state->settings->practice_multiplechoice_variants;
					multiplechoice::variant variant = (multiplechoice::variant)element.practice_type;
					practice_variants_to_perform ^= (int)variant;
				} break;
				case treeview_practice_type_drawing_variant: {
					auto& practice_variants_to_perform = state->settings->practice_drawing_variants;
					drawing::variant variant = (drawing::variant)element.practice_type;
					practice_variants_to_perform ^= (int)variant;
				} break;
				default:
					Assert(0);
				}

				};

			static treeview_element<bool, treeview_practice, countAvailablePractices> settings_practices_root{ (u8)-1, true, 0, {} }; //TODO: move to page state

			static_assert(settings_practices_root.children.cnt_allocd() >= countAvailablePractices, "Enlarge the array");
			static_assert(countAvailablePractices == 3, "Add the new practice to the list");

			static auto onClickTreeviewPracticeSettingsItem = [](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				treeview_practice* treeview = (treeview_practice*)element;

				//TODO: only pass click if it didnt hit the open/close section (right now we have no way to know this, which would indicate that we need to either provide that info from the listbox, or create a treeview component)
				if (treeview->children.cnt) {
					treeview->open = !treeview->open;
					listbox_practices_set_elements(state->pages.practice.settings_listbox_practices, settings_practices_root);
					ask_for_resize(state);
					ask_for_repaint(state);
				}

				onClickListboxPracticeSettingsItem(treeview->data, user_extra);
				};
			listbox::set_function_on_click(controls.settings_listbox_practices, onClickTreeviewPracticeSettingsItem);



			treeview_practice practice_writing{ 0, 0, treeview_practice_data{treeview_practice_type_root, (u8)available_practices::writing}, {} };
			settings_practices_root.add_child(practice_writing, false);
			treeview_practice practice_multiplechoice{ 0, 0, treeview_practice_data{treeview_practice_type_root, (u8)available_practices::multiplechoice}, {} };
			settings_practices_root.add_child(practice_multiplechoice, false);
			treeview_practice practice_drawing{ 0, 0, treeview_practice_data{treeview_practice_type_root, (u8)available_practices::drawing}, {} };
			settings_practices_root.add_child(practice_drawing, false);


			constexpr u32 countWritingVariants = get_enumflag_element_count<writing::variant>();
			static_assert(practice_writing.children.cnt_allocd() >= countWritingVariants, "Enlarge the array");
			for (u8 i = 0; i < countWritingVariants; i++)
			{
				settings_practices_root.children[0].add_child(treeview_practice_variation{ 0, 0, treeview_practice_data{treeview_practice_type_writing_variant, (u8)(1 << i)} }, false);
			}

			constexpr u32 countMultipleChoiceVariants = get_enumflag_element_count<multiplechoice::variant>();
			static_assert(practice_multiplechoice.children.cnt_allocd() >= countMultipleChoiceVariants, "Enlarge the array");
			for (u8 i = 0; i < countMultipleChoiceVariants; i++)
			{
				settings_practices_root.children[1].add_child(treeview_practice_variation{ 0, 0, treeview_practice_data{treeview_practice_type_multiplechoice_variant, (u8)(1 << i)} }, false);
			}

			constexpr u32 countDrawingVariants = get_enumflag_element_count<drawing::variant>();
			static_assert(practice_drawing.children.cnt_allocd() >= countDrawingVariants, "Enlarge the array");
			for (u8 i = 0; i < countDrawingVariants; i++)
			{
				settings_practices_root.children[2].add_child(treeview_practice_variation{ 0, 0, treeview_practice_data{treeview_practice_type_drawing_variant, (u8)(1 << i)} }, false);
			}
			//TODO(fran): actually assigning the level at this point is pointless, we know the levels here cause we have the entire array of childrens structure, the level is something only to be added when inserting the object into the listbox

			listbox_practices_set_elements(controls.settings_listbox_practices, settings_practices_root);

			controls.settings_button_practices = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.settings_button_practices, 1300);
			button::set_theme(controls.settings_button_practices, &dark_btn_theme);
			button::set_user_extra(controls.settings_button_practices, state);
			button::set_function_render(controls.settings_button_practices, button_recents_func_render);
			button::set_function_on_click(controls.settings_button_practices,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					HWND listbox = state->pages.practice.settings_listbox_practices;
					state->pagestate.practice.settings_visibility = !state->pagestate.practice.settings_visibility;

					ask_for_resize(state);
					force_repaint(listbox);
					ask_for_repaint(state);
				}
			);

			controls.button_start = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_start, 350);
			button::set_theme(controls.button_start, &base_btn_theme);
			button::set_user_extra(controls.button_start, state);
			button::set_function_on_click(controls.button_start,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& pagestate = state->pagestate.practice;
					i64 word_cnt = get_user_stats(state->settings->db).word_cnt; //TODO(fran): I dont know if this is the best way to do it but it is the most accurate
					if (!word_cnt)
						MessageBoxW(state->wnd, RCS(360), 0, MB_OK, MBP::center);
					else if (!state->settings->practices)
						MessageBoxW(state->wnd, RCS(361), 0, MB_OK, MBP::center);
					else if ((state->settings->practices & (u32)available_practices::writing) && !state->settings->practice_writing_variants)
						MessageBoxW(state->wnd, RCS(362), 0, MB_OK, MBP::center);
					else if ((state->settings->practices & (u32)available_practices::multiplechoice) && !state->settings->practice_multiplechoice_variants)
						MessageBoxW(state->wnd, RCS(363), 0, MB_OK, MBP::center);
					else if ((state->settings->practices & (u32)available_practices::drawing) && !state->settings->practice_drawing_variants)
						MessageBoxW(state->wnd, RCS(364), 0, MB_OK, MBP::center);
					else
					{
						int practice_cnt = 10;
						state->practice_cnt = (u32)min(word_cnt, practice_cnt);//set the practice counter (and if there arent enough words reduce the practice size, not sure how useful this is)
						store_previous_page(state, state->current_page);

						//Clear previous practice data:
						clear_practices_vector(state->multipagestate.temp_practices);

						//NOTE: there's the possibility of precalculating the entire array of practice levels from the start which "guarantees" to avoid duplicates (same word shown twice), also simplifies the code a bit since there's only one place where we'd allocate and store. But the good thing about not precalculating the array is increased randomness, each time we get a word we get a random choice, therefore say 15 random choices is more random than 1 random choice for 15 words (based on how bad I think sql's random is)

						next_practice_level(state, false);
					}
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
			auto& controls = state->pages.practice;

			listbox::set_dimensions(controls.listbox_words_practiced, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			listbox::set_dimensions(controls.settings_listbox_practices, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			HFONT font = GetWindowFont(controls.button_start);
			i32 _rem = avg_str_dim(font, 1).cx;
			static const auto rem = [=](f32 n) {return n * _rem; };
			f32 layout_bounds_w = (f32)minimum((i32)rem(100), max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer button_words_practiced{ controls.button_words_practiced };
			ssizer listbox_words_practiced{ controls.listbox_words_practiced };
			vsizer practiced_column{
				{&button_words_practiced,wnd_h},
				{&listbox_words_practiced, wnd_h * (int)listbox::get_element_cnt(controls.listbox_words_practiced)}, };

			ssizer settings_button_practices{ controls.settings_button_practices };
			ssizer settings_listbox_practices{ controls.settings_listbox_practices };
			ssizer button_start{ controls.button_start };
			hcsizer start{ {&button_start,rem(10)} };

			i32 settings_w = (i32)minimum(rem(35), layout_bounds_w / 2);
			hcsizer settings_button{ {&settings_button_practices,settings_w} }; //TODO(fran): have I got no way of declaring a general w for both?
			hcsizer settings_list{ {&settings_listbox_practices,settings_w} }; //TODO(fran): ability to define a percentage instead of a fixed number, what I would like here would be 50% of the parent's width

			vsizer start_practice_column{
				{&settings_button,wnd_h},
				{&settings_list, state->pagestate.practice.settings_visibility ? wnd_h * (int)listbox::get_element_cnt(controls.settings_listbox_practices) : 0},
				{&lvpad, h_pad},
				{&start,wnd_h}, };

			hsizer layout{
				{&practiced_column,(int)(.4f * (f32)layout_bounds_w)},
				{&lhpad,(int)(.05f * (f32)layout_bounds_w)},
				{&start_practice_column,(int)(.55f * (f32)layout_bounds_w)} };

			rect_i32 layout_rc;
			layout_rc.w = (i32)layout_bounds_w;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			page_scroll(controls.page, w, page_space_h, layout_rc.h);

			layout.resize(layout_rc);
		}
	}
}