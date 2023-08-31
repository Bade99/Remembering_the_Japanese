#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_べんきょう_db.h"
#include "unCap_Serialization.h"
#include "win32_Global.h"
#include "sqlite3.h"
#include "win32_Helpers.h"
#include "win32_button.h"
#include "LANGUAGE_MANAGER.h"
#include "win32_edit_oneline.h"
#include "win32_combobox_subclass.h"
#include "unCap_Math.h"
#include "win32_score.h"
#include "win32_static_oneline.h"
#include "win32_graph.h"
#include "win32_gridview.h"
#include "win32_searchbox.h"
#include "windows_extra_msgs.h"
#include "win32_multibutton.h"
#include "win32_paint.h"
#include "win32_Char.h"
#include "win32_navbar.h"
#include "win32_combobox.h"
#include "win32_べんきょう_embedded.h"
#include "win32_Sizer.h"
#include "win32_notify.h"
#include "win32_page.h"

//#include "win32_eyecandy.h"

#include <string>
#include <algorithm>

//TODO(fran): page practice: ability to create lists of words to study, this lists are stored & can be edited and deleted
//TODO(fran): page wordbook_all: feed the list via a separate thread
	//Provide filters: word group
	//Extra: once some filter/order is applied add that column to the word list (we'll need to change the render function)
//TODO(fran): page new_word, show_word & new page: add ability to create word groups, lists of known words the user can create and add to, also ask to practice a specific group. we can include a "word group" combobox in the new_word and show_word pages (also programatically generated comboboxes to add to multiple word groups)
	//Each word group should have a different color, set either by the user or ourselves
	//Start by only allowing words to belong to one group, that way we stay away from lists for now
//TODO(fran): landing page?: track user "dedication", for example number of consecutive days the app has been opened, number of days of inactivity (that one would be quite useful for me)
//TODO(fran): landing page: add hide/reveal checkmark that remembers the hidden state when the app is closed, there should be one checkmark for each column so each one can be hidden individually, while on this state the cell should show a 'Reveal' button that shows the original cell while clicked and returns to hidden when the click is released, or maybe better simply reveal the text when on mouseover, and hide it again when the mouse moves away
//TODO(fran): landing page: give the user the option to choose how many previous day words they want to see, giving a range of 1 to 10 days (not literal days but days in which they added new words)
//TODO(fran): page practice_drawing: kanji detection via OCR
//TODO(fran): mascot: have some kind of character that interacts with the user, japanese kawaii style
//TODO(fran): whole application: get rid of null terminator, or better said "do as if it doesnt exist" and store an extra parameter with the string size everywhere (utf8_str,...)
//TODO(fran): whole application: double buffered rendering

//TODO(fran): IDEA: navbar: what if I used the WM_PARENTNOTIFY to allow for my childs to tell me when they are resized, maybe not using parent notify but some way of not having to manually resize the navbar. Instead of this I'd say it's better that a child that needs resizing sends that msg to its parent (WM_REQ_RESIZE), and that trickles up trough the parenting chain til someone handles it
//TODO(fran): IDEA: all controls: we can add a crude transparency by making the controls layered windows and define a color as the color key, the better but much more expensive approach would be to use UpdateLayeredWindow to be able to also add semi-transparency though it may not be necessary. There also seems to be a supported way using WS_EX_COMPOSITED + WS_EX_TRANSPARENT
	//TODO(fran): all controls: check for a valid brush and if it's invalid dont draw, that way we give the user the possibility to create transparent controls (gotta check that that works though)

//TODO(fran): BUG: practice writing/...: the edit control has no concept of its childs, therefore situations can arise were it is updated & redrawn but the children arent, which causes the space they occupy to be left blank (thanks to WS_CLIPCHILDREN), the edit control has to tell its childs to redraw after it does
//TODO(fran):BUG searchbox: caret on the searchbox gets misplaced. howto reproduce: search for a word, then using the down arrow go to some option and then press enter, now the searchbox caret will be misplaced
//TODO(fran): BUG: landing page: howto reproduce: press the recently added button to hide the elements, then press it again to show them, nothing will show cause we seem to not be sending a redraw request



//Leftover IDEAs:
//IDEA: page review_practice_...: When opening practices for the review we could add new pages to the enum, this are like virtual pages, using the same controls, but now can have different layout or behaviour simply by adding them to the switch statements, this is very similar to the scene flag idea; the bennefit of virtual pages is that no code needs to be added to already existing things, on the other side the scene flag has to be handled by each page, adding an extra switch(state->current_scene), the annoying thing is on resizing, with a scene flag we remain on the same part of the code and can simply append more controls to the end, change the "bottommost_control" and be correclty resized, we could cheat by putting multiple case statements together, and inside check which virtual page we're in now. The problem for virtual pages is code repetition, yeah the old stuff wont be affected, but we need to copy parts of that old stuff since virtual pages will share most things
	//NOTE: on iterating through elements, we probably can still only iterate over each virtual page's specific elements by adding some extra separator on the struct and checking for their address, subtracting from some base, dividing by the type size and doing for(int i= base; i < cnt; i++) func( all[i] )
//IDEA: page review_practice: alternative idea: cliking an element of the gridview redirects to the show_word page
//IDEA: application icon: japanese schools seem to usually be represented as "cabildo" like structures with a rectangle and a column coming out the middle, try to retrofit that into an icon



//INFO: this wnd is divided into two, the UI side and the persistence/db interaction side, the first one operates on utf16 for input and output, and the second one in utf8 for input and utf8 for output (also utf16 but only if you manually handle it)
//INFO: this window is made up of many separate ones, as if they were in different tabs, in the sense only one is shown at any single time, and there isn't any relationship between them
//INFO: the hiragana aid on top of kanji is called furigana
//INFO: Similar applications/Possible Inspiration: anki, memrise, wanikani
//INFO: dates on the db are stored in GMT, REMEMBER to convert them for the UI
//INFO: use parameterized queries for any query that requires direct user input (avoid sql injection)

//NOTE: on the subject of Disambiguation: one thing I realize now is that the disambiguation serves a kind of pointless purpose, if the user thinks there's more than one option then they probably know the both of them, making the disambiguation only a frustration removal device in case the user puts one of the valid answers and gets told it's wrong, and a cheating device for every other situation allowing the user to take a peek, is this good, is this bad, idk

//REMEMBER: have checks in place to make sure the user cant execute operations twice by quickly pressing a button again


//Structures for different practice levels
struct practice_writing_word {
	learnt_word16 word;//TODO(fran): change name to 'question' and add extra param 'answer' that points to an element inside of 'question'
	enum class type {//TODO(fran): the type differentiation is kinda pointless, instead I could bake all the differences into variables, eg to check the right answer have a separate pointer to the needed string inside the learnt_word
		hiragana_to_meaning,
		meaning_to_hiragana,
		kanji_to_hiragana,
		kanji_to_meaning,
		//NOTE: I'd add translate_translation_to_kanji but it's basically translating to hiragana and then letting the IME answer correctly
	}practice_type;
};

HBRUSH brush_for(learnt_word_elem type) {
	HBRUSH res{ 0 };//NOTE: compiler cant know this will always be initialized so I gotta zero it
	switch (type) {
	case decltype(type)::hiragana: {
		res = global::colors.hiragana;
	} break;
	case decltype(type)::kanji: {
		res = global::colors.kanji;
	} break;
	case decltype(type)::meaning: {
		res = global::colors.meaning;
	} break;
	default:Assert(0);
	}
	return res;
};

template<typename T>
T str_for(_learnt_word<T>* word, learnt_word_elem type) {
	T res;
	switch (type) {
	case decltype(type)::hiragana: res = (decltype(res))word->attributes.hiragana; break;
	case decltype(type)::kanji: res = (decltype(res))word->attributes.kanji; break;
	case decltype(type)::meaning: res = (decltype(res))word->attributes.meaning; break;
	default:res = { 0 }; Assert(0);
	}
	return res;
};

struct practice_multiplechoice_word {
	learnt_word16 question;//#free
	learnt_word_elem question_type;//NOTE: the type allows for choosing the correct color of the word in the UI
	utf16* question_str;//Points to some element inside of 'question'
	ptr<utf16*> choices; //#free
	learnt_word_elem choices_type;
	u32 idx_answer;//index of the correct answer in the 'choices' array, starting from 0
};

struct practice_drawing_word {
	learnt_word16 question;//#free
	utf16* question_str;//Points to some element inside of 'question'
	learnt_word_elem question_type;//NOTE: the type allows for choosing the correct color of the word in the UI
};

void languages_setup_combobox(HWND cb) {
	auto langs = LANGUAGE_MANAGER::Instance().GetAllLanguages();
	auto current_lang = LANGUAGE_MANAGER::Instance().GetCurrentLanguage();

	for (const auto& lang : *langs) {
		utf16_str* l = (decltype(l))malloc(sizeof(*l)); *l = alloc_any_str(lang);
		listbox::add_elements(combobox::get_controls(cb).listbox, (void**)l, 1);
		//SendMessage(cb, CB_INSERTSTRING, -1, (LPARAM)lang.c_str());
		
		//if(!lang.compare(current_lang)) SendMessage(cb, CB_SETCURSEL, (int)SendMessage(cb,CB_GETCOUNT,0,0)-1, 0);
		if (!lang.compare(current_lang)) combobox::set_cur_sel(cb, combobox::get_count(cb) - 1);
	}
	InvalidateRect(cb, NULL, TRUE);
}

//-------------Data retrieval from UI-----------------: (UI strings are always utf16)

// use for controls whose value is obtained via WM_GETTEXT, eg static, button, edit, ...
//#define _get_edit_str(edit,any_str) \
//			{ \
//				int _sz_char = (int)SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0) + 1; \
//				any_str = alloc_any_str(_sz_char * sizeof(utf16)); \
//				SendMessageW(edit, WM_GETTEXT, _sz_char, (WPARAM)any_str.str); \
//			}

#define _get_combo_sel_idx_as_str(cb,any_str) \
			{ \
				int lex_categ = (int)SendMessageW(cb, CB_GETCURSEL, 0, 0); \
				int sz_char = _snwprintf(nullptr, 0, L"%d", lex_categ) + 1; \
				any_str = alloc_any_str(sz_char * sizeof(utf16)); \
				_snwprintf(any_str.str, sz_char, L"%d", lex_categ); \
			}

#define _clear_combo_sel(cb) SendMessageW(cb, CB_SETCURSEL, -1, 0)

#define _clear_edit(edit) SendMessageW(edit, WM_SETTEXT, 0, 0)

#define _clear_static(st) SendMessageW(st, WM_SETTEXT, 0, 0)



struct べんきょうSettings {

#define foreach_べんきょうSettings_member(op) \
		op(RECT, rc,200,200,700,900 ) \

	foreach_べんきょうSettings_member(_generate_member);
	sqlite3* db;
	bool is_primary_wnd;//TODO(fran): not sure this should go here instead of ProcState

	_generate_default_struct_serialize(foreach_べんきょうSettings_member);
	_generate_default_struct_deserialize(foreach_べんきょうSettings_member);

};
_add_struct_to_serialization_namespace(べんきょうSettings);

namespace べんきょう {
	constexpr cstr wndclass[] = L"win32_wndclass_べんきょう";

	struct {
		int next_practice_level = 0x5;
	} constexpr timerIDs;

	//NOTE: Since comboboxes return -1 on no selection lexical_category maps perfectly from UI's combobox index to value
	//TODO(fran): store lexical_category value together with it's string in the combobox, that way we dont depend on the order of the elements for mapping

	str lexical_category_to_str(lexical_category cat) {
		return RS(200 + cat); //NOTE: dont_care should never be shown
	}
	//usage example: RS(lexical_category_str_lang_id(lexical_category::verb))
	u32 lexical_category_str_lang_id(lexical_category cat) {
		return 200 + cat; //NOTE: dont_care should never be shown
	}
	void lexical_category_setup_combobox(HWND cb) {
		//INFO: the first element to add to a combobox _must_ be at index 0, it does not support starting at any index, conclusion: windows' combobox is terrible
		ACT(cb, lexical_category::noun, lexical_category_str_lang_id(lexical_category::noun));
		ACT(cb, lexical_category::verb, lexical_category_str_lang_id(lexical_category::verb));
		ACT(cb, lexical_category::adj_い, lexical_category_str_lang_id(lexical_category::adj_い));
		ACT(cb, lexical_category::adj_な, lexical_category_str_lang_id(lexical_category::adj_な));
		ACT(cb, lexical_category::adverb, lexical_category_str_lang_id(lexical_category::adverb));
		ACT(cb, lexical_category::conjunction, lexical_category_str_lang_id(lexical_category::conjunction));
		ACT(cb, lexical_category::pronoun, lexical_category_str_lang_id(lexical_category::pronoun));
		ACT(cb, lexical_category::counter, lexical_category_str_lang_id(lexical_category::counter));
		ACT(cb, lexical_category::particle, lexical_category_str_lang_id(lexical_category::particle));
	}

	str word_filter_to_str(word_filter::type filter) {
		return RS(1100 + filter); //NOTE: dont_care should never be shown
	}
	//usage example: RS(lexical_category_str_lang_id(lexical_category::verb))
	u32 word_filter_str_lang_id(word_filter::type filter) {
		return 1100 + filter; //NOTE: dont_care should never be shown
	}

	str word_order_to_str(word_order::type order) {
		return RS(1000 + order); //NOTE: dont_care should never be shown
	}
	//usage example: RS(lexical_category_str_lang_id(lexical_category::verb))
	u32 word_order_str_lang_id(word_order::type order) {
		return 1000 + order; //NOTE: dont_care should never be shown
	}

	//TODO(fran): not yet sure whether to follow up on this or not
	//struct word_order_modifier {
	//	void (*set_order)(word_order*);
	//	s16 text;
	//};

	void word_order_setup_combobox(HWND cb) {
		for (int i = word_order::__first; i < word_order::__last; i++)
			ACT(cb, i, word_order_str_lang_id((word_order::type)i));
		SendMessageW(cb, CB_SETCURSEL, 0, 0);
	}
	void apply_word_order_element(int element_idx, word_order::type* order) {
		*order = (word_order::type)(word_order::__first + element_idx);
	}

	void word_filter_setup_combobox(HWND cb) {
		int idx_correction = word_filter::__first >= 0 ? 0 : abs(word_filter::__first);
		for (int i = word_filter::__first; i < word_filter::__last; i++)
			ACT(cb, i + idx_correction, word_filter_str_lang_id((word_filter::type)i));
		_clear_combo_sel(cb);
	}
	void apply_word_filter_element(int element_idx, word_filter::type* filter) {
		*filter = element_idx!=-1 ? (word_filter::type)(word_filter::__first + element_idx) : word_filter::none;
	}

	struct ProcState {
		HWND wnd;
		HWND nc_parent;
		べんきょうSettings* settings;

		struct {
			HBRUSH bk;
		} brushes;

		enum class page {
			landing,
			new_word,
			practice,
			practice_writing,
			practice_multiplechoice,
			practice_drawing,//drawing kanji
			review_practice,
			show_word,
			wordbook,
			wordbook_all,

			//-----Virtual Pages-----: (dont have controls of their own, show some other page and steal what they need from it
			review_practice_writing,
			review_practice_multiplechoice,
			review_practice_drawing,
		} current_page;

		struct prev_page_fifo_queue{
			page pages[10];
			u32 cnt;
		}previous_pages;

		i32 practice_cnt;//counter for current completed stages/levels while on a pratice run, gets set to eg 10 and is decremented by -1 with each completed stage, when practice_cnt == 0  the practice ends

		struct {
			using type = HWND;
			type navbar;
			type sidebar;
			type page_space;//all pages go inside this one

			union landingpage_controls {
				struct {
					type page;//parent of all other controls

					//type candy;

					type button_recents;
					type listbox_recents;
				
					type static_word_cnt_title;
					type static_word_cnt;

					type static_practice_cnt_title;
					type static_practice_cnt;

					type static_accuracy_title;
					type score_accuracy;

					type static_accuracy_timeline_title;
					type graph_accuracy_timeline;
				};
				type all[11]; //NOTE: make sure you understand structure padding before implementing this, also this should be re-tested if trying with different compilers or alignment
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			} landing;

			union new_word_controls {
				struct {
					type page;

					type edit_hiragana;
					type edit_kanji;
					type edit_meaning;
					type combo_lexical_category;
					type edit_mnemonic;//create a story/phrase around the word
					//TODO(fran): here you should be able to add more than one meaning
					type edit_notes;
					type edit_example_sentence;
					type button_save;
					type static_notify;
				};
				type all[10];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			} new_word;

			union practice_controls {
				struct {
					type page;
					type button_start;

					type button_words_practiced;//TODO(fran): should simply be a static control
					type listbox_words_practiced;
				};
				type all[4];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			} practice;

			union practice_writing_controls {
				struct {
					type page;

					type static_test_word;

					type edit_answer;

					type button_next;//TODO(fran): not the best name

					type button_show_word;

					type button_show_disambiguation;

					type embedded_show_word_reduced;//#hidden by default

					type embedded_show_word_disambiguation;//#hidden by default
				};
				type all[8];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			}practice_writing;

			union practice_multiplechoice_controls {
				struct {
					type page;

					type static_question;

					type multibutton_choices;

					type button_next;//#disabled by default

					type button_show_word;//#disabled by default

					type embedded_show_word_reduced;//#hidden by default

					type button_show_disambiguation;

					type embedded_show_word_disambiguation;//#hidden by default
				};
				type all[8];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			}practice_multiplechoice;

			union practice_drawing_controls {
				struct {
					type page;

					type static_question;

					type paint_answer;

					type button_next;//#disabled by default (gets enabled when the user drew smth)
					type button_show_word;//#disabled
					type button_show_disambiguation;

					type static_correct_answer;//#hidden
					//NOTE: working on a good handwriting recognition pipeline so this can be automatically checked (probably google translate style)

					type button_right;//#hidden by default
					type button_wrong;//#hidden by default

					type embedded_show_word_reduced;//#hidden by default
					type embedded_show_word_disambiguation;//#hidden by default
				};
				type all[11];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			}practice_drawing;

			union review_practice_controls {
				struct {
					type page;

					type static_review;
					type gridview_practices;
					type button_continue;
				};
				type all[4];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			}review_practice;

			union show_word_controls {
				struct {
					type page;

					type static_id;//HACK?: probably nicer solution would be for the page to have a pagestate where it saves it's current word id
					type edit_hiragana;
					type edit_kanji;
					type edit_meaning;
					type combo_lexical_category;
					type edit_mnemonic;
					type edit_notes;
					type edit_example_sentence;
					//TODO(fran): here you should be able to add more than one meaning

					type static_creation_date;
					type static_last_practiced_date;
					type static_score; //eg Score: 4/5 - 80%

					type button_modify;
					type button_delete;
					type button_remember;//the user can request the system to prioritize showing this word on practices (the same as if it was a new word that the user just added)
				};
				type all[15];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			} show_word;

			union wordbook_controls {
				struct {
					type page;
					//TODO(fran): sorting options
					type listbox_last_days_words[4];
					type button_all_words;
				};
				type all[6];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			} wordbook;

			union wordbook_all_controls {
				struct {
					type page;

					//type static_orderby;
					type combo_orderby;
					//type static_filterby;
					type combo_filterby;
					
					type listbox_words;
				};
				type all[4];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
			} wordbook_all;

		}pages;

		enum class available_practices {
			writing,
			multiplechoice,
			drawing,
		};

		struct practice_header {
			available_practices type;
		};
		struct practice_writing {
			practice_header header;
			practice_writing_word* practice;//#free
			const utf16_str* question;//points to some element inside practice.word
			utf16_str user_answer;//#free
			const utf16_str* correct_answer;//points to some element inside practice.word
			bool answered_correctly;//precalculated value so strcmp is used only once
		};
		struct practice_multiplechoice {
			practice_header header;
			practice_multiplechoice_word* practice;//#free
			size_t user_answer_idx;
			bool answered_correctly;//precalculated value
		};
		struct practice_drawing {
			practice_header header;
			practice_drawing_word* practice;//#free
			HBITMAP user_answer;//#free (DeleteBitmap)
			bool answered_correctly;//precalculated value
		};

		struct {

			struct landing_state {
				bool hide_recents;//hide or show the 'recently added words' listbox
			}landing;

			struct search_state {
				learnt_word_elem search_type;
			}search;

			struct practice_review_state {
				std::vector<practice_header*> practices;
			}practice_review;

			struct practice_writing_state {
				practice_writing_word* practice;
			}practice_writing;//TODO(fran): if we already had the entire practices array from the start we could simplify this to a simple size_t idx and the multipage_mem.temp_practices[pagestate.practice_writing.idx]

			struct practice_multiplechoice_state {
				practice_multiplechoice_word* practice;
			}practice_multiplechoice;

			struct practice_drawing_state {
				practice_drawing_word* practice;
			}practice_drawing;

		} pagestate;

		struct {
			std::vector<practice_header*> temp_practices;
		}multipagestate;

