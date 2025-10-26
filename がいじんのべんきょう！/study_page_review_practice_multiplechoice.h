#pragma once
namespace study::practice::review::multiplechoice {
	void preload_page(ProcState* state, practice::multiplechoice::page_controls& controls, practice_multiplechoice* pagedata) {
		HBRUSH question_txt_br = brush_for(pagedata->practice->question_type);
		HBRUSH choice_txt_br = brush_for(pagedata->practice->choices_type);
		HBRUSH user_choice_txt_br = global::colors.ControlTxt;
		//TODO(fran): use brush_group
		brush_group user_choice_bk;
		if (pagedata->answered_correctly) {
			user_choice_bk.normal = global::colors.Bk_right_answer;
			user_choice_bk.mouseover = global::colors.BkMouseover_right_answer;
			user_choice_bk.clicked = global::colors.BkPush_right_answer;
		}
		else {
			user_choice_bk.normal = global::colors.Bk_wrong_answer;
			user_choice_bk.mouseover = global::colors.BkMouseover_wrong_answer;
			user_choice_bk.clicked = global::colors.BkPush_wrong_answer;
		}

		SendMessageW(controls.static_question, WM_SETTEXT, 0, (LPARAM)pagedata->practice->question_str);
		static_oneline::Theme static_theme;
		static_theme.brushes.foreground.normal = question_txt_br;
		static_oneline::set_theme(controls.static_question, &static_theme);

		//TODO(fran): borders for the right/wrong button dont seem to match when the button is pressed
		//TODO(fran): controls.multibutton_choices should be disabled
		multibutton::set_buttons(controls.multibutton_choices, pagedata->practice->choices);
		button::Theme multibutton_button_theme;
		multibutton_button_theme.brushes.foreground.normal = choice_txt_br;
		multibutton::set_button_theme(controls.multibutton_choices, &multibutton_button_theme);

		button::Theme multibutton_user_choice_button_theme;
		multibutton_user_choice_button_theme.brushes.foreground.normal = user_choice_txt_br;
		multibutton_user_choice_button_theme.brushes.bk = user_choice_bk;
		multibutton_user_choice_button_theme.brushes.border = user_choice_bk;
		multibutton::set_button_theme(controls.multibutton_choices, &multibutton_user_choice_button_theme, pagedata->user_answer_idx);

		button::Theme btn_next_theme;
		btn_next_theme.brushes.bk = user_choice_bk;
		btn_next_theme.brushes.border = user_choice_bk;
		btn_next_theme.brushes.foreground.normal = global::colors.Img;
		button::set_theme(controls.button_next, &btn_next_theme);

		EnableWindow(controls.button_show_word, TRUE);

		embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->question);
		embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &pagedata->practice->question);
	}

	void handle_event(ProcState* state, HWND event_wnd) {
		auto& page = state->pages.practice_multiplechoice;
		/*if (child == page.button_show_word) {
			flip_visibility(page.embedded_show_word_reduced);
		}*/
		/*else*/ if (event_wnd == page.button_next) {
			goto_previous_page(state);
		}
	}

	void show_page(ProcState* state, u32 ShowWindow_cmd) {
		show_page(state, page_type::practice_multiplechoice, ShowWindow_cmd);
	}
}