#pragma once
namespace べんきょう::practice::drawing {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;

			control_type static_question;

			control_type paint_answer;

			control_type button_next;//#disabled by default (gets enabled when the user drew smth)
			control_type button_show_word;//#disabled
			control_type button_show_disambiguation;

			control_type static_correct_answer;//#hidden
			//NOTE: working on a good handwriting recognition pipeline so this can be automatically checked (probably google translate style)

			control_type button_right;//#hidden by default
			control_type button_wrong;//#hidden by default

			control_type embedded_show_word_reduced;//#hidden by default
			control_type embedded_show_word_disambiguation;//#hidden by default
		};
		control_type all[11];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};

	enum class variant : i32 {
		hiragana_to_kanji = 1 << 0,
		meaning_to_kanji = 1 << 1,

		_last_bit,
	};

	struct word {
		learnt_word16 question;//#free
		utf16* question_str;//Points to some element inside of 'question'
		learnt_word_elem question_type;//NOTE: the type allows for choosing the correct color of the word in the UI
	};

	struct page_state {
		word* practice;
	};
}