		struct anim_number_range {
			HWND wnd;//window to be updated with new numbers //TODO(fran): should I keep this or use the HWND provided by SetTimer?
			i64 origin, dest;
			f32 t;//[0.0,1.0]
			f32 dt;//increment in t, not actually a delta of time since we are normalized 0 to 1
		};
		struct {
			anim_number_range word_count, practice_count;
		} pageanim;
	};

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }
	void ask_for_resize(ProcState* state) { PostMessage(state->wnd, WM_SIZE, 0, 0); }

	//NOTE: a null HBRUSH means dont change the current one
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	//NOTE: you must previously specify wnd, origin and dest inside the animation struct
	void animate_number_range(ProcState::anim_number_range* anim_state, u32 ms) {
		Assert(anim_state);
		const f32 duration_sec = (f32)ms / 1000.f;
		const u32 total_frames = (u32)ceilf(duration_sec / (1.f / win32_get_refresh_rate_hz(anim_state->wnd)));
		const f32 dt = 1.f / total_frames;
		const UINT_PTR timer_id = (decltype(timer_id))anim_state;

		anim_state->t = 0.f;
		anim_state->dt = dt;

		static void (*number_range_anim)(HWND, UINT, UINT_PTR, DWORD) =
			[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
			ProcState::anim_number_range* anim_state = (decltype(anim_state))anim_id;
			if (anim_state) {
#if 0
				f32 delta = ParametricBlend(anim_state->t);//TODO(fran): we may want linear for this one
#else
				f32 delta = anim_state->t;//linear blend
#endif
				i64 pos = lerp(anim_state->origin, delta, anim_state->dest);

				auto txt = std::to_wstring(pos);
				SendMessage(anim_state->wnd, WM_SETTEXT, 0, (LPARAM)txt.c_str());
				f32 oldt = anim_state->t;
				anim_state->t += anim_state->dt;
				anim_state->t = clamp01(anim_state->t);
				if (oldt >= 1.f) {
					KillTimer(hwnd, anim_id);
				}
				else {
					i32 ms = (i32)((1.f / win32_get_refresh_rate_hz(hwnd)) * 1000.f);
					SetTimer(hwnd, anim_id, ms, number_range_anim);
				}
			}
			else KillTimer(hwnd, anim_id);//this should never happen, if we get here we got a bug
		};

		SetTimer(anim_state->wnd, timer_id, 0, number_range_anim);
	}

	//TODO(fran): there are two things I view as possibly necessary extra params: HWND wnd (of the gridview), void* user_extra
	void gridview_practices_renderfunc(HDC dc, rect_i32 r, gridview::render_flags flags, void* element, void* user_extra) {
		ProcState::practice_header* header = (decltype(header))element;

		//------Render Setup------:
		HBRUSH border_br{ 0 };
		utf16_str txt{ 0 };

		//TODO(fran): we can bake this switch into the header by adding the params answered_correctly and question_str there
		bool answered_correctly = false;
		switch (header->type) {
		case decltype(header->type)::writing:
		{
			ProcState::practice_writing* data = (decltype(data))header;
			answered_correctly = data->answered_correctly;
#if 1 //TODO(fran): idk which is better
			txt = *data->question;
#else
			txt = data->user_answer;//the user will remember better what they wrote rather than what they saw
#endif
		} break;
		case decltype(header->type)::multiplechoice:
		{
			ProcState::practice_multiplechoice* data = (decltype(data))header;
			answered_correctly = data->answered_correctly;
#if 1
			txt.str = data->practice->question_str;
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
#else
			txt.str = data->practice->choices[data->user_answer_idx];
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
#endif
		} break;
		case decltype(header->type)::drawing:
		{
			ProcState::practice_drawing* data = (decltype(data))header;
			answered_correctly = data->answered_correctly;
			txt.str = data->practice->question_str;
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
		} break;
		default: Assert(0);
		}

		if (flags.onMouseover)
			border_br = answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
		else if (flags.onClicked)
			border_br = answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;
		else 
			border_br = answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;

		//------Rendering------:
		int w = r.w, h = r.h;
		RECT rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT

		//Draw border
		int thickness = 3;
#if 0
		FillRectBorder(dc, rc, thickness, border_br, BORDERALL);
#elif 0
		{
			int roundedness = (int)ceilf(min((f32)w * .1f, (f32)h * .1f));
			//NOTE: border == pen, bk == brush
			HPEN border_pen = CreatePen(PS_SOLID, thickness, ColorFromBrush(border_br)); defer{ DeletePen(border_pen); };
			HPEN oldpen = SelectPen(dc, border_pen); defer{ SelectPen(dc,oldpen); };
			HBRUSH oldbr = SelectBrush(dc, GetStockBrush(HOLLOW_BRUSH)); defer{ SelectBrush(dc,oldbr); };
			RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, roundedness, roundedness);
		}
#else
		u16 degrees = 20;
		//TODO(fran): I dont quite love how this looks
		urender::RoundRectangleFill(dc, border_br, rc, degrees);
