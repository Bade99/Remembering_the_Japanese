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

#include <string>

//TODO(fran): mascot: have some kind of character that interacts with the user, japanese kawaii style
//TODO(fran): application icon: IDEA: japanese schools seem to usually be represented as "cabildo" like structures with a rectangle and a column coming out the middle, maybe try to retrofit that into an icon
//TODO(fran): application icon: chidori (talk to bren)
//TODO(fran): db: table words: go back to using rowid and add an id member to the learnt_word struct (hiragana words arent unique)
//TODO(fran): db: load the whole db in ram
//TODO(fran): all controls: check for a valid brush and if it's invalid dont draw, that way we give the user the possibility to create transparent controls (gotta check that that works though)
//TODO(fran): all pages: navbar that has a button trigger on the top left of the window, like on youtube chess.com etc etc, we could also add some extra buttons for triggers to other things, eg the buttons on the landing page, we'd have a row of buttons and if you click the typical three dots/lines button the we open a side bar that shows more options, for example to change the language
//TODO(fran): all pages: sanitize input where needed, make sure the user cant execute SQL code
//TODO(fran): all pages & db: change "translation" to "meaning"
//TODO(fran): all pages & db: lexical category should correspond to each 'meaning' not the hiragana since the translations are the ones that can have different lexical categories
//TODO(fran): all pages: it'd be nice to have a scrolling background with jp text going in all directions
//TODO(fran): all pages: hiragana text must always be rendered in the violet color I use in my notes, and the translation in my red, for kanji I dont yet know
//TODO(fran): all pages: can keyboard input be automatically changed to japanese when needed?
//TODO(fran): all pages: I dont know who should be in charge of converting from utf16 to utf8, the backend or front, Im now starting to lean towards the first one, that way we dont have conversion code all over the place, it's centralized in the functions themselves
//TODO(fran): all pages: any button that executes an operation, eg next practice, create word, etc, _must_ be somehow disabled after the first click, otherwise the user can spam clicks and possibly even crash the application
//TODO(fran): all pages: we need an extra control that acts as a page, and is the object that's shown and hidden, this way we can hide and show elements of the page which we currently cannot do since we sw_show each individual one (the control is very easy to implement, it should simply be a passthrough and have no logic at all other than redirect msgs)
//TODO(fran): all pages: color templates for set_brushes so I dont have to rewrite it each time I add a control
//TODO(fran): page landing: make it a 2x2 grid and add a stats button that redirect to a new stats page to put stuff that's in practice page, like "word count" and extra things like a list of last words added
//TODO(fran): page show_word: restructure to have a look more similar to jisho.org's with kanji & hiragana on the left and a list of meanings & lexical categories on the right
//TODO(fran): page new_word: check that no kanji is written to the hiragana box
//TODO(fran): page new_word: add edit box called "Notes" where the user can write anything eg make a clarification
//TODO(fran): page new_word: extra button 'Add More' that adds the current word and restarts the page so the user can write a new one with no time loss
//TODO(fran): page practice: everything animated (including things like word_cnt going from 0 to N)
//TODO(fran): page practice: precalculate the entire array of practice leves from the start (avoids duplicates)
//TODO(fran): page search: add a list of all the learnt words below the searchbox, feed the list via a separate thread
//TODO(fran): BUG: practice writing/...: the edit control has no concept of its childs, therefore situations can arise were it is updated & redrawn but the children arent, which causes the space they occupy to be left blank (thanks to WS_CLIPCHILDREN), the edit control has to tell its childs to redraw after it does
//TODO(fran): page practice_drawing: practice page for kanji via OCR, give the user translation or hiragana and ask them to draw kanji, for now limit it to anki style, the user draws, then presses on 'test' or 'check', sees the correct answer and we provide two buttons 'Right' and 'Wrong', and the user tells us how it went
//TODO(fran): practice_drawing: explain in some subtle way that we want the user to draw kanji? I'd say it's pretty obvious that if we ask you to draw it's not gonna be your language nor hiragana
//TODO(fran): navbar: what if I used the WM_PARENTNOTIFY to allow for my childs to tell me when they are resized, maybe not using parent notify but some way of not having to manually resize the navbar
//TODO(fran)?: new page?: add a place where you can see the words you added grouped by day


//Leftover IDEAs:
//IDEA: page review_practice_...: When opening practices for the review we could add new pages to the enum, this are like virtual pages, using the same controls, but now can have different layout or behaviour simply by adding them to the switch statements, this is very similar to the scene flag idea; the bennefit of virtual pages is that no code needs to be added to already existing things, on the other side the scene flag has to be handled by each page, adding an extra switch(state->current_scene), the annoying thing is on resizing, with a scene flag we remain on the same part of the code and can simply append more controls to the end, change the "bottommost_control" and be correclty resized, we could cheat by putting multiple case statements together, and inside check which virtual page we're in now. The problem for virtual pages is code repetition, yeah the old stuff wont be affected, but we need to copy parts of that old stuff since virtual pages will share most things
	//NOTE: on iterating through elements, we probably can still only iterate over each virtual page's specific elements by adding some extra separator on the struct and checking for their address, subtracting from some base, dividing by the type size and doing for(int i= base; i < cnt; i++) func( all[i] )
//IDEA: page review_practice: alternative idea: cliking an element of the gridview redirects to the show_word page


//INFO: this wnd is divided into two, the UI side and the persistence/db interaction side, the first one operates on utf16 for input and output, and the second one in utf8 for input and utf8 for output (also utf16 but only if you manually handle it)
//INFO: this window is made up of many separate ones, as if they were in different tabs, in the sense only one is shown at any single time, and there isn't any relationship between them
//INFO: the hiragana aid on top of kanji is called furigana
//INFO: Similar applications/Possible Inspiration: anki, memrise, wanikani
//INFO: dates on the db are stored in GMT, REMEMBER to convert them for the UI


#define MyMoveWindow(wnd,xywh,repaint) MoveWindow(wnd, xywh.x, xywh.y, xywh.w, xywh.h, repaint)

#define MyMoveWindow_offset(wnd,xywh,repaint) MoveWindow(wnd, xywh.x, xywh.y + y_offset, xywh.w, xywh.h, repaint)

#define TIMER_next_practice_level 0x5


//NOTE: Since comboboxes return -1 on no selection lexical_category maps perfectly from UI's combobox index to value
enum lexical_category { //this value is stored on the db
	dont_care = -1, //I tend to annotate the different adjectives, but not much else
	noun,
	verb,
	adj_い,
	adj_な,
	adverb,
	conjunction, //and, or, but, ...
	pronoun,
	counter,
};
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
}

//Structures for different practice levels
struct practice_writing_word {
	learnt_word16 word;//TODO(fran): change name to 'question' and add extra param 'answer' that points to an element inside of 'question'
	enum class type {//TODO(fran): the type differentiation is kinda pointless, instead I could bake all the differences into variables, eg to check the right answer have a separate pointer to the needed string inside the learnt_word
		hiragana_to_translation,
		translation_to_hiragana,
		kanji_to_hiragana,
		kanji_to_translation,
		//NOTE: I'd add translate_translation_to_kanji but it's basically translating to hiragana and then letting the IME answer correctly
	}practice_type;
};

HBRUSH brush_for_learnt_word_elem(learnt_word_elem type) {
	HBRUSH res{ 0 };//NOTE: compiler cant know this will always be initialized so I gotta zero it
	switch (type) {
	case decltype(type)::hiragana: {
		res = global::colors.hiragana;
	} break;
	case decltype(type)::kanji: {
		res = global::colors.kanji;
	} break;
	case decltype(type)::meaning: {
		res = global::colors.translation;
	} break;
	default:Assert(0);
	}
	return res;
};

template<typename T>
T str_for_learnt_word_elem(_learnt_word<T>* word, learnt_word_elem type) {//TODO(fran): this is ugly
	T res;
	switch (type) {
	case decltype(type)::hiragana: res = (decltype(res))word->attributes.hiragana; break;
	case decltype(type)::kanji: res = (decltype(res))word->attributes.kanji; break;
	case decltype(type)::meaning: res = (decltype(res))word->attributes.translation; break;
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

namespace embedded {
	namespace show_word_reduced {
		//---------------Creation Example---------------:
		// CreateWindowW(embedded::show_word_reduced::wndclass, 0, WS_CHILD | embedded::show_word_reduced::style::roundrect, 0, 0, 0, 0, parent, 0, 0, 0);
		//---------------API---------------:
		// set_theme()
		// set_word()

		constexpr cstr wndclass[] = L"がいじんの_wndclass_embedded_showwordreduced";
		struct Theme {
			struct {
				brush_group txt, bk, border;
			} brushes;
			struct {
				u32 border_thickness=U32MAX;
			}dimensions;
			HFONT font = 0;
		};
		enum style {
			roundrect = (1<<0),

		};
		struct ProcState {
			HWND wnd;
			HWND parent;
			Theme theme;
			union {
				using type = HWND;
				struct { type hiragana, kanji, meaning, mnemonic, lexical_category; };
				type all[5];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Come update the array to the correct count!"); }
			} controls;
		};
		ProcState* get_state(HWND wnd) { ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0); return state; }
		void set_state(HWND wnd, ProcState* state) { SetWindowLongPtr(wnd, 0, (LONG_PTR)state); }
		void ask_for_repaint(ProcState* state){ InvalidateRect(state->wnd, NULL, TRUE); }
		void update_controls_theme(ProcState* state) {
			for (auto& c : state->controls.all) {
				static_oneline::set_brushes(c, true, state->theme.brushes.txt.normal, state->theme.brushes.bk.normal, state->theme.brushes.border.normal, state->theme.brushes.txt.disabled, state->theme.brushes.bk.disabled, state->theme.brushes.border.disabled);
				static_oneline::set_dimensions(c, state->theme.dimensions.border_thickness);
				SendMessage(c, WM_SETFONT, (WPARAM)state->theme.font, TRUE);
			}
		}
		//Set only what you need, what's not set wont be used
		//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
		void set_theme(HWND wnd, const Theme* t) {
			ProcState* state = get_state(wnd);
			if (state && t) {
				bool repaint = false;
				repaint |= copy_brush_group(&state->theme.brushes.txt, &t->brushes.txt);
				repaint |= copy_brush_group(&state->theme.brushes.bk, &t->brushes.bk);
				repaint |= copy_brush_group(&state->theme.brushes.border, &t->brushes.border);
				
				if (t->dimensions.border_thickness != U32MAX) {
					state->theme.dimensions.border_thickness = t->dimensions.border_thickness; repaint = true;
				}

				if (t->font) { state->theme.font = t->font; repaint = true; }

				if (repaint) { update_controls_theme(state); ask_for_repaint(state); }
			}
		}
		void resize_controls(ProcState* state) {
			RECT r; GetClientRect(state->wnd, &r);
			int w = RECTWIDTH(r);
			int h = RECTHEIGHT(r);
			int w_pad = (int)((float)w * .05f);//TODO(fran): hard limit for max padding
			int h_pad = (int)((float)h * .05f);
			int max_w = w - w_pad * 2;
			int start_y = 0;

			auto& controls = state->controls;

			//Hiragana, kanji and lexical category on the left, meaning, notes, mnemonic on the right
			int cell_w = (max_w-w_pad/2)/2;
			auto grid = create_grid_2x2(30, cell_w, start_y, w_pad / 2, h_pad / 2, max_w, w);
			//TODO(fran): I think 4x1 config is better cause the wnd isnt expected to be very wide

			rect_i32 hiragana = grid[0][0];
			rect_i32 kanji = grid[0][1];
			rect_i32 meaning = grid[1][0];
			rect_i32 mnemonic = grid[1][1];

			//TODO(fran): do I wanna put the lexical_category? it actually feels kinda pointless

			rect_i32 bottom_most_control = mnemonic;

			int used_h = bottom_most_control.bottom();// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.hiragana, hiragana, FALSE);
			MyMoveWindow_offset(controls.kanji, kanji, FALSE);
			MyMoveWindow_offset(controls.meaning, meaning, FALSE);
			MyMoveWindow_offset(controls.mnemonic, mnemonic, FALSE);
		}
		void show_controls(ProcState* state, bool show) {
			for (auto& c : state->controls.all) ShowWindow(c, show ? SW_SHOW : SW_HIDE);
		}
		//NOTE: a copy of the word is kept internally
		void set_word(HWND wnd, learnt_word16* word) {
			ProcState* state = get_state(wnd);
			if (state && word) {
				//TODO(fran): we could resize_controls in case we resize by the lenght of the strings
				SendMessage(state->controls.hiragana, WM_SETTEXT, 0, (LPARAM)word->attributes.hiragana.str);
				SendMessage(state->controls.kanji, WM_SETTEXT, 0, (LPARAM)word->attributes.kanji.str);
				SendMessage(state->controls.meaning, WM_SETTEXT, 0, (LPARAM)word->attributes.translation.str);
				SendMessage(state->controls.lexical_category, WM_SETTEXT, 0, (LPARAM)word->attributes.lexical_category.str);
				SendMessage(state->controls.mnemonic, WM_SETTEXT, 0, (LPARAM)word->attributes.mnemonic.str);
				//TODO(fran): ask_for_repaint(state) ?
			}
		}
		LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			ProcState* state = get_state(hwnd);
			switch (msg) {
				//TODO?(fran): WM_MOUSEWHEEL, we might no be able to fit all the controls
			case WM_NCCREATE: {
				CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;
				ProcState* st = (ProcState*)calloc(1, sizeof(ProcState)); Assert(st);
				set_state(hwnd, st);
				st->wnd = hwnd;
				st->parent = creation_nfo->hwndParent;
				return TRUE; //continue creation
			} break;
			case WM_NCCALCSIZE: {
				if (wparam) {
					NCCALCSIZE_PARAMS* calcsz = (NCCALCSIZE_PARAMS*)lparam;
					return 0;
				} else {
					RECT* client_rc = (RECT*)lparam;//TODO(fran): make client_rc cover the full window area
					return 0;
				}
			} break;
			case WM_CREATE: {
				CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;
				for (auto& c : state->controls.all) {
					c = CreateWindowW(static_oneline::wndclass, 0, WS_CHILD | SS_CENTERIMAGE | SS_LEFT | SO_AUTOFONTSIZE
						, 0, 0, 0, 0, state->wnd, 0, 0, 0);
				}
				show_controls(state, true);

				return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): remove once we know all the things this does
			} break;
			case WM_SETFONT: {
				HFONT font = (HFONT)wparam;
				Theme new_theme; 
				new_theme.font = font;
				set_theme(state->wnd, &new_theme);
				return 0;
			} break;
			case WM_SHOWWINDOW: {
				bool show = wparam;
				//show_controls(state,show); //TODO(fran): if we ever need controls that start hidden
				return DefWindowProc(hwnd, msg, wparam, lparam);
			} break;
			case WM_SIZE: {
				resize_controls(state);
				return DefWindowProc(hwnd, msg, wparam, lparam);
			} break;
			case WM_NCDESTROY: {
				free(state);
				return 0;
			}break;
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rc; GetClientRect(state->wnd, &rc);
				int w = RECTW(rc), h = RECTH(rc);
				u16 radius = (u16)(min(w, h) * .05f);
				//ps.rcPaint
				HDC dc = BeginPaint(state->wnd, &ps);
				bool window_enabled = IsWindowEnabled(state->wnd);
				LONG_PTR style = GetWindowLongPtr(state->wnd, GWL_STYLE);
				HBRUSH bkbr = window_enabled ? state->theme.brushes.bk.normal : state->theme.brushes.bk.disabled;
				HBRUSH borderbr = window_enabled ? state->theme.brushes.border.normal : state->theme.brushes.border.disabled;
				//TODO(fran): maybe fillrect alpha is needed to allow for transparency?
				//Bk
				if (bkbr) {

					if (style & style::roundrect) {
						urender::RoundRectangleFill(dc, bkbr, rc, radius);
					}
					else {
						FillRect(dc, &rc, bkbr);
					}
				}
				//Border
				if (borderbr) {

					if (style & style::roundrect) {
						urender::RoundRectangleBorder(dc, borderbr, rc, radius, (f32)state->theme.dimensions.border_thickness);
					}
					else {
						FillRectBorder(dc, rc, state->theme.dimensions.border_thickness, borderbr, BORDERALL);
					}
				}

				EndPaint(hwnd, &ps);
				return 0;
			} break;
			case WM_MOVE:
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
			case WM_DESTROY:
			case WM_SETCURSOR:
			case WM_MOUSEMOVE:
			{
				return DefWindowProc(hwnd, msg, wparam, lparam);
			} break;
			case WM_NCPAINT:
			case WM_ERASEBKGND:
			case WM_LBUTTONDOWN://TODO(fran): we may want to SetFocus(0);
			case WM_RBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_IME_SETCONTEXT:
			case WM_GETTEXT:
			case WM_GETTEXTLENGTH:
			case WM_PARENTNOTIFY:
			{
				return 0;
			} break;
			case WM_SETTEXT:
			{
				return 1;
			}
			case WM_NCHITTEST: {
				POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Screen coords, relative to upper-left corner
				RECT rw; GetWindowRect(state->wnd, &rw);
				LRESULT hittest = HTNOWHERE;
				if (test_pt_rc(mouse, rw))hittest = HTCLIENT;
				return hittest;
			} break;
			case WM_MOUSEACTIVATE: {
				return MA_ACTIVATE;
			} break;
			default:
#ifdef _DEBUG
				Assert(0);
#else 
				return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
			}
			return 0;
		}
		void init_wndclass(HINSTANCE instance) {
			WNDCLASSEXW cl;
			cl.cbSize = sizeof(cl);
			cl.style = CS_HREDRAW | CS_VREDRAW;
			cl.lpfnWndProc = Proc;
			cl.cbClsExtra = 0;
			cl.cbWndExtra = sizeof(ProcState*);
			cl.hInstance = instance;
			cl.hIcon = NULL;
			cl.hCursor = LoadCursor(nullptr, IDC_ARROW);
			cl.hbrBackground = NULL;
			cl.lpszMenuName = NULL;
			cl.lpszClassName = wndclass;
			cl.hIconSm = NULL;
			ATOM class_atom = RegisterClassExW(&cl);
			runtime_assert(class_atom, (str(L"Failed to initialize class ") + wndclass).c_str());
		}

		struct pre_post_main {
			pre_post_main() { init_wndclass(GetModuleHandleW(NULL)); }
			~pre_post_main() { }
		}static const PREMAIN_POSTMAIN;
	}
}

