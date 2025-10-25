#pragma once
namespace べんきょう {
	namespace practice {
		namespace multiplechoice {
			union page_controls {
				using control_type = HWND;
				struct {
					control_type page;

					control_type static_question;

					control_type multibutton_choices;

					control_type button_next;//#disabled by default

					control_type button_show_word;//#disabled by default

					control_type embedded_show_word_reduced;//#hidden by default

					control_type button_show_disambiguation;

					control_type embedded_show_word_disambiguation;//#hidden by default
				};
				control_type all[8];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			};

			enum class variant : i32 {
				hiragana_to_meaning = 1 << 0,
				hiragana_to_kanji = 1 << 1,
				meaning_to_hiragana = 1 << 2,
				meaning_to_kanji = 1 << 3,
				kanji_to_hiragana = 1 << 4,
				kanji_to_meaning = 1 << 5,

				_last_bit,
			};

			struct word {
				learnt_word16 question;//#free
				learnt_word_elem question_type;//NOTE: the type allows for choosing the correct color of the word in the UI
				utf16* question_str;//Points to some element inside of 'question'
				ptr<utf16*> choices; //#free
				learnt_word_elem choices_type;
				u32 idx_answer;//index of the correct answer in the 'choices' array, starting from 0
			};

			struct page_state {
				word* practice;
			};
		}
	}
}