#endif
		InflateRect(&rc, -thickness, -thickness);

		//Draw text
		//TODO(fran): idk whether I want to show the question or what the user answered
		urender::draw_text_max_coverage(dc, rc, txt, global::fonts.General, global::colors.ControlTxt, urender::txt_align::center);

	}

	//Page Search: Searchbox functions
	void searchbox_func_free_elements(ptr<void*> elements, void* user_extra){
		//TODO(fran): right now Im allocating the whole array which means I only need to free the very first element, this is probably not the way to go for the future, for example if we wanted to do async search we wouldnt know which elements are the first in the array and therefore the only ones that need freeing
		//for(auto& e : elements) free_any_str(e);

		for(auto e : elements) for(auto& m : ((learnt_word16*)e)->all) free_any_str(m.str);

		//TODO(fran): MEMLEAK, BUG: Im pretty sure I need to free the first element of 'elements' in order for the dynamic array to be freed, but it crashes right now, Im pretty sure there's a bug higher in the chain, and also the array idea is good from a performance standpoint but doesnt look too good for my needs here, there are never gonna be more than say 15 elements so speed isnt really an issue
		if(elements.cnt)free(elements[0]); //memleak solved, for now
		//elements.free(); //NOTE: again, all the elements are allocated continually in memory as an array, so we only need to deallocate the first one and the whole mem section is freed
	}

	ptr<void*> searchbox_func_retrieve_search_options(utf16_str user_input, void* user_extra) {
		ptr<void*> res{ 0 };
		ProcState* state = (decltype(state))user_extra;
		if (user_input.cnt()) {

			if (any_kanji(user_input)) state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::kanji;
			else if (any_hiragana_katakana(user_input)) state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::hiragana;
			else state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::meaning;

			auto search_res = search_word_matches(state->settings->db, state->pagestate.search.search_type, user_input, 8); //defer{ free(search_res.matches);/*free the dinamically allocated array*/ };
			//TODO(fran): search_word_matches should return a ptr
			res.alloc(search_res.cnt);//TODO(fran): am I ever freeing this?
			for (size_t i = 0; i < search_res.cnt; i++)
				//res[i] = search_res.matches[i];
				res[i] = &search_res[i];
		}
		return res;
	}

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra);

	void searchbox_func_show_on_editbox(HWND editbox, void* element, void* user_extra) {
		learnt_word16* word = (decltype(word))element;
		ProcState* state = (decltype(state))user_extra;

		utf16_str txt = str_for(word, state->pagestate.search.search_type);
			
		SendMessage(editbox, WM_SETTEXT_NO_NOTIFY, 0, (LPARAM)txt.str);
	}

	void searchbox_func_listbox_render(HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
		int w = r.w, h = r.h;
		learnt_word16* txt = (decltype(txt))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk;
		if (flags.onSelected || flags.onMouseover)bk_br = global::colors.ControlBkMouseOver;
		if(flags.onClicked) bk_br = global::colors.ControlBkPush;

		RECT bk_rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		FillRect(dc, &bk_rc, bk_br);

		//Draw text
		i32 third_w = r.w / 3;
		rect_i32 tempr = r; tempr.w = third_w;

		RECT hira_rc = to_RECT(tempr);

		tempr.left += tempr.w;

		RECT kanji_rc = to_RECT(tempr);

		tempr.left += tempr.w;

		RECT meaning_rc = to_RECT(tempr);
		
		urender::draw_text(dc, hira_rc, txt->attributes.hiragana, global::fonts.General, brush_for(learnt_word_elem::hiragana), bk_br, urender::txt_align::left, 3);
		urender::draw_text(dc, kanji_rc, txt->attributes.kanji, global::fonts.General, brush_for(learnt_word_elem::kanji), bk_br, urender::txt_align::left, 3);
		urender::draw_text(dc, meaning_rc, txt->attributes.meaning, global::fonts.General, brush_for(learnt_word_elem::meaning), bk_br, urender::txt_align::left, 3);

	}

	void langbox_func_free_elements(ptr<void*> elements, void* user_extra) {
		for (auto& e : elements) { free_any_str(((utf16_str*)e)->str); free(e); }
	}

	void langbox_func_render_listbox_element(HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {

		int w = r.w, h = r.h;
		utf16_str* txt = (decltype(txt))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk, txt_br = global::colors.ControlTxt;
		if (flags.onMouseover || flags.onSelected)bk_br = global::colors.ControlBkMouseOver;
		if (flags.onClicked) bk_br = global::colors.ControlBkPush;

		RECT bk_rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		FillRect(dc, &bk_rc, bk_br);

		//Draw text
		HFONT font = global::fonts.General;
		RECT txt_rc = to_RECT(r);

		urender::draw_text(dc, txt_rc, *txt, font, txt_br, bk_br, urender::txt_align::left, avg_str_dim(font,1).cx);
	}

	void langbox_func_on_selection_accepted(void* element, void* user_extra){
		utf16_str* lang = (decltype(lang))element;
		LANGUAGE_MANAGER::Instance().ChangeLanguage(lang->str);
		ProcState* state = (decltype(state))user_extra;

		navbar::ask_for_resize(state->pages.navbar);
		navbar::ask_for_repaint(state->pages.navbar);
		//TODO(fran): ask for resize and repaint to the entire page too
		//TODO(fran):I dont like having to do this manually
	}

	void langbox_func_on_listbox_opening(HWND combo, HWND listbox, void* user_extra) {
		listbox::clear_selected_noNotify(listbox);
	}

	void langbox_func_render_combobox(HDC dc, rect_i32 r, combobox::render_flags flags, void* element, void* user_extra) {

		HFONT font = global::fonts.General;
		HBRUSH bk_br, txt_br = global::colors.ControlTxt, border_br;
		if (flags.isListboxOpen) {
			bk_br = global::colors.ControlBk;
		}
		else if (flags.onClicked) {
			bk_br = global::colors.ControlBkPush;
		}
		else if (flags.onMouseover) {
			bk_br = global::colors.ControlBkMouseOver;
		}
		else {
			bk_br = global::colors.ControlBk_Disabled;
		}
		border_br = bk_br;


		int border_thickness_pen = 0;//means 1px when creating pens
		int border_thickness = 1;
		int x_pad = avg_str_dim(font, 1).cx;

		//Border an Bk
		{
			HPEN pen = CreatePen(PS_SOLID, border_thickness_pen, ColorFromBrush(border_br)); defer{ DeletePen(pen); };
			HPEN oldpen = SelectPen(dc, pen); defer{ SelectObject(dc, oldpen); };
			HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
			i32 extent = min(r.w, r.h);
			i32 roundedness = max(1, (i32)roundf((f32)extent * .2f));
			RoundRect(dc, r.left, r.top, r.right(), r.bottom(), roundedness, roundedness);
		}

		//Dropbox icon
		int icon_x=r.w;
		{//TODO(fran): flicker free
			HBITMAP bmp = global::bmps.dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				int max_sz = roundNdown(bitmap.bmWidth, (int)((f32)r.h * .6f)); //HACK: instead use png + gdi+ + color matrices
				if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

				int bmp_height = max_sz;
				int bmp_width = bmp_height;
				int bmp_align_width = r.w - bmp_width - x_pad;
				int bmp_align_height = (r.h - bmp_height) / 2;
				urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, global::colors.Img);//TODO(fran): parametric color

				icon_x = bmp_align_width;
			}
		}
		//TODO(fran): clamp txt rect to not go over the icon

		//Text
		if (element) {
			SelectFont(dc, font);
			utf16_str* s = (decltype(s))element;

			SetBkColor(dc, ColorFromBrush(bk_br));
			SetTextColor(dc, ColorFromBrush(txt_br));

			RECT txt_rc = to_RECT(r);
			txt_rc.left += x_pad;
			txt_rc.right = icon_x;

			txt_rc.right -= avg_str_dim(font, 1).cx;//Add spacing between txt & icon for right aligned

			DrawTextW(dc, s->str, (int)s->cnt(), &txt_rc, DT_EDITCONTROL | DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		}

	}

	int langbox_func_desired_size(SIZE* min, SIZE* max, HDC dc, void* element, void* user_extra) {
		SIZE sz;
		HFONT font = global::fonts.General;

		if (element) {
			utf16_str* s = (decltype(s))element;
			HFONT oldfont = SelectFont(dc, font); defer{ SelectFont(dc,oldfont); };
			GetTextExtentPoint32W(dc, s->str, (int)s->cnt(), &sz);
			int char_cnt = 2 + 3;//icon, spacing
			sz.cx += avg_str_dim(font, char_cnt).cx;
		}
		else {
			int char_cnt = 8 + 2 + 3;//chars, icon, spacing
			sz = avg_str_dim(font, char_cnt);
		}

		min->cx = minimum((int)min->cx, (int)((float)sz.cx * 1.f));
		min->cy = minimum((int)min->cy, (int)((float)sz.cy * 1.2f));

		*max = *min;
		return 2;
	}

	void button_practice_func_on_click(void* element, void* user_extra);

	void render_hiragana_kanji_meaning(HDC dc, rect_i32 r,HBRUSH bk_br, HBRUSH hira_br, HBRUSH kanji_br, HBRUSH meaning_br, learnt_word16* word) {
		int w = r.w, h = r.h;
		
		//Draw bk
		RECT bk_rc = to_RECT(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		FillRect(dc, &bk_rc, bk_br);

		//Draw text
		HFONT font = global::fonts.General;
		i32 third_w = r.w / 3;
		rect_i32 tempr = r; tempr.w = third_w;

		RECT hira_rc = to_RECT(tempr);

		tempr.left += tempr.w;

		RECT kanji_rc = to_RECT(tempr);

		tempr.left += tempr.w;

		RECT meaning_rc = to_RECT(tempr);

		
		urender::draw_text(dc, hira_rc, word->attributes.hiragana, font, hira_br, bk_br, urender::txt_align::left, avg_str_dim(font, 1).cx);
		//if(*txt->attributes.kanji.str)
		urender::draw_text(dc, kanji_rc, word->attributes.kanji, font, kanji_br, bk_br, urender::txt_align::left, 3);
		//else {
		//	rect_i32 kanji_placeholder_rc;
		//	kanji_placeholder_rc.x = kanji_rc.left;
		//	auto rc_dim = avg_str_dim(font, 8);
		//	kanji_placeholder_rc.w = minimum(rc_dim.cx,RECTW(kanji_rc));
		//	kanji_placeholder_rc.h = minimum(rc_dim.cy, RECTH(kanji_rc));
		//	kanji_placeholder_rc.y = kanji_rc.top + (RECTH(kanji_rc) - kanji_placeholder_rc.h) / 2;
		//	HBRUSH kanji_br = global::colors.kanji;
		//	HPEN pen = CreatePen(PS_SOLID, 0, ColorFromBrush(kanji_br)); defer{ DeletePen(pen); };
		//	HPEN oldpen = SelectPen(dc, pen); defer{ SelectObject(dc, oldpen); };
		//	HBRUSH oldbr = SelectBrush(dc, kanji_br); defer{ SelectBrush(dc,oldbr); };
		//	i32 extent = min(kanji_placeholder_rc.w, kanji_placeholder_rc.h);
		//	i32 roundedness = max(1, (i32)roundf((f32)extent * .2f));
		//	RoundRect(dc, kanji_placeholder_rc.x, kanji_placeholder_rc.y, kanji_placeholder_rc.right(), kanji_placeholder_rc.bottom(), roundedness, roundedness);
		//}
		urender::draw_text(dc, meaning_rc, word->attributes.meaning, font, meaning_br, bk_br, urender::txt_align::left, 3);
	}

	void listbox_recents_func_render(HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
		//TODO(fran): make common function between this and the searchbox's listbox rendering func
		int w = r.w, h = r.h;
		learnt_word16* word = (decltype(word))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk_Light;
		if (flags.onSelected || flags.onMouseover)bk_br = global::colors.ControlBkMouseOver;
		if (flags.onClicked) bk_br = global::colors.ControlBkPush;

		HBRUSH hira_br = brush_for(learnt_word_elem::hiragana);
		HBRUSH kanji_br = brush_for(learnt_word_elem::kanji);
		HBRUSH meaning_br = brush_for(learnt_word_elem::meaning);

		render_hiragana_kanji_meaning(dc, r, bk_br, hira_br, kanji_br, meaning_br, word);
	}

	void button_recents_func_render(HWND wnd, HDC dc, rect_i32 r, button::render_flags flags, const button::Theme* theme, void* element, void* user_extra) {
		//TODO(fran): join with langbox_func_render_combobox to create a rendering function that generates combobox looking wnds
		HFONT font = theme->font;// global::fonts.General;
		HBRUSH bk_br, txt_br = theme->brushes.foreground.normal /*global::colors.ControlTxt*/, border_br, icon_br = global::colors.Img;//TODO(fran): use the button Theme
		/*if (flags.isListboxOpen) {
			bk_br = global::colors.ControlBk;
		}
		else*/
		if (!flags.isEnabled) {
			bk_br = theme->brushes.bk.disabled; // global::colors.ControlBk_Disabled;
			txt_br = theme->brushes.foreground.disabled; // global::colors.ControlTxt_Disabled;
			icon_br = global::colors.Img_Disabled;
		}
		else if (flags.onClicked) {
			bk_br = theme->brushes.bk.clicked; // global::colors.ControlBkPush;
		}
		else if (flags.onMouseover) {
			bk_br = theme->brushes.bk.mouseover; // global::colors.ControlBkMouseOver;
		}
		else {
			bk_br = theme->brushes.bk.normal; // global::colors.ControlBk_Dark;//TODO(fran): still not completely sold on the color, maybe if I also tint the bk of the listbox a little blue it will fit better
		}
		border_br = bk_br;


		int border_thickness_pen = theme->dimensions.border_thickness;//NOTE: 0 means 1px when creating pens
		int x_pad = avg_str_dim(font, 1).cx;

		//Border an Bk
		{
			HPEN pen = CreatePen(PS_SOLID, border_thickness_pen, ColorFromBrush(border_br)); defer{ DeletePen(pen); };
			HPEN oldpen = SelectPen(dc, pen); defer{ SelectObject(dc, oldpen); };
			HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
			i32 extent = min(r.w, r.h);
			i32 roundedness = max(1, (i32)roundf((f32)extent * .2f));
			RoundRect(dc, r.left, r.top, r.right(), r.bottom(), roundedness, roundedness);
		}

		//Dropbox icon
		int icon_x = r.w;
		{//TODO(fran): flicker free
			HBITMAP bmp = global::bmps.dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				int max_sz = roundNdown(bitmap.bmWidth, (int)((f32)r.h * .6f)); //HACK: instead use png + gdi+ + color matrices
				if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

				int bmp_height = max_sz;
				int bmp_width = bmp_height;
				int bmp_align_width = r.w - bmp_width - x_pad;
				int bmp_align_height = (r.h - bmp_height) / 2;
				urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, icon_br);

				icon_x = bmp_align_width;
			}
		}
		//TODO(fran): clamp txt rect to not go over the icon

		//Text
		utf16 txt[40];
		int len = Button_GetText(wnd, txt, ARRAYSIZE(txt));
		if (len) {
			HFONT oldfont = SelectFont(dc, font); defer{ SelectFont(dc, oldfont); };

			SetBkColor(dc, ColorFromBrush(bk_br));
			SetTextColor(dc, ColorFromBrush(txt_br));

			RECT txt_rc = to_RECT(r);
			txt_rc.left += x_pad;
			txt_rc.right = icon_x;

			DrawTextW(dc, txt, len, &txt_rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}
	}

	void save_settings(ProcState* state) {
		RECT rc; GetWindowRect(state->wnd, &rc);
		state->settings->rc = rc;
	}

	//returns true if the input is valid and usable, returns false otherwise
	bool check_new_word(ProcState* state) {
		auto& page = state->pages.new_word;
		HWND edit_required[] = { page.edit_hiragana,page.edit_meaning };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
				return false;
			}
		}

		//TODO(fran): we could build smth strange like an array of tuples in order to be able to repeat the same code but use different function calls and string notifs in each case
		{
			HWND edit = page.edit_hiragana;
			const auto& txt = edit_oneline::get_state(edit)->char_text;//quick HACK
			if (!std::all_of(txt.begin(), txt.end(), [](utf16 c) {return is_hiragana(c) || is_katakana(c); })) {
				edit_oneline::show_tip(edit, RCS(12), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
				return false;
			}
		}

		{
			HWND edit = page.edit_kanji;
			const auto& txt = edit_oneline::get_state(edit)->char_text;
			if (!txt.empty()) {
				if (!std::any_of(txt.begin(), txt.end(), [](utf16 c) {return is_kanji(c); })//must have at least one kanji
					|| !std::all_of(txt.begin(), txt.end(), [](utf16 c) {return is_hiragana(c) || is_kanji(c); })
					) {
					edit_oneline::show_tip(edit, RCS(13), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
					return false;
				}
			}
		}

		return true;
	}

	bool check_show_word(ProcState* state) {
		//TODO(fran): macro to check both new_word and show_word (show_word will now have some of its static edit controls changed to edit controls instead)
		auto& page = state->pages.show_word;
		HWND edit_required[] = { page.edit_meaning };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
				return false;
			}
		}

		{
			HWND edit = page.edit_kanji;
			const auto& txt = edit_oneline::get_state(edit)->char_text;
			if (!txt.empty()) {
				if (!std::any_of(txt.begin(), txt.end(), [](utf16 c) {return is_kanji(c); }) //at least one kanji
					|| !std::all_of(txt.begin(), txt.end(), [](utf16 c) {return is_hiragana(c) || is_kanji(c); })//only kanji/hira
					) {
					edit_oneline::show_tip(edit, RCS(13), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
					return false;
				}
			}
		}

		return true;
	}

	word_filters wordbook_all_get_filters(ProcState* state) {
		word_filters res;
		const auto& controls = state->pages.wordbook_all;
		apply_word_order_element((int)SendMessageW(controls.combo_orderby, CB_GETCURSEL, 0, 0), &res.order);
		apply_word_filter_element((int)SendMessageW(controls.combo_filterby, CB_GETCURSEL, 0, 0), &res.filter);
		return res;
	}

	void clear_practices_vector(decltype(decltype(ProcState::pagestate)::practice_review_state::practices)& practices) {
		for (auto p : practices) {
			switch (p->type) {
			case decltype(p->type)::writing:
			{
				ProcState::practice_writing* data = (decltype(data))p;
				if (data->user_answer.str) { free_any_str(data->user_answer.str); data->user_answer.sz = 0; }
				for (auto& _ : data->practice->word.all)free_any_str(_.str);
				free(data->practice);
			} break;
			case decltype(p->type)::multiplechoice:
			{
				ProcState::practice_multiplechoice* data = (decltype(data))p;
				data->practice->choices.free();
				for (auto& _ : data->practice->question.all)free_any_str(_.str);
				free(data->practice);
			} break;
			case decltype(p->type)::drawing:
			{
				ProcState::practice_drawing* data = (decltype(data))p;
				for (auto& _ : data->practice->question.all)free_any_str(_.str);
				DeleteBitmap(data->user_answer);
				free(data->practice);
			} break;
			default: Assert(0);
			}
			free(p);//the object, no matter what type, can always be freed the same way since p is the ptr returned by the allocator
		}
		practices.clear();
	}

	//Sets the items in the corresponding page to the values on *data, prepares the page so it can be shown to the user
	void preload_page(ProcState* state, ProcState::page page, void* data) {
		//TODO(fran): we probably want to clear the whole page before we start adding stuff
		switch (page) {
		case decltype(page)::landing:
		{
			user_stats* stats = (decltype(stats))data;
			auto controls = state->pages.landing;
//#define PRACTICE_TEST_STATS
#ifndef PRACTICE_TEST_STATS
			SendMessage(controls.score_accuracy, SC_SETSCORE, f32_to_WPARAM(stats->accuracy()), 0);

			state->pageanim.word_count.origin = 0;
			state->pageanim.word_count.dest = stats->word_cnt;
			state->pageanim.word_count.wnd = controls.static_word_cnt;
			animate_number_range(&state->pageanim.word_count, state->pageanim.word_count.dest > 100 ? 1000 : 500);
			//SendMessage(controls.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->word_cnt).c_str());
			state->pageanim.practice_count.origin = 0;
			state->pageanim.practice_count.dest = stats->times_practiced;
			state->pageanim.practice_count.wnd = controls.static_practice_cnt;
			animate_number_range(&state->pageanim.practice_count, state->pageanim.practice_count.dest > 100 ? 1000 : 500);
			//SendMessage(controls.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->times_practiced).c_str());
			//TODO(fran): timeline, we'll probably need to store that as blob or text in the db, this is were mongodb would be nice, just throw a js obj for each timepoint
			//TODO(fran): if the timeline is empty we should simply put the current accuracy, or leave it empty idk
			graph::set_points(controls.graph_accuracy_timeline, stats->accuracy_timeline.mem, stats->accuracy_timeline.cnt);
			graph::graph_dimensions grid_dims;
			grid_dims.set_top_point(100);
			grid_dims.set_bottom_point(0);
			grid_dims.set_viewable_points_range(0, stats->accuracy_timeline.cnt);
			graph::set_dimensions(controls.graph_accuracy_timeline, grid_dims);
			//graph::set_top_point(controls.graph_accuracy_timeline, 100);
			//graph::set_bottom_point(controls.graph_accuracy_timeline, 0);
			//graph::set_viewable_points_range(controls.graph_accuracy_timeline, 0, stats->accuracy_timeline.cnt);
#else
			SendMessage(controls.score_accuracy, SC_SETSCORE, f32_to_WPARAM(.6f), 0);
			SendMessage(controls.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(1452).c_str());
			SendMessage(controls.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(559).c_str());
			i32 accu[]{ 77,56,32,12,48,95,65,32,54,67,79,88,100 };
			graph::set_points(controls.graph_accuracy_timeline, accu, ARRAYSIZE(accu));
			graph::set_top_point(controls.graph_accuracy_timeline, 100);
			graph::set_bottom_point(controls.graph_accuracy_timeline, 0);
			graph::set_viewable_points_range(controls.graph_accuracy_timeline, 0, ARRAYSIZE(accu));
#endif
		} break;
		case decltype(page)::new_word:
		{
			learnt_word16* new_word = (decltype(new_word))data;//NOTE: since we are in UI we expect utf16 strings
			auto controls = state->pages.new_word;
			//NOTE: any of the values in new_word can be invalid, we gotta check before using
			//TODO(fran): if the controls only had name and no identifier eg "edit" for "edit_hiragana" we could directly map everything with a foreach by having the same name in the word and controls structs
			//NOTE: settext already has null checking
			SendMessageW(controls.edit_hiragana, WM_SETTEXT, 0, (LPARAM)new_word->attributes.hiragana.str);
			SendMessageW(controls.edit_kanji, WM_SETTEXT, 0, (LPARAM)new_word->attributes.kanji.str);
			SendMessageW(controls.edit_meaning, WM_SETTEXT, 0, (LPARAM)new_word->attributes.meaning.str);
			SendMessageW(controls.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)new_word->attributes.mnemonic.str);
			SendMessageW(controls.edit_notes, WM_SETTEXT, 0, (LPARAM)new_word->attributes.notes.str);
			SendMessageW(controls.edit_example_sentence, WM_SETTEXT, 0, (LPARAM)new_word->attributes.example_sentence.str);
			int lex_categ_sel;
			if (new_word->attributes.lexical_category.str) {
				try { lex_categ_sel = std::stoi(new_word->attributes.lexical_category.str); }
				catch (...) { lex_categ_sel = -1; }
			} else lex_categ_sel = -1;
			SendMessageW(controls.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

		} break;
		case decltype(page)::show_word:
		{
			stored_word16* word_to_show = (decltype(word_to_show))data;
			auto controls = state->pages.show_word;
			//IDEA: in this page we could reuse the controls from new_word, that way we first call preload_page(new_word) with word_to_show.user_defined and then do our thing (this idea doesnt quite work)

			SendMessageW(controls.static_id, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.id.str);
			SendMessageW(controls.edit_hiragana, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.hiragana.str);
			SendMessageW(controls.edit_kanji, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.kanji.str);
			SendMessageW(controls.edit_meaning, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.meaning.str);
			SendMessageW(controls.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.mnemonic.str);
			SendMessageW(controls.edit_notes, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.notes.str);
			SendMessageW(controls.edit_example_sentence, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.example_sentence.str);

			int lex_categ_sel;
			if (word_to_show->user_defined.attributes.lexical_category.str) {
				try { lex_categ_sel = std::stoi((utf16*)word_to_show->user_defined.attributes.lexical_category.str); }
				catch (...) { lex_categ_sel = -1; }
			}
			else lex_categ_sel = -1;
			SendMessageW(controls.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

			if(word_to_show->application_defined.attributes.creation_date.str)
				SendMessageW(controls.static_creation_date, WM_SETTEXT, 0, (LPARAM)(RS(270) + L" " + (utf16*)word_to_show->application_defined.attributes.creation_date.str).c_str());

			if (word_to_show->application_defined.attributes.last_practiced_date.str)
				SendMessageW(controls.static_last_practiced_date, WM_SETTEXT, 0, (LPARAM)(RS(271) + L" " + (utf16*)word_to_show->application_defined.attributes.last_practiced_date.str).c_str());

			if (word_to_show->application_defined.attributes.times_right.str && word_to_show->application_defined.attributes.times_practiced.str) {
				std::wstring score = (RS(272) + L" " + (utf16*)word_to_show->application_defined.attributes.times_right.str + L" / " + (utf16*)word_to_show->application_defined.attributes.times_practiced.str);
				try {
					float numerator = std::stof((utf16*)word_to_show->application_defined.attributes.times_right.str);
					float denominator = std::stof((utf16*)word_to_show->application_defined.attributes.times_practiced.str);
					if (denominator != 0.f) {
						int percentage = (int)((numerator/denominator) * 100.f);
						score += L" - " + std::to_wstring(percentage) + L"%";
					}
				}
				catch (...) {}
				SendMessageW(controls.static_score, WM_SETTEXT, 0, (LPARAM)score.c_str());
			}

			button::Theme accent_btn_theme;
			accent_btn_theme.brushes.foreground.normal = global::colors.Accent;
			accent_btn_theme.brushes.border.normal = global::colors.Accent;
			button::set_theme(controls.button_remember, &accent_btn_theme);

		} break;
		case decltype(page)::practice_writing:
		{
			//TODO(fran): shouldnt preload_page also call clear_page?
			practice_writing_word* practice = (decltype(practice))data;
			auto controls = state->pages.practice_writing;
			//store data for future proof checking
			state->pagestate.practice_writing.practice = practice;

			utf16* test_word{0};//NOTE: compiler cant know that these guys will always be initialized so I gotta zero them
			HBRUSH test_word_br{0};
			str answer_placeholder;//NOTE: using string so the object doesnt get destroyed inside the switch statement
			HBRUSH answer_br{0};

			str answer_hiragana{ L"こたえ" };

			bool show_ime_suggestions = true;

			switch (practice->practice_type) {
			case decltype(practice->practice_type)::hiragana_to_meaning:
			{
				test_word = (utf16*)practice->word.attributes.hiragana.str;
				test_word_br = brush_for(learnt_word_elem::hiragana);
				//TODO(fran): idk if I should put "meaning" or "answer", and for hiragana "hiragana" or "こたえ" (meaning answer), and "kanji" or "答え"
				answer_placeholder = RS(380);
				answer_br = brush_for(learnt_word_elem::meaning);
			} break;
			case decltype(practice->practice_type)::kanji_to_hiragana:
			{
				test_word = (utf16*)practice->word.attributes.kanji.str;
				test_word_br = brush_for(learnt_word_elem::kanji);

				answer_placeholder = answer_hiragana;
				answer_br = brush_for(learnt_word_elem::hiragana);

				show_ime_suggestions = false;
			} break;
			case decltype(practice->practice_type)::kanji_to_meaning:
			{
				test_word = (utf16*)practice->word.attributes.kanji.str;
				test_word_br = brush_for(learnt_word_elem::kanji);

				answer_placeholder = RS(380);
				answer_br = brush_for(learnt_word_elem::meaning);
			} break;
			case decltype(practice->practice_type)::meaning_to_hiragana:
			{
				test_word = (utf16*)practice->word.attributes.meaning.str;
				test_word_br = brush_for(learnt_word_elem::meaning);

				answer_placeholder = answer_hiragana;
				answer_br = brush_for(learnt_word_elem::hiragana);

				show_ime_suggestions = false;
			} break;
			default:Assert(0);
			}

			SendMessageW(controls.static_test_word, WM_SETTEXT, 0, (LPARAM)test_word);
			static_oneline::Theme static_theme;
			static_theme.brushes.foreground.normal = test_word_br;
			static_oneline::set_theme(controls.static_test_word, &static_theme);
			
			edit_oneline::Theme editoneline_theme;
			editoneline_theme.dimensions.border_thickness = 1;
			editoneline_theme.brushes.foreground.normal = answer_br;
			editoneline_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
			editoneline_theme.brushes.bk.normal = global::colors.ControlBk;
			editoneline_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
			editoneline_theme.brushes.border.normal = global::colors.Img;
			editoneline_theme.brushes.border.disabled = global::colors.Img_Disabled;
			editoneline_theme.brushes.selection.normal = global::colors.Selection;
			editoneline_theme.brushes.selection.disabled = global::colors.Selection_Disabled;

			edit_oneline::set_theme(controls.edit_answer, &editoneline_theme);

			SendMessageW(controls.edit_answer, WM_SETDEFAULTTEXT, 0, (LPARAM)answer_placeholder.c_str());
			edit_oneline::maintain_placerholder_when_focussed(controls.edit_answer,true);
			edit_oneline::set_IME_wnd(controls.edit_answer, !show_ime_suggestions);

			button::Theme btn_theme;
			btn_theme.brushes.bk.normal = global::colors.ControlBk;
			btn_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			btn_theme.brushes.bk.clicked = global::colors.ControlBkPush;
			btn_theme.brushes.border.normal = global::colors.ControlBk;
			btn_theme.brushes.foreground.normal = global::colors.Img;
			button::set_theme(controls.button_next, &btn_theme);

			EnableWindow(controls.button_show_word, FALSE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->word);
			embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &practice->word);

		} break;
		case decltype(page)::review_practice:
		{
			std::vector<ProcState::practice_header*>* practices = (decltype(practices))data;
			auto controls = state->pages.review_practice;

			//free current elements before switching, I think this is the best place to do it
			clear_practices_vector(state->pagestate.practice_review.practices);

			state->pagestate.practice_review.practices = std::move(*practices);
			size_t practices_cnt = state->pagestate.practice_review.practices.size(); Assert(practices_cnt != 0);
			void** practices_data = (void**)malloc(sizeof(void*) * practices_cnt); defer{ free(practices_data); };
			for (size_t i = 0; i < practices_cnt; i++) practices_data[i] = state->pagestate.practice_review.practices[i];
			gridview::set_elements(controls.gridview_practices, practices_data, practices_cnt);

		} break;
		case decltype(page)::review_practice_writing:
		{
			ProcState::practice_writing* pagedata = (decltype(pagedata))data;
			auto& controls = state->pages.practice_writing;
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

		} break;
		case decltype(page)::practice_multiplechoice:
		{
			practice_multiplechoice_word* practice = (decltype(practice))data;
			auto controls = state->pages.practice_multiplechoice;
			state->pagestate.practice_multiplechoice.practice = practice;
			
			HBRUSH question_txt_br = brush_for(practice->question_type);
			HBRUSH choice_txt_br = brush_for(practice->choices_type);


			SendMessageW(controls.static_question, WM_SETTEXT, 0, (LPARAM)practice->question_str);
			static_oneline::Theme static_theme;
			static_theme.brushes.foreground.normal = question_txt_br;
			static_oneline::set_theme(controls.static_question, &static_theme);

			multibutton::set_buttons(controls.multibutton_choices, practice->choices);
			button::Theme multibutton_button;
			multibutton_button.brushes.foreground.normal = choice_txt_br;
			multibutton_button.brushes.bk.normal = global::colors.ControlBk;
			multibutton_button.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			multibutton_button.brushes.bk.clicked = global::colors.ControlBkPush;
			multibutton_button.brushes.border.normal = global::colors.Img;//TODO(fran): = choice_txt_br ?
			multibutton::set_button_theme(controls.multibutton_choices, &multibutton_button);
			

			button::Theme button_next_theme;
			button_next_theme.brushes.bk.normal = global::colors.ControlBk;
			button_next_theme.brushes.border.normal = global::colors.Img;
			button_next_theme.brushes.foreground.normal = global::colors.Img;
			button_next_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			button_next_theme.brushes.bk.clicked = global::colors.ControlBkPush;
			button::set_theme(controls.button_next, &button_next_theme);


			EnableWindow(controls.button_show_word, FALSE);


			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->question);
			embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &practice->question);

		} break;
		case decltype(page)::review_practice_multiplechoice:
		{
			ProcState::practice_multiplechoice* pagedata = (decltype(pagedata))data;
			auto& controls = state->pages.practice_multiplechoice;

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
			multibutton::set_button_theme(controls.multibutton_choices, &multibutton_user_choice_button_theme,pagedata->user_answer_idx);

			button::Theme btn_next_theme;
			btn_next_theme.brushes.bk = user_choice_bk;
			btn_next_theme.brushes.border = user_choice_bk;
			btn_next_theme.brushes.foreground.normal = global::colors.Img;
			button::set_theme(controls.button_next, &btn_next_theme);

			EnableWindow(controls.button_show_word, TRUE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->question);
			embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &pagedata->practice->question);

		} break;
		case decltype(page)::practice_drawing:
		{
			practice_drawing_word* practice = (decltype(practice))data;
			auto controls = state->pages.practice_drawing;
			state->pagestate.practice_drawing.practice = practice;

			HBRUSH question_txt_br = brush_for(practice->question_type);

			SendMessageW(controls.static_question, WM_SETTEXT, 0, (LPARAM)practice->question_str);
			static_oneline::Theme static_theme;
			static_theme.brushes.foreground.normal = question_txt_br;
			static_oneline::set_theme(controls.static_question, &static_theme);

			button::Theme button_next_theme;
			button_next_theme.brushes.bk.normal = global::colors.ControlBk;
			button_next_theme.brushes.border.normal = global::colors.Img;
			button_next_theme.brushes.foreground.normal = global::colors.Img;
			button_next_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			button_next_theme.brushes.bk.clicked = global::colors.ControlBkPush;
			button::set_theme(controls.button_next, &button_next_theme);

			SendMessageW(controls.static_correct_answer, WM_SETTEXT, 0, (LPARAM)practice->question.attributes.kanji.str /*TODO(fran): add answer_str*/);
			

			EnableWindow(controls.paint_answer, TRUE);
			EnableWindow(controls.button_show_word, FALSE);
			EnableWindow(controls.button_next, FALSE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->question);
			embedded::show_word_disambiguation::set_word(controls.embedded_show_word_disambiguation, &practice->question);

			{
				HDC _dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,_dc); };
				HDC dc = CreateCompatibleDC(_dc); defer{ DeleteDC(dc); };//TODO(fran): use already existing dc
				int w = 100, h = 50;//TODO(fran): this size is pretty good, though idk how it'll look on different dpi
				HBITMAP paint_placeholder = CreateCompatibleBitmap(_dc, w, h); defer{ DeleteBitmap(paint_placeholder); };
				{
					auto oldbmp = SelectBitmap(dc, paint_placeholder); defer{ SelectBitmap(dc,oldbmp); };
					RECT r{0,0,w,h};
					FillRect(dc, &r, paint::get_state(controls.paint_answer)->brushes.bk/*HACK*/);
					utf16 _s[] = L"答え";
					utf16_str s{ _s, sizeof(_s) };
					urender::draw_text_max_coverage(dc, r, s, global::fonts.General, global::colors.ControlTxt_Disabled, urender::txt_align::center);
				}
				paint::set_placeholder(controls.paint_answer, paint_placeholder);
			}


		} break;
		case decltype(page)::review_practice_drawing:
		{
			ProcState::practice_drawing* pagedata = (decltype(pagedata))data;
			auto& controls = state->pages.practice_drawing;

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

		} break;

		default: Assert(0);
		}
	}

	void show_page(ProcState* state, ProcState::page p, u32 ShowWindow_cmd /*SW_SHOW,...*/) {
		switch (p) {
		case decltype(p)::landing: 
			for (auto ctl : state->pages.landing.all) ShowWindow(ctl, ShowWindow_cmd);
			//TODO(fran): we shouldnt do this no more, simply create controls as visible. Actually im not too sure now, it's good to know exactly what you want to you on realtime and not be stuck with compile time, this add more possibilities and allows each page to function independently of each other, even for the interaction with virtual pages, otherwise virtual pages would have to manually re-show whatever they hide, showing the page _and_ also each individual control doesnt look so bad to me anymore, we'll see in the future
			ShowWindow(state->pages.landing.page, ShowWindow_cmd);
			break;
		case decltype(p)::new_word: 
			for (auto ctl : state->pages.new_word.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->pages.new_word.static_notify, SW_HIDE);
			ShowWindow(state->pages.new_word.page, ShowWindow_cmd);
			break;
		case decltype(p)::show_word: 
			for (auto ctl : state->pages.show_word.all) ShowWindow(ctl, ShowWindow_cmd); 
			ShowWindow(state->pages.show_word.page, ShowWindow_cmd);
			break;
		case decltype(p)::practice:
			for (auto ctl : state->pages.practice.all) ShowWindow(ctl, ShowWindow_cmd); 
			ShowWindow(state->pages.practice.page, ShowWindow_cmd);
			break;
		case decltype(p)::practice_writing:
			for (auto ctl : state->pages.practice_writing.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->pages.practice_writing.embedded_show_word_reduced, SW_HIDE);
			ShowWindow(state->pages.practice_writing.embedded_show_word_disambiguation, SW_HIDE);
			ShowWindow(state->pages.practice_writing.page, ShowWindow_cmd);
			break;
		case decltype(p)::practice_multiplechoice:
			for (auto ctl : state->pages.practice_multiplechoice.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->pages.practice_multiplechoice.embedded_show_word_reduced, SW_HIDE);
			ShowWindow(state->pages.practice_multiplechoice.embedded_show_word_disambiguation, SW_HIDE);
			ShowWindow(state->pages.practice_multiplechoice.page, ShowWindow_cmd);
			break;
		case decltype(p)::practice_drawing:
			for (auto ctl : state->pages.practice_drawing.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->pages.practice_drawing.static_correct_answer, SW_HIDE);//HACK:we should have some sort of separation between hidden and shown controls
			ShowWindow(state->pages.practice_drawing.button_wrong, SW_HIDE);
			ShowWindow(state->pages.practice_drawing.button_right, SW_HIDE);
			ShowWindow(state->pages.practice_drawing.embedded_show_word_reduced, SW_HIDE);
			ShowWindow(state->pages.practice_drawing.embedded_show_word_disambiguation, SW_HIDE);
			ShowWindow(state->pages.practice_drawing.page, ShowWindow_cmd);
			break;
		case decltype(p)::review_practice:
			for (auto ctl : state->pages.review_practice.all) ShowWindow(ctl, ShowWindow_cmd); 
			ShowWindow(state->pages.review_practice.page, ShowWindow_cmd);
			break;
		case decltype(p)::review_practice_writing: 
			show_page(state, ProcState::page::practice_writing, ShowWindow_cmd); 
			break;
		case decltype(p)::review_practice_multiplechoice: 
			show_page(state, ProcState::page::practice_multiplechoice, ShowWindow_cmd); 
			break;
		case decltype(p)::review_practice_drawing: 
			show_page(state, ProcState::page::practice_drawing, ShowWindow_cmd); 
			break;
		case decltype(p)::wordbook:
			for (auto ctl : state->pages.wordbook.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->pages.wordbook.page, ShowWindow_cmd);//#pointless duplicate of above
			break;
		case decltype(p)::wordbook_all:
			for (auto ctl : state->pages.wordbook_all.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->pages.wordbook_all.page, ShowWindow_cmd);//#pointless duplicate of above
			break;
		default:Assert(0);
		}
	}

	void set_default_focus(ProcState* state, ProcState::page p) {
		switch (p) {
		case decltype(p)::new_word:SetFocus(state->pages.new_word.edit_hiragana); break;
		case decltype(p)::landing: SetFocus(0); break;//Remove focus from whoever had it
		//case decltype(p)::search: SetFocus(state->pages.search.searchbox_search); break;
		case decltype(p)::practice_writing: SetFocus(state->pages.practice_writing.edit_answer); break;

		//default:Assert(0);
		}
	}

	void resize_controls(ProcState* state);//HACK

	void wordbook_all__update_wordlist(ProcState* state) {
		auto& controls = state->pages.wordbook_all;

		word_filters filters = wordbook_all_get_filters(state);
		ptr<learnt_word16> words = get_all_learnt_words(state->settings->db, filters);
		//TODO(fran): new struct reduced_word (or smth like that) that only contains hira,kanji,meaning

		ptr<void*> elems{ 0 }; elems.alloc(words.cnt); defer{ elems.free(); };
		for (size_t i = 0; i < words.cnt; i++) elems[i] = &words[i];

		{//Free previous elements
			ptr<void*> elements = listbox::get_all_elements(controls.listbox_words);//HACK
			for (auto e : elements) for (auto& m : ((decltype(words.mem))e)->all) free_any_str(m.str);
			if (elements.cnt)free(elements[0]);
		}

		listbox::set_elements(controls.listbox_words, elems.mem, elems.cnt);
	}

	void set_current_page(ProcState* state, ProcState::page new_page) {
		show_page(state, state->current_page, SW_HIDE);
		state->current_page = new_page;

		switch (new_page) {
		case decltype(new_page)::practice_writing:
		{
			//TODO(fran): idk where to put this, I'd put it in preload but preload doesnt guarantee we're switching pages, even though it's mostly true currently
			auto& controls = state->pages.practice_writing;
			//SetFocus(controls.edit_answer);//TODO(fran): problem is now we cant see in which lang we have to write, we have two options, either give the edit control an extra flag of show placeholder even if cursor is visible; option 2 setfocus to us (main wnd) wait for the user to press something, redirect the msg to the edit box and setfocus. INFO: wanikani shows the placeholder and the caret at the same time, lets do that
		} break;
		case decltype(new_page)::landing:
		{
			//The 'Recently Added' listbox needs to have elements added to it, TODO(fran): this could be handled on smth like func_on_wm_show
			//TODO(fran): this should only be done if there was a change on the db since the last time it was called

			auto& controls = state->pages.landing;

			//TODO(fran): date information requested from the db or sent to it should all be in local time, the db can bother with storing gmt and doing the appropiate conversions, but the application shouldnt have to even bother with that
			auto [start, end] = day_range(get_latest_word_creation_date(state->settings->db));

			ptr<learnt_word16> recents = get_learnt_word_by_date(state->settings->db, start, end);

			//TODO(fran): this idea of returning an array and then having to add an extra array in order to be able to use it is pretty annoying, yeah it's faster and scales really well, but it's beyond confusing
			ptr<void*> elems{ 0 }; elems.alloc(recents.cnt); defer{ elems.free(); };
			for (size_t i = 0; i < recents.cnt; i++) elems[i] = &recents[i];

			{//Free previous elements
				ptr<void*> elements = listbox::get_all_elements(controls.listbox_recents);//HACK
				for (auto e : elements) for (auto& m : ((decltype(recents.mem))e)->all) free_any_str(m.str);
				if (elements.cnt)free(elements[0]);
			}

			listbox::set_elements(controls.listbox_recents, elems.mem, elems.cnt);

			EnableWindow(controls.button_recents, (BOOL)recents.cnt);


			user_stats stats = get_user_stats(state->settings->db);
			get_user_stats_accuracy_timeline(state->settings->db, &stats, 30); defer{ stats.accuracy_timeline.free(); };
			preload_page(state, ProcState::page::landing, &stats);//TODO(fran): this seems dumb, I either initialize everything here or on preload_page, but not in both

		} break;
		case decltype(new_page)::practice:
		{
			auto& controls = state->pages.practice;

			//TODO(fran): only ask for new values if we know something changed
			//TODO(fran): I dont think we're freeing this if the app is closed, not that it matters in that case though
			ptr<practiced_word16> practiced = get_previously_practiced_words(state->settings->db, 15);

			ptr<void*> elems{ 0 }; elems.alloc(practiced.cnt); defer{ elems.free(); };
			for (size_t i = 0; i < practiced.cnt; i++) elems[i] = &practiced[i];

			{//Free previous elements
				ptr<void*> elements = listbox::get_all_elements(controls.listbox_words_practiced);//HACK
				for (auto e : elements) for (auto& m : ((decltype(practiced.mem))e)->word.all) free_any_str(m.str);
				if (elements.cnt)free(elements[0]);
			}

			listbox::set_elements(controls.listbox_words_practiced, elems.mem, elems.cnt);
		} break;
		case decltype(new_page)::wordbook:
		{
			auto& controls = state->pages.wordbook;

			time64 upper_bound = I64MAX;
			for (int i = 0; i < ARRAYSIZE(controls.listbox_last_days_words); i++) {
				const auto [start, end] = day_range(get_latest_word_creation_date(state->settings->db, upper_bound));
				upper_bound = start - 1;

				//TODO(fran): get and set words in different threads
				ptr<learnt_word16> words = get_learnt_word_by_date(state->settings->db, start, end);

				ptr<void*> elems{ 0 }; elems.alloc(words.cnt); defer{ elems.free(); };
				for (size_t i = 0; i < words.cnt; i++) elems[i] = &words[i];

				HWND listbox = controls.listbox_last_days_words[i];
				{//Free previous elements
					ptr<void*> elements = listbox::get_all_elements(listbox);//HACK
					for (auto e : elements) for (auto& m : ((decltype(words.mem))e)->all) free_any_str(m.str);
					if (elements.cnt)free(elements[0]);
				}

				listbox::set_elements(listbox, elems.mem, elems.cnt);
			}
		} break;
		case decltype(new_page)::wordbook_all:
		{
			wordbook_all__update_wordlist(state);
		} break;
		}
		resize_controls(state);
		show_page(state, state->current_page, SW_SHOW);
		set_default_focus(state, state->current_page);
	}

	void show_backbtn(ProcState* state, bool show) {
		PostMessage(state->nc_parent, WM_SHOWBACKBTN, show, 0);
	}

	void goto_previous_page(ProcState* state) {
		if (state->previous_pages.cnt > 0) {
			set_current_page(state, state->previous_pages.pages[--state->previous_pages.cnt]);
			if (state->previous_pages.cnt == 0) show_backbtn(state, false);
		}
	}

	//TODO(fran): maybe set_current_page should store the page it's replacing into the queue, problem there would be with goto_previous_page, which will cause a store that we dont want, but maybe some better defined functions with the goto_previous distinction in mind could work well
	void store_previous_page(ProcState* state, ProcState::page prev_page) {
		if (state->previous_pages.cnt == ARRAYSIZE(state->previous_pages.pages)) {
			//cnt stays the same
			//we move all the entries one position down and place the new one on top
			//decltype(state->previous_pages.pages) temp;//IMPORTANT INFO: you can create arrays in ways similar to this without the need to put [...] _after_ the name
			memcpy(state->previous_pages.pages, &state->previous_pages.pages[1], (ARRAYSIZE(state->previous_pages.pages) - 1) * sizeof(*state->previous_pages.pages));
			state->previous_pages.pages[ARRAYSIZE(state->previous_pages.pages) - 1] = prev_page;
		}
		else {
			state->previous_pages.pages[state->previous_pages.cnt++] = prev_page;
		}

		show_backbtn(state, true);
	}

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra) {
		ProcState* state = (decltype(state))user_extra;

		learnt_word16 search{0};

		stored_word16_res res; defer{ if(res.found) free_stored_word(res.word); };
		if (is_element) {
			//TODO(fran): at this point we already know which word to find on the db, the problem is we dont have its ID, we gotta search by string (hiragana) again, we need to retrieve and store IDs on the db. And add a function get_word(ID)
			search = *(decltype(&search))element;
			
			res = get_stored_word(state->settings->db, search);
		}
		else {
			//NOTE: here we dont know the "ID" so the search will simply take the first word it finds that matches the requirements //TODO(fran): we could present multiple results and ask the user which one to open
			learnt_word_elem search_type = state->pagestate.search.search_type;
			switch (search_type) {
			case decltype(search_type)::hiragana: search.attributes.hiragana = *((utf16_str*)element); break;
			case decltype(search_type)::kanji:    search.attributes.kanji = *((utf16_str*)element); break;
			case decltype(search_type)::meaning:  search.attributes.meaning= *((utf16_str*)element); break;
			default:Assert(0);//TODO
			}
			//search = ((utf16_str*)element)->str;

			res = get_word(state->settings->db, search_type, str_for(&search,search_type)); 
		}

		if (res.found) {
			
			preload_page(state, ProcState::page::show_word, &res.word);//NOTE: considering we no longer have separate pages this might be a better idea than sending the struct at the time of creating the window
			store_previous_page(state, state->current_page);
			set_current_page(state, ProcState::page::show_word);
		}
		else {
			HWND focuswnd = GetFocus();
			int ret = MessageBoxW(state->nc_parent, RCS(300), L"", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
			if (ret == IDYES) {
				//learnt_word2<utf16_str> new_word{ 0 };
				//new_word.attributes.hiragana = { search, (cstr_len(search)+1)*sizeof(*search) };
				//preload_page(state, ProcState::page::new_word, &new_word);
				preload_page(state, ProcState::page::new_word, &search);
				store_previous_page(state, state->current_page);
				set_current_page(state, ProcState::page::new_word);
			}
			else SetFocus(focuswnd);//Restore focus to the edit window since messagebox takes it away 
			//TODO(fran): a way to make this more streamlined would be to implement:
			//int MessageBoxW(...){ oldfocus=getfocus(); messagebox(); setfocus(oldfocus); }
		}
	}

	bool modify_word(ProcState* state) {
		bool res = false;
		if (check_show_word(state)) {
			learnt_word16 w16; defer{ for (auto& _ : w16.all) free_any_str(_.str); };
			auto& page = state->pages.show_word;

			_get_edit_str(page.static_id, w16.attributes.id);
			_get_edit_str(page.edit_hiragana, w16.attributes.hiragana);
			_get_edit_str(page.edit_kanji, w16.attributes.kanji);
			_get_edit_str(page.edit_meaning, w16.attributes.meaning);
			_get_edit_str(page.edit_mnemonic, w16.attributes.mnemonic);
			_get_edit_str(page.edit_notes, w16.attributes.notes);
			_get_edit_str(page.edit_example_sentence, w16.attributes.example_sentence);
			_get_combo_sel_idx_as_str(page.combo_lexical_category, w16.attributes.lexical_category);

			res = update_word(state->settings->db, w16);
		}
		return res;
	}

	learnt_word8 show_word_getPks(ProcState* state) {//TODO(fran): do not convert to utf8, let the db decide what to do with our utf16
		auto& page = state->pages.show_word;

		learnt_word16 word16;
		_get_edit_str(page.static_id, word16.attributes.id); defer{ free_any_str(word16.attributes.id); };
		static_assert(word16.pk_count == 1, "Additional primary keys need to be retrieved");

		learnt_word8 word8;
		for (int i = 0; i < word8.pk_count; i++) word8.all[i] = s16_to_s8(word16.all[i]);

		return word8;
	}

	bool remove_word(ProcState* state) {
		bool res = false;
		
		learnt_word8 word = show_word_getPks(state);
		defer{ for (int i = 0; i < word.pk_count; i++) free_any_str(word.all[i]); };

		res = delete_word(state->settings->db, word);
		return res;
	}

	bool prioritize_word(ProcState* state) {
		//TODO(fran): should accept any word, not manually take it from the UI
		bool res = false;
		learnt_word8 word = show_word_getPks(state);
		defer{ for (int i = 0; i < word.pk_count; i++) free_any_str(word.all[i]); };

		res = reset_word_priority(state->settings->db,word);

		return res;
	}

	bool save_new_word(ProcState* state) {
		bool res = false;
		if (check_new_word(state)) {
			learnt_word16 w16; defer{ for (int i = w16.pk_count; i < ARRAYSIZE(w16.all); i++) free_any_str(w16.all[i]); };

			auto& page = state->pages.new_word;
			_get_edit_str(page.edit_hiragana, w16.attributes.hiragana);
			_get_edit_str(page.edit_kanji, w16.attributes.kanji);
			_get_edit_str(page.edit_meaning, w16.attributes.meaning);
			_get_edit_str(page.edit_mnemonic, w16.attributes.mnemonic);
			_get_edit_str(page.edit_notes, w16.attributes.notes);
			_get_edit_str(page.edit_example_sentence, w16.attributes.example_sentence);
			_get_combo_sel_idx_as_str(page.combo_lexical_category, w16.attributes.lexical_category);
			
			//TODO(fran): check for similar words, and consult the user whether they want to create it as a new word or cancel cause the same word already exists

			//Now we can finally do the insert
			//TODO(fran): see if there's some way to go straight from utf16 to the db, and to send things like ints without having to convert them to strings. we could show a list, like we do on the landing page, with the similar words, and only if they then click on an element of the list we open a separate window to allow them the see it in full / edit it
			int insert_res = insert_word(state->settings->db, w16);

			//Error handling
			switch (insert_res) {
			case SQLITE_OK: { res = true; } break;
			case SQLITE_CONSTRAINT:
			{
				Assert(0); break;//TODO(fran): check for duplicate words manually, this codepath is not being triggered now
				//TODO(fran): this should actually be a more specific check for word.hiragana but for now we know that's the only constraint check there is

				//The user tried to add a word that already exists, we must notify them about it and ask wether to override the previous word or cancel
				//INFO: we mustnt allow the user to edit the word while the msgbox is active since we dont retrieve the data, we use the one we already have
				//On a separate window we show the previously existing word so the user can compare and or copy it

				{//Open separate page with the currently stored word
					//TODO(fran): streamline this process
					べんきょうSettings* べんきょう_cl = (decltype(べんきょう_cl))malloc(sizeof(べんきょうSettings));//TODO(fran): MEMLEAK: maybe we can say that non primary windows have to release this memory but it's pretty hacky
					RECT べんきょう_nc_rc; GetWindowRect(state->nc_parent, &べんきょう_nc_rc);
					int w = RECTW(べんきょう_nc_rc);
					べんきょう_nc_rc.left = べんきょう_nc_rc.right;
					べんきょう_nc_rc.right += w;
					//TODO(fran): place new window on the left if no space is available on the right
					べんきょう_cl->db = state->settings->db;
					べんきょう_cl->is_primary_wnd = false;

					unCapNcLpParam べんきょう_nclpparam;
					べんきょう_nclpparam.client_class_name = べんきょう::wndclass;
					べんきょう_nclpparam.client_lp_param = べんきょう_cl;
					//TODO(fran): tell the window which pages we want it to create, otherwise window creation takes a couple of seconds, hanging the whole application with it

					HWND べんきょう_nc = CreateWindowEx(WS_EX_CONTROLPARENT, nonclient::wndclass, global::app_name, WS_VISIBLE | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						べんきょう_nc_rc.left, べんきょう_nc_rc.top, RECTWIDTH(べんきょう_nc_rc), RECTHEIGHT(べんきょう_nc_rc), nullptr, nullptr, GetModuleHandleW(NULL), &べんきょう_nclpparam);
					Assert(べんきょう_nc);

					べんきょう::set_brushes(nonclient::get_state(べんきょう_nc)->client, TRUE, global::colors.ControlBk);
					べんきょう::set_current_page(べんきょう::get_state(nonclient::get_state(べんきょう_nc)->client), ProcState::page::show_word);

					if (stored_word16_res old_word = get_stored_word(state->settings->db, w16); old_word.found) {
						defer{ free_stored_word(old_word.word); };
						べんきょう::preload_page(べんきょう::get_state(nonclient::get_state(べんきょう_nc)->client), ProcState::page::show_word, &old_word.word);
					}
					UpdateWindow(べんきょう_nc);
				}

				int ret = MessageBoxW(state->nc_parent, RCS(170), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
				if (ret == IDYES) {
					Assert(0); break;//TODO(fran): again, manually check for repeated words, we could offer this option of updating an already existing word with the new contents
					//res = update_word(state->settings->db, &w8);
				}

			} break;
			default: { sqlite_runtime_check(false, state->settings->db); } break;
			}
			//TODO(fran): maybe handle repeated words here
		}
		return res;
	}


	struct practice_data {
		ProcState::page page;
		void* data;
	};
	practice_data prepare_practice(ProcState* state) {
		practice_data res;
		ProcState::available_practices practices[]{
			ProcState::available_practices::writing,
			ProcState::available_practices::multiplechoice,
			ProcState::available_practices::drawing,
		};
		
		ProcState::available_practices practice = practices[random_between(0u,(u32)ARRAYSIZE(practices)-1)];

		auto get_hiragana_kanji_meaning = [](learnt_word16* w)->u32 { /*Says which parts of the word are filled/valid*/
			u32 res = 0;
			if (w) {
				if (w->attributes.hiragana.str && *w->attributes.hiragana.str) res |= (u32)learnt_word_elem::hiragana;
				if (w->attributes.kanji.str && *w->attributes.kanji.str) res |= (u32)learnt_word_elem::kanji;
				if (w->attributes.meaning.str && *w->attributes.meaning.str) res |= (u32)learnt_word_elem::meaning;
			}
			return res;
		};

		switch (practice) {
		case ProcState::available_practices::writing:
		{
			//NOTE: writing itself has many different practices
			practice_writing_word* data = (decltype(data))malloc(sizeof(*data));//TODO(fran): MEMLEAK practice_writing page will take care of freeing this once the page completes its practice
			learnt_word16 practice_word = get_practice_word(state->settings->db);//get a target word
			data->word = std::move(practice_word);

			std::vector<practice_writing_word::type> practice_types;
			auto word_types = get_hiragana_kanji_meaning(&data->word); Assert(word_types);
			//bool hiragana = has_hiragana(&data->word), translation = has_translation(&data->word), kanji = has_kanji(&data->word);
			practice_types.push_back(practice_writing_word::type::hiragana_to_meaning);//we always add a default to make sure there can never be no items
			if (word_types & (u32)learnt_word_elem::hiragana) {
				if (word_types & (u32)learnt_word_elem::meaning) 
					practice_types.push_back(practice_writing_word::type::meaning_to_hiragana);
				if (word_types & (u32)learnt_word_elem::kanji) 
					practice_types.push_back(practice_writing_word::type::kanji_to_hiragana);
			}
			if (word_types & (u32)learnt_word_elem::kanji && word_types & (u32)learnt_word_elem::meaning) 
				practice_types.push_back(practice_writing_word::type::kanji_to_meaning);

			data->practice_type = practice_types[random_between(0, (i32)(practice_types.size() - 1))];
			res.page = ProcState::page::practice_writing;
			res.data = data;
		} break;
		case ProcState::available_practices::multiplechoice:
		{
			practice_multiplechoice_word* data = (decltype(data))malloc(sizeof(*data));

			learnt_word16 practice_word = get_practice_word(state->settings->db);//get a target word
			data->question = std::move(practice_word);
			u32 q_and_a = get_hiragana_kanji_meaning(&data->question);//NOTE: the type of the question and choices is limited by our target word
			data->question_type = (decltype(data->question_type))random_bit_set(q_and_a);
			data->choices_type = (decltype(data->question_type))random_bit_set(q_and_a & (~(u32)data->question_type));
			
			ptr<utf16*> _choices = get_word_choices(state->settings->db, data->choices_type, 5, &data->question); defer{ _choices.free(); };
			data->idx_answer = random_between(0u, (u32)_choices.cnt);
			
			data->choices = { 0 };//clear it to avoid free() problems, TODO(fran): I dont like the free() inside alloc() idea, maaybe we could create two functions alloc() and alloc_free_prev()
			data->choices.alloc(_choices.cnt + 1);

			//Move all the choices plus the correct choice into the new array
			for (int i = 0, j = 0; i < data->choices.cnt; i++) {
				if (i == data->idx_answer) data->choices[i] = _wcsdup(str_for((learnt_word16*)&data->question, data->choices_type).str);//TODO(fran): replace for my own duplication method
				else data->choices[i] = _choices[j++];
			}

			data->question_str = str_for(&data->question, data->question_type).str;
			
			res.page = ProcState::page::practice_multiplechoice;
			res.data = data;
		} break;
		case ProcState::available_practices::drawing:
		{
			practice_drawing_word* data = (decltype(data))malloc(sizeof(*data));
			learnt_word16 practice_word = get_practice_word(state->settings->db, false,true,false);//get a target word
			//TODO(fran): check the word is valid, otherwise recursively call this function (cut recursion after say 5 tries and simply end the practice)
			data->question = std::move(practice_word);
			
			u32 q_elems = get_hiragana_kanji_meaning(&data->question);
			data->question_type = (decltype(data->question_type))random_bit_set(q_elems & (~(u32)learnt_word_elem::kanji));
			data->question_str = str_for(&data->question, data->question_type).str;

			res.page = ProcState::page::practice_drawing;
			res.data = data;
		} break;

		default: Assert(0);
		}
		return res;
	}

	void init_cpp_objects(ProcState* state) {
		state->pagestate.practice_review.practices = decltype(state->pagestate.practice_review.practices)();
	}

	//IMPORTANT: handling the review page, we'll have two vectors, one floating in space to which each practice level will add to each time it completes, once the whole practice is complete we'll preload() review_practice with std::move(this vector), this page has it's own vector which at this point will be emptied and its contents freed and replaced with the new vector (the floating in space one). This guarantees the review page is always on a valid state. //TODO(fran): idk if std::move works when I actually need to send the vector trough a pointer, it may be better to allocate an array and send it together with its size (which makes the vector pointless all together), I do think allocating arrays is beter, we simply alloc when the start button is pressed on the practice page and at the end of the practice send that same pointer to review. The single annoying problem is the back button, solution: we'll go simple for now, hide the back button until the practice is complete, the review page will have the back button active and that will map back to the practice page (at least at the time Im writing this). This way we guarantee valid states for everything, the only semi problem would be if the user decides to close the application, in that case we could, if we wanted, ask for confirmation depending on the page, but once the user says yes we care no more about memory, yes there will be some mem leaks but at that point it no longer matters since that memory will be removed automatically by the OS

	//TODO(fran): in practice_writing: I like the idea of putting a bar in the middle between the question and answer and that it lights up when the user responds, it either goes green and it's text says "Correct!" or red and text "Incorrect: right answer was [...]". The good thing about this is that I dont need to add extra controls for the review page, I can simply reconstruct the exact same page and there's already a spot to indicate the correction, would be the same with a multiple choice, eg 3 words user clicks wrong answer and it lights up red and the correct one lights up green, the thing with this is that I'd need the text colors to be fixed in order to not be opaqued by the new red/green light, that's kind of annoying but I dont see an easy solution. Sidenode: actually wanikani doesnt have a separate bar, it chages the color of the edit box


	void reset_page(ProcState* state, ProcState::page page) {
		//NOTE: one solution here would be to destroy all the controls remove_controls(page) and then call addcontrols(page)
		switch (page) {
		case decltype(page)::new_word:
		{
			//NOTE: the problem here is that its not enough to settext to null to everyone, that can be done fairly easily implementing some WM_RESET msg, but there are some controls that shouldnt be reset like the buttons, we'd need to implement a .reseteable() or somehow via union magic or smth that gives us only the controls that should be reset
			auto& controls = state->pages.new_word;
			_clear_combo_sel(controls.combo_lexical_category);
			_clear_edit(controls.edit_hiragana);
			_clear_edit(controls.edit_kanji);
			_clear_edit(controls.edit_mnemonic);
			_clear_edit(controls.edit_notes);
			_clear_edit(controls.edit_example_sentence);
			_clear_edit(controls.edit_meaning);
		} break;
		case decltype(page)::practice_writing:
		{
			auto& controls = state->pages.practice_writing;

			_clear_static(controls.static_test_word);
			_clear_edit(controls.edit_answer);
			//TODO(fran): I really need to destroy the controls and call addcontrols, I cant be restoring colors here when that's already done there
		} break;
		case decltype(page)::practice_multiplechoice:
		{
			auto& controls = state->pages.practice_multiplechoice;
			//TODO(fran): do we wanna clear smth here?
		} break;
		case decltype(page)::practice_drawing:
		{
			auto& controls = state->pages.practice_drawing;
			paint::clear_canvas(controls.paint_answer);
			paint::set_placeholder(controls.paint_answer, nullptr);
		} break;

		default:Assert(0);
		}
	}

	void _next_practice_level(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
		ProcState* state = get_state(hwnd);
		KillTimer(state->wnd, anim_id);

		if (state->practice_cnt-- > 0) {
			practice_data practice = prepare_practice(state);//get a random practice
			reset_page(state, practice.page);
			preload_page(state, practice.page, practice.data);//load the practice
			set_current_page(state, practice.page);//go practice!
		}
		else {
			user_stats_increment_times_practiced(state->settings->db);
			preload_page(state, ProcState::page::review_practice,&state->multipagestate.temp_practices);
			set_current_page(state, ProcState::page::review_practice);
		}
	}

	//decreases the practice counter and loads/sets a new practice level or goes to the review page if the practice is over
	void next_practice_level(ProcState* state, bool add_delay=true) {
		u32 delay = add_delay ? 500 : USER_TIMER_MINIMUM;
		SetTimer(state->wnd, timerIDs.next_practice_level, delay, _next_practice_level);
	}

	void button_practice_func_on_click(void* element, void* user_extra) {
		ProcState* state = get_state((HWND)user_extra);
		if (state) {
			store_previous_page(state, state->current_page);
			set_current_page(state, ProcState::page::practice);
		}
	}

	enum notification_relevance { success, error };
	void notify(ProcState* state, ProcState::page page, notification_relevance category, const utf16* notif) {
		HBRUSH notif_br;
		switch (category) {
		case decltype(category)::success: notif_br = global::colors.Bk_right_answer; break;
		case decltype(category)::error: notif_br = global::colors.Bk_wrong_answer; break;
		default:notif_br = 0; Assert(0);
		}

		switch (page) {
		case decltype(page)::new_word:
		{
			HWND notifier = state->pages.new_word.static_notify;
			static_oneline::Theme notif_theme;
			notif_theme.brushes.foreground.normal = notif_br;
			static_oneline::set_theme(notifier, &notif_theme);
			SendMessageW(notifier, WM_SETTEXT, 0, (LPARAM)notif);
		} break;
		default: Assert(0);
		}
	}
	void notify(ProcState* state, notification_relevance category, const utf16* notif) {
		notify(state, state->current_page, category, notif);
	}

	HWND create_page(ProcState* state, const page::Theme& theme) {
		HWND page = CreateWindowW(page::wndclass, NULL, WS_CHILD //TODO(fran): WS_CLIPCHILDREN?
			, 0, 0, 0, 0, state->pages.page_space, 0, NULL, NULL);
		Assert(page);
		page::set_theme(page, &theme);
		page::set_scrolling(page, true);
		return page;
	}

	struct sidebar_state_animate {
		rect_i32 origin;//Where we started moving //TODO(fran): there may be a way of doing this without the origin
		rect_i32 target;//Where the sidebar will be placed on the last frame of animation, the sidebar's width and height will be set equal to the target's during the whole animation
		bool show;//if show == true sidebar is shown on first animation frame, if false sidebar is hidden on last animation frame
		f32 t; //[0.0,1.0]
		f32 dt;//increment for 't' per frame
	};

	void animate_sidebar(ProcState* state, u32 ms) {
		static constexpr utf16 state_key[] = L"sidebar_state_animate";//NOTE: I need to make this static so the lambda below can capture it
		HWND sidebar = state->pages.sidebar;
		sidebar_state_animate* sidebar_state = (decltype(sidebar_state))GetPropW(sidebar, state_key);
		const bool show = !IsWindowVisible(sidebar);
		const f32 duration_sec = (f32)ms/1000.f;
		const u32 total_frames = (u32)ceilf(duration_sec / (1.f/win32_get_refresh_rate_hz(sidebar)));
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
			sidebar_state = (decltype(sidebar_state))malloc(sizeof(sidebar_state_animate));
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
			sidebar_state_animate* sidebar_state = (decltype(sidebar_state))GetPropW(hwnd, state_key);
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

	void button_function_on_click_goto_page_new(void* element, void* user_extra) {
		ProcState* state = (decltype(state))user_extra;

		store_previous_page(state, state->current_page);
		reset_page(state, ProcState::page::new_word);
		set_current_page(state, ProcState::page::new_word);
	}

	void button_function_on_click_goto_page_practice(void* element, void* user_extra) {
		ProcState* state = (decltype(state))user_extra;

		store_previous_page(state, state->current_page);
		set_current_page(state, ProcState::page::practice);
	}

	void create_controls(ProcState* state) {
		DWORD style_button_txt = WS_CHILD | WS_TABSTOP | button::style::roundrect;
		DWORD style_button_bmp = WS_CHILD | WS_TABSTOP | button::style::roundrect | BS_BITMAP;
		DWORD style_button_icon = WS_CHILD | WS_TABSTOP | button::style::roundrect | BS_ICON;
		button::Theme base_btn_theme;
		base_btn_theme.dimensions.border_thickness = 1;
		base_btn_theme.brushes.bk.normal = global::colors.ControlBk;
		base_btn_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
		base_btn_theme.brushes.bk.clicked = global::colors.ControlBkPush;
		base_btn_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
		base_btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
		base_btn_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
		base_btn_theme.brushes.border.normal = global::colors.Img;//TODO(fran): global::colors.ControlBorder
		//TODO(fran): use the extra brushes, fore_push,... , border_mouseover,...

		button::Theme img_btn_theme = base_btn_theme;
		img_btn_theme.brushes.foreground.normal = global::colors.Img;

		button::Theme accent_btn_theme = base_btn_theme;
		accent_btn_theme.brushes.foreground.normal = global::colors.Accent;
		accent_btn_theme.brushes.border.normal = global::colors.Accent;

		static_oneline::Theme base_static_theme;
		base_static_theme.brushes.foreground.normal = global::colors.ControlTxt;
		base_static_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
		base_static_theme.brushes.bk.normal = global::colors.ControlBk;
		base_static_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;

		static_oneline::Theme kanji_static_theme = base_static_theme;
		kanji_static_theme.brushes.foreground.normal = brush_for(learnt_word_elem::kanji);

		navbar::Theme nav_theme;
		nav_theme.brushes.bk.normal = global::colors.ControlBk_Disabled;//TODO(fran): darker color than bk
		nav_theme.dimensions.spacing = 3;
		nav_theme.dimensions.is_vertical = false;

		navbar::Theme sidebar_theme = nav_theme;
		sidebar_theme.dimensions.spacing = 0;
		sidebar_theme.dimensions.is_vertical = true;

		button::Theme navbar_btn_theme = base_btn_theme;
		navbar_btn_theme.brushes.bk.normal = nav_theme.brushes.bk.normal;
		navbar_btn_theme.brushes.border = navbar_btn_theme.brushes.bk;

		button::Theme navbar_img_btn_theme = img_btn_theme;
		navbar_img_btn_theme.brushes.bk.normal = nav_theme.brushes.bk.normal;
		navbar_img_btn_theme.brushes.border = navbar_img_btn_theme.brushes.bk;

		button::Theme dark_btn_theme = base_btn_theme;
		dark_btn_theme.brushes.bk.normal = global::colors.ControlBk_Dark;

		button::Theme dark_nonclickable_btn_theme = dark_btn_theme;
		for (auto& b : dark_nonclickable_btn_theme.brushes.bk.all) b = dark_nonclickable_btn_theme.brushes.bk.normal;
		for (auto& b : dark_nonclickable_btn_theme.brushes.border.all) b = dark_nonclickable_btn_theme.brushes.border.normal;

		embedded::show_word_reduced::Theme eswr_theme;
		eswr_theme.font = global::fonts.General;
		eswr_theme.dimensions.border_thickness = 1;
		eswr_theme.brushes.bk.normal = global::colors.ControlBk;
		eswr_theme.brushes.txt.normal = global::colors.ControlTxt;
		eswr_theme.brushes.border.normal = global::colors.ControlTxt;

		embedded::show_word_disambiguation::Theme eswd_theme;
		eswd_theme.font = global::fonts.General;
		eswd_theme.dimensions.border_thickness = 1;
		eswd_theme.brushes.bk.normal = global::colors.ControlBk;
		eswd_theme.brushes.txt.normal = global::colors.ControlTxt;
		eswd_theme.brushes.border.normal = global::colors.ControlTxt;

		edit_oneline::Theme base_editoneline_theme;
		base_editoneline_theme.dimensions.border_thickness = 1;
		base_editoneline_theme.brushes.foreground.normal = global::colors.ControlTxt;
		base_editoneline_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
		base_editoneline_theme.brushes.bk.normal = global::colors.ControlBk;
		base_editoneline_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
		base_editoneline_theme.brushes.border.normal = global::colors.Img;
		base_editoneline_theme.brushes.border.disabled = global::colors.Img_Disabled;
		base_editoneline_theme.brushes.selection.normal = global::colors.Selection;
		base_editoneline_theme.brushes.selection.disabled = global::colors.Selection_Disabled;

		edit_oneline::Theme hiragana_editoneline_theme = base_editoneline_theme;
		hiragana_editoneline_theme.brushes.foreground.normal = brush_for(learnt_word_elem::hiragana);

		edit_oneline::Theme kanji_editoneline_theme = base_editoneline_theme;
		kanji_editoneline_theme.brushes.foreground.normal = brush_for(learnt_word_elem::kanji);

		edit_oneline::Theme meaning_editoneline_theme = base_editoneline_theme;
		meaning_editoneline_theme.brushes.foreground.normal = brush_for(learnt_word_elem::meaning);

		page::Theme base_page_theme;
		base_page_theme.brushes.bk.normal = global::colors.ControlBk;
		base_page_theme.brushes.border = base_page_theme.brushes.bk;
		base_page_theme.dimensions.border_thickness = 0;

		//---------------------Navbar----------------------:

		{
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
					animate_sidebar(state, 200);
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
			searchbox::maintain_placerholder_when_focussed(search, true);
			edit_oneline::set_IME_wnd(searchbox::get_controls(search).editbox, true);
			navbar::attach(navbar, search, navbar::attach_point::center, -1);
			SendMessage(search, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
			//TODO(fran): searchbox: changing color of editbox text based on whether it has meaning,hiragana,kanji?
			//TODO(fran): searchbox: restore what the user wrote when they press the escape key
		}

		//---------------------Page Space----------------------: //TODO(fran): rename to page_slot?
		state->pages.page_space = CreateWindowW(page::wndclass, NULL, WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE //TODO(fran): WS_CLIPCHILDREN?
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		//IMPORTANT INFO: WS_CLIPSIBLINGS is crucial in order to make the sidebar work and not be occluded by the page, it prevents the page from drawing over the sidebar, otherwise it gets occluded
		page::set_theme(state->pages.page_space, &base_page_theme);

		//---------------------Sidebar----------------------: 
		{
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

		//---------------------Landing page----------------------:
		{
			auto& controls = state->pages.landing;
		
			controls.page = create_page(state, base_page_theme);

			auto page = controls.page;

#if 0
			//NOTE: for this to be usable as a page background we need to work with alpha in order to do proper transparency on everything that goes on top of this
			controls.candy = CreateWindowW(eyecandy::wndclass, 0, WS_CHILD | WS_CLIPCHILDREN
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			eyecandy::Theme candy_theme;
			candy_theme.brushes.bk.normal = global::colors.ControlBk;//global::colors.Bk_wrong_answer
			candy_theme.brushes.foreground.normal = global::colors.Img;
			candy_theme.dimensions.border_thickness = 0;
			eyecandy::set_theme(controls.candy, &candy_theme);
			eyecandy::set_db(controls.candy, state->settings->db);
			page = controls.candy;
#endif

			controls.listbox_recents = CreateWindowW(listbox::wndclass, 0, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_function_render(controls.listbox_recents, listbox_recents_func_render);
			listbox::set_user_extra(controls.listbox_recents, state);
			listbox::set_function_on_click(controls.listbox_recents, [](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					learnt_word16* txt = (decltype(txt))element;

					stored_word16_res res = get_stored_word(state->settings->db, *txt/*TODO(fran): make sure this isnt a copy*/);  defer{ if (res.found) free_stored_word(res.word); };
					if (res.found) {
						preload_page(state, ProcState::page::show_word, &res.word);
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::show_word);
					}
					//TODO(fran): else {notify user of error finding the word}, we need to get good error info from the db functions
				}
			);

			controls.button_recents = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.button_recents, 103);
			button::set_theme(controls.button_recents, &dark_btn_theme);
			button::set_user_extra(controls.button_recents, state);
			button::set_function_render(controls.button_recents, button_recents_func_render);
			button::set_function_on_click(controls.button_recents,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					HWND listbox = state->pages.landing.listbox_recents;
					state->pagestate.landing.hide_recents = !state->pagestate.landing.hide_recents;
					//flip_visibility(listbox);
					ask_for_resize(state);
					ask_for_repaint(state);
					//TODO(fran): BUG: listbox doesnt re-render when being shown after being hidden
				}
			);

			controls.static_word_cnt_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_word_cnt_title, 351);
			static_oneline::set_theme(controls.static_word_cnt_title, &base_static_theme);

			controls.static_word_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_word_cnt, &base_static_theme);

			controls.static_practice_cnt_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_practice_cnt_title, 352);
			static_oneline::set_theme(controls.static_practice_cnt_title, &base_static_theme);

			controls.static_practice_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_practice_cnt, &base_static_theme);

			controls.static_accuracy_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_accuracy_title, 353);
			static_oneline::set_theme(controls.static_accuracy_title, &base_static_theme);

			controls.score_accuracy = CreateWindowW(score::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			score::set_brushes(controls.score_accuracy, FALSE, global::colors.ControlBk, global::colors.Score_RingBk, global::colors.Score_RingFull, global::colors.Score_RingEmpty, global::colors.Score_InnerCircle);


			controls.static_accuracy_timeline_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_accuracy_timeline_title, 354);
			static_oneline::set_theme(controls.static_accuracy_timeline_title, &base_static_theme);

			controls.graph_accuracy_timeline = CreateWindowW(graph::wndclass, NULL, WS_CHILD | GP_CURVE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			graph::set_brushes(controls.graph_accuracy_timeline, FALSE, global::colors.Graph_Line, global::colors.Graph_BkUnderLine, global::colors.Graph_Bk, global::colors.Graph_Border);

			//TODO(fran): we may want to add smth else like a total number of words practiced

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------New word----------------------:
		{
			auto& controls = state->pages.new_word;

			controls.page = create_page(state, base_page_theme);

			//TODO(fran): hide primary IME window, the one that shows the composition string, we no longer need it now we show the string straight into the editbox
			controls.edit_hiragana = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_hiragana, &hiragana_editoneline_theme);
			edit_oneline::maintain_placerholder_when_focussed(controls.edit_hiragana, true);
			AWDT(controls.edit_hiragana, 120);

			controls.edit_kanji = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_kanji, &kanji_editoneline_theme);
			edit_oneline::maintain_placerholder_when_focussed(controls.edit_kanji, true);
			AWDT(controls.edit_kanji, 121);

			controls.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.combo_lexical_category);
			SetWindowSubclass(controls.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.combo_lexical_category, 123);

			controls.edit_meaning = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_meaning, &meaning_editoneline_theme);
			edit_oneline::maintain_placerholder_when_focussed(controls.edit_meaning, true);
			AWDT(controls.edit_meaning, 122);

			controls.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_mnemonic, &base_editoneline_theme);
			AWDT(controls.edit_mnemonic, 125);

			controls.edit_example_sentence = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_example_sentence, &base_editoneline_theme);
			AWDT(controls.edit_example_sentence, 127);

			controls.edit_notes = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_notes, &base_editoneline_theme);
			AWDT(controls.edit_notes, 126);
			//NOTE: remember that the window switching order because of tabstop is the same as the window creation order

			controls.button_save = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_save, 124);
			button::set_theme(controls.button_save, &base_btn_theme);
			button::set_user_extra(controls.button_save, state);
			button::set_function_on_click(controls.button_save,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					if (save_new_word(state)) {
						learnt_word16 empty{ 0 };
						preload_page(state, state->current_page, &empty);//TODO(fran): function restart_page()

						notify(state, notification_relevance::success, RS(600).c_str());
					}
					else notify(state, notification_relevance::error, RS(601).c_str());
				}
			);

			controls.static_notify = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_RIGHT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_notify, &base_static_theme);
			SetWindowSubclass(controls.static_notify, NotifyProc, 0, 0);
			Notify_SetTextDuration(controls.static_notify, 2000);


			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------Show word----------------------:
		{
			auto& controls = state->pages.show_word;

			controls.page = create_page(state, base_page_theme);

			controls.static_id = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);

			controls.edit_hiragana = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWDT(controls.edit_hiragana, 120);
			edit_oneline::set_theme(controls.edit_hiragana, &hiragana_editoneline_theme);

			controls.edit_kanji = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_kanji, &kanji_editoneline_theme);
			AWDT(controls.edit_kanji, 121);

			controls.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.combo_lexical_category);
			SetWindowSubclass(controls.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.combo_lexical_category, 123);

			controls.edit_meaning = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_meaning, &meaning_editoneline_theme);
			AWDT(controls.edit_meaning, 122);

			controls.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_mnemonic, &base_editoneline_theme);
			AWDT(controls.edit_mnemonic, 125);

			controls.edit_example_sentence = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_example_sentence, &base_editoneline_theme);
			AWDT(controls.edit_example_sentence, 127);

			controls.edit_notes = CreateWindowW(edit_oneline::wndclass, NULL, WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT | ES_MULTILINE | ES_EXPANSIBLE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			edit_oneline::set_theme(controls.edit_notes, &base_editoneline_theme);
			AWDT(controls.edit_notes, 126);

			controls.static_creation_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_creation_date, &base_static_theme);

			controls.static_last_practiced_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_last_practiced_date, &base_static_theme);

			controls.static_score = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_score, &base_static_theme);

			controls.button_modify = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_modify, 273);
			button::set_theme(controls.button_modify, &base_btn_theme);
			button::set_user_extra(controls.button_modify, state);
			button::set_function_on_click(controls.button_modify,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					if (modify_word(state)) {
						goto_previous_page(state);//TODO(fran): dont go back, simply notify the user of the successful modification; or first notify and only later go back
					}
				}
			);

			controls.button_delete = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			//AWT(controls.button_modify, 273);
			button::set_theme(controls.button_delete, &img_btn_theme);
			SendMessage(controls.button_delete, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.bin);
			button::set_user_extra(controls.button_delete, state);
			button::set_function_on_click(controls.button_delete,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					int ret = MessageBoxW(state->nc_parent, RCS(280), L"", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
					if (ret == IDYES) {
						if (remove_word(state)) {
							goto_previous_page(state);
						}
					}
				}
			);

			controls.button_remember = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_remember, 275);
			AWTT(controls.button_remember, 276);
			button::set_theme(controls.button_remember, &accent_btn_theme);
			button::set_user_extra(controls.button_remember, state);
			button::set_function_on_click(controls.button_remember,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					Assert(state->current_page == ProcState::page::show_word);
					auto& page = state->pages.show_word;
					//TODO(fran): change button bk and mouseover color to green
					if (prioritize_word(state)) {
						button::Theme accent_btn_theme;
						accent_btn_theme.brushes.foreground.normal = global::colors.Bk_right_answer;
						accent_btn_theme.brushes.border.normal = global::colors.Bk_right_answer;
						button::set_theme(page.button_remember, &accent_btn_theme);
					}
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice----------------------:
		{
			auto& controls = state->pages.practice;

			controls.page = create_page(state, base_page_theme);

			auto page = controls.page;

			controls.listbox_words_practiced = CreateWindowW(listbox::wndclass, 0, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_user_extra(controls.listbox_words_practiced, state);
			listbox::set_function_render(controls.listbox_words_practiced, 
				[](HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
					int w = r.w, h = r.h;
					practiced_word16* practiced = (decltype(practiced))element;

					//Draw bk
					HBRUSH bk_br = practiced->answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
					if (flags.onSelected || flags.onMouseover)
						bk_br = practiced->answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
					if (flags.onClicked) 
						bk_br = practiced->answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;

					HBRUSH hira_br = global::colors.ControlTxt;
					HBRUSH kanji_br = global::colors.ControlTxt;
					HBRUSH meaning_br = global::colors.ControlTxt;

					render_hiragana_kanji_meaning(dc, r, bk_br, hira_br, kanji_br, meaning_br, &practiced->word);
				}
			);
			listbox::set_function_on_click(controls.listbox_words_practiced, 
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					practiced_word16* practiced = (decltype(practiced))element;

					stored_word16_res res = get_stored_word(state->settings->db, practiced->word);  defer{ if (res.found) free_stored_word(res.word); };
					if (res.found) {
						preload_page(state, ProcState::page::show_word, &res.word);
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::show_word);
					}
					else {
						MessageBoxW(state->wnd, RCS(401), 0, MB_OK, MBP::center);
					}
				}
			);

			controls.button_words_practiced = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.button_words_practiced, 400);
			button::set_theme(controls.button_words_practiced, &dark_nonclickable_btn_theme);
			button::set_user_extra(controls.button_words_practiced, state);
			button::set_function_render(controls.button_words_practiced, button_recents_func_render);
			button::set_function_on_click(controls.button_words_practiced,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					//do nothing, we're here just for looks
				}
			);

			controls.button_start = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_start, 350);
			button::set_theme(controls.button_start, &base_btn_theme);
			button::set_user_extra(controls.button_start, state);
			button::set_function_on_click(controls.button_start,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					i64 word_cnt = get_user_stats(state->settings->db).word_cnt; //TODO(fran): I dont know if this is the best way to do it but it is the most accurate
					if (word_cnt > 0) {
						int practice_cnt = 10;
#if 0 && defined(_DEBUG)
						practice_cnt = 2;
#endif
						state->practice_cnt = (u32)min(word_cnt, practice_cnt);//set the practice counter (and if there arent enough words reduce the practice size, not sure how useful this is)
						store_previous_page(state, state->current_page);

						//Clear previous practice data:
						clear_practices_vector(state->multipagestate.temp_practices);

						//NOTE: there's the possibility of precalculating the entire array of practice levels from the start which "guarantees" to avoid duplicates (same word shown twice), also simplifies the code a bit since there's only one place where we'd allocate and store. But the good thing about not precalculating the array is increased randomness, each time we get a word we get a random choice, therefore say 15 random choices is more random than 1 random choice for 15 words (based on how bad I think sql's random is)

						next_practice_level(state, false);
					}
					else MessageBoxW(state->wnd, RCS(360), 0, MB_OK, MBP::center);
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice Writing----------------------:
		{
			auto& controls = state->pages.practice_writing;

			controls.page = create_page(state, base_page_theme);

			controls.static_test_word = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_test_word, &base_static_theme);
			//NOTE: text color will be set according to the type of word being shown

			controls.edit_answer = CreateWindowW(edit_oneline::wndclass, 0, WS_CHILD | ES_CENTER | WS_TABSTOP | WS_CLIPCHILDREN | ES_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			//NOTE: text color and default text will be set according to the type of word that has to be written

			controls.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.edit_answer, 0, NULL, NULL);
			SendMessage(controls.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			controls.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, 0, 0);
			button::set_theme(controls.button_show_word, &base_btn_theme);
			SendMessage(controls.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);
			button::set_user_extra(controls.button_show_word, state);
			button::set_function_on_click(controls.button_show_word,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& page = state->pages.practice_writing;
					flip_visibility(page.embedded_show_word_reduced);
					if (IsWindowVisible(page.embedded_show_word_disambiguation))ShowWindow(page.embedded_show_word_disambiguation, SW_HIDE);
				}
			);

			controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
				0, 0, 0, 0, controls.page, 0, 0, 0);
			embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

			controls.button_show_disambiguation = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			button::set_theme(controls.button_show_disambiguation, &base_btn_theme);
			SendMessage(controls.button_show_disambiguation, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.disambiguation);
			AWTT(controls.button_show_disambiguation, 700);
			button::set_user_extra(controls.button_show_disambiguation, state);
			button::set_function_on_click(controls.button_show_disambiguation,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& page = state->pages.practice_writing;
					flip_visibility(page.embedded_show_word_disambiguation);
					if (IsWindowVisible(page.embedded_show_word_reduced))ShowWindow(page.embedded_show_word_reduced, SW_HIDE);
				}
			);

			controls.embedded_show_word_disambiguation = CreateWindow(embedded::show_word_disambiguation::wndclass, NULL, WS_CHILD | embedded::show_word_disambiguation::style::roundrect,
				0, 0, 0, 0, controls.page, 0, 0, 0);
			embedded::show_word_disambiguation::set_theme(controls.embedded_show_word_disambiguation, &eswd_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice Multiplechoice----------------------:
		{
			auto& controls = state->pages.practice_multiplechoice;

			controls.page = create_page(state, base_page_theme);

			controls.static_question = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_question, &base_static_theme);
			//NOTE: text color will be set according to the type of word being shown

			//TODO(fran): should I simply use a gridview instead?
			controls.multibutton_choices = CreateWindowW(multibutton::wndclass, 0, WS_CHILD | WS_CLIPCHILDREN | multibutton::style::roundrect
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			multibutton::Theme multibutton_choices_theme;
			multibutton_choices_theme.dimensions.border_thickness = 1;
			multibutton_choices_theme.brushes.bk.normal = global::colors.ControlBk;//TODO(fran): try with a different color to make it destacar
			multibutton_choices_theme.brushes.border.normal = global::colors.Img;
			multibutton::set_theme(controls.multibutton_choices, &multibutton_choices_theme);
			//NOTE: buttons' colors will be set according to the type of word that has to be written

			controls.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			SendMessage(controls.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			controls.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, 0, 0);
			button::set_theme(controls.button_show_word, &base_btn_theme);
			SendMessage(controls.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);
			button::set_user_extra(controls.button_show_word, state);
			button::set_function_on_click(controls.button_show_word,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& page = state->pages.practice_multiplechoice;
					flip_visibility(page.embedded_show_word_reduced);
					if (IsWindowVisible(page.embedded_show_word_disambiguation))ShowWindow(page.embedded_show_word_disambiguation, SW_HIDE);
				}
			);

			controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
				0, 0, 0, 0, controls.page, 0, 0, 0);
			embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

			controls.button_show_disambiguation = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			button::set_theme(controls.button_show_disambiguation, &base_btn_theme);
			SendMessage(controls.button_show_disambiguation, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.disambiguation);
			AWTT(controls.button_show_disambiguation, 700);
			button::set_user_extra(controls.button_show_disambiguation, state);
			button::set_function_on_click(controls.button_show_disambiguation,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& page = state->pages.practice_multiplechoice;
					flip_visibility(page.embedded_show_word_disambiguation);
					if (IsWindowVisible(page.embedded_show_word_reduced))ShowWindow(page.embedded_show_word_reduced, SW_HIDE);
				}
			);

			controls.embedded_show_word_disambiguation = CreateWindow(embedded::show_word_disambiguation::wndclass, NULL, WS_CHILD | embedded::show_word_disambiguation::style::roundrect,
				0, 0, 0, 0, controls.page, 0, 0, 0);
			embedded::show_word_disambiguation::set_theme(controls.embedded_show_word_disambiguation, &eswd_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice Drawing----------------------:
		{
			auto& controls = state->pages.practice_drawing;

			controls.page = create_page(state, base_page_theme);

			controls.static_question = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_question, &base_static_theme);
			//NOTE: text color will be set according to the type of word being shown

			controls.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			button::set_theme(controls.button_next, &base_btn_theme);
			SendMessage(controls.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			controls.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, 0, 0);
			button::set_theme(controls.button_show_word, &base_btn_theme);
			SendMessage(controls.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);
			button::set_user_extra(controls.button_show_word, state);
			button::set_function_on_click(controls.button_show_word,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& page = state->pages.practice_drawing;
					flip_visibility(page.embedded_show_word_reduced);
					if (IsWindowVisible(page.embedded_show_word_disambiguation))ShowWindow(page.embedded_show_word_disambiguation, SW_HIDE);
				}
			);

			controls.button_right = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_right, 500);
			button::set_theme(controls.button_right, &base_btn_theme);//TODO(fran): maybe green bk

			controls.button_wrong = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_wrong, 501);
			button::set_theme(controls.button_wrong, &base_btn_theme);//TODO(fran): maybe red bk

			controls.paint_answer = CreateWindow(paint::wndclass, 0, WS_CHILD | WS_VISIBLE //TODO(fran): rounded?
				, 0, 0, 0, 0, controls.page, 0, 0, 0);
			paint::set_brushes(controls.paint_answer, true, brush_for(learnt_word_elem::kanji), global::colors.ControlBk, brush_for(learnt_word_elem::kanji), global::colors.Img_Disabled);
			paint::set_dimensions(controls.paint_answer, 7);//TODO(fran): find good brush size

			controls.static_correct_answer = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_correct_answer, &kanji_static_theme);

			controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
				0, 0, 0, 0, controls.page, 0, 0, 0);//TODO(fran): must be shown on top of all the other wnds
			embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

			controls.button_show_disambiguation = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			button::set_theme(controls.button_show_disambiguation, &base_btn_theme);
			SendMessage(controls.button_show_disambiguation, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.disambiguation);
			AWTT(controls.button_show_disambiguation, 700);
			button::set_user_extra(controls.button_show_disambiguation, state);
			button::set_function_on_click(controls.button_show_disambiguation,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					auto& page = state->pages.practice_drawing;
					flip_visibility(page.embedded_show_word_disambiguation);
					if (IsWindowVisible(page.embedded_show_word_reduced))ShowWindow(page.embedded_show_word_reduced, SW_HIDE);
				}
			);

			controls.embedded_show_word_disambiguation = CreateWindow(embedded::show_word_disambiguation::wndclass, NULL, WS_CHILD | embedded::show_word_disambiguation::style::roundrect,
				0, 0, 0, 0, controls.page, 0, 0, 0);
			embedded::show_word_disambiguation::set_theme(controls.embedded_show_word_disambiguation, &eswd_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Review practice----------------------:
		{
			auto& controls = state->pages.review_practice;

			controls.page = create_page(state, base_page_theme);

			controls.static_review = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_review, &base_static_theme);
			AWT(controls.static_review, 450);

			controls.gridview_practices = CreateWindowW(gridview::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			//#define TEST_GRIDVIEW
#ifndef TEST_GRIDVIEW
			gridview::set_brushes(controls.gridview_practices, TRUE, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);//TODO(fran): add border brushes
#else
			gridview::set_brushes(controls.gridview_practices, TRUE, global::colors.CaptionBk, 0, global::colors.CaptionBk_Inactive, 0);
#endif
			gridview::set_user_extra(controls.gridview_practices, state);
			gridview::set_function_render_element(controls.gridview_practices, gridview_practices_renderfunc);
			gridview::set_function_on_click_element(controls.gridview_practices, 
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					void* data = element;
					ProcState::practice_header* header = (decltype(header))data;
					ProcState::page new_review_page;
					switch (header->type) {
						case decltype(header->type)::writing: new_review_page = decltype	(new_review_page)::review_practice_writing; break;
						case decltype(header->type)::multiplechoice: new_review_page = decltype (new_review_page)::review_practice_multiplechoice; break;
						case decltype(header->type)::drawing: new_review_page = decltype	(new_review_page)::review_practice_drawing; break;
						default: Assert(0);
					}
					preload_page(state, new_review_page, element/*pagedata*/);
					store_previous_page(state, state->current_page);
					set_current_page(state, new_review_page);
				}
			);

			controls.button_continue = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_continue, 451);
			button::set_theme(controls.button_continue, &base_btn_theme);
			button::set_user_extra(controls.button_continue, state);
			button::set_function_on_click(controls.button_continue, 
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::landing);//TODO(fran): this isnt best, it'd be nice if we went back to the practice page but from here it'd require a goto_previous_page and the fact that we know prev page is practice and that we should preload cause it's values have changed
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Wordbook----------------------:
		{
			auto& controls = state->pages.wordbook;

			controls.page = create_page(state, base_page_theme);

			auto page = controls.page;

			controls.button_all_words = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			AWT(controls.button_all_words, 800);
			button::set_theme(controls.button_all_words, &base_btn_theme);
			button::set_user_extra(controls.button_all_words, state);
			button::set_function_on_click(controls.button_all_words,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::wordbook_all);
				}
			);

			for (auto& listbox : controls.listbox_last_days_words) {
				listbox = CreateWindowW(listbox::wndclass, 0, WS_CHILD
					, 0, 0, 0, 0, page, 0, NULL, NULL);
				listbox::set_function_render(listbox, listbox_recents_func_render);
				listbox::set_user_extra(listbox, state);
				listbox::set_function_on_click(listbox, [](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					learnt_word16* txt = (decltype(txt))element;

					stored_word16_res res = get_stored_word(state->settings->db, *txt/*TODO(fran): make sure this isnt a copy*/);  defer{ if (res.found) free_stored_word(res.word); };
					if (res.found) {
						preload_page(state, ProcState::page::show_word, &res.word);
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::show_word);
					}
					//TODO(fran): else {notify user of error finding the word}, we need to get good error info from the db functions
					}
				);
			}

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Wordbook All----------------------:
		{
			auto& controls = state->pages.wordbook_all;

			controls.page = create_page(state, base_page_theme);

			auto page = controls.page;

			//controls.static_orderby = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_LEFT | WS_VISIBLE
			//	, 0, 0, 0, 0, page, 0, NULL, NULL);
			//AWT(controls.static_orderby, 900);
			//static_oneline::set_theme(controls.static_orderby, &base_static_theme);

			//controls.static_filterby = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_LEFT | WS_VISIBLE
			//	, 0, 0, 0, 0, page, 0, NULL, NULL);
			//AWT(controls.static_filterby, 901);
			//static_oneline::set_theme(controls.static_filterby, &base_static_theme);

			//TODO(fran): custom rendering, they shouldnt have a border
			controls.combo_orderby = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			word_order_setup_combobox(controls.combo_orderby);
			SetWindowSubclass(controls.combo_orderby, ComboProc, 0, 0);
			SendMessage(controls.combo_orderby, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.combo_orderby, 900);

			controls.combo_filterby = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, controls.page, 0, NULL, NULL);
			word_filter_setup_combobox(controls.combo_filterby);
			SetWindowSubclass(controls.combo_filterby, ComboProc, 0, 0);
			SendMessage(controls.combo_filterby, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.combo_filterby, 901);

			controls.listbox_words = CreateWindowW(listbox::wndclass, 0, WS_CHILD | WS_VISIBLE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_function_render(controls.listbox_words, listbox_recents_func_render);
			listbox::set_user_extra(controls.listbox_words, state);
			listbox::set_function_on_click(controls.listbox_words, [](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				learnt_word16* txt = (decltype(txt))element;

				stored_word16_res res = get_stored_word(state->settings->db, *txt/*TODO(fran): make sure this isnt a copy*/);  defer{ if (res.found) free_stored_word(res.word); };
				if (res.found) {
					preload_page(state, ProcState::page::show_word, &res.word);
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::show_word);
				}
				//TODO(fran): else {notify user of error finding the word}, we need to get good error info from the db functions
				}
			);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

	}

	void resize_controls(ProcState* state, ProcState::page page) {
		static int rez = 0; printf("Resize calls: %d\n", ++rez);
		RECT r; GetClientRect(state->wnd, &r);

		const int w = RECTWIDTH(r);
		const int wnd_h = (int)((f32)avg_str_dim(global::fonts.General, 1).cy * 1.5f);
		const int half_wnd_h = wnd_h / 2;

		rect_i32 navbar;
		{//navbar
			navbar.left = r.left;
			navbar.top = r.top;
			navbar.w = w;
			navbar.h = wnd_h + 5;
			MyMoveWindow(state->pages.navbar, navbar, FALSE);
		}

		const int h = RECTHEIGHT(r) - navbar.h; //correct height to subtract navbar
		const int half_w = w / 2;
		const int w_pad = (int)((f32)w * .05f);//TODO(fran): hard limit for max padding
		const int h_pad = (int)((f32)h * .05f);
		const int max_w = w - w_pad * 2;

		rect_i32 sidebar;
		{//sidebar
			RECT correctsidebar; GetWindowRect(state->pages.sidebar, &correctsidebar); MapWindowRect(0, GetParent(state->pages.sidebar), &correctsidebar);
			sidebar.left = correctsidebar.left;
			sidebar.top = navbar.bottom();
			sidebar.w = avg_str_dim(global::fonts.General, 20).cx;
			sidebar.h = h;
			MyMoveWindow(state->pages.sidebar, sidebar, FALSE);
		}

		rect_i32 page_space;
		{//page
			page_space.left = r.left;
			page_space.top = navbar.bottom();
			page_space.w = w;
			page_space.h = h;
			MyMoveWindow(state->pages.page_space, page_space, FALSE);
		}

		//TODO(fran): this is good but not perfect, inconsistencies occur when resizing, mainly with the starting position of the page
			//Demonstration: scroll the page down a bit, and then resize the window making it at least half small, you'll see that the top elements of the page get cropped since the page's starting point is now past the beginning of the page_space
			//Solution: I think this problem should solve itself once we clamp scrolling, since the page shouldnt actually be allowed to scroll in the way we do here
		#define べんきょう_page_scroll(used_h) \
			{ \
				RECT page_r; GetWindowRect(controls.page, &page_r);  MapWindowPoints(0, GetParent(controls.page),	(POINT*)  &page_r, 2); \
				rect_i32 page; \
				page.left = 0; \
				page.top = page_r.top; \
				page.w = w; \
				page.h = maximum(page_space.h, used_h); \
				MyMoveWindow(controls.page, page, FALSE); \
			}

		switch (page) {
		case ProcState::page::landing:
		{
			auto& controls = state->pages.landing;

			int start_y = 0/*(i32)state->scroll*/;

			rect_i32 button_recents;
			button_recents.y = start_y;
			button_recents.h = wnd_h;
			button_recents.w = min(max_w, avg_str_dim(GetWindowFont(controls.button_recents), 40).cx);
			button_recents.x = (w - button_recents.w) / 2;

			rect_i32 listbox_recents;
			listbox_recents.y = button_recents.bottom();
			listbox_recents.h = state->pagestate.landing.hide_recents ? 0 : wnd_h * (int)listbox::get_element_cnt(controls.listbox_recents);
			listbox_recents.w = button_recents.w;
			listbox_recents.x = (w - listbox_recents.w) / 2;

			int grid_h = wnd_h * 4;
			int grid_w = grid_h * 16 / 9;
			auto grid = create_grid_2x2(grid_h, grid_w, listbox_recents.bottom() + h_pad, w_pad / 2, h_pad / 2, max_w, w);

			rect_i32 cell;

			//First cell
			cell = grid[0][0];
			rect_i32 static_word_cnt_title;
			static_word_cnt_title.w = cell.w;
			static_word_cnt_title.h = min(wnd_h, cell.h);
			static_word_cnt_title.x = cell.center_x() - static_word_cnt_title.w / 2;
			static_word_cnt_title.y = cell.top;

			//NOTE: the values should use a much bigger font
			rect_i32 static_word_cnt;
			static_word_cnt.w = cell.w;
			static_word_cnt.x = cell.center_x() - static_word_cnt.w / 2;
			static_word_cnt.h = distance(cell.bottom(), static_word_cnt_title.bottom());
			static_word_cnt.y = cell.bottom() - static_word_cnt.h;

			//Second cell
			cell = grid[0][1];
			rect_i32 static_practice_cnt_title;
			static_practice_cnt_title.w = cell.w;
			static_practice_cnt_title.h = min(wnd_h, cell.h);
			static_practice_cnt_title.x = cell.center_x() - static_practice_cnt_title.w / 2;
			static_practice_cnt_title.y = cell.top;

			rect_i32 static_practice_cnt;
			static_practice_cnt.w = cell.w;
			static_practice_cnt.x = cell.center_x() - static_practice_cnt.w / 2;
			static_practice_cnt.h = distance(cell.bottom(), static_practice_cnt_title.bottom());
			static_practice_cnt.y = cell.bottom() - static_practice_cnt.h;

			//3rd cell
			cell = grid[1][0];
			rect_i32 static_accuracy_title;
			static_accuracy_title.w = cell.w;
			static_accuracy_title.h = min(wnd_h, cell.h);
			static_accuracy_title.x = cell.center_x() - static_accuracy_title.w / 2;
			static_accuracy_title.y = cell.top;

			rect_i32 score_accuracy;
			score_accuracy.h = min(cell.w, distance(cell.bottom(), static_accuracy_title.bottom()));
			score_accuracy.w = score_accuracy.h;
			score_accuracy.x = cell.center_x() - score_accuracy.w / 2;
			score_accuracy.y = cell.bottom() - score_accuracy.h;

			//4th cell
			cell = grid[1][1];
			rect_i32 static_accuracy_timeline_title;
			static_accuracy_timeline_title.w = cell.w;
			static_accuracy_timeline_title.h = min(wnd_h, cell.h);
			static_accuracy_timeline_title.x = cell.center_x() - static_accuracy_timeline_title.w / 2;
			static_accuracy_timeline_title.y = cell.top;

			rect_i32 graph_accuracy_timeline;
			graph_accuracy_timeline.h = min(cell.w, distance(cell.bottom(), static_accuracy_timeline_title.bottom()));
			graph_accuracy_timeline.w = min(graph_accuracy_timeline.h * 16 / 9, cell.w);
			graph_accuracy_timeline.x = cell.center_x() - graph_accuracy_timeline.w / 2;
			graph_accuracy_timeline.y = cell.bottom() - graph_accuracy_timeline.h;

			rect_i32 bottom_most_control = graph_accuracy_timeline;

			int used_h = distance(start_y,bottom_most_control.bottom());// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			//TODO(fran): correct y_offset: if (used_h > h) dont try centering instead (maybe) apply one h_pad

			べんきょう_page_scroll(used_h);

			MyMoveWindow_offset(controls.button_recents, button_recents, FALSE);
			MyMoveWindow_offset(controls.listbox_recents, listbox_recents, FALSE);
			listbox::set_dimensions(controls.listbox_recents, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			MyMoveWindow_offset(controls.static_word_cnt_title, static_word_cnt_title, FALSE);
			MyMoveWindow_offset(controls.static_word_cnt, static_word_cnt, FALSE);
			MyMoveWindow_offset(controls.static_practice_cnt_title, static_practice_cnt_title, FALSE);
			MyMoveWindow_offset(controls.static_practice_cnt, static_practice_cnt, FALSE);
			MyMoveWindow_offset(controls.static_accuracy_title, static_accuracy_title, FALSE);
			MyMoveWindow_offset(controls.score_accuracy, score_accuracy, FALSE);
			MyMoveWindow_offset(controls.static_accuracy_timeline_title, static_accuracy_timeline_title, FALSE);
			MyMoveWindow_offset(controls.graph_accuracy_timeline, graph_accuracy_timeline, FALSE);

#if 0
			//MoveWindow(controls.candy, 0, 0, min(max_w, avg_str_dim(GetWindowFont(controls.button_recents), 10).cx), h, FALSE);//place one on the left (and one on the right of the page)
			MoveWindow(controls.candy, 0, 0, w, h, FALSE);//cover the entire page
#endif
			

		} break;
		case ProcState::page::new_word:
		{
			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned
			auto& controls = state->pages.new_word;

			HFONT font = GetWindowFont(controls.edit_hiragana);
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			//IMPORTANT: fran: language feature request, scope unnamed variables in some way (maybe giving names to a scope so you can tell it in which one to live) so we can do the following:
			//	jp_sizer{ {&ssizer(edit_hiragana),wnd_h}, {&ssizer(edit_kanji),wnd_h} };
			//The only way I know to be able to do this is to allocate the variables:
			//	jp_sizer{ {new ssizer(edit_hiragana),wnd_h}, { new ssizer(edit_kanji),wnd_h} };
			//why do we have to pay for memory allocation when it's completely unnecesary and simply a sintax limitation, you can simply declare everything beforehand:
			//	ssizer edit_hiragana{ controls.edit_hiragana };
			//	ssizer edit_kanji{ controls.edit_kanji };
			//	vsizer jp_sizer{ {&edit_hiragana,wnd_h}, {&edit_kanji,wnd_h} };
			//but now you need to give everything a name and your code becomes more bloated and confusing
			//it'd much rather do:
			//	jp_sizer{ {&(scope new_word)ssizer(edit_hiragana),wnd_h}, {&(scope new_word)ssizer(edit_kanji),wnd_h} };

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer edit_hiragana{ controls.edit_hiragana };
			ssizer edit_kanji{ controls.edit_kanji };
			vsizer jp_sizer{
				{&edit_hiragana,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_kanji,wnd_h} };

			ssizer combo_lexical_category{ controls.combo_lexical_category };
			hsizer lexical_category{ {&combo_lexical_category,avg_str_dim(font, 10).cx} };//TODO(fran): request desired size
			ssizer edit_meaning{ controls.edit_meaning };
			ssizer edit_mnemonic{ controls.edit_mnemonic };
			ssizer edit_notes{ controls.edit_notes };
			ssizer edit_example_sentence{ controls.edit_example_sentence };
			ssizer btn_save{ controls.button_save };
			ssizer static_notify{ controls.static_notify };
			hrsizer save{ {&btn_save,avg_str_dim(font, 10).cx}, {&static_notify,layout_bounds.cx / 2} };
			vsizer meaning_sizer{
				{&lexical_category,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_meaning,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_mnemonic,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_example_sentence,wnd_h}, //TODO(fran): first 'example sentence' or 'notes'?
				{&lvpad,half_wnd_h},
				{&edit_notes,wnd_h},
				{&lvpad,half_wnd_h},
				{&save,wnd_h} };


			hsizer layout{
				{&jp_sizer,(int)(.4f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{&meaning_sizer,(int)(.55f * (f32)layout_bounds.cx)} };

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

		} break;
		case ProcState::page::practice:
		{
			auto& controls = state->pages.practice;

			listbox::set_dimensions(controls.listbox_words_practiced, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			HFONT font = GetWindowFont(controls.button_start);
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};

			ssizer button_words_practiced{ controls.button_words_practiced};
			ssizer listbox_words_practiced{ controls.listbox_words_practiced};
			vsizer practiced_column{
				{&button_words_practiced,wnd_h},
				{&listbox_words_practiced, wnd_h * (int)listbox::get_element_cnt(controls.listbox_words_practiced)}, };

			ssizer button_start{ controls.button_start };
			hcsizer start{ {&button_start,avg_str_dim(font, 10).cx} };
			vsizer practice_column{
				{&start,wnd_h}, };

			hsizer layout{
				{&practiced_column,(int)(.4f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{&practice_column,(int)(.55f * (f32)layout_bounds.cx)} };

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

		} break;
		case ProcState::page::practice_writing:
		{
			auto& controls = state->pages.practice_writing;

			int bigwnd_h = wnd_h * 4;
			int smallwnd_h = (i32)(wnd_h * .8f);

			HFONT font = GetWindowFont(controls.edit_answer);
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer static_test_word{ controls.static_test_word };
			ssizer _edit_answer{ controls.edit_answer };
			hcsizer edit_answer{ {&_edit_answer,min(max_w, avg_str_dim(GetWindowFont(controls.edit_answer), 20).cx)} };

			ssizer button_disambiguation{ controls.button_show_disambiguation };
			ssizer button_show_word{ controls.button_show_word };
			hcsizer helpers{
				{&button_disambiguation, smallwnd_h * 16 / 9},
				{&lhpad,3},
				{&button_show_word, smallwnd_h * 16 / 9},
			};

			vsizer practice_column{
				{&static_test_word,bigwnd_h},
				{&edit_answer, wnd_h},
				{&lvpad, 3},
				{&helpers,smallwnd_h} };

			hsizer layout{
				{&practice_column,layout_bounds.cx} };

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

			//TODO(fran): resizing for embedded controls via ssizer,...
			rect_i32 button_next;//child inside edit_answer
			RECT _edit_answer_rc;  GetClientRect(controls.edit_answer, &_edit_answer_rc); 
			auto edit_answer_rc = to_rect_i32(_edit_answer_rc);
			button_next.y = 1; //the button is inside the edit box (and past the border) //TODO(fran): we should ask the parent for its border size
			button_next.h = edit_answer_rc.h - 2;
			button_next.w = min(button_next.h, max(0, edit_answer_rc.w - 4/*avoid covering rounded borders*/));
			button_next.x = edit_answer_rc.w - button_next.w - 2;//TODO(fran): if the style of the edit box parent is  ES_ROUNDRECT we gotta subtract one more, in this case we went from -1 to -2
			MyMoveWindow(controls.button_next, button_next, FALSE);

			rect_i32 embedded_show_word_reduced;
			RECT _button_show_word_rc;  GetWindowRect(controls.button_show_word, &_button_show_word_rc); MapWindowRect(0, controls.page, &_button_show_word_rc);
			auto button_show_word_rc = to_rect_i32(_button_show_word_rc);
			embedded_show_word_reduced.w = layout_bounds.cx;
			embedded_show_word_reduced.h = wnd_h * 3;
			embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
			embedded_show_word_reduced.y = button_show_word_rc.bottom() + 3;

			MyMoveWindow(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);

			rect_i32 embedded_show_word_disambiguation = embedded_show_word_reduced;

			MyMoveWindow(controls.embedded_show_word_disambiguation, embedded_show_word_disambiguation, FALSE);

		} break;
		case ProcState::page::practice_multiplechoice:
		{
			auto& controls = state->pages.practice_multiplechoice;

			int start_y = 0;
			int bigwnd_h = wnd_h * 4;

			HFONT font = GetWindowFont(controls.multibutton_choices);
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer static_question{ controls.static_question };
			ssizer multibutton_choices{ controls.multibutton_choices };

			ssizer button_show_disambiguation{ controls.button_show_disambiguation };
			ssizer button_show_word{ controls.button_show_word };
			ssizer button_next{ controls.button_next };
			hcsizer control_buttons{
				{&button_show_disambiguation, wnd_h * 16 / 9}, 
				{&lhpad,3},
				{&button_show_word, wnd_h * 16 / 9},
				{&lhpad,3},
				{&button_next, wnd_h} };

			vsizer layout{
				{&static_question,bigwnd_h},
				{&lvpad, h_pad},
				{&multibutton_choices,bigwnd_h},
				{&lvpad, h_pad},
				{&control_buttons,wnd_h} };

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

			rect_i32 embedded_show_word_reduced;
			RECT _button_show_word_rc;  GetWindowRect(controls.button_show_word, &_button_show_word_rc); MapWindowRect(0, controls.page, &_button_show_word_rc);
			auto button_show_word_rc = to_rect_i32(_button_show_word_rc);
			embedded_show_word_reduced.w = layout_bounds.cx;
			embedded_show_word_reduced.h = wnd_h * 3;
			embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
			embedded_show_word_reduced.y = button_show_word_rc.bottom() + 3;

			MyMoveWindow(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);

			rect_i32 embedded_show_word_disambiguation = embedded_show_word_reduced;

			MyMoveWindow(controls.embedded_show_word_disambiguation, embedded_show_word_disambiguation, FALSE);


			multibutton::Theme multibutton_choices_theme;
			multibutton_choices_theme.dimensions.btn = { avg_str_dim((HFONT)SendMessage(controls.multibutton_choices, WM_GETFONT, 0, 0), 15).cx,wnd_h };
			multibutton_choices_theme.dimensions.inbetween_pad = { 3,3 };
			multibutton::set_theme(controls.multibutton_choices, &multibutton_choices_theme);

		} break;
		case ProcState::page::practice_drawing:
		{
			auto& controls = state->pages.practice_drawing;

			int bigwnd_h = wnd_h * 4;
			int mediumwnd_h = wnd_h * 3;

			HFONT font = GetWindowFont(controls.button_wrong);
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer static_question{ controls.static_question };
			ssizer paint_answer{ controls.paint_answer };

			ssizer button_show_disambiguation{ controls.button_show_disambiguation};
			ssizer button_show_word{ controls.button_show_word };
			ssizer button_next{ controls.button_next };
			hcsizer control_buttons{ 
				{&button_show_disambiguation, wnd_h * 16 / 9},
				{&lhpad,3},
				{&button_show_word, wnd_h * 16 / 9},
				{&lhpad,3},
				{&button_next, wnd_h} };

			ssizer static_correct_answer{ controls.static_correct_answer };

			ssizer button_wrong{ controls.button_wrong };
			ssizer button_right{ controls.button_right };
			hcsizer response_buttons{
				//TODO(fran): better check for the actual char cnt
				{&button_wrong, min(max_w / 2, avg_str_dim((HFONT)SendMessage(controls.button_wrong, WM_GETFONT, 0, 0), 20).cx)},
				{&lhpad,w_pad},
				{&button_right, min(max_w / 2, avg_str_dim((HFONT)SendMessage(controls.button_right, WM_GETFONT, 0, 0), 20).cx)} };

			vsizer layout{ 
				{&static_question, bigwnd_h}, 
				{&lvpad,h_pad},
				{&paint_answer, bigwnd_h * 2},
				{&lvpad,h_pad},
				{&control_buttons, wnd_h},
				{&lvpad,h_pad},
				{&static_correct_answer,mediumwnd_h },
				{&lvpad,h_pad},
				{&response_buttons, wnd_h}
			};

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);
			

			rect_i32 embedded_show_word_reduced;
			RECT _button_show_word_rc;  GetWindowRect(controls.button_show_word, &_button_show_word_rc); MapWindowRect(0, controls.page, &_button_show_word_rc);
			auto button_show_word_rc = to_rect_i32(_button_show_word_rc);
			embedded_show_word_reduced.w = layout_bounds.cx;
			embedded_show_word_reduced.h = wnd_h * 3;
			embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
			embedded_show_word_reduced.y = button_show_word_rc.bottom() + 3;

			MyMoveWindow(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);

			rect_i32 embedded_show_word_disambiguation = embedded_show_word_reduced;

			MyMoveWindow(controls.embedded_show_word_disambiguation, embedded_show_word_disambiguation, FALSE);

		} break;
		case ProcState::page::review_practice:
		{
			auto& controls = state->pages.review_practice;

			int bigwnd_h = wnd_h * 3;
			int start_y = 0;

			rect_i32 static_review;
			static_review.y = start_y;
			static_review.h = wnd_h * 2;
			static_review.w = max_w;
			static_review.x = (w - static_review.w) / 2;

			rect_i32 gridview_practices;
			gridview_practices.y = static_review.bottom() + h_pad;

			gridview::element_dimensions gridview_practices_dims;
			gridview_practices_dims.border_pad_y = 3;
			gridview_practices_dims.inbetween_pad = { 5,5 };
			gridview_practices_dims.element_dim = { bigwnd_h,bigwnd_h };
			gridview::set_dimensions(controls.gridview_practices, gridview_practices_dims);

			SIZE gridview_practices_wh;
			{
				//TODO(fran): this is the worst code I've written in quite a while, this and the gridview code that was needed need a revision
				const size_t elems_per_row = 5;
				size_t curr_elem_cnt = gridview::get_elem_cnt(controls.gridview_practices);
				i32 full_w = gridview::get_dim_for_elemcnt_elemperrow(controls.gridview_practices, curr_elem_cnt, elems_per_row).cx;

				i32 real_w = min(w - w_pad * 2, full_w);

				i32 max_h = gridview::get_dim_for_elemcnt_elemperrow(controls.gridview_practices, elems_per_row * 4, elems_per_row).cy;//4 rows

				i32 full_h = gridview::get_dim_for_elemcnt_w(controls.gridview_practices, curr_elem_cnt, real_w).cy;

				//SIZE full_dim = gridview::get_dim_for(controls.gridview_practices, row_cnt, elems_per_row);
				gridview_practices_wh = { real_w,min(full_h,max_h) };
			}

			gridview_practices.h = gridview_practices_wh.cy;//TODO(fran): should get smaller if the controls below it cant fit, as small as to only allow 1 row to be visible
			gridview_practices.w = gridview_practices_wh.cx;
			gridview_practices.x = (w - gridview_practices.w) / 2;

			rect_i32 button_continue;
			button_continue.h = wnd_h;
			button_continue.w = 70;
			button_continue.x = (w - button_continue.w) / 2;
			button_continue.y = gridview_practices.bottom() + h_pad;

			rect_i32 bottom_most_control = button_continue;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			べんきょう_page_scroll(used_h);

			MyMoveWindow_offset(controls.static_review, static_review, FALSE);
			MyMoveWindow_offset(controls.gridview_practices, gridview_practices, FALSE);
			MyMoveWindow_offset(controls.button_continue, button_continue, FALSE);

		} break;
		case ProcState::page::show_word:
		{
			auto& controls = state->pages.show_word;

			HFONT font = GetWindowFont(controls.edit_meaning);
			int bigwnd_h = wnd_h * 2;
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer edit_hiragana{ controls.edit_hiragana };
			ssizer edit_kanji{ controls.edit_kanji };
			vsizer jp_sizer{
				{&edit_hiragana,bigwnd_h},
				{&lvpad,half_wnd_h},
				{&edit_kanji,bigwnd_h}, //TODO(fran): text editor needs to adapt to font size changing
			};

			ssizer combo_lexical_category{ controls.combo_lexical_category };
			hsizer lexical_category{ {&combo_lexical_category,avg_str_dim(font, 10).cx} };//TODO(fran): request desired size
			ssizer edit_meaning{ controls.edit_meaning };
			ssizer edit_mnemonic{ controls.edit_mnemonic };
			ssizer edit_notes{ controls.edit_notes };
			ssizer edit_example_sentence{ controls.edit_example_sentence };
			
			vsizer meaning_sizer{
				{&lexical_category,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_meaning,wnd_h},
				{&lvpad,half_wnd_h},
				{&edit_mnemonic,GetWindowDesiredSize(edit_mnemonic.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
				{&lvpad,half_wnd_h},
				{&edit_example_sentence,GetWindowDesiredSize(edit_example_sentence.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
				{&lvpad,half_wnd_h},
				{&edit_notes,GetWindowDesiredSize(edit_notes.wnd, { 0,wnd_h }, {0,wnd_h * 5}).max.cy},
			};

			hsizer word_info{
				{(sizer*)&jp_sizer,(int)(.4f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{(sizer*)&meaning_sizer,(int)(.55f * (f32)layout_bounds.cx)}
			};

			ssizer static_creation_date{ controls.static_creation_date };
			ssizer static_last_practiced_date{ controls.static_last_practiced_date };
			vsizer left_stats{
				{&static_creation_date,wnd_h},
				{&lvpad,half_wnd_h},
				{&static_last_practiced_date,wnd_h},
			};//TODO(fran): vcsizer

			ssizer static_score{ controls.static_score };
			vsizer right_stats{ {&static_score,wnd_h} };

			hsizer word_stats{ //TODO(fran): switch from 2x2 to 1x4 grid depending on width
				{&left_stats,(int)(.45f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{&right_stats,(int)(.45f * (f32)layout_bounds.cx)},
			};

			ssizer button_delete{ controls.button_delete };
			ssizer button_remember{ controls.button_remember };
			ssizer button_modify{ controls.button_modify };

			hrsizer buttons{
				{&button_modify,GetWindowDesiredSize(button_modify.wnd,{200,200},{200,200}).max.cx},
				{&lhpad,half_wnd_h},
				{&button_remember,GetWindowDesiredSize(button_remember.wnd,{200,200},{200,200}).max.cx},
				{&lhpad,half_wnd_h},
				{&button_delete,GetWindowDesiredSize(button_delete.wnd,{200,200},{200,200}).max.cx},
			};//TODO(fran): idk whether I want to reverse the order for hrsizer, so that the first wnd added is leftmost

			vsizer layout{
				{&word_info,word_info.get_bottom({0,0,layout_bounds.cx,h}).y},
				{&lhpad,wnd_h},
				{&word_stats,wnd_h * 3},
				{&lhpad,wnd_h},
				{&buttons,wnd_h},
			};

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.y = (h - layout.get_bottom(layout_rc).y) / 2;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

		} break;
		case ProcState::page::review_practice_writing:
		{
			//TODO(fran): different layout?
			resize_controls(state, ProcState::page::practice_writing);
		} break;
		case ProcState::page::review_practice_multiplechoice:
		{
			//TODO(fran): different layout?
			resize_controls(state, ProcState::page::practice_multiplechoice);
		} break;
		case ProcState::page::review_practice_drawing:
		{
			//TODO(fran): different layout?
			resize_controls(state, ProcState::page::practice_drawing);
		} break;
		case ProcState::page::wordbook:
		{
			auto& controls = state->pages.wordbook;

			for(const auto& listbox : controls.listbox_last_days_words)
				listbox::set_dimensions(listbox, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			HFONT font = GetWindowFont(controls.button_all_words);//TODO(fran): listboxes dont store fonts, change to a different control
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer _button_all_words{ controls.button_all_words};
			hcsizer button_all_words{ {&_button_all_words, GetWindowDesiredSize(_button_all_words.wnd,{200,200},{200,200}).max.cx} };

			ssizer listbox_last_days_words[ARRAYSIZE(controls.listbox_last_days_words)]{ controls.listbox_last_days_words[0],controls.listbox_last_days_words[1],controls.listbox_last_days_words[2],controls.listbox_last_days_words[3] };
			static_assert(ARRAYSIZE(listbox_last_days_words) == 4);//TODO(fran): make parametric somehow
			
			vsizer lists_left{
				{&listbox_last_days_words[0],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[0].wnd)},
				{&lvpad,half_wnd_h},
				{&listbox_last_days_words[2],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[2].wnd)} };

			vsizer lists_right{
				{&listbox_last_days_words[1],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[1].wnd)},
				{&lvpad,half_wnd_h},
				{&listbox_last_days_words[3],wnd_h * (int)listbox::get_element_cnt(listbox_last_days_words[3].wnd)} };

			hsizer lists{
				{&lists_left,(int)(.475f * (f32)layout_bounds.cx)},
				{&lhpad,(int)(.05f * (f32)layout_bounds.cx)},
				{&lists_right,(int)(.475f * (f32)layout_bounds.cx)},
			};

			vsizer layout{
				{&lvpad,wnd_h},
				{&button_all_words, wnd_h},//TODO(fran): standard layout logic tells me the button should go last of all, bottom-most
				{&lvpad,wnd_h},
				{&lists,lists.get_bottom({ 0,0,0,0 }).y},
			};

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.h = layout.get_bottom(layout_rc).y;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

		} break;
		case ProcState::page::wordbook_all:
		{
			auto& controls = state->pages.wordbook_all;

			listbox::set_dimensions(controls.listbox_words, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			HFONT font = GetWindowFont(controls.listbox_words);//TODO(fran): listboxes dont store fonts, change to a different control
			SIZE layout_bounds = avg_str_dim(font, 100);
			layout_bounds.cx = minimum((int)layout_bounds.cx, max_w);

			hpsizer lhpad{};
			vpsizer lvpad{};

			ssizer combo_orderby{ controls.combo_orderby };
			ssizer combo_filterby{ controls.combo_filterby };
			hrsizer filters{ {&combo_orderby,avg_str_dim(font, 20).cx}, {&lhpad,2},{&combo_filterby,avg_str_dim(font, 20).cx} };//TODO(fran): request desired size

			ssizer listbox_words{ controls.listbox_words };
			vsizer layout{
				{&lvpad,wnd_h},
				{&filters,wnd_h},
				{&lvpad,wnd_h},
				{&listbox_words, wnd_h * (int)listbox::get_element_cnt(controls.listbox_words)},
			};

			rect_i32 layout_rc;
			layout_rc.w = layout_bounds.cx;
			layout_rc.y = 0;
			layout_rc.h = h;
			layout_rc.x = (w - layout_rc.w) / 2;
			layout_rc.h = layout.get_bottom(layout_rc).y;

			べんきょう_page_scroll(layout_rc.h);

			layout.resize(layout_rc);

		} break;
		default:Assert(0);
		}
	}
	void resize_controls(ProcState* state) {
		resize_controls(state, state->current_page);
	}
	
	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			decltype(state) state = (decltype(state))calloc(1, sizeof(decltype(*state)));
			Assert(state);
			state->nc_parent = GetParent(hwnd);
			state->wnd = hwnd;
			state->settings = ((べんきょうSettings*)create_nfo->lpCreateParams);
			state->current_page = ProcState::page::landing;
			init_cpp_objects(state);
			set_state(hwnd, state);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			create_controls(state);

			set_current_page(state, ProcState::page::landing);//TODO(fran): page:: would be nicer than ProcState::page::

//#define TEST_SCORE_ANIM
#if defined(TEST_SCORE_ANIM)
			static void (*testp)(HWND, UINT, UINT_PTR, DWORD) = [](HWND wnd, UINT, UINT_PTR id, DWORD) {
				static int c;
				flip_visibility(get_state(wnd)->controls.landingpage.score_accuracy);
				flip_visibility(get_state(wnd)->controls.landingpage.score_accuracy);
				SetTimer(wnd, id, 3000, testp);
			};
			SetTimer(state->wnd, 5555, 0, testp);
#endif


//#define TEST_IME_MODE_SWITCH
#if defined(TEST_IME_MODE_SWITCH)
			//NOTE: there seems to be no easy way of setting IME to hiragana by default, the windows defaults for a new process for japanese are fullwidth alphanumeric(aka _not_ japanese)(contrary to all other IMEs that do default to using the f*cking language they're supposed to) //TODO(fran): keep trying? (maybe with TSF text services framework)
			SetFocus(state->wnd); //https://docs.microsoft.com/en-us/previous-versions//hh994466(v=vs.85)?redirectedfrom=MSDN
			int sz_elem = GetKeyboardLayoutList(0, 0);
			ptr<HKL>layouts; layouts.alloc(sz_elem); defer{ layouts.free(); };
			int res = GetKeyboardLayoutList(sz_elem, layouts.mem); Assert(res == sz_elem);
			HKL currenthkl = GetKeyboardLayout(0);
			for (const auto& l : layouts) {
				ActivateKeyboardLayout(l, KLF_SETFORPROCESS);
				char layoutname[KL_NAMELENGTH]; GetKeyboardLayoutNameA(layoutname);
				printf("%s\n", HKLtoString(layoutname));
				if (!strcmp("00000411", layoutname)) {
					#if 0
					HIMC imc = ImmGetContext(state->wnd);
					if (imc != NULL) {
						defer{ ImmReleaseContext(state->wnd, imc); };
						DWORD c_mode, s_mode;
						auto res = ImmGetConversionStatus(imc, &c_mode, &s_mode); Assert(res);
						c_mode = IME_CMODE_NATIVE;//japanese with hiragana
						res = ImmSetConversionStatus(imc,c_mode,s_mode); Assert(res);
					}
					#else
					#if 0 //Both this solutions switch from alphanumeric to hiragana, but the change isnt sticky, once we restore the old keyboard layout the IME decides to no longer remember that it was changed to hiragana and when the user switches to jp input it goes back to alphanumeric. The problem seems to be even worse, the change is actually sticky if the user straight away changes the keyboard to jp, _but_ if they first click on an edit box and only then switch to jp then it is set to alphanumeric, therefore it seems like the IME in the application doesnt quite know of the change and remains on a semi default state
					INPUT ip;
					ip.type = INPUT_KEYBOARD;
					ip.ki.wScan = 0; // hardware scan code for key
					ip.ki.time = 0;
					ip.ki.dwExtraInfo = 0;
					ip.ki.wVk = VK_CONTROL; // virtual-key code for the key
					ip.ki.dwFlags = 0; // 0 for key press
					SendInput(1, &ip, sizeof(ip));
					ip.ki.wVk = VK_CAPITAL; // virtual-key code for the key
					SendInput(1, &ip, sizeof(ip));
					ip.ki.wVk = VK_CAPITAL;
					ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
					SendInput(1, &ip, sizeof(ip));

					ip.ki.wVk = VK_CONTROL;
					SendInput(1, &ip, sizeof(ip));
					#else
					HWND hShellTrayWnd = FindWindowEx(NULL, NULL, TEXT("Shell_TrayWnd"), NULL);
					if (hShellTrayWnd) {
						HWND hTrayNotifyWnd = FindWindowEx(hShellTrayWnd, NULL, TEXT("TrayNotifyWnd"), NULL);
						if (hTrayNotifyWnd) {
							HWND hTrayInputIndicator = FindWindowEx(hTrayNotifyWnd, NULL, TEXT("TrayInputIndicatorWClass"), NULL);
							if (hTrayInputIndicator) {
								HWND imebtn = FindWindowEx(hTrayInputIndicator, NULL, TEXT("IMEModeButton"), NULL);
								if (imebtn) {
									SendMessage(imebtn, WM_LBUTTONDOWN, 0, 0);
									SendMessage(imebtn, WM_LBUTTONUP, 0, 0);
								}
							}
						}
					}
					#endif
						
					SetTimer(state->wnd, (UINT_PTR)currenthkl, 1000, [](HWND wnd, UINT, UINT_PTR id, DWORD) {
						KillTimer(wnd, id);
						ActivateKeyboardLayout((HKL)id, KLF_SETFORPROCESS);
						});
					break;
#endif
				}

			}
			//ActivateKeyboardLayout(currenthkl, KLF_SETFORPROCESS);
#endif

			return 0;
		} break;
		case WM_SIZE:
		{
			resize_controls(state);
			return 0;
		} break;
		case WM_CLOSE:
		{
			return 0;
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				save_settings(state);

				clear_practices_vector(state->pagestate.practice_review.practices);
				state->pagestate.practice_review.practices.~vector();
				
				//#free? state->pagestate.practice_writing.practice;

				clear_practices_vector(state->multipagestate.temp_practices);
				state->multipagestate.temp_practices.~vector();
				
				if(state->settings->is_primary_wnd) PostQuitMessage(0);//TODO(fran): this aint gonna be enough if we ever have multiple main windows

				free(state);
				state = nullptr;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_COMMAND:
		{
			HWND child = (HWND)lparam;
			if (child) {//Notifs from our childs
				switch (state->current_page) {
				case ProcState::page::landing:
				case ProcState::page::new_word:
				case ProcState::page::practice:
				case ProcState::page::show_word:
				case ProcState::page::review_practice:
				case ProcState::page::wordbook:
				{
				} break;
				case ProcState::page::wordbook_all:
				{
					auto& page = state->pages.wordbook_all;
					
					if (WORD notif = HIWORD(wparam); (child == page.combo_filterby || child == page.combo_orderby) && notif == CBN_SELENDOK) {
						wordbook_all__update_wordlist(state);
					}
				} break;
				case ProcState::page::practice_writing:
				{
					auto& page = state->pages.practice_writing;
					auto& pagestate = state->pagestate.practice_writing;
					WORD notif = HIWORD(wparam);
					if (child == page.button_next || (child == page.edit_answer && notif == EN_ENTER)) {
						bool already_answered = IsWindowEnabled(page.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
						if (!already_answered) {
							utf16_str user_answer; _get_edit_str(page.edit_answer, user_answer);
							if (user_answer.cnt()) {
								const utf16_str* correct_answer{ 0 };
								const utf16_str* question{ 0 };//TODO(fran); this should already come calculated at this point
								switch (pagestate.practice->practice_type) {
								case decltype(pagestate.practice->practice_type)::hiragana_to_meaning:
								{
									correct_answer = &pagestate.practice->word.attributes.meaning;
									question = &pagestate.practice->word.attributes.hiragana;
								} break;
								case decltype(pagestate.practice->practice_type)::kanji_to_meaning:
								{
									correct_answer = &pagestate.practice->word.attributes.meaning;
									question = &pagestate.practice->word.attributes.kanji;
								} break;
								case decltype(pagestate.practice->practice_type)::kanji_to_hiragana:
								{
									correct_answer = &pagestate.practice->word.attributes.hiragana;
									question = &pagestate.practice->word.attributes.kanji;

								} break;
								case decltype(pagestate.practice->practice_type)::meaning_to_hiragana:
								{
									correct_answer = &pagestate.practice->word.attributes.hiragana;
									question = &pagestate.practice->word.attributes.meaning;
								} break;
								default:Assert(0);
								}

								bool answered_correctly = !StrCmpIW(correct_answer->str, user_answer.str);

								//NOTE: we can also change text color here, if we set it to def text color then we can change the bk without fear of the text not being discernible from the bk
								HBRUSH bk = answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
								HBRUSH bk_mouseover = answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
								HBRUSH bk_push = answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;

								edit_oneline::Theme editoneline_theme;
								editoneline_theme.brushes.foreground.normal = global::colors.ControlTxt;
								editoneline_theme.brushes.bk.normal = bk;
								editoneline_theme.brushes.border.normal = bk;

								edit_oneline::set_theme(page.edit_answer, &editoneline_theme);

								button::Theme btn_theme;
								btn_theme.brushes.bk.normal = bk;
								btn_theme.brushes.bk.mouseover = bk_mouseover;
								btn_theme.brushes.bk.clicked = bk_push;
								btn_theme.brushes.border.normal = bk;
								btn_theme.brushes.border.mouseover = bk_mouseover;
								btn_theme.brushes.border.clicked = bk_push;
								btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
								button::set_theme(page.button_next, &btn_theme);

								//Update word stats
								word_update_last_practiced_date(state->settings->db, pagestate.practice->word);
								word_increment_times_practiced__times_right(state->settings->db, pagestate.practice->word, answered_correctly);

								//Add this practice to the list of current completed ones
								ProcState::practice_writing* p = (decltype(p))malloc(sizeof(*p));
								p->header.type = decltype(p->header.type)::writing;
								p->practice = pagestate.practice;
								pagestate.practice = nullptr;//clear the pointer just in case
								p->user_answer = user_answer;
								p->correct_answer = correct_answer;
								p->answered_correctly = answered_correctly;
								p->question = question;

								state->multipagestate.temp_practices.push_back((ProcState::practice_header*)p);

								//TODO(fran): I think we actually want to show the correct answer right here, so the user can make a direct connection with it and not forget, because of this I'd either make the timer longer or simply wait for the user to click next so they have all the time they want to look at the answer, what we can also do is implement this only when the user fails, on success just wait a moment and follow to the next level

								EnableWindow(page.button_show_word, TRUE);
								if (answered_correctly) next_practice_level(state);
							}
							else {
								free_any_str(user_answer.str);
							}
						}
						else {
							//The user already answered and got it wrong, they checked what was wrong and pressed continue again
							next_practice_level(state,false);
						}
					}
					
				} break;
				case ProcState::page::review_practice_writing:
				{
					auto& page = state->pages.practice_writing;
					if (child == page.button_next) {
						goto_previous_page(state);
					}
				} break;
				case ProcState::page::practice_multiplechoice:
				{
					auto& page = state->pages.practice_multiplechoice;
					auto& pagestate = state->pagestate.practice_multiplechoice;
					bool already_answered = IsWindowEnabled(page.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
					if (child == page.multibutton_choices) {
						//The user has selected a choice
						if (!already_answered) {
							size_t user_answer_idx = wparam;
							bool answered_correctly = pagestate.practice->idx_answer == user_answer_idx;


							HBRUSH bk = answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
							HBRUSH bk_mouseover = answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
							HBRUSH bk_push = answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;
							button::Theme multibtn_btn_theme;
							multibtn_btn_theme.brushes.bk.normal = bk;
							multibtn_btn_theme.brushes.bk.mouseover = bk_mouseover;
							multibtn_btn_theme.brushes.bk.clicked = bk_push;
							multibtn_btn_theme.brushes.border.normal = bk;
							multibtn_btn_theme.brushes.border.mouseover = bk_mouseover;
							multibtn_btn_theme.brushes.border.clicked = bk_push;
							multibtn_btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
							multibutton::set_button_theme(page.multibutton_choices, &multibtn_btn_theme, user_answer_idx);
							//TODO(fran): when the user answers incorrectly we want to visually show the correct answer too, maybe with a lower brightness global::colors.Bk_right_answer or a yellow

							button::Theme btn_theme;
							btn_theme.brushes.bk.normal = bk;
							btn_theme.brushes.bk.mouseover = bk_mouseover;
							btn_theme.brushes.bk.clicked = bk_push;
							btn_theme.brushes.border.normal = bk;
							btn_theme.brushes.border.mouseover = bk_mouseover;
							btn_theme.brushes.border.clicked = bk_push;
							btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
							button::set_theme(page.button_next, &btn_theme);

							//Update word stats
							word_update_last_practiced_date(state->settings->db, pagestate.practice->question);
							word_increment_times_practiced__times_right(state->settings->db, pagestate.practice->question, answered_correctly);

							//Add this practice to the list of current completed ones
							ProcState::practice_multiplechoice* p = (decltype(p))malloc(sizeof(*p));
							p->header.type = decltype(p->header.type)::multiplechoice;
							p->practice = pagestate.practice;
							pagestate.practice = nullptr;//clear the pointer just in case
							p->answered_correctly = answered_correctly;
							p->user_answer_idx = user_answer_idx;

							state->multipagestate.temp_practices.push_back((ProcState::practice_header*)p);

							EnableWindow(page.button_show_word, TRUE);
							if (answered_correctly) next_practice_level(state);
						}
					}
					else if (child == page.button_next) {
						if (already_answered) {
							next_practice_level(state,false);
						}
					}
					/*else if (child == page.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}*/
					else
					{
						printf("FIX ERROR\n");
						//NOTE: we're getting an EN_KILLFOCUS from the edit control in practice_writing, TODO(fran): do like windows and add a msg to specify which notifications you want to receive from a specific control
					}
				} break;
				case ProcState::page::review_practice_multiplechoice:
				{
					auto& page = state->pages.practice_multiplechoice;
					/*if (child == page.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}*/
					/*else*/ if (child == page.button_next) {
						goto_previous_page(state);
					}
				} break;
				case ProcState::page::practice_drawing:
				{
					auto& page = state->pages.practice_drawing;
					auto& pagestate = state->pagestate.practice_drawing;
					bool already_answered = IsWindowEnabled(page.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
					bool can_answer = paint::get_stroke_count(page.paint_answer);

					if (child == page.paint_answer) {//Notifies when a change finished on the canvas (eg the user drew a complete stroke)
						EnableWindow(page.button_next, can_answer);
					}
					else if (child == page.button_next) {
						if (already_answered) {
							next_practice_level(state,false);
						}
						else {
							EnableWindow(page.paint_answer, FALSE);
							ShowWindow(page.static_correct_answer, SW_SHOW);
							ShowWindow(page.button_wrong, SW_SHOW);
							ShowWindow(page.button_right, SW_SHOW);
						}
					}
					else if (child == page.button_right || child == page.button_wrong) {
						if (!already_answered) {
							bool answered_correctly = child == page.button_right;

							HBRUSH bk = answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
							HBRUSH bk_mouseover = answered_correctly ? global::colors.BkMouseover_right_answer : global::colors.BkMouseover_wrong_answer;
							HBRUSH bk_push = answered_correctly ? global::colors.BkPush_right_answer : global::colors.BkPush_wrong_answer;
							button::Theme btn_theme;
							btn_theme.brushes.bk.normal = bk;
							btn_theme.brushes.bk.mouseover = bk_mouseover;
							btn_theme.brushes.bk.clicked = bk_push;
							btn_theme.brushes.border.normal = bk;
							btn_theme.brushes.border.mouseover = bk_mouseover;
							btn_theme.brushes.border.clicked = bk_push;
							btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
							button::set_theme(page.button_next, &btn_theme);

							//Update word stats
							word_update_last_practiced_date(state->settings->db, pagestate.practice->question);
							word_increment_times_practiced__times_right(state->settings->db, pagestate.practice->question, answered_correctly);

							//Add this practice to the list of current completed ones
							ProcState::practice_drawing* p = (decltype(p))malloc(sizeof(*p));
							p->header.type = decltype(p->header.type)::drawing;
							p->practice = pagestate.practice;
							pagestate.practice = nullptr;//clear the pointer just in case
							p->answered_correctly = answered_correctly;
							//Get what the user drew
							{
								auto canvas = paint::get_canvas(page.paint_answer);//TODO(fran): canvas.bmp could be selected into the paint dc, we need a paint::copy_canvas(RECT) function
								BITMAP bmnfo; GetObject(canvas.bmp, sizeof(bmnfo), &bmnfo);
								runtime_assert(bmnfo.bmBitsPixel == 32, L"Invalid Bitmap Format, bpp != 32, what system is this?");
								InflateRect(&canvas.rc_used, 20, 20);
								canvas.rc_used.left = clamp(1, canvas.rc_used.left, canvas.dim.cx - 1);
								canvas.rc_used.right = clamp(1, canvas.rc_used.right, canvas.dim.cx - 1);
								canvas.rc_used.top = clamp(1, canvas.rc_used.top, canvas.dim.cy - 1);
								canvas.rc_used.bottom = clamp(1, canvas.rc_used.bottom, canvas.dim.cy - 1);

								//Crop the image
								i32 cropped_w = RECTW(canvas.rc_used);
								i32 cropped_h = RECTH(canvas.rc_used);
								i32 cropped_Bpp = 4;
								HBITMAP cropbmp;
								{
									HDC _mydc = GetDC(state->wnd);
									HDC fulldc = CreateCompatibleDC(_mydc); defer{ DeleteDC(fulldc); };
									HDC cropdc = CreateCompatibleDC(_mydc); defer{ DeleteDC(cropdc); };
									cropbmp = CreateCompatibleBitmap(_mydc, cropped_w, cropped_h);
									ReleaseDC(state->wnd, _mydc);
									HBITMAP oldcrop = SelectBitmap(cropdc, cropbmp); defer{ SelectBitmap(cropdc, oldcrop); };
									HBITMAP oldfull = SelectBitmap(fulldc, canvas.bmp); defer{ SelectBitmap(fulldc, oldfull); };
									BitBlt(cropdc, 0, 0, cropped_w, cropped_h, fulldc, canvas.rc_used.left, canvas.rc_used.top, SRCCOPY);
								}
								p->user_answer = cropbmp;
							}
							state->multipagestate.temp_practices.push_back((ProcState::practice_header*)p);

							EnableWindow(page.button_show_word, TRUE);
							if (answered_correctly) next_practice_level(state);
						}
					}
					/*else if (child == page.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}*/
				} break;
				case ProcState::page::review_practice_drawing:
				{
					auto& page = state->pages.practice_drawing;
					/*if (child == page.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}*/
					/*else */if (child == page.button_next) {
						goto_previous_page(state);
					}
				} break;

				default:Assert(0);
				}
			}
			else {
				//switch (LOWORD(wparam)) {//Menu notifications
				//default:
					Assert(0);
				//}
			}
			return 0;
		} break;
		case WM_NCCALCSIZE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SHOWWINDOW:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_GETTEXTLENGTH:
		{
			return 0;
		} break;
		case WM_IME_SETCONTEXT://sent the first time on SetFocus
		{
			return 0; //We dont want IME for the general wnd, the childs can decide
		} break;
		case WM_IME_NOTIFY://for some reason you still get this even when not doing WM_IME_SETCONTEXT
		{
			return 0;
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCPAINT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_ERASEBKGND:
		{
			return 0;//we dont erase the bk here, instead we do everything on wm_paint
		} break;
		case WM_CTLCOLORLISTBOX: //for combobox list //TODO(fran): this has to go
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(global::colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(global::colors.ControlTxt));

			return (INT_PTR)global::colors.ControlBk;
		} break;
		case WM_BACK:
		{
			goto_previous_page(state);
			//TODO(fran): going back is a bit of a problem, eg the user finishes a practice and presses back, then they'll see outdated values, we could live with that saying well yeah you went back, or we could do preloading, for specific pages or have a flag that says for each page if it needs preloading
			return 0;
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(state->wnd, &ps); defer{ EndPaint(state->wnd,&ps); };
			if (state->brushes.bk) {
				//TODO(fran): idk if WS_CLIPCHILDREN | WS_CLIPSIBLINGS automatically clip that regions when I draw or I have to do it manually to avoid flicker
				RECT r; GetClientRect(state->wnd, &r);
				FillRect(dc, &r, state->brushes.bk);
			}
			return 0;
		} break;
		case WM_NCHITTEST:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETCURSOR:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEMOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEACTIVATE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONDOWN:
		{
			SetFocus(0);//remove focus from whoever had it
			return 0;
		} break;
		case WM_LBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PARENTNOTIFY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_KILLFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PRINTCLIENT://For some reason the base combobox (not subclassed) sends this msg, it makes no sense to me why the cb would need my client rendering
		{
			return 0;
		} break;
		case WM_DELETEITEM://Sent by the cb when I delete its items (which I need to do one by one since there's no msg to send it that can do that)
		{
			return TRUE;
		} break;
		case WM_INPUTLANGCHANGE:
		{
			return 1;
		} break;
		case WM_XBUTTONDOWN://Used for forwards and backwards navigation with mouse that have those additional buttons, TODO(fran): this seems like more of a convention, find out if there's some way to know for sure this was the user's intent when they pressed those buttons
		{
			//TODO(fran): this may be better placed in nonclient and all we do here is redirect to there
			switch (HIWORD(wparam)) {
			case XBUTTON1 /*Back button*/: SendMessage(state->wnd, WM_BACK, 0, 0); break;
			case XBUTTON2 /*Forward button*/: /*We dont allow going forward*/ break;
			default: Assert(0);
			}
			return 1;
		} break;
		case WM_XBUTTONUP:
		{
			return 1;
		} break;
		case WM_MOUSEWHEEL:
		{
			return SendMessage(state->nc_parent, msg, wparam, lparam);
		} break;
		case WM_ASK_FOR_RESIZE:
		{
			resize_controls(state); //TODO(fran): only process resizes every X milliseconds (later on we should get rid of this concept entirely and provide resize info to the control directly so it knows when it can send the ask_for_resize msg)
			ask_for_repaint(state); //TODO(fran): controls dont re-render otherwise, even though Im resizing them, maybe because they only change position and not size? also maybe because I dont ask the page to be resized?
			return 0;
		} break;

#if defined(TEST_IME_MODE_SWITCH)
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_IME_REQUEST:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
#endif

		default:
#ifdef _DEBUG
#if defined(TEST_IME_MODE_SWITCH)
			if (msg >= 0xC000 && msg <= 0xFFFF) {//String messages for use by applications  
				TCHAR arr[256];
				int res = GetClipboardFormatName(msg, arr, 256);
				cstr_printf(arr); printf("\n");
				//After Alt+Shift to change the keyboard (and some WM_IMENOTIFY) we receive "MSIMEQueryPosition"
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}
#endif
			Assert(0);
#else 
			return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
		}
		return 0;
	}

	void init_wndclass(HINSTANCE inst) { //INFO: now that we use pre_post_main we cant depend on anything that isnt calculated at compile time
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = Proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(ProcState*);
		wcex.hInstance = inst;
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = 0;
		wcex.lpszMenuName = 0;
		wcex.lpszClassName = wndclass;
		wcex.hIconSm = 0;

		ATOM class_atom = RegisterClassExW(&wcex);
		Assert(class_atom);
	}

	struct pre_post_main {
		pre_post_main() {
			init_wndclass(GetModuleHandleW(NULL));
		}
		~pre_post_main() { //INFO: you can also use the atexit function
			//Classes are de-registered automatically by the os
		}
	};
	static const pre_post_main PREMAIN_POSTMAIN;
}