struct べんきょうSettings {

#define foreach_べんきょうSettings_member(op) \
		op(RECT, rc,200,200,600,800 ) \

	foreach_べんきょうSettings_member(_generate_member);
	sqlite3* db;
	bool is_primary_wnd;//TODO(fran): not sure this should go here instead of ProcState

	_generate_default_struct_serialize(foreach_べんきょうSettings_member);
	_generate_default_struct_deserialize(foreach_べんきょうSettings_member);

};
_add_struct_to_serialization_namespace(べんきょうSettings);

struct べんきょうProcState {
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
		search,
		show_word,

		//-----Virtual Pages-----: (dont have controls of their own, show some other page and steal what they need from it
		review_practice_writing,
		review_practice_multiplechoice,
		review_practice_drawing,
	} current_page;

	struct prev_page_fifo_queue{
		decltype(current_page) pages[10];
		u32 cnt;
	}previous_pages;

	u32 practice_cnt;//counter for current completed stages/levels while on a pratice run, gets set to eg 10 and is decremented by -1 with each completed stage, when practice_cnt == 0  the practice ends

	struct {
		HWND navbar;

		union landingpage_controls {
			using type = HWND;
			struct {
				type button_new;
				type button_practice;
				type button_search;
				//type combo_languages;
			}list; //INFO: unfortunately in order to make 'all' auto-update we need to give a name to this struct
			type all[sizeof(list)/sizeof(type)]; //NOTE: make sure you understand structure padding before implementing this, also this should be re-tested if trying with different compilers or alignment
		} landingpage;

		union new_word_controls {
			using type = HWND;
			struct {
				type edit_hiragana;
				type edit_kanji;
				type edit_translation;
				type combo_lexical_category;
				type edit_mnemonic;//create a story/phrase around the word
				//TODO(fran): here you should be able to add more than one translation
				//TODO(fran): type edit_notes
				type button_save;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} new_word;

		union practice_controls {
			using type = HWND;
			struct {
				type static_word_cnt_title;
				type static_word_cnt;

				type static_practice_cnt_title;
				type static_practice_cnt;

				type static_accuracy_title;
				type score_accuracy;

				type static_accuracy_timeline_title;
				type graph_accuracy_timeline;

				type button_start;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice;

		struct practice_writing_controls {
			using type = HWND;
			union {
				struct {
					type static_test_word;

					type edit_answer;

					type button_next;//TODO(fran): not the best name

					type button_show_word;
				}list;
				type all[sizeof(list) / sizeof(type)];
			};
			type embedded_show_word_reduced;//NOTE: not shown by default
		}practice_writing;

		struct practice_multiplechoice_controls {
			using type = HWND;
			union {
				struct {
					type static_question;

					type multibutton_choices;

					type button_next;//#disabled by default

					type button_show_word;//#disabled by default
				}list;
				type all[sizeof(list) / sizeof(type)];
			};
			type embedded_show_word_reduced;//#hidden by default
		}practice_multiplechoice;

		struct practice_drawing_controls {
			using type = HWND;
			union {
				struct {
					type static_question;

					type paint_answer;

					type button_next;//#disabled by default (gets enabled when the user drew smth)
					type button_show_word;//#disabled

					type static_correct_answer;//#hidden
					//NOTE: working on a good handwriting recognition pipeline so this can be automatically checked (probably google translate style)

					type button_right, button_wrong;//#hidden //the user tells us how they did

				}list;
				type all[sizeof(list) / sizeof(type)];
			};
			type embedded_show_word_reduced;//#hidden by default
		}practice_drawing;

		union search_controls {
			using type = HWND;
			struct {
				type searchbox_search;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} search;

		union review_practice_controls {
			using type = HWND;
			struct {
				type static_review;
				type gridview_practices;
				type button_continue;
			}list;
			type all[sizeof(list) / sizeof(type)];
		}review_practice;

		union show_word_controls {
			using type = HWND;
			struct {
				type static_hiragana;
				type edit_kanji;
				type edit_translation;
				type combo_lexical_category;
				type edit_mnemonic;
				//TODO(fran): here you should be able to add more than one translation

				type static_creation_date;
				type static_last_shown_date;
				type static_score; //eg Score: 4/5 - 80%

				type button_modify;
				type button_delete;
				type button_remember;//the user can request the system to prioritize showing this word on practices (the same as if it was a new word that the user just added)
			}list;
			type all[sizeof(list) / sizeof(type)];
		} show_word;

	}controls; //TODO(fran): should be called pages

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
};

namespace べんきょう {
	constexpr cstr wndclass[] = L"がいじんの_wndclass_べんきょう";

	using ProcState = べんきょうProcState;

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	//NOTE: a null HBRUSH means dont change the current one
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	RECT to_rect(const rect_i32& r) {//TODO(fran): get this out of here
		RECT res;
		res.left = r.left;
		res.top = r.top;
		res.right = r.right();
		res.bottom = r.bottom();
		return res;
	}

	//TODO(fran): there are two things I view as possibly necessary extra params: HWND wnd (of the gridview), void* user_extra
	void gridview_practices_renderfunc(HDC dc, rect_i32 r, void* element) {
		ProcState::practice_header* header = (decltype(header))element;

		//------Render Setup------:
		HBRUSH border_br{ 0 };
		utf16_str txt{ 0 };

		//TODO(fran): we can bake this switch into the header by adding the params answered_correctly and question_str there
		switch (header->type) {
		case decltype(header->type)::writing:
		{
			ProcState::practice_writing* data = (decltype(data))header;
			border_br = data->answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
#if 1 //TODO(fran): idk which is better
			txt = *data->question;
#else
			txt = data->user_answer;//the user will remember better what they wrote rather than what they saw
#endif
		} break;
		case decltype(header->type)::multiplechoice:
		{
			ProcState::practice_multiplechoice* data = (decltype(data))header;
			border_br = data->answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
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
			border_br = data->answered_correctly ? global::colors.Bk_right_answer : global::colors.Bk_wrong_answer;
			txt.str = data->practice->question_str;
			txt.sz = (cstr_len(txt.str) + 1) * sizeof(*txt.str);
		} break;
		default: Assert(0);
		}

		//------Rendering------:
		int w = r.w, h = r.h;
		RECT rc = to_rect(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT

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
	void searchbox_free_elements_func(ptr<void*> elements, void* user_extra){
		//TODO(fran): right now Im allocating the whole array which means I only need to free the very first element, this is probably not the way to go for the future, for example if we wanted to do async search we wouldnt know which elements are the first in the array and therefore the only ones that need freeing
		//for(auto& e : elements) free_any_str(e);

		for(auto e : elements) for(auto& m : ((learnt_word16*)e)->all) free_any_str(m.str);

		//TODO(fran): MEMLEAK, BUG: Im pretty sure I need to free the first element of 'elements' in order for the dynamic array to be freed, but it crashes right now, Im pretty sure there's a bug higher in the chain, and also the array idea is good from a performance standpoint but doesnt look too good for my needs here, there are never gonna be more than say 15 elements so speed isnt really an issue
		if(elements.cnt)free(elements[0]); //memleak solved, for now
		//elements.free(); //NOTE: again, all the elements are allocated continually in memory as an array, so we only need to deallocate the first one and the whole mem section is freed
	}

	ptr<void*> searchbox_retrieve_search_options_func(utf16_str user_input, void* user_extra);

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra);

	void searchbox_func_show_on_editbox(HWND editbox, void* element, void* user_extra) {
		learnt_word16* word = (decltype(word))element;
		ProcState* state = get_state((HWND)user_extra);
		if (state) {
			utf16_str txt;
			switch (state->pagestate.search.search_type) {
			case decltype(state->pagestate.search.search_type)::hiragana: txt = word->attributes.hiragana; break;
			case decltype(state->pagestate.search.search_type)::kanji: txt = word->attributes.kanji; break;
			case decltype(state->pagestate.search.search_type)::meaning: txt = word->attributes.translation; break;
			default: txt = { 0 };/*must zero initialize so compiler doesnt complain*/  Assert(0);
			}
			SendMessage(editbox, WM_SETTEXT_NO_NOTIFY, 0, (LPARAM)txt.str);
		}
	}

	void listbox_search_renderfunc(HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {
		int w = r.w, h = r.h;
		learnt_word16* txt = (decltype(txt))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk;
		if (flags.onSelected)bk_br = global::colors.ControlBkMouseOver;
		if(flags.onClicked) bk_br = global::colors.ControlBkPush;

		RECT bk_rc = to_rect(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		FillRect(dc, &bk_rc, bk_br);

		//Draw text
		i32 third_w = r.w / 3;
		rect_i32 tempr = r; tempr.w = third_w;

		RECT hira_rc = to_rect(tempr);

		tempr.left += tempr.w;

		RECT kanji_rc = to_rect(tempr);

		tempr.left += tempr.w;

		RECT meaning_rc = to_rect(tempr);

		urender::draw_text(dc, hira_rc, txt->attributes.hiragana, global::fonts.General, global::colors.ControlTxt, bk_br, urender::txt_align::left, 3);
		urender::draw_text(dc, kanji_rc, txt->attributes.kanji, global::fonts.General, global::colors.ControlTxt, bk_br, urender::txt_align::left, 3);
		urender::draw_text(dc, meaning_rc, txt->attributes.translation, global::fonts.General, global::colors.ControlTxt, bk_br, urender::txt_align::left, 3);

	}

	void langbox_func_free_elements(ptr<void*> elements, void* user_extra) {
		for (auto& e : elements) { free_any_str(((utf16_str*)e)->str); free(e); }
	}

	void langbox_func_render_listbox_element(HDC dc, rect_i32 r, listbox::renderflags flags, void* element, void* user_extra) {

		int w = r.w, h = r.h;
		utf16_str* txt = (decltype(txt))element;

		//Draw bk
		HBRUSH bk_br = global::colors.ControlBk, txt_br = global::colors.ControlTxt;
		if (flags.onSelected)bk_br = global::colors.ControlBkMouseOver;
		if (flags.onClicked) bk_br = global::colors.ControlBkPush;

		RECT bk_rc = to_rect(r);//TODO(fran): I should be using rect_i32 otherwise I should change the func to use RECT
		FillRect(dc, &bk_rc, bk_br);

		//Draw text
		HFONT font = global::fonts.General;
		RECT txt_rc = to_rect(r);

		urender::draw_text(dc, txt_rc, *txt, font, txt_br, bk_br, urender::txt_align::left, avg_str_dim(font,1).cx);
	}

	void langbox_func_on_selection_accepted(void* element, void* user_extra){
		utf16_str* lang = (decltype(lang))element;
		LANGUAGE_MANAGER::Instance().ChangeLanguage(lang->str);
		//TODO(fran): we gotta readjust the size of the combobox to account for the new element, which in turn means updating the navbar
	}

	void langbox_func_render_combobox(HDC dc, rect_i32 r, combobox::render_flags flags, void* element, void* user_extra) {

		HFONT font = global::fonts.General;
		HBRUSH bk_br, txt_br = global::colors.ControlTxt, border_br = global::colors.Img;
		if (flags.isListboxOpen || flags.onClicked) {
			bk_br = global::colors.ControlBkPush;
		}
		else if (flags.onMouseover) {
			bk_br = global::colors.ControlBkMouseOver;
		}
		else {
			bk_br = global::colors.ControlBk;
		}

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

			RECT txt_rc = toRECT(r);
			txt_rc.left += x_pad;
			txt_rc.right = icon_x;

			DrawTextW(dc, s->str, (int)s->sz_char(), &txt_rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}

	}

	int langbox_func_desired_size(SIZE* min, SIZE* max, HDC dc, void* element, void* user_extra) {

		int char_cnt = 8 + 3 + 3;//chars, icon, spacing
		if (element) {
			utf16_str* s = (decltype(s))element;
			char_cnt = (int)s->sz_char()-1 + 3 + 3;
		}

		SIZE sz = avg_str_dim(global::fonts.General, char_cnt);

		min->cx = minimum((int)min->cx, (int)((float)sz.cx * 1.f));
		min->cy = minimum((int)min->cy, (int)((float)sz.cy * 1.2f));

		*max = *min;
		return 2;
	}

	void add_controls(ProcState* state) {
		DWORD style_button_txt = WS_CHILD | WS_TABSTOP | button::style::roundrect;
		DWORD style_button_bmp = WS_CHILD | WS_TABSTOP | button::style::roundrect | BS_BITMAP;
		button::Theme base_btn_theme;
		base_btn_theme.dimensions.border_thickness = 1;
		base_btn_theme.brushes.bk.normal    = global::colors.ControlBk;
		base_btn_theme.brushes.bk.disabled  = global::colors.ControlBk_Disabled;
		base_btn_theme.brushes.bk.clicked   = global::colors.ControlBkPush;
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
		
		button::Theme navbar_btn_theme = base_btn_theme;
		navbar_btn_theme.brushes.border = navbar_btn_theme.brushes.bk;


		embedded::show_word_reduced::Theme eswr_theme;
		brush_group eswr_bk, eswr_txt, eswr_border;
		eswr_bk.normal = global::colors.ControlBk;
		eswr_txt.normal = global::colors.ControlTxt;
		eswr_border.normal = global::colors.ControlTxt;
		eswr_theme.font = global::fonts.General;
		eswr_theme.dimensions.border_thickness = 1;
		eswr_theme.brushes.bk = eswr_bk;
		eswr_theme.brushes.txt = eswr_txt;
		eswr_theme.brushes.border = eswr_border;

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

		//---------------------Header & Footer----------------------:
		auto& navbar = state->controls.navbar;
		navbar = CreateWindowW(navbar::wndclass, NULL, WS_CHILD | WS_VISIBLE //TODO(fran): WS_CLIPCHILDREN?
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		navbar::Theme nav_theme;
		nav_theme.brushes.bk.normal = global::colors.ControlBk_Disabled;//TODO(fran): darker color than bk
		nav_theme.dimensions.spacing = 3;
		navbar::set_theme(navbar, &nav_theme);

		{//navbar test
			HWND btn1 = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			AWT(btn1, 100);
			button::set_theme(btn1, &navbar_btn_theme);
			navbar::attach(navbar, btn1, navbar::attach_point::left, -1);
			SendMessage(btn1, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			HWND btn2 = CreateWindowW(button::wndclass, NULL, style_button_txt | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			AWT(btn2, 101);
			button::set_theme(btn2, &navbar_btn_theme);
			navbar::attach(navbar, btn2, navbar::attach_point::left, -1);
			SendMessage(btn2, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			edit_oneline::Theme search_editoneline_theme = base_editoneline_theme;
			search_editoneline_theme.brushes.border = search_editoneline_theme.brushes.bk;

			HWND search = CreateWindowW(searchbox::wndclass, NULL, WS_CHILD | WS_TABSTOP | SRB_ROUNDRECT | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			ACC(search, 251);
			searchbox::set_editbox_theme(search, &search_editoneline_theme);
			navbar::attach(navbar, search, navbar::attach_point::center, -1);
			SendMessage(search, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);

			HWND combo_lang = CreateWindowW(combobox::wndclass, NULL, WS_CHILD | WS_VISIBLE
				, 0, 0, 0, 0, navbar, 0, NULL, NULL);
			languages_setup_combobox(combo_lang);
			combobox::set_function_free_elements(combo_lang, langbox_func_free_elements);
			combobox::set_function_render_combobox(combo_lang, langbox_func_render_combobox);
			combobox::set_function_on_selection_accepted(combo_lang, langbox_func_on_selection_accepted);
			combobox::set_function_desired_size_combobox(combo_lang, langbox_func_desired_size);
			combobox::set_function_render_listbox_element(combo_lang, langbox_func_render_listbox_element);
			navbar::attach(navbar, combo_lang, navbar::attach_point::right, -1);
			SendMessage(search, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}


		//---------------------Landing page----------------------:
		{
			auto& controls = state->controls.landingpage;

			controls.list.button_new = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_new, 100);
			button::set_theme(controls.list.button_new, &base_btn_theme);

			controls.list.button_practice = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_practice, 101);
			button::set_theme(controls.list.button_practice, &base_btn_theme);

			controls.list.button_search = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_search, 102);
			button::set_theme(controls.list.button_search, &base_btn_theme);

			/*controls.list.combo_languages = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			languages_setup_combobox(controls.list.combo_languages);
			SetWindowSubclass(controls.list.combo_languages, ComboProc, 0, 0);
			SendMessage(controls.list.combo_languages, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);*/
			//TODO(fran): more subdued colors for the lang combo, also no border, possibly also bold text, and right aligned

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------New word----------------------:
		{
			auto& controls = state->controls.new_word;

			controls.list.edit_hiragana = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_hiragana, &base_editoneline_theme);
			AWDT(controls.list.edit_hiragana, 120);

			controls.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.list.combo_lexical_category);
			SetWindowSubclass(controls.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.list.combo_lexical_category, 123);

			controls.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_kanji, &base_editoneline_theme);
			AWDT(controls.list.edit_kanji, 121);

			controls.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_translation, &base_editoneline_theme);
			AWDT(controls.list.edit_translation, 122);

			controls.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_mnemonic, &base_editoneline_theme);
			AWDT(controls.list.edit_mnemonic, 125);

			controls.list.button_save = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_save, 124);
			button::set_theme(controls.list.button_save, &base_btn_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------Search----------------------:
		{
			auto& controls = state->controls.search;

			controls.list.searchbox_search = CreateWindowW(searchbox::wndclass, NULL, WS_CHILD | WS_TABSTOP | SRB_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			ACC(controls.list.searchbox_search, 251);
			searchbox::set_editbox_theme(controls.list.searchbox_search, &base_editoneline_theme);
			searchbox::set_user_extra(controls.list.searchbox_search, state->wnd);
			searchbox::set_function_free_elements(controls.list.searchbox_search, searchbox_free_elements_func);
			searchbox::set_function_retrieve_search_options(controls.list.searchbox_search, searchbox_retrieve_search_options_func);
			searchbox::set_function_perform_search(controls.list.searchbox_search, searchbox_func_perform_search);
			searchbox::set_function_show_element_on_editbox(controls.list.searchbox_search,searchbox_func_show_on_editbox);
			searchbox::set_function_render_listbox_element(controls.list.searchbox_search, listbox_search_renderfunc);
			searchbox::maintain_placerholder_when_focussed(controls.list.searchbox_search, true);
			edit_oneline::set_IME_wnd(searchbox::get_state(controls.list.searchbox_search)->controls.editbox, true); //HACK: request searchbox for its controls //TODO(fran): as always windows disappoints with very limited configurability, we need to create our own IME window that can be hidden

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
		//---------------------Show word----------------------:
		{
			auto& controls = state->controls.show_word;

			controls.list.static_hiragana = CreateWindowW( static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_hiragana, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.edit_kanji = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_kanji, &base_editoneline_theme);
			AWDT(controls.list.edit_kanji, 121);

			controls.list.edit_translation = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_CENTER | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_translation, &base_editoneline_theme);
			AWDT(controls.list.edit_translation, 122);

			controls.list.combo_lexical_category = CreateWindowW(L"ComboBox", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			lexical_category_setup_combobox(controls.list.combo_lexical_category);
			SetWindowSubclass(controls.list.combo_lexical_category, ComboProc, 0, 0);//TODO(fran): create my own cb control (edit + list probably)
			SendMessage(controls.list.combo_lexical_category, CB_SETDROPDOWNIMG, (WPARAM)global::bmps.dropdown, 0);
			ACC(controls.list.combo_lexical_category, 123);

			controls.list.edit_mnemonic = CreateWindowW(edit_oneline::wndclass, L"", WS_CHILD | ES_LEFT | WS_TABSTOP | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			edit_oneline::set_theme(controls.list.edit_mnemonic, &base_editoneline_theme);
			AWDT(controls.list.edit_mnemonic, 125);

			controls.list.static_creation_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_creation_date, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.static_last_shown_date = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_last_shown_date, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.static_score = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_score, TRUE, global::colors.ControlTxt, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);

			controls.list.button_modify = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_modify, 273);
			button::set_theme(controls.list.button_modify, &base_btn_theme);

			controls.list.button_delete = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			//AWT(controls.list.button_modify, 273);
			button::set_theme(controls.list.button_delete, &img_btn_theme);
			SendMessage(controls.list.button_delete, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.bin);

			controls.list.button_remember = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_remember, 275);
			AWTT(controls.list.button_remember, 276);
			button::set_theme(controls.list.button_remember, &accent_btn_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice----------------------:
		{
			auto& controls = state->controls.practice;

			controls.list.static_word_cnt_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_word_cnt_title, 351);

			controls.list.static_word_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_word_cnt, TRUE, global::colors.ControlTxt, global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);
			
			controls.list.static_practice_cnt_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_practice_cnt_title, 352);

			controls.list.static_practice_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_practice_cnt, TRUE, global::colors.ControlTxt, global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);//TODO(fran): add border colors
			
			controls.list.static_accuracy_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_accuracy_title, 353);

			controls.list.score_accuracy = CreateWindowW(score::wndclass, NULL, WS_CHILD 
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			score::set_brushes(controls.list.score_accuracy, FALSE, global::colors.ControlBk, global::colors.Score_RingBk, global::colors.Score_RingFull, global::colors.Score_RingEmpty, global::colors.Score_InnerCircle);

			controls.list.button_start = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_start, 350);
			button::set_theme(controls.list.button_start, &base_btn_theme);
			
			controls.list.static_accuracy_timeline_title = CreateWindowW(L"Static", NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.static_accuracy_timeline_title, 354);

			controls.list.graph_accuracy_timeline = CreateWindowW(graph::wndclass, NULL, WS_CHILD | GP_CURVE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			graph::set_brushes(controls.list.graph_accuracy_timeline, FALSE, global::colors.Graph_Line, global::colors.Graph_BkUnderLine, global::colors.Graph_Bk, global::colors.Graph_Border);

			//TODO(fran): we may want to add smth else like a total number of words practiced


			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice Writing----------------------:
		{
			auto& controls = state->controls.practice_writing;

			controls.list.static_test_word = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_test_word, TRUE, 0, global::colors.ControlBk, 0, 0, global::colors.ControlBk_Disabled, 0);
			//NOTE: text color will be set according to the type of word being shown

			controls.list.edit_answer = CreateWindowW(edit_oneline::wndclass, 0, WS_CHILD | ES_CENTER | WS_TABSTOP | WS_CLIPCHILDREN | ES_ROUNDRECT
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			//NOTE: text color and default text will be set according to the type of word that has to be written

			controls.list.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, controls.list.edit_answer, 0, NULL, NULL);
			SendMessage(controls.list.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			controls.list.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, 0, 0);
			button::set_theme(controls.list.button_show_word, &base_btn_theme);
			SendMessage(controls.list.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);

			controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL,WS_CHILD |  embedded::show_word_reduced::style::roundrect,
				0,0,0,0,state->wnd,0,0,0);
			embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice Multiplechoice----------------------:
		{
			auto& controls = state->controls.practice_multiplechoice;

			controls.list.static_question = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_question, TRUE, 0, global::colors.ControlBk, 0, 0, global::colors.ControlBk_Disabled, 0);
			//NOTE: text color will be set according to the type of word being shown
			
			//TODO(fran): should I simply use a gridview instead?
			controls.list.multibutton_choices = CreateWindowW(multibutton::wndclass, 0, WS_CHILD | WS_CLIPCHILDREN | multibutton::style::roundrect
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			multibutton::Theme multibutton_choices_theme;
			multibutton_choices_theme.dimensions.border_thickness = 1;
			multibutton_choices_theme.brushes.bk.normal = global::colors.ControlBk;//TODO(fran): try with a different color to make it destacar
			multibutton_choices_theme.brushes.border.normal = global::colors.Img;
			multibutton::set_theme(controls.list.multibutton_choices, &multibutton_choices_theme);
			//NOTE: buttons' colors will be set according to the type of word that has to be written

			controls.list.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			SendMessage(controls.list.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			controls.list.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, 0, 0);
			button::set_theme(controls.list.button_show_word, &base_btn_theme);
			SendMessage(controls.list.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);

			controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
				0, 0, 0, 0, state->wnd, 0, 0, 0);
			embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Practice Drawing----------------------:
		{
			auto& controls = state->controls.practice_drawing;
			
			controls.list.static_question = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_question, TRUE, 0, global::colors.ControlBk, 0, 0, global::colors.ControlBk_Disabled, 0);
			//NOTE: text color will be set according to the type of word being shown

			controls.list.button_next = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			button::set_theme(controls.list.button_next, &base_btn_theme);
			SendMessage(controls.list.button_next, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.arrowSimple_right);

			controls.list.button_show_word = CreateWindowW(button::wndclass, NULL, style_button_bmp
				, 0, 0, 0, 0, state->wnd, 0, 0, 0);
			button::set_theme(controls.list.button_show_word, &base_btn_theme);
			SendMessage(controls.list.button_show_word, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)global::bmps.eye);


			controls.list.button_right = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_right, 500);
			button::set_theme(controls.list.button_right, &base_btn_theme);//TODO(fran): maybe green bk

			controls.list.button_wrong = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_wrong, 501);
			button::set_theme(controls.list.button_wrong, &base_btn_theme);//TODO(fran): maybe red bk

			controls.list.paint_answer = CreateWindow(paint::wndclass, 0, WS_CHILD | WS_VISIBLE //TODO(fran): rounded?
				, 0, 0, 0, 0, state->wnd, 0, 0, 0);
			paint::set_brushes(controls.list.paint_answer, true, brush_for_learnt_word_elem(learnt_word_elem::kanji), global::colors.ControlBk, global::colors.Img, global::colors.Img_Disabled);
			paint::set_dimensions(controls.list.paint_answer, 7);//TODO(fran): find good brush size

			controls.list.static_correct_answer = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_correct_answer, TRUE, brush_for_learnt_word_elem(learnt_word_elem::kanji), global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);

			controls.embedded_show_word_reduced = CreateWindow(embedded::show_word_reduced::wndclass, NULL, WS_CHILD | embedded::show_word_reduced::style::roundrect,
				0, 0, 0, 0, state->wnd, 0, 0, 0);//TODO(fran): must be shown on top of all the other wnds
			embedded::show_word_reduced::set_theme(controls.embedded_show_word_reduced, &eswr_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		//---------------------Review practice----------------------:
		{
			auto& controls = state->controls.review_practice;

			controls.list.static_review= CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			static_oneline::set_brushes(controls.list.static_review, TRUE, global::colors.ControlTxt, global::colors.ControlBk, 0, global::colors.ControlTxt_Disabled, global::colors.ControlBk_Disabled, 0);//TODO(fran): add border colors
			AWT(controls.list.static_review, 450);

			controls.list.gridview_practices = CreateWindowW(gridview::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
//#define TEST_GRIDVIEW
#ifndef TEST_GRIDVIEW
			gridview::set_brushes(controls.list.gridview_practices, TRUE, global::colors.ControlBk, global::colors.ControlBk, global::colors.ControlBk_Disabled, global::colors.ControlBk_Disabled);//TODO(fran): add border brushes
#else
			gridview::set_brushes(controls.list.gridview_practices, TRUE, global::colors.CaptionBk, 0, global::colors.CaptionBk_Inactive, 0);
#endif
			gridview::set_render_function(controls.list.gridview_practices, gridview_practices_renderfunc);

			controls.list.button_continue = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
			AWT(controls.list.button_continue, 451);
			button::set_theme(controls.list.button_continue, &base_btn_theme);

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}
	}

	void resize_controls(ProcState* state, ProcState::page page) {
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTWIDTH(r);
		int h = RECTHEIGHT(r);
		int half_w = w / 2;
		int w_pad = (int)((float)w * .05f);//TODO(fran): hard limit for max padding
		int h_pad = (int)((float)h * .05f);
		
		{//navbar test
			rect_i32 navbar;
			navbar.left = r.left;
			navbar.top = r.top;
			navbar.w = w;
			navbar.h = 35;
			MoveWindow(state->controls.navbar, navbar.left, navbar.top, navbar.w, navbar.h, FALSE);
		}

		switch (page) {
		case ProcState::page::landing:
		{
			auto& controls = state->controls.landingpage;
			//One button on top of the other vertically, all buttons must be squares
			//TODO(fran): we could, at least to try how it looks, to check whether the wnd is longer on w or h and place the controls vertically or horizontally next to each other
			int wnd_cnt = ARRAYSIZE(controls.all);
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control

			int max_h = h - pad_cnt * h_pad;
			int max_w = w - 2 * w_pad;
			int wnd_h = clamp(0, max_h / wnd_cnt, max_w);
			int wnd_w = wnd_h;
			int wnd_x = (w - wnd_w) / 2;
			int h_step = (h - (wnd_h*wnd_cnt)) / pad_cnt;

			rect_i32 button_new;
			button_new.x = wnd_x;
			button_new.y = h_step;
			button_new.w = wnd_h;
			button_new.h = wnd_h;

			rect_i32 button_practice;
			button_practice.x = wnd_x;
			button_practice.y = button_new.bottom() + h_step;
			button_practice.w= wnd_h;
			button_practice.h= wnd_h;

			rect_i32 button_search;
			button_search.x = wnd_x;
			button_search.y = button_practice.bottom() + h_step;
			button_search.w= wnd_h;
			button_search.h= wnd_h;

			//rect_i32 combo_languages;
			//combo_languages.h = wnd_h;
			//combo_languages.y = h_pad / 2;
			//combo_languages.w = min(distance(w,button_new.right()+w_pad/2), avg_str_dim((HFONT)SendMessage(controls.list.combo_languages, WM_GETFONT, 0, 0), 20).cx);
			//combo_languages.x = w - combo_languages.w - w_pad / 2;

			MyMoveWindow(controls.list.button_new, button_new,FALSE);
			MyMoveWindow(controls.list.button_practice, button_practice,FALSE);
			MyMoveWindow(controls.list.button_search, button_search,FALSE);
			//MyMoveWindow(controls.list.combo_languages, combo_languages,FALSE);
		} break;
		case ProcState::page::new_word:
		{
			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned
			auto& controls = state->controls.new_word;

			//int wnd_cnt = ARRAYSIZE(controls.all);
			//int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
#if 0
			int start_y = (h - (pad_cnt-1 /*we only have 1 real pad for lex_categ*/) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): centering aint quite right
#else
			int start_y = 0;
#endif

			rect_i32 edit_hiragana;
			edit_hiragana.y = start_y;
			edit_hiragana.h = wnd_h;
			edit_hiragana.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_hiragana, WM_GETFONT, 0, 0), 30).cx);
			edit_hiragana.x = (w- edit_hiragana.w)/2;

			rect_i32 cb_lex_categ;
			cb_lex_categ.w = (i32)((f32)edit_hiragana.w * .7f);
			cb_lex_categ.x = (w- cb_lex_categ.w)/2;//TODO(fran): placement doesnt look too natural nor good; maybe if I centered the text of the cb?
			cb_lex_categ.y = edit_hiragana.bottom() + h_pad/2;
			cb_lex_categ.h = edit_hiragana.h;//TODO(fran): for some reason comboboxes are always a little smaller than you ask, find out how to correctly correct that
			
			rect_i32 edit_kanji;
			edit_kanji.y = cb_lex_categ.bottom() + h_pad/2;
			edit_kanji.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_kanji, WM_GETFONT, 0, 0), 30).cx);
			edit_kanji.h = wnd_h;
			edit_kanji.x = (w- edit_kanji.w)/2;

			rect_i32 edit_translation;
			edit_translation.y = edit_kanji.bottom() + h_pad;
			edit_translation.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_translation, WM_GETFONT, 0, 0), 30).cx);
			edit_translation.h = wnd_h;
			edit_translation.x = (w- edit_translation.w)/2;

			rect_i32 edit_mnemonic;
			edit_mnemonic.y = edit_translation.bottom() + h_pad;
			edit_mnemonic.w = max_w;
			edit_mnemonic.h = wnd_h;
			edit_mnemonic.x = (w- edit_mnemonic.w)/2;

			rect_i32 btn_save;
			btn_save.w = 70;
			btn_save.h = wnd_h;
			btn_save.y = edit_mnemonic.bottom() + h_pad;
			btn_save.x = (w - btn_save.w) / 2;

			rect_i32 bottom_most_control = btn_save;

			int used_h = bottom_most_control.bottom();// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.list.edit_hiragana, edit_hiragana, FALSE);
			MyMoveWindow_offset(controls.list.combo_lexical_category, cb_lex_categ, FALSE);
			MyMoveWindow_offset(controls.list.edit_kanji, edit_kanji, FALSE);
			MyMoveWindow_offset(controls.list.edit_translation, edit_translation, FALSE);
			MyMoveWindow_offset(controls.list.edit_mnemonic, edit_mnemonic, FALSE);
			MyMoveWindow_offset(controls.list.button_save, btn_save, FALSE);

		} break;
		case ProcState::page::practice: 
		{
			auto& controls = state->controls.practice;

			//Stats first, then start button
			//For stats I imagine a 2x2 "grid":	first row has "word count" and "times practiced"
			//									second row has "score" and a chart of accuracy for last 30 days for example


			//int wnd_cnt = ARRAYSIZE(controls.all);
			//int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
#if 0
			int start_y = (h - (pad_cnt) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): not good, what we want is the total height of all windows combined, an use that to center correctly, eg start_y = (h-total_h)/2; and we offset everything by start_y at the end
#else
			int start_y = 0;//We start from 0 and offset once we know the sizes and positions for everything
#endif

			int grid_h = wnd_h * 4;
			int grid_w = grid_h * 16 / 9;
			auto grid = create_grid_2x2(grid_h, grid_w, start_y, w_pad / 2, h_pad / 2, max_w, w);
			
			rect_i32 cell;

			//First cell
			cell = grid[0][0];
			rect_i32 static_word_cnt_title;
			static_word_cnt_title.w = cell.w;
			static_word_cnt_title.h = min(wnd_h,cell.h);
			static_word_cnt_title.x = cell.center_x() - static_word_cnt_title.w/2;
			static_word_cnt_title.y = cell.top;

			//NOTE: the values should use a much bigger font
			rect_i32 static_word_cnt;
			static_word_cnt.w = cell.w;
			static_word_cnt.x = cell.center_x() - static_word_cnt.w/2;
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
			graph_accuracy_timeline.w = min(graph_accuracy_timeline.h * 16 / 9,cell.w);
			graph_accuracy_timeline.x = cell.center_x() - graph_accuracy_timeline.w / 2;
			graph_accuracy_timeline.y = cell.bottom() - graph_accuracy_timeline.h;


			rect_i32 last_stat = graph_accuracy_timeline;

			rect_i32 button_start;
			button_start.y = last_stat.bottom() + h_pad;
			button_start.w = 70;
			button_start.h = wnd_h;
			button_start.x = (w - button_start.w)/2;

			rect_i32 bottom_most_control = button_start;

			int used_h = bottom_most_control.bottom();// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			
			MyMoveWindow_offset(controls.list.static_word_cnt_title, static_word_cnt_title, FALSE);
			MyMoveWindow_offset(controls.list.static_word_cnt, static_word_cnt, FALSE);
			MyMoveWindow_offset(controls.list.static_practice_cnt_title, static_practice_cnt_title, FALSE);
			MyMoveWindow_offset(controls.list.static_practice_cnt, static_practice_cnt, FALSE);
			MyMoveWindow_offset(controls.list.static_accuracy_title, static_accuracy_title, FALSE);
			MyMoveWindow_offset(controls.list.score_accuracy, score_accuracy, FALSE);
			MyMoveWindow_offset(controls.list.static_accuracy_timeline_title, static_accuracy_timeline_title, FALSE);
			MyMoveWindow_offset(controls.list.graph_accuracy_timeline, graph_accuracy_timeline, FALSE);
			MyMoveWindow_offset(controls.list.button_start, button_start, FALSE);

		} break;
		case ProcState::page::practice_writing:
		{
			auto& controls = state->controls.practice_writing;

			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int start_y = 0;
			int bigwnd_h = wnd_h * 4;

			rect_i32 static_test_word;
			static_test_word.y = start_y;
			static_test_word.h = bigwnd_h;
			static_test_word.w = w;
			static_test_word.x = (w - static_test_word.w)/2;

			rect_i32 edit_answer;
			edit_answer.y = static_test_word.bottom() + h_pad;
			edit_answer.h = wnd_h;
			edit_answer.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_answer, WM_GETFONT, 0, 0), 20).cx);
			edit_answer.x = (w - edit_answer.w) / 2;

			rect_i32 button_next;//child inside edit_answer
			button_next.y = 1; //the button is inside the edit box (and past the border) //TODO(fran): we should ask the parent for its border size
			button_next.h = edit_answer.h - 2;
			button_next.w = min(button_next.h, max(0, edit_answer.w-4/*avoid covering rounded borders*/));
			button_next.x = edit_answer.w - button_next.w - 2;//TODO(fran): if the style of the edit box parent is  ES_ROUNDRECT we gotta subtract one more, in this case we went from -1 to -2

			rect_i32 button_show_word;
			button_show_word.h = (i32)(wnd_h * .8f);
			button_show_word.y = edit_answer.bottom() + 3;
			button_show_word.w = button_show_word.h * 16 / 9;
			button_show_word.x = edit_answer.center_x() - button_show_word.w / 2;

			rect_i32 embedded_show_word_reduced;
			embedded_show_word_reduced.w = max_w;
			embedded_show_word_reduced.h = wnd_h * 3;
			embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
			embedded_show_word_reduced.y = button_show_word.bottom() + 3;

			rect_i32 bottom_most_control = button_show_word;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			MyMoveWindow_offset(controls.list.static_test_word, static_test_word, FALSE);
			MyMoveWindow_offset(controls.list.edit_answer, edit_answer, FALSE);
			MyMoveWindow_offset(controls.list.button_show_word, button_show_word, FALSE);
			MyMoveWindow_offset(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);
			MyMoveWindow(controls.list.button_next, button_next, FALSE);

		} break;
		case ProcState::page::practice_multiplechoice:
		{
			auto& controls = state->controls.practice_multiplechoice;

			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int start_y = 0;
			int bigwnd_h = wnd_h * 4;

			rect_i32 static_question;
			static_question.y = start_y;
			static_question.h = bigwnd_h;
			static_question.w = w;
			static_question.x = (w - static_question.w) / 2;

			rect_i32 multibutton_choices;
			multibutton_choices.y = static_question.bottom() + h_pad;
			multibutton_choices.h = bigwnd_h;
			multibutton_choices.w = max_w;
			multibutton_choices.x = (w - multibutton_choices.w) / 2;

			multibutton::Theme multibutton_choices_theme;
			multibutton_choices_theme.dimensions.btn = { avg_str_dim((HFONT)SendMessage(controls.list.multibutton_choices, WM_GETFONT, 0, 0), 15).cx,wnd_h };
			multibutton_choices_theme.dimensions.inbetween_pad = { 3,3 };
			multibutton::set_theme(controls.list.multibutton_choices, &multibutton_choices_theme);

			rect_i32 button_next;
			button_next.y = multibutton_choices.bottom() + h_pad;
			button_next.h = wnd_h;
			button_next.w = button_next.h;

			rect_i32 button_show_word;
			button_show_word.h = button_next.h;
			button_show_word.y = button_next.y;
			button_show_word.w = button_show_word.h * 16 / 9;
			button_show_word.x = (w - (button_show_word.w + button_next.w)) / 2;

			button_next.x = button_show_word.right() + 1;

			rect_i32 embedded_show_word_reduced;
			embedded_show_word_reduced.w = max_w;
			embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
			embedded_show_word_reduced.h = wnd_h * 3;
			embedded_show_word_reduced.y = button_show_word.bottom() + 3;

			rect_i32 bottom_most_control = button_show_word;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			MyMoveWindow_offset(controls.list.static_question, static_question, FALSE);
			MyMoveWindow_offset(controls.list.multibutton_choices, multibutton_choices, FALSE);
			MyMoveWindow_offset(controls.list.button_next, button_next, FALSE);
			MyMoveWindow_offset(controls.list.button_show_word, button_show_word, FALSE);
			MyMoveWindow_offset(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);

		} break;
		case ProcState::page::practice_drawing:
		{
			auto& controls = state->controls.practice_drawing;

			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int start_y = 0;
			int bigwnd_h = wnd_h * 4;

			rect_i32 static_question;
			static_question.y = start_y;
			static_question.h = bigwnd_h;
			static_question.w = w;
			static_question.x = (w - static_question.w) / 2;

			rect_i32 paint_answer;
			paint_answer.y = static_question.bottom() + h_pad;
			paint_answer.h = bigwnd_h*2;
			paint_answer.w = max_w;
			paint_answer.x = (w - paint_answer.w) / 2;

			rect_i32 button_next;
			button_next.y = paint_answer.bottom() + h_pad;
			button_next.h = wnd_h;
			button_next.w = button_next.h;

			rect_i32 button_show_word;
			button_show_word.h = button_next.h;
			button_show_word.y = button_next.y;
			button_show_word.w = button_show_word.h * 16 / 9;
			button_show_word.x = (w - (button_show_word.w + button_next.w)) / 2;

			button_next.x = button_show_word.right() + 1;

			rect_i32 embedded_show_word_reduced;
			embedded_show_word_reduced.w = max_w;
			embedded_show_word_reduced.x = (w - embedded_show_word_reduced.w) / 2;
			embedded_show_word_reduced.h = wnd_h * 3;
			embedded_show_word_reduced.y = button_show_word.bottom() + 3;

			rect_i32 static_correct_answer;
			static_correct_answer.y = button_next.bottom() + h_pad;
			static_correct_answer.h = bigwnd_h-wnd_h;
			static_correct_answer.w = w;
			static_correct_answer.x = (w - static_correct_answer.w) / 2;

			rect_i32 button_wrong;
			button_wrong.y = static_correct_answer.bottom() + h_pad;
			button_wrong.h = wnd_h;
			button_wrong.w = min(max_w/2, avg_str_dim((HFONT)SendMessage(controls.list.button_wrong, WM_GETFONT, 0, 0), 20).cx);
			button_wrong.x = half_w - w_pad / 2 - button_wrong.w;

			rect_i32 button_right;
			button_right.y = static_correct_answer.bottom() + h_pad;
			button_right.h = wnd_h;
			button_right.w = min(max_w / 2, avg_str_dim((HFONT)SendMessage(controls.list.button_right, WM_GETFONT, 0, 0), 20).cx);
			button_right.x = half_w + w_pad / 2;

			rect_i32 bottom_most_control = button_right;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			MyMoveWindow_offset(controls.list.static_question, static_question, FALSE);
			MyMoveWindow_offset(controls.list.paint_answer, paint_answer, FALSE);
			MyMoveWindow_offset(controls.list.button_next, button_next, FALSE);
			MyMoveWindow_offset(controls.list.button_show_word, button_show_word, FALSE);
			MyMoveWindow_offset(controls.embedded_show_word_reduced, embedded_show_word_reduced, FALSE);
			MyMoveWindow_offset(controls.list.static_correct_answer, static_correct_answer, FALSE);
			MyMoveWindow_offset(controls.list.button_wrong, button_wrong, FALSE);
			MyMoveWindow_offset(controls.list.button_right, button_right, FALSE);


		} break;
		case ProcState::page::review_practice:
		{
			auto& controls = state->controls.review_practice;

			int wnd_h = 30;
			int bigwnd_h = wnd_h * 3;
			int max_w = w - w_pad * 2;
			int start_y = 0;

			rect_i32 static_review;
			static_review.y = start_y;
			static_review.h = wnd_h*2;
			static_review.w = max_w;
			static_review.x = (w - static_review.w) / 2;

			rect_i32 gridview_practices;
			gridview_practices.y = static_review.bottom() + h_pad;

			gridview::element_dimensions gridview_practices_dims;
			gridview_practices_dims.border_pad_y = 3;
			gridview_practices_dims.inbetween_pad = { 5,5 };
			gridview_practices_dims.element_dim = { bigwnd_h,bigwnd_h };
			gridview::set_dimensions(controls.list.gridview_practices, gridview_practices_dims);

			SIZE gridview_practices_wh;
			{
				//TODO(fran): this is the worst code I've written in quite a while, this and the gridview code that was needed need a revision
				const size_t elems_per_row = 5;
				size_t curr_elem_cnt = gridview::get_elem_cnt(controls.list.gridview_practices);
				i32 full_w = gridview::get_dim_for_elemcnt_elemperrow(controls.list.gridview_practices, curr_elem_cnt, elems_per_row).cx;

				i32 real_w = min(w - w_pad * 2, full_w);

				i32 max_h = gridview::get_dim_for_elemcnt_elemperrow(controls.list.gridview_practices, elems_per_row*4, elems_per_row).cy;//4 rows

				i32 full_h = gridview::get_dim_for_elemcnt_w(controls.list.gridview_practices,curr_elem_cnt, real_w).cy;

				//SIZE full_dim = gridview::get_dim_for(controls.list.gridview_practices, row_cnt, elems_per_row);
				gridview_practices_wh = { real_w,min(full_h,max_h)};
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

			MyMoveWindow_offset(controls.list.static_review, static_review, FALSE);
			MyMoveWindow_offset(controls.list.gridview_practices, gridview_practices, FALSE);
			MyMoveWindow_offset(controls.list.button_continue, button_continue, FALSE);

		} break;
		case ProcState::page::search: 
		{
			auto& controls = state->controls.search;
			int wnd_h = 30;
			int max_w = w - w_pad * 2;

			//rect_i32 cb_search;
			//cb_search.x = w_pad;
			//cb_search.y = h_pad;
			//cb_search.h = 40;//TODO(fran): cbs dont respect this at all, they set a minimum h by the font size I think
			//cb_search.w = max_w;

			rect_i32 searchbox_search;
			searchbox_search.w = max_w;
			searchbox_search.x = (w- searchbox_search.w) /2;
			searchbox_search.h = wnd_h;
			searchbox_search.y = h_pad;

			//MyMoveWindow(controls.list.combo_search, cb_search, FALSE);
			MyMoveWindow(controls.list.searchbox_search, searchbox_search, FALSE);
			searchbox::set_listbox_dimensions(controls.list.searchbox_search, listbox::dimensions().set_border_thickness(1).set_element_h(wnd_h));

		} break;
		case ProcState::page::show_word: 
		{
			auto& controls = state->controls.show_word;

			//One edit control on top of the other, centered in the middle of the wnd, the lex_category covering less than half of the w of the other controls, and right aligned, non user editable controls go below the rest

			//int wnd_cnt = ARRAYSIZE(controls.all) - 2;//subtract the controls that go next to one another
			//int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control
			int max_w = w - w_pad * 2;
			int wnd_h = 30;
			int bigwnd_h = wnd_h * 4;
#if 0
			int start_y = (h - (pad_cnt - 1 /*we only have 1 real pad for lex_categ*/) * h_pad - wnd_cnt * wnd_h) / 2;//TODO(fran): centering aint quite right
#else
			int start_y = 0;
#endif

			rect_i32 static_hiragana;
			static_hiragana.x = w_pad;
			static_hiragana.y = start_y;
			static_hiragana.h = bigwnd_h;
			static_hiragana.w = max_w; //TODO(fran): we may want to establish a max_w that's more fixed, as a clamp, instead of continually increasing as the wnd width does

			rect_i32 cb_lex_categ;
			cb_lex_categ.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.combo_lexical_category, WM_GETFONT, 0, 0), 20).cx);
