#pragma once
namespace べんきょう {
	namespace study_sidebar {
		constexpr utf16 state_key[] = L"sidebar_state_animate";

		constexpr i32 animation_time_ms = 150;

		struct animation_state {
			rect_i32 origin;//Where we started moving //TODO(fran): there may be a way of doing this without the origin
			rect_i32 target;//Where the sidebar will be placed on the last frame of animation, the sidebar's width and height will be set equal to the target's during the whole animation
			bool show;//if show == true sidebar is shown on first animation frame, if false sidebar is hidden on last animation frame
			f32 t; //[0.0,1.0]
			f32 dt;//increment for 't' per frame
		};

		void create(ProcState* state) {
			auto& sidebar = state->pages.sidebar;
			sidebar = CreateWindowW(navbar::wndclass, NULL, WS_CHILD //TODO(fran): WS_CLIPCHILDREN?
				, -2000/*HACK: simplifies sidebar handling, we know for sure it isnt visible at the start, nor if we call showwindow, otherwise it can do a ghost appearance for one frame*/, 0, 0, 0, state->wnd, 0, NULL, NULL);
			navbar::set_theme(sidebar, &sidebar_theme);

			//TODO(fran): left align everything, also add icons together with the text

			//TODO(fran): fix flickering

			HWND button_landing = CreateWindowW(button::wndclass, NULL, style_button_icon | WS_VISIBLE
				, 0, 0, 0, 0, sidebar, 0, NULL, NULL);
			button::set_theme(button_landing, &navbar_img_btn_theme);
			//TODO(fran): store application icon on global::
			HICON ico_logo; LoadIconMetric(GetModuleHandle(0), MAKEINTRESOURCE(ICO_LOGO), LIM_LARGE, &ico_logo); //TODO(fran): #free
			SendMessage(button_landing, BM_SETIMAGE, IMAGE_ICON, (LPARAM)ico_logo);
			button::set_user_extra(button_landing, state);
			button::set_function_on_click(button_landing,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;

					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::landing);
				}
			);
			navbar::attach(sidebar, button_landing, navbar::attach_point::left, -1);

