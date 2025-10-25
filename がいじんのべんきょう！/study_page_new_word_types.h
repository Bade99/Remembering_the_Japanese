#pragma once
namespace べんきょう {
	namespace new_word {
		union page_controls {
			using control_type = HWND;
			struct {
				control_type page;

				control_type edit_hiragana;
				control_type edit_kanji;
				control_type edit_meaning;
				control_type combo_lexical_category;
				control_type edit_mnemonic;//create a story/phrase around the word
				//TODO(fran): here you should be able to add more than one meaning
				control_type edit_notes;
				control_type edit_example_sentence;
				control_type button_save;
				control_type static_notify;
			};
			control_type all[10];
		private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
		};
	}
}