#if 0
			cb_lex_categ.x = min( w - w_pad - cb_lex_categ.w, w/2 + cb_lex_categ.w);//TODO(fran): doesnt look good, maybe just center it
#else
			cb_lex_categ.x = (w - cb_lex_categ.w) / 2;
#endif
			cb_lex_categ.y = static_hiragana.bottom() + h_pad / 2;
			cb_lex_categ.h = wnd_h;//TODO(fran): for some reason comboboxes are always a little smaller than you ask, find out how to correctly correct that

			rect_i32 edit_kanji;
			edit_kanji.y = cb_lex_categ.bottom() + h_pad / 2;
			edit_kanji.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_kanji, WM_GETFONT, 0, 0), 30).cx);
			edit_kanji.x = (w- edit_kanji.w)/2;
			edit_kanji.h = wnd_h;

			rect_i32 edit_translation;
			edit_translation.y = edit_kanji.bottom() + h_pad;
			edit_translation.w = min(max_w, avg_str_dim((HFONT)SendMessage(controls.list.edit_translation, WM_GETFONT, 0, 0), 30).cx);
			edit_translation.x = (w - edit_translation.w) / 2;
			edit_translation.h = wnd_h;

			rect_i32 edit_mnemonic;
			edit_mnemonic.w = max_w;
			edit_mnemonic.x = (w- edit_mnemonic.w)/2;
			edit_mnemonic.y = edit_translation.bottom() + h_pad;
			edit_mnemonic.h = wnd_h;

			rect_i32 last_user = edit_mnemonic;

			auto stats_grid = create_grid_2x2(
				wnd_h, avg_str_dim((HFONT)SendMessage(controls.list.static_creation_date, WM_GETFONT, 0, 0), 25).cx
				, last_user.bottom() + h_pad, w_pad/2, h_pad/2, max_w, w);

			rect_i32 static_creation_date = stats_grid[0][0];

			rect_i32 static_last_shown_date = stats_grid[0][1];

			rect_i32 static_score = stats_grid[1][0];

			rect_i32 last_cell = static_score;//TODO(fran): we still got one more space at [1][1]

			rect_i32 btn_modify;
			btn_modify.w = 70;
			btn_modify.h = wnd_h;
			btn_modify.y = last_cell.bottom() + h_pad;
			btn_modify.x = (w - btn_modify.w) / 2;

			rect_i32 btn_remember;
			btn_remember.w = 90;
			btn_remember.h = wnd_h;
			btn_remember.y = btn_modify.y;
			btn_remember.x = btn_modify.right() + w_pad;

			rect_i32 btn_delete;
			btn_delete.h = wnd_h;
			btn_delete.w = btn_delete.h;
