#pragma once
namespace study::practice::review::writing {
	void preload_page(ProcState* state, practice::writing::page_controls& controls, practice_writing* pagedata) {
		utf16* question = pagedata->question->str;
		HBRUSH question_br{ 0 };
		brush_group answer_bk;
		if (pagedata->answered_correctly) {
			answer_bk.normal = global::colors.Bk_right_answer;
			answer_bk.mouseover = global::colors.BkMouseover_right_answer;
			answer_bk.clicked = global::colors.BkPush_right_answer;
		}
		else {
			answer_bk.normal = global::colors.Bk_wrong_answer;
			answer_bk.mouseover = global::colors.BkMouseover_wrong_answer;
			answer_bk.clicked = global::colors.BkPush_wrong_answer;
		}

		switch (pagedata->practice->practice_type) {//TODO(fran): should be a common function call
		case decltype(pagedata->practice->practice_type)::hiragana_to_meaning:
		{
			question_br = brush_for(learnt_word_elem::hiragana);
		} break;
		case decltype(pagedata->practice->practice_type)::kanji_to_hiragana:
		{
			question_br = brush_for(learnt_word_elem::kanji);
		} break;
		case decltype(pagedata->practice->practice_type)::kanji_to_meaning:
		{
			question_br = brush_for(learnt_word_elem::kanji);
		} break;
		case decltype(pagedata->practice->practice_type)::meaning_to_hiragana:
		{
			question_br = brush_for(learnt_word_elem::meaning);
		} break;
		default:Assert(0);
		}

		SendMessageW(controls.static_test_word, WM_SETTEXT, 0, (LPARAM)question);
		static_oneline::Theme static_theme;
		static_theme.brushes.foreground.normal = question_br;
		static_oneline::set_theme(controls.static_test_word, &static_theme);

		//TODO(fran): controls.edit_answer should be disabled

		edit_oneline::Theme editoneline_theme;
		editoneline_theme.brushes.foreground.normal = global::colors.ControlTxt;
		editoneline_theme.brushes.bk.normal = answer_bk.normal;
		editoneline_theme.brushes.border.normal = answer_bk.normal;

		edit_oneline::set_theme(controls.edit_answer, &editoneline_theme);

		SendMessageW(controls.edit_answer, WM_SETTEXT, 0, (LPARAM)pagedata->user_answer.str);

		button::Theme btn_next_theme;
		btn_next_theme.brushes.bk = answer_bk;
		btn_next_theme.brushes.border = answer_bk;
		btn_next_theme.brushes.foreground.normal = global::colors.ControlTxt;
		button::set_theme(controls.button_next, &btn_next_theme);

		EnableWindow(controls.button_show_word, TRUE);

		embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->word);
		embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &pagedata->practice->word);
	}

	void handle_event(ProcState* state, HWND event_wnd) {
		auto& page = state->pages.practice_writing;
		if (event_wnd == page.button_next) {
			goto_previous_page(state);
		}
	}

	void show_page(ProcState* state, u32 ShowWindow_cmd) {
		show_page(state, page_type::practice_writing, ShowWindow_cmd);
	}
}