			HWND button_new = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, sidebar, 0, NULL, NULL);
			AWT(button_new, 100);
			button::set_theme(button_new, &navbar_btn_theme);
			button::set_user_extra(button_new, state);
			button::set_function_on_click(button_new, button_function_on_click_goto_page_new);
			navbar::attach(sidebar, button_new, navbar::attach_point::left, -1);
			SendMessage(button_new, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			HWND button_practice = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, sidebar, 0, NULL, NULL);
			AWT(button_practice, 101);
			button::set_theme(button_practice, &navbar_btn_theme);
			button::set_user_extra(button_practice, state);
			button::set_function_on_click(button_practice, button_function_on_click_goto_page_practice);
			navbar::attach(sidebar, button_practice, navbar::attach_point::left, -1);
			SendMessage(button_practice, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			HWND button_wordbook = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, sidebar, 0, NULL, NULL);
			AWT(button_wordbook, 104);
			button::set_theme(button_wordbook, &navbar_btn_theme);
			button::set_user_extra(button_wordbook, state);
			button::set_function_on_click(button_wordbook,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;

					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::wordbook);
				}
			);
			navbar::attach(sidebar, button_wordbook, navbar::attach_point::left, -1);
			SendMessage(button_wordbook, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			//TODO(fran): add separator before options

			HWND combo_lang = CreateWindowW(combobox::wndclass, NULL, WS_CHILD | WS_VISIBLE
				, 0, 0, 0, 0, sidebar, 0, NULL, NULL);
			languages_setup_combobox(combo_lang);
			combobox::set_user_extra(combo_lang, state);
			combobox::set_function_free_elements(combo_lang, langbox_func_free_elements);
			combobox::set_function_render_combobox(combo_lang, langbox_func_render_combobox);
			combobox::set_function_on_listbox_opening(combo_lang, langbox_func_on_listbox_opening);
			combobox::set_function_on_selection_accepted(combo_lang, langbox_func_on_selection_accepted);
			combobox::set_function_desired_size_combobox(combo_lang, langbox_func_desired_size);
			combobox::set_function_render_listbox_element(combo_lang, langbox_func_render_listbox_element);
			navbar::attach(sidebar, combo_lang, navbar::attach_point::right, -1);
			SendMessage(sidebar, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		void animate(ProcState* state, u32 ms = animation_time_ms) {
			HWND sidebar = state->pages.sidebar;
			animation_state* sidebar_state = (decltype(sidebar_state))GetPropW(sidebar, state_key);
			const bool show = !IsWindowVisible(sidebar);
			const f32 duration_sec = (f32)ms / 1000.f;
			const u32 total_frames = (u32)ceilf(duration_sec / (1.f / win32_get_refresh_rate_hz(sidebar)));
			const f32 dt = 1.f / total_frames;
			const i32 timer_id = 0x458;

			RECT navrc; GetWindowRect(state->pages.navbar, &navrc); MapWindowRect(0, state->wnd, &navrc);
			RECT siderc; GetClientRect(state->pages.sidebar, &siderc);
			int sidebar_w = RECTW(siderc);
			int sidebar_h = RECTH(siderc);
			//taking as reference point going from outside in:
			rect_i32 origin{ 0 - sidebar_w,navrc.top + RECTH(navrc),sidebar_w,sidebar_h };
			rect_i32 target{ 0,navrc.top + RECTH(navrc),sidebar_w,sidebar_h };

			if (sidebar_state) {
				//opposite animation is currently in progress, we have to start our animation offset by the inverse number of frames
				sidebar_state->t = 1.f - sidebar_state->t;
				sidebar_state->origin = sidebar_state->target;
			}
			else {
				sidebar_state = (decltype(sidebar_state))malloc(sizeof(animation_state));
				SetPropW(sidebar, state_key, sidebar_state);

				sidebar_state->t = 0.f;
				sidebar_state->origin = show ? origin : target;
				//RECT r; GetWindowRect(sidebar, &r);
				//MapWindowRect(0, state->wnd, &r);
				//sidebar_state->origin = to_rect_i32(r);
				//TODO(fran): this is wrong, sidebar origin when there's no animation should be set outside the wnd rect when starting from hidden and on the wnd rect when starting from shown
			}

			sidebar_state->dt = dt;
			sidebar_state->show = show;
			sidebar_state->target = sidebar_state->show ? target : origin;

			static void (*sidebar_anim)(HWND, UINT, UINT_PTR, DWORD) =
				[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
				animation_state* sidebar_state = (decltype(sidebar_state))GetPropW(hwnd, state_key);
				if (sidebar_state) {
					f32 delta = ParametricBlend(sidebar_state->t);
					v2_i32 pos = lerp(sidebar_state->origin.xy, delta, sidebar_state->target.xy);

					//TODO(fran): update target w & h to match the one returned by getclientrect, and with that we should also correct x & y to make sure we dont stop before/after we should

					//TODO(fran): there's a off by one error with the 'y' placement of the window, could be a floating point problem. it seems to go wrong only when doing the anim to show the window, when hiding it takes the correct 'y' I think, also resizing places it on the right 'y'

					MoveWindow(hwnd, pos.x, pos.y, sidebar_state->target.w, sidebar_state->target.h, TRUE);//TODO(fran): store f32 y position (PROBLEM with this idea is then I have a different value from the real one, and if someone else moves us then we'll cancel that move on the next scroll, it may be better to live with this imprecise scrolling for now)
					navbar::ask_for_repaint(hwnd);//NOTE: there seem to be two different kinds of painting that are required, I believe movewindow with TRUE as the last param causes repainting of the area left behind after the move, but not of the window that's being moved, so I have to manually ask for the wnd to be redrawn on the new place, which seems very odd to me, this may simply be a problem when multiple windows are overlapping as is the case here
					sidebar_state->t += sidebar_state->dt;
					if (sidebar_state->t <= 1.f) {
						i32 ms = (i32)((1.f / win32_get_refresh_rate_hz(hwnd)) * 1000.f);
						SetTimer(hwnd, anim_id, ms, sidebar_anim);
					}
					else {
						if (!sidebar_state->show) ShowWindow(hwnd, SW_HIDE);
						RemovePropW(hwnd, state_key);
						free(sidebar_state);
						KillTimer(hwnd, anim_id);
					}
				}
				else KillTimer(hwnd, anim_id);//this should never happen, if we get here we got a bug
				};

			if (sidebar_state->show) {
				ShowWindow(sidebar, SW_SHOW);

				SetWindowPos(sidebar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW); //TODO(fran): this does allow me to interact with the child windows but destroys drawing for some reason
				//NOTE: if I cant get this to work I can also hack it like youtube does, simply offset the page x position to not cover the sidebar
			}
			SetTimer(sidebar, timer_id, 0, sidebar_anim);
		}

		//Only performs the animation if the sidebar is visible
		void animate_hide(ProcState* state, u32 ms = animation_time_ms) {
			HWND sidebar = state->pages.sidebar;
			const bool hide = IsWindowVisible(sidebar);
			animation_state* sidebar_state = (decltype(sidebar_state))GetPropW(sidebar, state_key);

			if (hide || (sidebar_state && sidebar_state->show))
				animate(state, ms);
		}
	}
}