#if 0 //TODO(fran): which one to go with?
			btn_delete.x = btn_modify.right() + w_pad*;/*TODO(fran): clamp to not go beyond w*/
			btn_delete.y = btn_modify_y;
#else
			btn_delete.x = (w - btn_delete.w)/2;
			btn_delete.y = btn_modify.bottom() + h_pad;
#endif

			rect_i32 bottom_most_control = btn_delete;

			int used_h = bottom_most_control.bottom();
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.list.static_hiragana, static_hiragana, FALSE);
			MyMoveWindow_offset(controls.list.combo_lexical_category, cb_lex_categ, FALSE);
			MyMoveWindow_offset(controls.list.edit_kanji, edit_kanji, FALSE);
			MyMoveWindow_offset(controls.list.edit_translation, edit_translation, FALSE);
			MyMoveWindow_offset(controls.list.edit_mnemonic, edit_mnemonic, FALSE);

			//Dates one next to the other, the same for times_... related objs
			MyMoveWindow_offset(controls.list.static_creation_date, static_creation_date, FALSE);
			MyMoveWindow_offset(controls.list.static_last_shown_date, static_last_shown_date, FALSE);
			MyMoveWindow_offset(controls.list.static_score, static_score, FALSE);
			MyMoveWindow_offset(controls.list.button_modify, btn_modify, FALSE);
			MyMoveWindow_offset(controls.list.button_remember, btn_remember, FALSE);
			MyMoveWindow_offset(controls.list.button_delete, btn_delete, FALSE);

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
		default:Assert(0);
		}
	}
	void resize_controls(ProcState* state) {
		resize_controls(state, state->current_page);
	}

	void save_settings(ProcState* state) {
		RECT rc; GetWindowRect(state->wnd, &rc);
		state->settings->rc = rc;
	}

	void show_page(ProcState* state, ProcState::page p, u32 ShowWindow_cmd /*SW_SHOW,...*/) {
		switch (p) {
		case decltype(p)::landing: for (auto ctl : state->controls.landingpage.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case decltype(p)::new_word: for (auto ctl : state->controls.new_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case decltype(p)::practice: for (auto ctl : state->controls.practice.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case decltype(p)::practice_writing: 
			for (auto ctl : state->controls.practice_writing.all) ShowWindow(ctl, ShowWindow_cmd); 
			ShowWindow(state->controls.practice_writing.embedded_show_word_reduced, SW_HIDE);
			break;
		case decltype(p)::search: for (auto ctl : state->controls.search.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case decltype(p)::show_word: for (auto ctl : state->controls.show_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case decltype(p)::review_practice: for (auto ctl : state->controls.review_practice.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case decltype(p)::review_practice_writing: show_page(state, ProcState::page::practice_writing, ShowWindow_cmd); break;
		case decltype(p)::practice_multiplechoice:
			for (auto ctl : state->controls.practice_multiplechoice.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->controls.practice_multiplechoice.embedded_show_word_reduced, SW_HIDE);
			break;
		case decltype(p)::review_practice_multiplechoice: show_page(state, ProcState::page::practice_multiplechoice, ShowWindow_cmd); break;
		case decltype(p)::practice_drawing:
			for (auto ctl : state->controls.practice_drawing.all) ShowWindow(ctl, ShowWindow_cmd);
			ShowWindow(state->controls.practice_drawing.list.static_correct_answer, SW_HIDE);//HACK:we should have some sort of separation between hidden and shown controls
			ShowWindow(state->controls.practice_drawing.list.button_wrong, SW_HIDE);
			ShowWindow(state->controls.practice_drawing.list.button_right, SW_HIDE);
			ShowWindow(state->controls.practice_drawing.embedded_show_word_reduced, SW_HIDE);
			break;
		case decltype(p)::review_practice_drawing: show_page(state, ProcState::page::practice_drawing, ShowWindow_cmd); break;
		default:Assert(0);
		}
	}

	void set_default_focus(ProcState* state, ProcState::page p) {
		switch (p) {
		case decltype(p)::landing: SetFocus(0); break;//Remove focus from whoever had it //HACK?
		case decltype(p)::search: SetFocus(state->controls.search.list.searchbox_search); break;
		case decltype(p)::practice_writing: SetFocus(state->controls.practice_writing.list.edit_answer); break;

		//default:Assert(0);
		}
	}

	void set_current_page(ProcState* state, ProcState::page new_page) {
		show_page(state, state->current_page,SW_HIDE);
		state->current_page = new_page;
		resize_controls(state);
		show_page(state, state->current_page,SW_SHOW);
		set_default_focus(state, state->current_page);

		switch (new_page) {
		case decltype(new_page)::practice_writing:
		{
			//TODO(fran): idk where to put this, I'd put it in preload but preload doesnt guarantee we're switching pages, even though it's mostly true currently
			auto& controls = state->controls.practice_writing;
			//SetFocus(controls.list.edit_answer);//TODO(fran): problem is now we cant see in which lang we have to write, we have two options, either give the edit control an extra flag of show placeholder even if cursor is visible; option 2 setfocus to us (main wnd) wait for the user to press something, redirect the msg to the edit box and setfocus. INFO: wanikani shows the placeholder and the caret at the same time, lets do that
		} break;
		}
	}

	void goto_previous_page(ProcState* state) {
		if (state->previous_pages.cnt>0) {
			set_current_page(state, state->previous_pages.pages[--state->previous_pages.cnt]);
			if(state->previous_pages.cnt==0)PostMessage(state->nc_parent, WM_SHOWBACKBTN, FALSE, 0);//hide the back button
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

		PostMessage(state->nc_parent, WM_SHOWBACKBTN, TRUE, 0);//show the back button
	}

	bool check_new_word(ProcState* state) {
		auto& page = state->controls.new_word;
		HWND edit_required[] = { page.list.edit_hiragana,page.list.edit_translation };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
				return false;
			}
		}
		return true;
	}

	bool check_show_word(ProcState* state) {
		auto& page = state->controls.show_word;
		HWND edit_required[] = { page.list.edit_translation };
		for (int i = 0; i < ARRAYSIZE(edit_required); i++) {
			int sz_char = (int)SendMessage(edit_required[i], WM_GETTEXTLENGTH, 0, 0);
			if (!sz_char) {
				edit_oneline::show_tip(edit_required[i], RCS(11), EDITONELINE_default_tooltip_duration, edit_oneline::ETP::top);
				return false;
			}
		}
		return true;
	}

	ptr<void*> searchbox_retrieve_search_options_func(utf16_str user_input, void* user_extra) {
		ptr<void*> res{ 0 };
		ProcState* state = get_state((HWND)user_extra);
		if (state && state->current_page == ProcState::page::search && user_input.sz) {

			if (any_kanji(user_input)) state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::kanji;
			else if (any_hiragana_katakana(user_input)) state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::hiragana;
			else state->pagestate.search.search_type = decltype(state->pagestate.search.search_type)::meaning;

			auto search_res = search_word_matches(state->settings->db, state->pagestate.search.search_type, user_input.str, 8); //defer{ free(search_res.matches);/*free the dinamically allocated array*/ };
			//TODO(fran): search_word_matches should return a ptr
			res.alloc(search_res.cnt);//TODO(fran): am I ever freeing this?
			for (size_t i = 0; i < search_res.cnt; i++)
				//res[i] = search_res.matches[i];
				res[i] = &search_res[i];
		}
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
		case decltype(page)::new_word:
		{
			learnt_word16* new_word = (decltype(new_word))data;//NOTE: since we are in UI we expect utf16 strings
			auto controls = state->controls.new_word;
			//NOTE: any of the values in new_word can be invalid, we gotta check before using
			//TODO(fran): if the controls only had name and no identifier eg "edit" for "edit_hiragana" we could directly map everything with a foreach by having the same name in the word and controls structs
			//NOTE: settext already has null checking
			SendMessageW(controls.list.edit_hiragana, WM_SETTEXT, 0, (LPARAM)new_word->attributes.hiragana.str);
			SendMessageW(controls.list.edit_kanji, WM_SETTEXT, 0, (LPARAM)new_word->attributes.kanji.str);
			SendMessageW(controls.list.edit_translation, WM_SETTEXT, 0, (LPARAM)new_word->attributes.translation.str);
			SendMessageW(controls.list.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)new_word->attributes.mnemonic.str);
			int lex_categ_sel;
			if (new_word->attributes.lexical_category.str) {
				try { lex_categ_sel = std::stoi(new_word->attributes.lexical_category.str); }
				catch (...) { lex_categ_sel = -1; }
			} else lex_categ_sel = -1;
			SendMessageW(controls.list.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

		} break;
		case decltype(page)::show_word:
		{
			stored_word16* word_to_show = (decltype(word_to_show))data;
			auto controls = state->controls.show_word;
			//IDEA: in this page we could reuse the controls from new_word, that way we first call preload_page(new_word) with word_to_show.user_defined and then do our thing (this idea doesnt quite work)

			SendMessageW(controls.list.static_hiragana, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.hiragana.str);
			SendMessageW(controls.list.edit_kanji, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.kanji.str);
			SendMessageW(controls.list.edit_translation, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.translation.str);
			SendMessageW(controls.list.edit_mnemonic, WM_SETTEXT, 0, (LPARAM)word_to_show->user_defined.attributes.mnemonic.str);

			int lex_categ_sel;
			if (word_to_show->user_defined.attributes.lexical_category.str) {
				try { lex_categ_sel = std::stoi((utf16*)word_to_show->user_defined.attributes.lexical_category.str); }
				catch (...) { lex_categ_sel = -1; }
			}
			else lex_categ_sel = -1;
			SendMessageW(controls.list.combo_lexical_category, CB_SETCURSEL, lex_categ_sel, 0);

			if(word_to_show->application_defined.attributes.creation_date.str)
				SendMessageW(controls.list.static_creation_date, WM_SETTEXT, 0, (LPARAM)(RS(270) + L" " + (utf16*)word_to_show->application_defined.attributes.creation_date.str).c_str());

			if (word_to_show->application_defined.attributes.last_shown_date.str)
				SendMessageW(controls.list.static_last_shown_date, WM_SETTEXT, 0, (LPARAM)(RS(271) + L" " + (utf16*)word_to_show->application_defined.attributes.last_shown_date.str).c_str());

			if (word_to_show->application_defined.attributes.times_right.str && word_to_show->application_defined.attributes.times_shown.str) {
				std::wstring score = (RS(272) + L" " + (utf16*)word_to_show->application_defined.attributes.times_right.str + L" / " + (utf16*)word_to_show->application_defined.attributes.times_shown.str);
				try {
					float numerator = std::stof((utf16*)word_to_show->application_defined.attributes.times_right.str);
					float denominator = std::stof((utf16*)word_to_show->application_defined.attributes.times_shown.str);
					if (denominator != 0.f) {
						int percentage = (int)((numerator/denominator) * 100.f);
						score += L" - " + std::to_wstring(percentage) + L"%";
					}
				}
				catch (...) {}
				SendMessageW(controls.list.static_score, WM_SETTEXT, 0, (LPARAM)score.c_str());
			}

			button::Theme accent_btn_theme;
			accent_btn_theme.brushes.foreground.normal = global::colors.Accent;
			accent_btn_theme.brushes.border.normal = global::colors.Accent;
			button::set_theme(controls.list.button_remember, &accent_btn_theme);

		} break;
		case decltype(page)::practice:
		{
			user_stats* stats = (decltype(stats))data;
			auto controls = state->controls.practice;
//#define PRACTICE_TEST_STATS
#ifndef PRACTICE_TEST_STATS
			SendMessage(controls.list.score_accuracy, SC_SETSCORE, f32_to_WPARAM(stats->accuracy()), 0);
			SendMessage(controls.list.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->word_cnt).c_str());
			SendMessage(controls.list.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->times_practiced).c_str());
			//TODO(fran): timeline, we'll probably need to store that as blob or text in the db, this is were mongodb would be nice, just throw a js obj for each timepoint
			//TODO(fran): if the timeline is empty we should simply put the current accuracy, or leave it empty idk
			graph::set_points(controls.list.graph_accuracy_timeline, stats->accuracy_timeline.mem, stats->accuracy_timeline.cnt);
			graph::graph_dimensions grid_dims;
			grid_dims.set_top_point(100);
			grid_dims.set_bottom_point(0);
			grid_dims.set_viewable_points_range(0, stats->accuracy_timeline.cnt);
			graph::set_dimensions(controls.list.graph_accuracy_timeline, grid_dims);
			//graph::set_top_point(controls.list.graph_accuracy_timeline, 100);
			//graph::set_bottom_point(controls.list.graph_accuracy_timeline, 0);
			//graph::set_viewable_points_range(controls.list.graph_accuracy_timeline, 0, stats->accuracy_timeline.cnt);
#else
			SendMessage(controls.list.score_accuracy, SC_SETSCORE, f32_to_WPARAM(.6f), 0);
			SendMessage(controls.list.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(1452).c_str());
			SendMessage(controls.list.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(559).c_str());
			i32 accu[]{ 77,56,32,12,48,95,65,32,54,67,79,88,100 };
			graph::set_points(controls.list.graph_accuracy_timeline, accu, ARRAYSIZE(accu));
			graph::set_top_point(controls.list.graph_accuracy_timeline, 100);
			graph::set_bottom_point(controls.list.graph_accuracy_timeline, 0);
			graph::set_viewable_points_range(controls.list.graph_accuracy_timeline, 0, ARRAYSIZE(accu));
#endif
			//BOOL can_practice = stats->word_cnt > 0;
			//EnableWindow(controls.list.button_start, can_practice);
		} break;
		case decltype(page)::practice_writing:
		{
			//TODO(fran): shouldnt preload_page also call clear_page?
			practice_writing_word* practice = (decltype(practice))data;
			auto controls = state->controls.practice_writing;
			//store data for future proof checking
			state->pagestate.practice_writing.practice = practice;

			utf16* test_word{0};//NOTE: compiler cant know that these guys will always be initialized so I gotta zero them
			HBRUSH test_word_br{0};
			str answer_placeholder;//NOTE: using string so the object doesnt get destroyed inside the switch statement
			HBRUSH answer_br{0};

			str answer_hiragana{ L"こたえ" };

			switch (practice->practice_type) {
			case decltype(practice->practice_type)::hiragana_to_translation:
			{
				test_word = (utf16*)practice->word.attributes.hiragana.str;
				test_word_br = global::colors.hiragana;
				//TODO(fran): idk if I should put "translation" or "answer", and for hiragana "hiragana" or "こたえ" (meaning answer), and "kanji" or "答え"
				answer_placeholder = RS(380);
				answer_br = global::colors.translation;
			} break;
			case decltype(practice->practice_type)::kanji_to_hiragana:
			{
				test_word = (utf16*)practice->word.attributes.kanji.str;
				test_word_br = global::colors.kanji;

				answer_placeholder = answer_hiragana;
				answer_br = global::colors.hiragana;
			} break;
			case decltype(practice->practice_type)::kanji_to_translation:
			{
				test_word = (utf16*)practice->word.attributes.kanji.str;
				test_word_br = global::colors.kanji;

				answer_placeholder = RS(380);
				answer_br = global::colors.translation;
			} break;
			case decltype(practice->practice_type)::translation_to_hiragana:
			{
				test_word = (utf16*)practice->word.attributes.translation.str;
				test_word_br = global::colors.translation;

				answer_placeholder = answer_hiragana;
				answer_br = global::colors.hiragana;
			} break;
			default:Assert(0);
			}

			SendMessageW(controls.list.static_test_word, WM_SETTEXT, 0, (LPARAM)test_word);
			static_oneline::set_brushes(controls.list.static_test_word, TRUE, test_word_br, 0, 0, 0, 0, 0);
			
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

			edit_oneline::set_theme(controls.list.edit_answer, &editoneline_theme);

			SendMessageW(controls.list.edit_answer, WM_SETDEFAULTTEXT, 0, (LPARAM)answer_placeholder.c_str());
			edit_oneline::maintain_placerholder_when_focussed(controls.list.edit_answer,true);

			button::Theme btn_theme;
			btn_theme.brushes.bk.normal = global::colors.ControlBk;
			btn_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			btn_theme.brushes.bk.clicked = global::colors.ControlBkPush;
			btn_theme.brushes.border.normal = global::colors.ControlBk;
			btn_theme.brushes.foreground.normal = global::colors.Img;
			button::set_theme(controls.list.button_next, &btn_theme);

			EnableWindow(controls.list.button_show_word, FALSE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->word);

		} break;
		case decltype(page)::review_practice:
		{
			std::vector<ProcState::practice_header*>* practices = (decltype(practices))data;
			auto controls = state->controls.review_practice;

			//free current elements before switching, I think this is the best place to do it
			clear_practices_vector(state->pagestate.practice_review.practices);

			state->pagestate.practice_review.practices = std::move(*practices);
			size_t practices_cnt = state->pagestate.practice_review.practices.size(); Assert(practices_cnt != 0);
			void** practices_data = (void**)malloc(sizeof(void*) * practices_cnt); defer{ free(practices_data); };
			for (size_t i = 0; i < practices_cnt; i++) practices_data[i] = state->pagestate.practice_review.practices[i];
			gridview::set_elements(controls.list.gridview_practices, practices_data, practices_cnt);

		} break;
		case decltype(page)::review_practice_writing:
		{
			ProcState::practice_writing* pagedata = (decltype(pagedata))data;
			auto& controls = state->controls.practice_writing;
			utf16* question = pagedata->correct_answer->str;
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
			//TODO(fran): set up all the colors and text, (use pagedata->practice->practice_type to know text colors)

			switch (pagedata->practice->practice_type) {//TODO(fran): should be a common function call
			case decltype(pagedata->practice->practice_type)::hiragana_to_translation:
			{
				question_br = global::colors.hiragana;
			} break;
			case decltype(pagedata->practice->practice_type)::kanji_to_hiragana:
			{
				question_br = global::colors.kanji;
			} break;
			case decltype(pagedata->practice->practice_type)::kanji_to_translation:
			{
				question_br = global::colors.kanji;
			} break;
			case decltype(pagedata->practice->practice_type)::translation_to_hiragana:
			{
				question_br = global::colors.translation;
			} break;
			default:Assert(0);
			}
			
			SendMessageW(controls.list.static_test_word, WM_SETTEXT, 0, (LPARAM)question);
			static_oneline::set_brushes(controls.list.static_test_word, TRUE, question_br, 0, 0, 0, 0, 0);

			//TODO(fran): controls.list.edit_answer should be disabled

			edit_oneline::Theme editoneline_theme;
			editoneline_theme.brushes.foreground.normal = global::colors.ControlTxt;
			editoneline_theme.brushes.bk.normal = answer_bk.normal;
			editoneline_theme.brushes.border.normal = answer_bk.normal;

			edit_oneline::set_theme(controls.list.edit_answer, &editoneline_theme);

			SendMessageW(controls.list.edit_answer, WM_SETTEXT, 0, (LPARAM)pagedata->user_answer.str);

			button::Theme btn_next_theme;
			btn_next_theme.brushes.bk = answer_bk;
			btn_next_theme.brushes.border = answer_bk;
			btn_next_theme.brushes.foreground.normal = global::colors.ControlTxt;
			button::set_theme(controls.list.button_next, &btn_next_theme);

			EnableWindow(controls.list.button_show_word, TRUE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->word);

		} break;
		case decltype(page)::practice_multiplechoice:
		{
			practice_multiplechoice_word* practice = (decltype(practice))data;
			auto controls = state->controls.practice_multiplechoice;
			state->pagestate.practice_multiplechoice.practice = practice;
			
			HBRUSH question_txt_br = brush_for_learnt_word_elem(practice->question_type);
			HBRUSH choice_txt_br = brush_for_learnt_word_elem(practice->choices_type);


			SendMessageW(controls.list.static_question, WM_SETTEXT, 0, (LPARAM)practice->question_str);
			static_oneline::set_brushes(controls.list.static_question, TRUE, question_txt_br, 0, 0, 0, 0, 0);


			multibutton::set_buttons(controls.list.multibutton_choices, practice->choices);
			button::Theme multibutton_button;
			multibutton_button.brushes.foreground.normal = choice_txt_br;
			multibutton_button.brushes.bk.normal = global::colors.ControlBk;
			multibutton_button.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			multibutton_button.brushes.bk.clicked = global::colors.ControlBkPush;
			multibutton_button.brushes.border.normal = global::colors.Img;//TODO(fran): = choice_txt_br ?
			multibutton::set_button_theme(controls.list.multibutton_choices, &multibutton_button);
			

			button::Theme button_next_theme;
			button_next_theme.brushes.bk.normal = global::colors.ControlBk;
			button_next_theme.brushes.border.normal = global::colors.Img;
			button_next_theme.brushes.foreground.normal = global::colors.Img;
			button_next_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			button_next_theme.brushes.bk.clicked = global::colors.ControlBkPush;
			button::set_theme(controls.list.button_next, &button_next_theme);


			EnableWindow(controls.list.button_show_word, FALSE);


			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->question);

		} break;
		case decltype(page)::review_practice_multiplechoice:
		{
			ProcState::practice_multiplechoice* pagedata = (decltype(pagedata))data;
			auto& controls = state->controls.practice_multiplechoice;

			HBRUSH question_txt_br = brush_for_learnt_word_elem(pagedata->practice->question_type);
			HBRUSH choice_txt_br = brush_for_learnt_word_elem(pagedata->practice->choices_type);
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

			SendMessageW(controls.list.static_question, WM_SETTEXT, 0, (LPARAM)pagedata->practice->question_str);
			static_oneline::set_brushes(controls.list.static_question, TRUE, question_txt_br, 0, 0, 0, 0, 0);
			
			//TODO(fran): borders for the right/wrong button dont seem to match when the button is pressed
			//TODO(fran): controls.list.multibutton_choices should be disabled
			multibutton::set_buttons(controls.list.multibutton_choices, pagedata->practice->choices);
			button::Theme multibutton_button_theme;
			multibutton_button_theme.brushes.foreground.normal = choice_txt_br;
			multibutton::set_button_theme(controls.list.multibutton_choices, &multibutton_button_theme);

			button::Theme multibutton_user_choice_button_theme;
			multibutton_user_choice_button_theme.brushes.foreground.normal = user_choice_txt_br;
			multibutton_user_choice_button_theme.brushes.bk = user_choice_bk;
			multibutton_user_choice_button_theme.brushes.border = user_choice_bk;
			multibutton::set_button_theme(controls.list.multibutton_choices, &multibutton_user_choice_button_theme,pagedata->user_answer_idx);

			button::Theme btn_next_theme;
			btn_next_theme.brushes.bk.normal = user_choice_bk.normal;
			btn_next_theme.brushes.border.normal = user_choice_bk.normal;
			btn_next_theme.brushes.foreground.normal = global::colors.Img;
			button::set_theme(controls.list.button_next, &btn_next_theme);

			EnableWindow(controls.list.button_show_word, TRUE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->question);

		} break;
		case decltype(page)::practice_drawing:
		{
			practice_drawing_word* practice = (decltype(practice))data;
			auto controls = state->controls.practice_drawing;
			state->pagestate.practice_drawing.practice = practice;

			HBRUSH question_txt_br = brush_for_learnt_word_elem(practice->question_type);

			SendMessageW(controls.list.static_question, WM_SETTEXT, 0, (LPARAM)practice->question_str);
			static_oneline::set_brushes(controls.list.static_question, TRUE, question_txt_br, 0, 0, 0, 0, 0);

			button::Theme button_next_theme;
			button_next_theme.brushes.bk.normal = global::colors.ControlBk;
			button_next_theme.brushes.border.normal = global::colors.Img;
			button_next_theme.brushes.foreground.normal = global::colors.Img;
			button_next_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
			button_next_theme.brushes.bk.clicked = global::colors.ControlBkPush;
			button::set_theme(controls.list.button_next, &button_next_theme);

			SendMessageW(controls.list.static_correct_answer, WM_SETTEXT, 0, (LPARAM)practice->question.attributes.kanji.str /*TODO(fran): add answer_str*/);
			

			EnableWindow(controls.list.paint_answer, TRUE);
			EnableWindow(controls.list.button_show_word, FALSE);
			EnableWindow(controls.list.button_next, FALSE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &practice->question);

		} break;
		case decltype(page)::review_practice_drawing:
		{
			ProcState::practice_drawing* pagedata = (decltype(pagedata))data;
			auto& controls = state->controls.practice_drawing;

			HBRUSH question_txt_br = brush_for_learnt_word_elem(pagedata->practice->question_type);
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

			SendMessageW(controls.list.static_question, WM_SETTEXT, 0, (LPARAM)pagedata->practice->question_str);
			static_oneline::set_brushes(controls.list.static_question, TRUE, question_txt_br, 0, 0, 0, 0, 0);

			paint::clear_canvas(controls.list.paint_answer);
			paint::set_placeholder(controls.list.paint_answer, pagedata->user_answer);

			button::Theme btn_next_theme;
			btn_next_theme.brushes.bk = user_choice_bk;
			btn_next_theme.brushes.border = user_choice_bk;
			btn_next_theme.brushes.foreground.normal = global::colors.Img;
			button::set_theme(controls.list.button_next, &btn_next_theme);

			SendMessageW(controls.list.static_correct_answer, WM_SETTEXT, 0, (LPARAM)pagedata->practice->question.attributes.kanji.str /*TODO(fran): add answer_str*/);

			EnableWindow(controls.list.paint_answer, FALSE);
			EnableWindow(controls.list.button_show_word, TRUE);
			EnableWindow(controls.list.button_next, TRUE);

			embedded::show_word_reduced::set_word(controls.embedded_show_word_reduced, &pagedata->practice->question);

		} break;

		default: Assert(0);
		}
	}

	void searchbox_func_perform_search(void* element, bool is_element, void* user_extra) {
		ProcState* state = get_state((HWND)user_extra);
		if (state && state->current_page == ProcState::page::search) {
			//TODO(fran): differentiate implementation for is_element true or false once *element isnt always a utf16*
			//utf16* search;
			learnt_word_elem search_type;

			//TODO(fran): search can be for kanji and meaning too, we must first find out which one it is, maybe that should be built in like a search_word struct that has learnt_word and search_type

			learnt_word16 search{0};

			if (is_element) {
				//TODO(fran): at this point we already know which word to find on the db, the problem is we dont have its ID, we gotta search by string (hiragana) again, we need to retrieve and store IDs on the db. And add a function get_word(ID)
				search = *(decltype(&search))element;
				search_type = decltype(search_type)::hiragana;
			}
			else {
				//NOTE: here we dont know the "ID" so the search will simply take the first word it finds that matches the requirements //TODO(fran): we could present multiple results and ask the user which one to open
				search_type = state->pagestate.search.search_type;
				switch (search_type) {
				case decltype(search_type)::hiragana: search.attributes.hiragana = *((utf16_str*)element); break;
				case decltype(search_type)::kanji: search.attributes.kanji = *((utf16_str*)element); break;
				case decltype(search_type)::meaning: search.attributes.translation= *((utf16_str*)element); break;
				default:Assert(0);//TODO
				}
				//search = ((utf16_str*)element)->str;
			}

			get_word_res res = get_word(state->settings->db, search_type, str_for_learnt_word_elem(&search,search_type).str); defer{ free_get_word(res.word); };
			if (res.found) {
				preload_page(state, ProcState::page::show_word, &res.word);//NOTE: considering we no longer have separate pages this might be a better idea than sending the struct at the time of creating the window
				store_previous_page(state, state->current_page);
				set_current_page(state, ProcState::page::show_word);
			}
			else {
				int ret = MessageBoxW(state->nc_parent, RCS(300), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
				if (ret == IDYES) {
					//learnt_word2<utf16_str> new_word{ 0 };
					//new_word.attributes.hiragana = { search, (cstr_len(search)+1)*sizeof(*search) };
					//preload_page(state, ProcState::page::new_word, &new_word);
					preload_page(state, ProcState::page::new_word, &search);
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::new_word);
				}
				else SetFocus(state->controls.search.list.searchbox_search);//Restore focus to the edit window since messagebox takes it away 
				//TODO(fran): a way to make this more streamlined would be to implement:
				//MessageBoxW(){oldfocus=getfocus(); messagebox(); setfocus(oldfocus);}
			}
		}
	}

	bool modify_word(ProcState* state) {
		bool res = false;
		if (check_show_word(state)) {
			learnt_word16 w16;
			auto& page = state->controls.show_word;

			_get_edit_str(page.list.static_hiragana, w16.attributes.hiragana);
			_get_edit_str(page.list.edit_kanji, w16.attributes.kanji);
			_get_edit_str(page.list.edit_translation, w16.attributes.translation);
			_get_edit_str(page.list.edit_mnemonic, w16.attributes.mnemonic);
			_get_combo_sel_idx_as_str(page.list.combo_lexical_category, w16.attributes.lexical_category);

			learnt_word8 w8; defer{ for (auto& _ : w8.all)free_any_str(_.str); };
			for (int i = 0; i < ARRAYSIZE(w16.all); i++) {
				w8.all[i] = convert_utf16_to_utf8(w16.all[i].str, (int)w16.all[i].sz);
				free_any_str(w16.all[i].str);//maybe set it to zero too
			}

			res = update_word(state->settings->db, &w8);
		}
		return res;
	}

	bool remove_word(ProcState* state) {
		bool res = false;
		learnt_word16 word16{ 0 };

		auto& page = state->controls.show_word;

		_get_edit_str(page.list.static_hiragana, word16.attributes.hiragana); //TODO(fran): should check for valid values? I feel it's pointless in this case
		defer{ free_any_str(word16.attributes.hiragana.str); };

		learnt_word8 word8; //TODO(fran): maybe converting the attributes in place would be nicer, also we could iterate over all the members and check for valid pointers and only convert those

		word8.attributes.hiragana = convert_utf16_to_utf8(word16.attributes.hiragana.str, (int)word16.attributes.hiragana.sz);
		defer{ free_any_str(word8.attributes.hiragana.str); };

		res = delete_word(state->settings->db, &word8);
		return res;
	}

	bool prioritize_word(ProcState* state) {
		//TODO(fran): should accept any word, not manually take it from the UI
		bool res = false;
		learnt_word16 w16{ 0 };

		auto& page = state->controls.show_word;

		_get_edit_str(page.list.static_hiragana, w16.attributes.hiragana); defer{ free_any_str(w16.attributes.hiragana.str); };

		res = reset_word_priority(state->settings->db,&w16);

		return res;
	}

	bool save_new_word(ProcState* state) {
		bool res = false;
		if (check_new_word(state)) {
			learnt_word16 w16;
			auto& page = state->controls.new_word;

			_get_edit_str(page.list.edit_hiragana, w16.attributes.hiragana);
			_get_edit_str(page.list.edit_kanji, w16.attributes.kanji);
			_get_edit_str(page.list.edit_translation, w16.attributes.translation);
			_get_edit_str(page.list.edit_mnemonic, w16.attributes.mnemonic);
			_get_combo_sel_idx_as_str(page.list.combo_lexical_category, w16.attributes.lexical_category);

			learnt_word8 w8;
			for (int i = 0; i < ARRAYSIZE(w16.all); i++) {
				auto res = convert_utf16_to_utf8(w16.all[i].str, (int)w16.all[i].sz);
				w8.all[i] = res;
				free_any_str(w16.all[i].str);//maybe set it to zero too
			}
			defer{ for (auto& _ : w8.all)free_any_str(_.str); };
			//Now we can finally do the insert, TODO(fran): see if there's some way to go straight from utf16 to the db, and to send things like ints without having to convert them to strings
			int insert_res = insert_word(state->settings->db, &w8);

			//Error handling
			switch (insert_res) {
			case SQLITE_OK: { res = true; } break;
			case SQLITE_CONSTRAINT:
			{
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

					HWND べんきょう_nc = CreateWindowEx(WS_EX_CONTROLPARENT, nonclient::wndclass, global::app_name, WS_VISIBLE | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						べんきょう_nc_rc.left, べんきょう_nc_rc.top, RECTWIDTH(べんきょう_nc_rc), RECTHEIGHT(べんきょう_nc_rc), nullptr, nullptr, GetModuleHandleW(NULL), &べんきょう_nclpparam);
					Assert(べんきょう_nc);

					べんきょう::set_brushes(nonclient::get_state(べんきょう_nc)->client, TRUE, global::colors.ControlBk);
					べんきょう::set_current_page(べんきょう::get_state(nonclient::get_state(べんきょう_nc)->client), ProcState::page::show_word);

					auto old_w16 = (utf16_str)convert_utf8_to_utf16((utf8*)w8.attributes.hiragana.str, (int)w8.attributes.hiragana.sz); defer{ free_any_str(old_w16.str); };//HACK: reconverting to utf16 for searching the old word
					get_word_res old_word = get_word(state->settings->db,learnt_word_elem::hiragana, old_w16.str); defer{ free_get_word(old_word.word); };
					べんきょう::preload_page(べんきょう::get_state(nonclient::get_state(べんきょう_nc)->client), ProcState::page::show_word, &old_word.word);
					UpdateWindow(べんきょう_nc);
					//TODO(fran): this window needs to be created without the privilege of being able to quit the program, either we let it know it is a secondary window or we do smth else idk what
				}

				int ret = MessageBoxW(state->nc_parent, RCS(170), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
				if (ret == IDYES) {
					res = update_word(state->settings->db, &w8);
				}

			} break;
			default: { sqlite_runtime_check(false, state->settings->db); } break;
			}
			//TODO(fran): maybe handle repeated words here
		}
		return res;
	}

	//TODO(fran): use get_hiragana_kanji_meaning, and rename to has_hiragana_kanji_meaning
	bool has_hiragana(const learnt_word16* word) {
		bool res;
		res = word && word->attributes.hiragana.str && *word->attributes.hiragana.str;//TODO(fran): learnt_word should have a defined str type, we cant be guessing here
		return res;
	}

	bool has_translation(const learnt_word16* word) {
		bool res;
		res = word && word->attributes.translation.str && *word->attributes.translation.str;//TODO(fran): learnt_word should have a defined str type, we cant be guessing here
		return res;
	}

	bool has_kanji(const learnt_word16* word) {
		bool res;
		res = word && word->attributes.kanji.str && *word->attributes.kanji.str;//TODO(fran): learnt_word should have a defined str type, we cant be guessing here
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
				if (w->attributes.translation.str && *w->attributes.translation.str) res |= (u32)learnt_word_elem::meaning;
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
			bool hiragana = has_hiragana(&data->word), translation = has_translation(&data->word), kanji = has_kanji(&data->word);
			practice_types.push_back(practice_writing_word::type::hiragana_to_translation);//we always add a default to make sure there can never be no items
			if (hiragana && translation) practice_types.push_back(practice_writing_word::type::translation_to_hiragana);
			if (hiragana && kanji) practice_types.push_back(practice_writing_word::type::kanji_to_hiragana);
			if (kanji && translation) practice_types.push_back(practice_writing_word::type::kanji_to_translation);

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
				if (i == data->idx_answer) data->choices[i] = _wcsdup(str_for_learnt_word_elem((learnt_word16*)&data->question, data->choices_type).str);//TODO(fran): replace for my own duplication method
				else data->choices[i] = _choices[j++];
			}

			data->question_str = str_for_learnt_word_elem(&data->question, data->question_type).str;
			
			res.page = ProcState::page::practice_multiplechoice;
			res.data = data;
		} break;
		case ProcState::available_practices::drawing:
		{
			practice_drawing_word* data = (decltype(data))malloc(sizeof(*data));
			learnt_word16 practice_word = get_practice_word(state->settings->db, false,true,false);//get a target word
			//TODO(fran): check the word is valid, otherwise recursively call this function
			data->question = std::move(practice_word);
			
			u32 q_elems = get_hiragana_kanji_meaning(&data->question);
			data->question_type = (decltype(data->question_type))random_bit_set(q_elems & (~(u32)learnt_word_elem::kanji));
			data->question_str = str_for_learnt_word_elem(&data->question, data->question_type).str;

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
		//NOTE: one solution here would be to destroy all the controls remove_controls(page) and then call add_controls(page)
		switch (page) {
		case decltype(page)::new_word:
		{
			//NOTE: the problem here is that its not enough to settext to null to everyone, that can be done fairly easily implementing some WM_RESET msg, but there are some controls that shouldnt be reset like the buttons, we'd need to implement a .reseteable() or somehow via union magic or smth that gives us only the controls that should be reset
			auto& controls = state->controls.new_word;
			_clear_combo_sel(controls.list.combo_lexical_category);
			_clear_edit(controls.list.edit_hiragana);
			_clear_edit(controls.list.edit_kanji);
			_clear_edit(controls.list.edit_mnemonic);
			_clear_edit(controls.list.edit_translation);
		} break;
		case decltype(page)::practice_writing:
		{
			auto& controls = state->controls.practice_writing;

			_clear_static(controls.list.static_test_word);
			_clear_edit(controls.list.edit_answer);
			//TODO(fran): I really need to destroy the controls and call add_controls, I cant be restoring colors here when that's already done there
		} break;
		case decltype(page)::practice_multiplechoice:
		{
			auto& controls = state->controls.practice_multiplechoice;
			//TODO(fran): do we wanna clear smth here?
		} break;
		case decltype(page)::practice_drawing:
		{
			auto& controls = state->controls.practice_drawing;
			paint::clear_canvas(controls.list.paint_answer);
			paint::set_placeholder(controls.list.paint_answer, nullptr);
		} break;

		default:Assert(0);
		}
	}

	void user_stats_increment_times_practiced(sqlite3* db) {
		utf8 update_increment_times_practiced[] = SQL(
			UPDATE _べんきょう_table_user SET times_practiced = times_practiced + 1;
		);
		utf8* errmsg;
		sqlite3_exec(db, update_increment_times_practiced, 0, 0, &errmsg);
		sqlite_exec_runtime_assert(errmsg);
	}

	void _next_practice_level(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
		ProcState* state = get_state(hwnd);
		KillTimer(state->wnd, anim_id);

		state->practice_cnt--;
		if (state->practice_cnt > 0) {
			practice_data practice = prepare_practice(state);//get a random practice
			reset_page(state, practice.page);
			preload_page(state, practice.page, practice.data);//load the practice
			set_current_page(state, practice.page);//go practice!
		}
		else {
			//TODO(fran): increase the practice counter on the db
			user_stats_increment_times_practiced(state->settings->db);
			preload_page(state, ProcState::page::review_practice,&state->multipagestate.temp_practices);
			set_current_page(state, ProcState::page::review_practice);
		}
	}

	//decreases the practice counter and loads/sets a new practice level or goes to the review page if the practice is over
	void next_practice_level(ProcState* state, u32 ms_delay = USER_TIMER_MAXIMUM) {
		u32 delay = ms_delay != USER_TIMER_MAXIMUM ? ms_delay : 500;
		SetTimer(state->wnd, TIMER_next_practice_level, delay, _next_practice_level);
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			decltype(state) st = (decltype(st))calloc(1, sizeof(decltype(*st)));
			Assert(st);
			st->nc_parent = GetParent(hwnd);
			st->wnd = hwnd;
			st->settings = ((べんきょうSettings*)create_nfo->lpCreateParams);
			st->current_page = ProcState::page::landing;
			init_cpp_objects(st);
			set_state(hwnd, st);//TODO(fran): now I wonder why I append everything with the name of the wnd when I can simply use function overloading, even for get_state since all my functions do the same I can simple have one
			startup(st->settings->db);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			add_controls(state);

			set_current_page(state, ProcState::page::landing);//TODO(fran): page:: would be nicer than ProcState::page::

			if constexpr(0) {//TODO(fran): now we have a way of detecting if japanese keyboard is present, now we need to change to it when we want
				int sz_elem = GetKeyboardLayoutList(0, 0);
				ptr<HKL>layouts; layouts.alloc(sz_elem); defer{ layouts.free(); };
				int res = GetKeyboardLayoutList(sz_elem, layouts.mem); Assert(res == sz_elem);
				HKL currenthkl = GetKeyboardLayout(0);
				for (const auto& l : layouts) {
					ActivateKeyboardLayout(l, KLF_SETFORPROCESS);
					char layoutname[KL_NAMELENGTH]; GetKeyboardLayoutNameA(layoutname);
					printf("%s\n", HKLtoString(layoutname));
				}
				ActivateKeyboardLayout(currenthkl, KLF_SETFORPROCESS);
			}

			return 0;
		} break;
		case WM_SIZE:
		{
			LRESULT res = DefWindowProc(hwnd, msg, wparam, lparam);
			resize_controls(state);
			return res;
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
				{
					auto& page = state->controls.landingpage;
					if (child == page.list.button_new) {
						store_previous_page(state, state->current_page);
						reset_page(state, ProcState::page::new_word);
						set_current_page(state, ProcState::page::new_word);
					}
					else if (child == page.list.button_practice) {
						store_previous_page(state, state->current_page);
						user_stats stats = get_user_stats(state->settings->db);
						get_user_stats_accuracy_timeline(state->settings->db, &stats, 30); defer{ stats.accuracy_timeline.free(); };
						preload_page(state, ProcState::page::practice, &stats);
						set_current_page(state, ProcState::page::practice);
					}
					else if (child == page.list.button_search) {
						store_previous_page(state, state->current_page);
						set_current_page(state, ProcState::page::search);
					}
					//else if (child == page.list.combo_languages) {
					//	WORD notif = HIWORD(wparam);
					//	switch (notif) {
					//	case CBN_SELENDOK:
					//	{
					//		//user requested a language change
					//		//TODO(fran): pretty sure I can reduce all this to _get_edit_str() instead of going through the listbox
					//		i32 cur_sel = (i32)SendMessage(child, CB_GETCURSEL, 0, 0);
					//		if (cur_sel != CB_ERR) {
					//			utf16_str lang = alloc_any_str(sizeof(*lang.str) * (SendMessage(child, CB_GETLBTEXTLEN, cur_sel, 0) + 1)); defer{ if(lang.sz) free_any_str(lang.str); };
					//			if (lang.sz) {
					//				SendMessage(child, CB_GETLBTEXT, cur_sel, (LPARAM)lang.str);
					//				LANGUAGE_MANAGER::Instance().ChangeLanguage(lang.str);
					//			}
					//		}

					//	} break;
					//	}
					//}
				} break;
				case ProcState::page::new_word:
				{
					auto& page = state->controls.new_word;
					if (child == page.list.button_save) {
						if (save_new_word(state)) {
							//If the new word was successfully saved then go back to the landing, TODO(fran): a better idea may be to reset the new_word page, so that the user can add multiple words faster
							store_previous_page(state, state->current_page);
							set_current_page(state, ProcState::page::landing);
							//TODO(fran): clear new_word controls, now this one's annoying cause not everything is cleared the same way
						}
					}
				} break;
				case ProcState::page::practice:
				{
					auto& page = state->controls.practice;
					if (child == page.list.button_start) {
						
						i64 word_cnt = get_user_stats(state->settings->db).word_cnt; //TODO(fran): I dont know if this is the best way to do it but it is the most accurate
						if (word_cnt > 0) {
#define TEST_QUICKPRACTICE
#if !defined(TEST_QUICKPRACTICE) && defined(_DEBUG)
							constexpr int practice_cnt = 15 + 1;//TODO(fran): I dont like this +1
#else
							constexpr int practice_cnt = 2 + 1;
#endif
							state->practice_cnt = (u32)min(word_cnt, practice_cnt);//set the practice counter (and if there arent enough words reduce the practice size, not sure how useful this is)
							store_previous_page(state, state->current_page);

							//Clear previous practice data:
							clear_practices_vector(state->multipagestate.temp_practices);

							next_practice_level(state, USER_TIMER_MINIMUM);
						}
						else MessageBoxW(state->wnd, RCS(360), 0, MB_OK, MBP::center);
					}
				} break;
				case ProcState::page::search:
				{
					auto& page = state->controls.search;
					//NOTE: for now the only thing on this page is the searchbox, which is handled in a decentralized fashion
				} break;
				case ProcState::page::show_word:
				{
					auto& page = state->controls.show_word;
					if (child == page.list.button_modify) {
						if (modify_word(state)) {
							goto_previous_page(state);
							//set_current_page(state, ProcState::page::search);//TODO(fran): this should be a "go back" once we have the back button and a "go back" queue(or similar) set up
						}
					}
					if (child == page.list.button_delete) {

						int ret = MessageBoxW(state->nc_parent, RCS(280), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL,MBP::center);
						if (ret == IDYES) {
							if (remove_word(state)) {
								goto_previous_page(state);
								//set_current_page(state, ProcState::page::search);//TODO(fran): this should also be go back, we can get to show_word from new_word
							}
						}
					}
					if (child == page.list.button_remember) {
						//TODO(fran): change button bk and mouseover color to green
						if (prioritize_word(state)) {
							button::Theme accent_btn_theme;
							accent_btn_theme.brushes.foreground.normal = global::colors.Bk_right_answer;
							accent_btn_theme.brushes.border.normal = global::colors.Bk_right_answer;
							button::set_theme(page.list.button_remember, &accent_btn_theme);
						}
					}
				} break;
				case ProcState::page::practice_writing:
				{
					auto& page = state->controls.practice_writing;
					auto& pagestate = state->pagestate.practice_writing;
					WORD notif = HIWORD(wparam);
					if (child == page.list.button_next || (child == page.list.edit_answer && notif == EN_ENTER)) {
						bool already_answered = IsWindowEnabled(page.list.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
						if (!already_answered) {
							utf16_str user_answer; _get_edit_str(page.list.edit_answer, user_answer);
							if (user_answer.sz_char() != 1) {
								const utf16_str* correct_answer{ 0 };
								const utf16_str* question{ 0 };//TODO(fran); this should already come calculated at this point
								switch (pagestate.practice->practice_type) {
								case decltype(pagestate.practice->practice_type)::hiragana_to_translation:
								{
									correct_answer = &pagestate.practice->word.attributes.translation;
									question = &pagestate.practice->word.attributes.hiragana;
								} break;
								case decltype(pagestate.practice->practice_type)::kanji_to_translation:
								{
									correct_answer = &pagestate.practice->word.attributes.translation;
									question = &pagestate.practice->word.attributes.kanji;
								} break;
								case decltype(pagestate.practice->practice_type)::kanji_to_hiragana:
								{
									correct_answer = &pagestate.practice->word.attributes.hiragana;
									question = &pagestate.practice->word.attributes.kanji;

								} break;
								case decltype(pagestate.practice->practice_type)::translation_to_hiragana:
								{
									correct_answer = &pagestate.practice->word.attributes.hiragana;
									question = &pagestate.practice->word.attributes.translation;
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

								edit_oneline::set_theme(page.list.edit_answer, &editoneline_theme);

								button::Theme btn_theme;
								btn_theme.brushes.bk.normal = bk;
								btn_theme.brushes.bk.mouseover = bk_mouseover;
								btn_theme.brushes.bk.clicked = bk_push;
								btn_theme.brushes.border.normal = bk;
								btn_theme.brushes.border.mouseover = bk_mouseover;
								btn_theme.brushes.border.clicked = bk_push;
								btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
								button::set_theme(page.list.button_next, &btn_theme);

								//TODO(fran): block input to the edit and btn controls, we dont want the user to be inputting new values or pressing next multiple times

								//Update word stats
								//TODO(fran): if we knew we had the old values in the word object we could simply update those and batch update the whole word
								word_update_last_shown_date(state->settings->db, pagestate.practice->word);
								word_increment_times_shown__times_right(state->settings->db, pagestate.practice->word, answered_correctly);

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

								EnableWindow(page.list.button_show_word, TRUE);
								if (answered_correctly) next_practice_level(state);
							}
							else {
								free_any_str(user_answer.str);
							}
						}
						else {
							//The user already answered and got it wrong, they checked what was wrong and pressed continue again
							next_practice_level(state);
						}
					}
					else if (child == page.list.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}
				} break;
				case ProcState::page::review_practice:
				{
					auto& page = state->controls.review_practice;
					if (child == page.list.button_continue) {
						store_previous_page(state, state->current_page);
						//user_stats stats = get_user_stats(state->settings->db);
						//get_user_stats_accuracy_timeline(state->settings->db, &stats, 30); defer{ stats.accuracy_timeline.free(); };//TODO(fran): this things should be joined into a function, I've already got to do this same code twice
						//preload_page(state, ProcState::page::practice, &stats);
						//set_current_page(state, ProcState::page::practice);
						set_current_page(state, ProcState::page::landing);//TODO(fran): this isnt best, it'd be nice if we went back to the practice page but from here it'd require a goto_previous_page and the fact that we know prev page is practice and that we should preload cause it's values have changed
					} 
					else if (child == page.list.gridview_practices) {
						//The user clicked an element
						void* data = (void*)wparam;
						ProcState::practice_header* header = (decltype(header))data;
						switch (header->type) {//TODO(fran): pointless code repetition, why would we do smth different for each case
						case decltype(header->type)::writing:
						{
							ProcState::practice_writing* pagedata = (decltype(pagedata))data;
							preload_page(state, ProcState::page::review_practice_writing, pagedata);
							store_previous_page(state, state->current_page);
							set_current_page(state, ProcState::page::review_practice_writing);

							//IDEA: implement ocarina of time's concept of a scene flag, the same page can be loaded in different scenes, eg on a practice page we'd have the default scene where we'd setup default colors, then the wrong_answer scene where we show a button or smth to allow the user to see the correct answer, also here we no longer accept new keyboard input beyond WM_ENTER, we also have the right_answer scene where we simply change colors and disable all user input since a timer will automatically change page. 
							//IMPORTANT IDEA: we want individual/independent pages in the case we do reviews and similar things where we simply want to show the data but dont care about user input, eg the user presses an element in the gridview and we create a new wnd, create the corresponding controls, set them up (colors, disabled, etc) and show that window as a separate entity, that's what we want, to call add_controls on a different window and simply give it to the user, whenever the user's done they can close the window, meanwhile they can still interact with the main window, I think this is really good, the user could for example open all the words they got wrong at the same time, instead of having to open one, go back to the grid, open another; the one problem with this is where to show the new window, say we have 5 windows open, how do we choose where to show the sixth

						} break;
						case decltype(header->type)::multiplechoice:
						{
							ProcState::practice_multiplechoice* pagedata = (decltype(pagedata))data;
							preload_page(state, ProcState::page::review_practice_multiplechoice, pagedata);
							store_previous_page(state, state->current_page);
							set_current_page(state, ProcState::page::review_practice_multiplechoice);
						} break;
						case decltype(header->type)::drawing:
						{
							ProcState::practice_drawing* pagedata = (decltype(pagedata))data;
							preload_page(state, ProcState::page::review_practice_drawing, pagedata);
							store_previous_page(state, state->current_page);
							set_current_page(state, ProcState::page::review_practice_drawing);
						} break;
						default: Assert(0);
						}
					}
				} break;
				case ProcState::page::review_practice_writing:
				{
					auto& page = state->controls.practice_writing;
					if (child == page.list.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}
					else if (child == page.list.button_next) {
						goto_previous_page(state);
					}
				} break;
				case ProcState::page::practice_multiplechoice:
				{
					auto& page = state->controls.practice_multiplechoice;
					auto& pagestate = state->pagestate.practice_multiplechoice;
					bool already_answered = IsWindowEnabled(page.list.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
					if (child == page.list.multibutton_choices) {
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
							multibutton::set_button_theme(page.list.multibutton_choices, &multibtn_btn_theme, user_answer_idx);
							//TODO(fran): when the user answers incorrectly we want to visually show the correct answer too, maybe with a lower brightness global::colors.Bk_right_answer or a yellow

							button::Theme btn_theme;
							btn_theme.brushes.bk.normal = bk;
							btn_theme.brushes.bk.mouseover = bk_mouseover;
							btn_theme.brushes.bk.clicked = bk_push;
							btn_theme.brushes.border.normal = bk;
							btn_theme.brushes.border.mouseover = bk_mouseover;
							btn_theme.brushes.border.clicked = bk_push;
							btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
							button::set_theme(page.list.button_next, &btn_theme);

							//Update word stats
							word_update_last_shown_date(state->settings->db, pagestate.practice->question);
							word_increment_times_shown__times_right(state->settings->db, pagestate.practice->question, answered_correctly);

							//Add this practice to the list of current completed ones
							ProcState::practice_multiplechoice* p = (decltype(p))malloc(sizeof(*p));
							p->header.type = decltype(p->header.type)::multiplechoice;
							p->practice = pagestate.practice;
							pagestate.practice = nullptr;//clear the pointer just in case
							p->answered_correctly = answered_correctly;
							p->user_answer_idx = user_answer_idx;

							state->multipagestate.temp_practices.push_back((ProcState::practice_header*)p);

							EnableWindow(page.list.button_show_word, TRUE);
							if (answered_correctly) next_practice_level(state);
						}
					}
					else if (child == page.list.button_next) {
						if (already_answered) {
							next_practice_level(state);
						}
					}
					else if (child == page.list.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}
					else
					{
						printf("FIX ERROR\n");
						//NOTE: we're getting an EN_KILLFOCUS from the edit control in practice_writing, TODO(fran): do like windows and add a msg to specify which notifications you want to receive from a specific control
					}
				} break;
				case ProcState::page::review_practice_multiplechoice:
				{
					auto& page = state->controls.practice_multiplechoice;
					if (child == page.list.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}
					else if (child == page.list.button_next) {
						goto_previous_page(state);
					}
				} break;
				case ProcState::page::practice_drawing:
				{
					auto& page = state->controls.practice_drawing;
					auto& pagestate = state->pagestate.practice_drawing;
					bool already_answered = IsWindowEnabled(page.list.button_show_word);//HACK, we need a real way to check whether this is the first time the user tried to answer
					bool can_answer = paint::get_stroke_count(page.list.paint_answer);

					if (child == page.list.paint_answer) {//Notifies when a change finished on the canvas (eg the user drew a complete stroke)
						EnableWindow(page.list.button_next, can_answer);
					}
					else if (child == page.list.button_next) {
						if (already_answered) {
							next_practice_level(state);
						}
						else {
							EnableWindow(page.list.paint_answer, FALSE);
							ShowWindow(page.list.static_correct_answer, SW_SHOW);
							ShowWindow(page.list.button_wrong, SW_SHOW);
							ShowWindow(page.list.button_right, SW_SHOW);
						}
					}
					else if (child == page.list.button_right || child == page.list.button_wrong) {
						if (!already_answered) {
							bool answered_correctly = child == page.list.button_right;

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
							button::set_theme(page.list.button_next, &btn_theme);

							//Update word stats
							word_update_last_shown_date(state->settings->db, pagestate.practice->question);
							word_increment_times_shown__times_right(state->settings->db, pagestate.practice->question, answered_correctly);

							//Add this practice to the list of current completed ones
							ProcState::practice_drawing* p = (decltype(p))malloc(sizeof(*p));
							p->header.type = decltype(p->header.type)::drawing;
							p->practice = pagestate.practice;
							pagestate.practice = nullptr;//clear the pointer just in case
							p->answered_correctly = answered_correctly;
							//Get what the user drew
							{
								auto canvas = paint::get_canvas(page.list.paint_answer);//TODO(fran): canvas.bmp could be selected into the paint dc, we need a paint::copy_canvas(RECT) function
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

							EnableWindow(page.list.button_show_word, TRUE);
							if (answered_correctly) next_practice_level(state);
						}
					}
					else if (child == page.list.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}
				} break;
				case ProcState::page::review_practice_drawing:
				{
					auto& page = state->controls.practice_drawing;
					if (child == page.list.button_show_word) {
						flip_visibility(page.embedded_show_word_reduced);
					}
					else if (child == page.list.button_next) {
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
			LRESULT res;
			if (state->brushes.bk) {
				//TODO(fran): idk if WS_CLIPCHILDREN | WS_CLIPSIBLINGS automatically clip that regions when I draw or I have to do it manually to avoid flicker
				HDC dc = (HDC)wparam;
				RECT r; GetClientRect(state->wnd, &r);
				FillRect(dc, &r, state->brushes.bk);
				res = 1;//We erased the bk
			}
			else res = DefWindowProc(hwnd, msg, wparam, lparam);
			return res;
		} break;
		case WM_CTLCOLORLISTBOX: //for combobox list //TODO(fran): this has to go
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(global::colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(global::colors.ControlTxt));

			return (INT_PTR)global::colors.ControlBk;
		} break;
		case WM_CTLCOLORSTATIC: //for the static controls //TODO(fran): this has to go, aka make my own
		{
			HDC listboxDC = (HDC)wparam;
			SetBkColor(listboxDC, ColorFromBrush(global::colors.ControlBk));
			SetTextColor(listboxDC, ColorFromBrush(global::colors.ControlTxt));//TODO(fran): maybe a lighter color to signalize non-editable?

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
			return DefWindowProc(hwnd, msg, wparam, lparam);
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
			//TODO(fran): BUG: probably on the editbox, if you click the searchbox in the navbar and then click on the 'new' button of the landing the cursor will get misplaced
			return DefWindowProc(hwnd, msg, wparam, lparam);
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

		default:
#ifdef _DEBUG
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
		wcex.hbrBackground = 0; //TODO(fran): global::colors.ControlBk hasnt been deserialized by the point pre_post_main gets executed!
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


//- WM_COMMAND page::search old implementation, has info about how windows manages comboboxes, specifically the listbox
//#if 0
//					if (child == page.list.combo_search) {
//						WORD notif = HIWORD(wparam);
//						switch (notif) {
//						case CBN_EDITCHANGE://The user has changed the content of the edit box, this msg is sent after the change is rendered in the control (if we use CBN_EDITUPDATE whick is sent before re-rendering it slows down user input and feels bad to type)
//						{
//
//							//For now we'll get a max of 5 results, the best would be to set the max to reach the bottom of the parent window (aka us) so it can get to look like a full window instead of feeling so empty //TODO(fran): we could also append the "searchbar" in the landing page
//
//							COMBOBOX_clear_list_items(page.list.combo_search);
//
//							int sz_char = (int)SendMessage(page.list.combo_search, WM_GETTEXTLENGTH, 0, 0)+1;
//							if (sz_char > 1) {
//								utf16* search = (decltype(search))malloc(sz_char * sizeof(*search));
//								SendMessage(page.list.combo_search, WM_GETTEXT, sz_char, (LPARAM)search);
//
//								//TODO(fran): this might be an interesting case for multithreading, though idk how big the db has to be before this search is slower than the user writing, also for jp text the IME wont allow for the search to start til the user presses enter, that may be another <-TODO(fran): chrome-like handling for IME, text goes to the edit control at the same time as the IME
//
//								auto search_res = search_word_matches(state->settings->db, search, 5); defer{ free_search_word(search_res); };
//								//TODO(fran): clear the listbox
//
//								//Semi HACK: set first item of the listbox to what the user just wrote, to avoid the stupid combobox from selecting something else
//								//Show the listbox
//								SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, TRUE, 0);
//								SendMessageW(page.list.combo_search, CB_INSERTSTRING, 0, (LPARAM)search);
//
//								if (search_res.cnt) {
//									for (int i = 0; i < search_res.cnt; i++)
//										SendMessageW(page.list.combo_search, CB_INSERTSTRING, -1, (LPARAM)search_res.matches[i]);
//
//									//TODO(fran): for some reason the cb decides to set the selection once we insert some strings, if we try to set it to "no selection" it erases the user's string, terrible, we must fix this from the subclass
//
//#if 1
//									//TODO(fran): why the hell does this make the mouse disappear, just terrible, I really need to make my own one
//									//TODO(fran): not all the items are shown if they are too many, I think there's a msg that increases the number of items to show
//
//									//NOTE: ok now we have even more garbage, the cb sets the selection to the whole word, which means that the next char the user writes _overrides_ the previous one, I really cant comprehend how this is so awful to use, I must be doing something wrong
//									//SendMessage(page.list.combo_search, CB_SETEDITSEL, 0, MAKELONG(sel_start,sel_end));//This is also stupid, you go from DWORD on CB_GETEDITSEL to WORD here, you loose a whole byte of info
//
//									//IMPORTANT INFO: I think that the "autoselection" problem only happens when you call CB_SHOWDROPDOWN, if we called it _before_ adding elements to the list we may be ok, the answer is yes and no, my idea works _but_ the listbox doesnt update its size to accomodate for the items added after showing it, gotta do that manually
//
//									//COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
//									//int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);
//									//int list_h = max((int)SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt,2);
//									//RECT combo_rw; GetWindowRect(page.list.combo_search, &combo_rw);
//									//MoveWindow(nfo.hwndList, combo_rw.left, combo_rw.bottom, RECTWIDTH(combo_rw), list_h, TRUE); //TODO(fran): handle showing it on top of the control if there's no space below
//									//AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
//#else
//									COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
//									int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);//returns -1 on error, but who cares
//									//NOTE: unless LBS_OWNERDRAWVARIABLE style is defined all items have the same height
//									int list_h = max(SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt,2);
//									RECT rw; GetWindowRect(page.list.combo_search, &rw);
//									MoveWindow(nfo.hwndList, rw.left, rw.bottom, RECTWIDTH(rw), list_h, TRUE);
//									SetWindowPos(nfo.hwndList, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);//INFO: here was the problem, the window was being occluded
//									//ShowWindow(nfo.hwndList, SW_SHOWNA);
//									AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
//									SendMessage(nfo.hwndList, 0x1ae/*LBCB_STARTTRACK*/, 0, 0); //does start mouse tracking
//									//NOTE: I think LBCB_ENDTRACK is handled by itself
//#endif
//									//TODO(fran): add something to the default list item or add extra info to the db results to differentiate the default first listbox item from the results
//									//TODO(fran): now we have semi success with the cb, but still the first character comes out a little too late, it feels bad, we probably cant escape creating our own combobox (I assume that happens because animatewindow is happening?)
//									//TODO(fran): actually we can implement this in the subclass, we can add a msg for a non stupid way of showing the listbox, we can probaly handle everything well from there
//								}
//								//else {
//									//Hide the listbox, well... maybe not since we now show the search string on the first item
//									//SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
//								//}
//								//Update listbox size
//								COMBOBOXINFO nfo = { sizeof(nfo) }; GetComboBoxInfo(page.list.combo_search, &nfo);
//								int item_cnt = (int)SendMessage(nfo.hwndList, LB_GETCOUNT, 0, 0);
//								int list_h = max((int)SendMessage(nfo.hwndList, LB_GETITEMHEIGHT, 0, 0) * item_cnt, 2);
//								RECT combo_rw; GetWindowRect(page.list.combo_search, &combo_rw);
//								MoveWindow(nfo.hwndList, combo_rw.left, combo_rw.bottom, RECTWIDTH(combo_rw), list_h, TRUE); //TODO(fran): handle showing it on top of the control if there's no space below
//#if 0
//								AnimateWindow(nfo.hwndList, 200, AW_VER_POSITIVE | AW_SLIDE);//TODO(fran): AW_VER_POSITIVE : AW_VER_NEGATIVE depending on space
//#else
//								//ShowWindow(nfo.hwndList, SW_SHOW);//NOTE: dont call AnimateWindow, for starters SW_SHOW appears to already animate, unfortunately we dont want animation, we want the results as fast as possible, also the animation seems to be performed by the same thread which slows down user input
//#endif
//							}
//							else {
//							//Hide the listbox
//							SendMessage(page.list.combo_search, CB_SHOWDROPDOWN, FALSE, 0);
//							}
//						} break;
//						case CBN_CLOSEUP:
//						{
//							//SendMessage(page.list.combo_search, CB_RESETCONTENT, 0, 0);//Terrible again, what if you just want to clear the listbox alone? we need the poor man's solution:
//							COMBOBOX_clear_list_items(page.list.combo_search);
//						} break;
//						case CBN_SELENDOK://User has selected an item from the list
//						{
//							//TODO(fran): lets hope this gets sent _before_ hiding the list, otherwise I'd have already deleted the elements by this point
//							int sel = (int)SendMessageW(page.list.combo_search, CB_GETCURSEL, 0, 0);
//							if (sel != CB_ERR) {
//								int char_sz = (int)SendMessageW(page.list.combo_search, CB_GETLBTEXTLEN, sel, 0) + 1;
//								any_str word = alloc_any_str(char_sz * sizeof(utf16)); defer{ free_any_str(word.str); };
//								SendMessageW(page.list.combo_search, CB_GETLBTEXT, sel, (LPARAM)word.str);
//								//We got the word, it may or may not be valid, so there are two paths, show error or move to the next page "show_word"
//								get_word_res res = get_word(state->settings->db, (utf16*)word.str); defer{ free_get_word(res); };
//								if (res.found) {
//									preload_page(state, ProcState::page::show_word, &res.word);//NOTE: considering we no longer have separate pages this might be a better idea than sending the struct at the time of creating the window
//									store_previous_page(state, state->current_page);
//									set_current_page(state, ProcState::page::show_word);
//								}
//								else {
//									int ret = MessageBoxW(state->nc_parent, RCS(300), L"", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL, MBP::center);
//									if (ret == IDYES) {
//										learnt_word new_word{ 0 };
//										new_word.attributes.hiragana = word;
//										preload_page(state, ProcState::page::new_word, &new_word);
//										store_previous_page(state, state->current_page);
//										set_current_page(state, ProcState::page::new_word);
//									}
//								}
//							}
//						} break;
//						}
//					}
//#endif
