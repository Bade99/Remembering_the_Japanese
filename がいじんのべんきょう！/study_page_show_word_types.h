#pragma once
namespace study::show_word {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;

			control_type static_id;//HACK?: probably nicer solution would be for the page to have a pagestate where it saves it's current word id
			control_type edit_hiragana;
			control_type edit_kanji;
			control_type edit_meaning;
			control_type combo_lexical_category;
			control_type edit_mnemonic;
			control_type edit_notes;
			control_type edit_example_sentence;
			//TODO(fran): here you should be able to add more than one meaning

			control_type static_creation_date;
			control_type static_last_practiced_date;
			control_type static_score; //eg Score: 4/5 - 80%

			control_type button_modify;
			control_type button_delete;
			control_type button_remember;//the user can request the system to prioritize showing this word on practices (the same as if it was a new word that the user just added)
		};
		control_type all[15];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};
}