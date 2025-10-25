#pragma once
namespace べんきょう::practice::review::drawing {
	void preload_page(ProcState* state, practice::drawing::page_controls& controls, practice_drawing* pagedata) {
		HBRUSH question_txt_br = brush_for(pagedata->practice->question_type);
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

		paint::clear_canvas(controls.paint_answer);
		paint::set_placeholder(controls.paint_answer, pagedata->user_answer);

		button::Theme btn_next_theme;
		btn_next_theme.brushes.bk = user_choice_bk;
		btn_next_theme.brushes.border = user_choice_bk;
		btn_next_theme.brushes.foreground.normal = global::colors.Img;
		button::set_theme(controls.button_next, &btn_next_theme);

		SendMessageW(controls.static_correct_answer, WM_SETTEXT, 0, (LPARAM)pagedata->practice->question.attributes.kanji.str /*TODO(fran): add answer_str*/);

		EnableWindow(controls.paint_answer, FALSE);
		EnableWindow(controls.button_show_word, TRUE);
		EnableWindow(controls.button_next, TRUE);

		embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->question);
		embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &pagedata->practice->question);
	}

	void handle_event(ProcState* state, HWND event_wnd) {
		auto& page = state->pages.practice_drawing;
		/*if (child == page.button_show_word) {
			flip_visibility(page.embedded_show_word_reduced);
		}*/
		/*else */if (event_wnd == page.button_next) {
			goto_previous_page(state);
		}
	}
}