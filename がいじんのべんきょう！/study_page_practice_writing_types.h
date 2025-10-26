#pragma once
namespace study::practice::writing {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;

			control_type static_test_word;

			control_type edit_answer;

			control_type button_next;//TODO(fran): not the best name

			control_type button_show_word;

			control_type button_show_disambiguation;

			control_type embedded_show_word_reduced;//#hidden by default

			control_type embedded_show_word_disambiguation;//#hidden by default
		};
		control_type all[8];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};

	enum class variant : i32 {
		hiragana_to_meaning = 1 << 0,
		meaning_to_hiragana = 1 << 1,
		kanji_to_hiragana = 1 << 2,
		kanji_to_meaning = 1 << 3,

		_last_bit,
		//NOTE: I could add translate_translation_to_kanji but it's basically translating to hiragana and then letting the IME answer correctly
	};

	//Structures for different practice levels
	struct word {
		learnt_word16 word;//TODO(fran): change name to 'question' and add extra param 'answer' that points to an element inside of 'question'

		variant practice_type; //TODO(fran): the type differentiation is kinda pointless, instead I could bake all the differences into variables, eg to check the right answer have a separate pointer to the needed string inside the learnt_word
	};

	struct page_state {
		word* practice;
	};
}