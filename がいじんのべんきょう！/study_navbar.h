#pragma once
namespace べんきょう {
	namespace study_navbar {
		void create(ProcState* state) {
			auto& navbar = state->pages.navbar;
			navbar = CreateWindowW(navbar::wndclass, NULL, WS_CHILD | WS_VISIBLE //TODO(fran): WS_CLIPCHILDREN?
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			navbar::set_theme(navbar, &nav_theme);

			HWND button_three_lines = CreateWindowW(button::wndclass, NULL, style_button_bmp | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			//TODO(fran): try one pixel thick lines, and also try only two lines
			//AWT(controls.button_modify, 273);
			button::set_theme(button_three_lines, &navbar_img_btn_theme);
			SendMessage(button_three_lines, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.threeLines);
			button::set_user_extra(button_three_lines, state);
			button::set_function_on_click(button_three_lines,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					study_sidebar::animate(state);
				}
			);
			navbar::attach(navbar, button_three_lines, navbar::attach_point::left, -1);

			HWND button_new = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			AWT(button_new, 100);
			button::set_theme(button_new, &navbar_btn_theme);
			button::set_user_extra(button_new, state);
			button::set_function_on_click(button_new, button_function_on_click_goto_page_new);
			navbar::attach(navbar, button_new, navbar::attach_point::left, -1);
			SendMessage(button_new, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			HWND button_practice = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			AWT(button_practice, 101);
			button::set_theme(button_practice, &navbar_btn_theme);
			button::set_user_extra(button_practice, state);
			button::set_function_on_click(button_practice, button_function_on_click_goto_page_practice);
			navbar::attach(navbar, button_practice, navbar::attach_point::left, -1);
			SendMessage(button_practice, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			edit_oneline::Theme search_editoneline_theme = base_editoneline_theme;
			search_editoneline_theme.brushes.bk.normal = CreateSolidBrush(RGB(30, 31, 25));
			search_editoneline_theme.brushes.border = search_editoneline_theme.brushes.bk;

			HWND search = CreateWindowW(searchbox::wndclass, NULL, WS_CHILD | WS_TABSTOP | SRB_ROUNDRECT | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			ACC(search, 251);
			searchbox::set_editbox_theme(search, &search_editoneline_theme);
			searchbox::set_user_extra(search, state);
			searchbox::set_function_free_elements(search, searchbox_func_free_elements);
			searchbox::set_function_retrieve_search_options(search, searchbox_func_retrieve_search_options);
			searchbox::set_function_perform_search(search, searchbox_func_perform_search);
			searchbox::set_function_show_element_on_editbox(search, searchbox_func_show_on_editbox);
			searchbox::set_function_render_listbox_element(search, searchbox_func_listbox_render);
			searchbox::maintain_placerholder_when_focussed(search, false);
			edit_oneline::set_IME_wnd(searchbox::get_controls(search).editbox, true);
			navbar::attach(navbar, search, navbar::attach_point::center, -1);
			SendMessage(search, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
			//TODO(fran): searchbox: changing color of editbox text based on whether it has meaning,hiragana,kanji?
			//TODO(fran): searchbox: restore what the user wrote when they press the escape key
		}
	}
}