#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_study_db.h"
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
#include "win32_study_embedded.h"
#include "win32_Sizer.h"
#include "win32_notify.h"
#include "win32_page.h"
#include "win32_Animation.h"
//#include "win32_eyecandy.h"
#include "basic_array.h"
#include "win32_study_common.h"
#include "study_lexical_category.h"
#include "style.h"
#include "study_page_landing_types.h"
#include "study_page_new_word_types.h"
#include "study_page_practice_writing_types.h"
#include "study_page_practice_multiplechoice_types.h"
#include "study_page_practice_drawing_types.h"
#include "study_page_practice_types.h"
#include "study_page_review_practice_types.h"
#include "study_page_show_word_types.h"
#include "study_page_wordbook_types.h"
#include "study_page_wordbook_all_types.h"

namespace べんきょう {
	constexpr cstr wndclass[] = L"win32_wndclass_べんきょう";

	struct {
		i32 next_practice_level = 0x5;
	} constexpr timerIDs;

	enum class page_type {
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
	};

	struct Settings {
#define foreach_べんきょうSettings_member(op) \
		op(RECT, rc,200,200,700,900 ) \
		op(multiflag<practice::available_practices>, practices, practice::filledAvailablePractices ) \
		op(multiflag<practice::writing::variant>, practice_writing_variants, practice::filledPracticeWritingVariants ) \
		op(multiflag<practice::multiplechoice::variant>, practice_multiplechoice_variants, practice::filledPracticeMultiplechoiceVariants ) \
		op(multiflag<practice::drawing::variant>, practice_drawing_variants, practice::filledPracticeDrawingVariants ) \

		foreach_べんきょうSettings_member(_generate_member);
		sqlite3* db;
		bool is_primary_wnd;//TODO(fran): not sure this should go here instead of ProcState

		_generate_default_struct_serialize(foreach_べんきょうSettings_member);
		_generate_default_struct_deserialize(foreach_べんきょうSettings_member);
	};

	struct ProcState {
		HWND wnd;
		HWND nc_parent;
		Settings* settings;

		struct {
			HBRUSH bk;
		} brushes;

		page_type current_page;

		struct prev_page_fifo_queue {
			page_type pages[10];
			u32 cnt;
		} previous_pages;

		i32 practice_cnt;//counter for current completed stages/levels while on a pratice run, gets set to eg 10 and is decremented by -1 with each completed stage, when practice_cnt == 0  the practice ends

		struct {
			using control_type = HWND;
			control_type navbar;
			control_type sidebar;
			control_type page_space;//all pages go inside this one

			landing::page_controls landing;
			new_word::page_controls new_word;
			practice::page_controls practice;
			practice::writing::page_controls practice_writing;
			practice::multiplechoice::page_controls practice_multiplechoice;
			practice::drawing::page_controls practice_drawing;
			practice::review::page_controls review_practice;
			show_word::page_controls show_word;
			wordbook::page_controls wordbook;
			wordbook_all::page_controls wordbook_all;
		} pages;

		struct {
			landing::page_state landing;

			struct search_state { learnt_word_elem search_type; } search;

			practice::page_state practice;
			practice::review::page_state practice_review;
			practice::writing::page_state practice_writing;//TODO(fran): if we already had the entire practices array from the start we could simplify this to a simple size_t idx and the multipage_mem.temp_practices[pagestate.practice_writing.idx]
			practice::multiplechoice::page_state practice_multiplechoice;
			practice::drawing::page_state practice_drawing;
		} pagestate;

		struct {
			std::vector<practice::practice_header*> temp_practices;
		} multipagestate;

		struct {
			anim_number_range word_count, practice_count;
		} pageanim;
	};

	ProcState* get_state(HWND wnd) { return (ProcState*)GetWindowLongPtr(wnd, 0); }
	void set_state(HWND wnd, ProcState* state) { SetWindowLongPtr(wnd, 0, (LONG_PTR)state); }

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }
	void ask_for_resize(ProcState* state) { PostMessage(state->wnd, WM_SIZE, 0, 0); }
	//TODO(fran): add to windows helpers
	void force_repaint(HWND wnd) { PostMessage(wnd, WM_MOUSEMOVE, 0, MAKEWORD(-1, -1)); }

	//NOTE: a null HBRUSH means dont change the current one
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	void save_settings(ProcState* state) {
		RECT rc; GetWindowRect(state->wnd, &rc);
		state->settings->rc = rc;
	}

	HWND create_empty_page(ProcState* state, const page::Theme& theme) {
		HWND page = CreateWindowW(page::wndclass, NULL, WS_CHILD //TODO(fran): WS_CLIPCHILDREN?
			, 0, 0, 0, 0, state->pages.page_space, 0, NULL, NULL);
		Assert(page);
		page::set_theme(page, &theme);
		page::set_scrolling(page, true);
		return page;
	}

	template<typename T>
	T str_for(_learnt_word<T>* word, learnt_word_elem type) {
		T res;
		switch (type) {
		case decltype(type)::hiragana: res = (decltype(res))word->attributes.hiragana; break;
		case decltype(type)::kanji: res = (decltype(res))word->attributes.kanji; break;
		case decltype(type)::meaning: res = (decltype(res))word->attributes.meaning; break;
		default: res = { 0 }; Assert(0);
		}
		return res;
	};

	//NOTE: For these enums the value of dont_care should never be shown
	str word_filter_to_str(word_filter::type filter) { return RS(1100 + filter); } 
	u32 word_filter_str_lang_id(word_filter::type filter) { return 1100 + filter; }
	str word_order_to_str(word_order::type order) { return RS(1000 + order); }
	u32 word_order_str_lang_id(word_order::type order) { return 1000 + order; }

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
		*filter = element_idx != -1 ? (word_filter::type)(word_filter::__first + element_idx) : word_filter::none;
	}

	void render_hiragana_kanji_meaning(HDC dc, rect_i32 r, HBRUSH bk_br, HBRUSH hira_br, HBRUSH kanji_br, HBRUSH meaning_br, learnt_word16* word) {
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
		int icon_x = draw_bitmap_1bpp(global::bmps.dropdown, dc, r, x_pad, icon_br);
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

	void preload_page(ProcState* state, page_type page, void* data);
	void show_page(ProcState* state, page_type p, u32 ShowWindow_cmd);
	void set_current_page(ProcState* state, page_type new_page);
	void reset_page(ProcState* state, page_type page);

	void show_backbtn(ProcState* state, bool show) { PostMessage(state->nc_parent, WM_SHOWBACKBTN, show, 0); }

	void goto_previous_page(ProcState* state) {
		if (state->previous_pages.cnt > 0) {
			set_current_page(state, state->previous_pages.pages[--state->previous_pages.cnt]);
			if (state->previous_pages.cnt == 0) show_backbtn(state, false);
		}
	}

	//TODO(fran): maybe set_current_page should store the page it's replacing into the queue, problem there would be with goto_previous_page, which will cause a store that we dont want, but maybe some better defined functions with the goto_previous distinction in mind could work well
	void store_previous_page(ProcState* state, page_type prev_page) {
		if (state->previous_pages.cnt == ARRAYSIZE(state->previous_pages.pages)) {
			//cnt stays the same
			//we move all the entries one position down and place the new one on top
			memcpy(state->previous_pages.pages, &state->previous_pages.pages[1], (ARRAYSIZE(state->previous_pages.pages) - 1) * sizeof(*state->previous_pages.pages));
			state->previous_pages.pages[ARRAYSIZE(state->previous_pages.pages) - 1] = prev_page;
		}
		else {
			state->previous_pages.pages[state->previous_pages.cnt++] = prev_page;
		}
		show_backbtn(state, true);
	}

	void page_scroll(HWND page_wnd, i32 w, i32 page_space_h, i32 used_h) {
		//TODO(fran): this is good but not perfect, inconsistencies occur when resizing, mainly with the starting position of the page
			//Demonstration: scroll the page down a bit, and then resize the window making it at least half small, you'll see that the top elements of the page get cropped since the page's starting point is now past the beginning of the page_space
			//Solution: I think this problem should solve itself once we clamp scrolling, since the page shouldnt actually be allowed to scroll in the way we do here
		RECT page_r; GetWindowRect(page_wnd, &page_r); MapWindowPoints(0, GetParent(page_wnd), (POINT*)&page_r, 2);
		rect_i32 page;
		page.left = 0;
		page.top = page_r.top;
		page.w = w;
		page.h = maximum(page_space_h, used_h);
		MyMoveWindow(page_wnd, page, FALSE);
	}

	enum class notification_relevance { success, error };
	void notify(ProcState* state, page_type page, notification_relevance category, const utf16* notif) {
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

	void button_function_on_click_goto_page_new(void* element, void* user_extra) {
		ProcState* state = (decltype(state))user_extra;

		store_previous_page(state, state->current_page);
		reset_page(state, page_type::new_word);
		set_current_page(state, page_type::new_word);
	}

	void button_function_on_click_goto_page_practice(void* element, void* user_extra) {
		ProcState* state = (decltype(state))user_extra;

		store_previous_page(state, state->current_page);
		set_current_page(state, page_type::practice);
	}
}
_add_struct_to_serialization_namespace(べんきょう::Settings);

#include "study_page_landing.h"
#include "study_page_new_word.h"
#include "study_page_practice_common.h"
#include "study_page_practice.h"
#include "study_page_practice_writing.h"
#include "study_page_practice_multiplechoice.h"
#include "study_page_practice_drawing.h"
#include "study_page_review_practice.h"
#include "study_page_review_practice_writing.h"
#include "study_page_review_practice_multiplechoice.h"
#include "study_page_review_practice_drawing.h"
#include "study_page_show_word.h"
#include "study_page_wordbook.h"
#include "study_page_wordbook_all.h"
#include "study_sidebar.h"
#include "study_navbar.h"

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
//TODO(fran): landing & new page: chart of activity github style, calendar showing in red the days where you either added words or practiced
//TODO(fran): new_word: components section where you can link this word to others that form it

//TODO(fran): IDEA: navbar: what if I used the WM_PARENTNOTIFY to allow for my childs to tell me when they are resized, maybe not using parent notify but some way of not having to manually resize the navbar. Instead of this I'd say it's better that a child that needs resizing sends that msg to its parent (WM_REQ_RESIZE), and that trickles up trough the parenting chain til someone handles it
//TODO(fran): IDEA: all controls: we can add a crude transparency by making the controls layered windows and define a color as the color key, the better but much more expensive approach would be to use UpdateLayeredWindow to be able to also add semi-transparency though it may not be necessary. There also seems to be a supported way using WS_EX_COMPOSITED + WS_EX_TRANSPARENT
	//TODO(fran): all controls: check for a valid brush and if it's invalid dont draw, that way we give the user the possibility to create transparent controls (gotta check that that works though)

//TODO(fran): BUG: practice writing/...: the edit control has no concept of its childs, therefore situations can arise were it is updated & redrawn but the children arent, which causes the space they occupy to be left blank (thanks to WS_CLIPCHILDREN), the edit control has to tell its childs to redraw after it does
//TODO(fran):BUG searchbox: caret on the searchbox gets misplaced. howto reproduce: search for a word, then using the down arrow go to some option and then press enter, now the searchbox caret will be misplaced

//TODO(fran): better word selector for practices, new words barely come up
//TODO(fran): when showing the recently added words (and the same thing in the word book) we should show them in reverse order, with the word at the top of the list being the last one of the day. Instead of right now where the top word is the first one of the day.
//TODO(fran): clicking on a listbox item from the "Recently Added" doesnt send the window to the top, it remains behind other windows. This must have smth to do with the way I handle ACTIVATE msgs in listbox.
//TODO(fran): practice_writing with hiragana and meaning, make it clearer what you have to write, I usually make mistakes by writing the word in hiragana when I wanted it in meaning and viceversa


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

//REMEMBER: have checks in place to make sure the user cant execute operations twice by quickly pressing a button again

//IMPORTANT: handling the review page, we'll have two vectors, one floating in space to which each practice level will add to each time it completes, once the whole practice is complete we'll preload() review_practice with std::move(this vector), this page has it's own vector which at this point will be emptied and its contents freed and replaced with the new vector (the floating in space one). This guarantees the review page is always on a valid state. //TODO(fran): idk if std::move works when I actually need to send the vector trough a pointer, it may be better to allocate an array and send it together with its size (which makes the vector pointless all together), I do think allocating arrays is beter, we simply alloc when the start button is pressed on the practice page and at the end of the practice send that same pointer to review. The single annoying problem is the back button, solution: we'll go simple for now, hide the back button until the practice is complete, the review page will have the back button active and that will map back to the practice page (at least at the time Im writing this). This way we guarantee valid states for everything, the only semi problem would be if the user decides to close the application, in that case we could, if we wanted, ask for confirmation depending on the page, but once the user says yes we care no more about memory, yes there will be some mem leaks but at that point it no longer matters since that memory will be removed automatically by the OS

//TODO(fran): in practice_writing: I like the idea of putting a bar in the middle between the question and answer and that it lights up when the user responds, it either goes green and it's text says "Correct!" or red and text "Incorrect: right answer was [...]". The good thing about this is that I dont need to add extra controls for the review page, I can simply reconstruct the exact same page and there's already a spot to indicate the correction, would be the same with a multiple choice, eg 3 words user clicks wrong answer and it lights up red and the correct one lights up green, the thing with this is that I'd need the text colors to be fixed in order to not be opaqued by the new red/green light, that's kind of annoying but I dont see an easy solution. Sidenode: actually wanikani doesnt have a separate bar, it changes the color of the edit box

namespace べんきょう {
	//Sets the items in the corresponding page to the values on *data, prepares the page so it can be shown to the user
	void preload_page(ProcState* state, page_type page, void* data) {
		//TODO(fran): we probably want to clear the whole page before we start adding stuff
		switch (page) {
		case decltype(page)::landing:
		{
			auto stats = (user_stats*)data;
			landing::preload_page(state, state->pages.landing, stats);
		} break;
		case decltype(page)::new_word:
		{
			auto new_word = (learnt_word16*)data;//NOTE: since we are in UI we expect utf16 strings
			new_word::preload_page(state, state->pages.new_word, new_word);
		} break;
		case decltype(page)::show_word:
		{
			auto word_to_show = (stored_word16*)data;
			show_word::preload_page(state, state->pages.show_word, word_to_show);
		} break;
		case decltype(page)::practice_writing:
		{
			//TODO(fran): shouldnt preload_page also call clear_page?
			auto practice = (practice::writing::word*)data;
			practice::writing::preload_page(state, state->pages.practice_writing, practice);
		} break;
		case decltype(page)::review_practice:
		{
			auto practices = (std::vector<practice::practice_header*>*)data;
			practice::review::preload_page(state, state->pages.review_practice, practices);
		} break;
		case decltype(page)::review_practice_writing:
		{
			auto pagedata = (practice::practice_writing*)data;
			practice::review::writing::preload_page(state, state->pages.practice_writing, pagedata);
		} break;
		case decltype(page)::practice_multiplechoice:
		{
			auto practice = (practice::multiplechoice::word*)data;
			practice::multiplechoice::preload_page(state, state->pages.practice_multiplechoice, practice);
		} break;
		case decltype(page)::review_practice_multiplechoice:
		{
			auto pagedata = (practice::practice_multiplechoice*)data;
			practice::review::multiplechoice::preload_page(state, state->pages.practice_multiplechoice, pagedata);
		} break;
		case decltype(page)::practice_drawing:
		{
			auto practice = (practice::drawing::word*)data;
			practice::drawing::preload_page(state, state->pages.practice_drawing, practice);
		} break;
		case decltype(page)::review_practice_drawing:
		{
			auto pagedata = (practice::practice_drawing*)data;
			practice::review::drawing::preload_page(state, state->pages.practice_drawing, pagedata);
		} break;
		default: Assert(0);
		}
	}

	void show_page(ProcState* state, page_type p, u32 ShowWindow_cmd /*SW_SHOW,...*/) {
		switch (p) {
		case decltype(p)::landing: landing::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::new_word: new_word::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::show_word: show_word::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::practice: practice::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::practice_writing: practice::writing::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::practice_multiplechoice: practice::multiplechoice::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::practice_drawing: practice::drawing::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::review_practice: practice::review::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::review_practice_writing: practice::review::writing::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::review_practice_multiplechoice: practice::review::multiplechoice::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::review_practice_drawing: practice::review::drawing::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::wordbook: wordbook::show_page(state, ShowWindow_cmd); break;
		case decltype(p)::wordbook_all: wordbook_all::show_page(state, ShowWindow_cmd); break;
		default: Assert(0);
		}
	}

	void set_default_focus(ProcState* state, page_type page) {
		switch (page) {
		case decltype(page)::new_word: new_word::set_default_focus(state); break;
		case decltype(page)::landing: landing::set_default_focus(state); break;
		case decltype(page)::practice_writing: practice::writing::set_default_focus(state); break;
		//default: Assert(0);
		}
	}

	void resize_page(ProcState* state, page_type page) {
		static int rez = 0; printf("Resize calls: %d\n", ++rez);
		RECT r; GetClientRect(state->wnd, &r);

		const int w = RECTWIDTH(r);
		const int half_w = w / 2;
		const int w_pad = (int)((f32)w * .05f);//TODO(fran): hard limit for max padding
		const int max_w = w - w_pad * 2;

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
		const int h_pad = (int)((f32)h * .05f);

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

		switch (page) {
		case page_type::landing:
			landing::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::new_word:
			new_word::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::practice:
			practice::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::practice_writing:
			practice::writing::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::practice_multiplechoice:
			practice::multiplechoice::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::practice_drawing:
			practice::drawing::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::review_practice:
			practice::review::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::show_word:
			show_word::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::review_practice_writing:
			//TODO(fran): different layout?
			resize_page(state, page_type::practice_writing); break;
		case page_type::review_practice_multiplechoice:
			//TODO(fran): different layout?
			resize_page(state, page_type::practice_multiplechoice); break;
		case page_type::review_practice_drawing:
			//TODO(fran): different layout?
			resize_page(state, page_type::practice_drawing); break;
		case page_type::wordbook:
			wordbook::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		case page_type::wordbook_all:
			wordbook_all::layout_page(state, w, half_w, w_pad, max_w, h, wnd_h, half_wnd_h, h_pad, page_space.h); break;
		default: Assert(0);
		}
	}
	void resize_page(ProcState* state) { resize_page(state, state->current_page); }

	void set_current_page(ProcState* state, page_type new_page) {
		show_page(state, state->current_page, SW_HIDE);
		state->current_page = new_page;
		study_sidebar::animate_hide(state);
		switch (new_page) {
		case decltype(new_page)::landing: landing::set_current_page(state); break;
		case decltype(new_page)::practice: practice::set_current_page(state); break;
		case decltype(new_page)::wordbook: wordbook::set_current_page(state); break;
		case decltype(new_page)::wordbook_all: wordbook_all::set_current_page(state); break;
		}
		resize_page(state);
		show_page(state, state->current_page, SW_SHOW);
		set_default_focus(state, state->current_page);
	}

	void init_cpp_objects(ProcState* state) {
		state->pagestate.practice_review.practices = decltype(state->pagestate.practice_review.practices)();
	}

	void reset_page(ProcState* state, page_type page) {
		//NOTE: one solution here would be to destroy all the controls remove_controls(page) and then call addcontrols(page)
		//NOTE: the problem here is that its not enough to settext to null to everyone, that can be done fairly easily implementing some WM_RESET msg, but there are some controls that shouldnt be reset like the buttons, we'd need to implement a .reseteable() or somehow via union magic or smth that gives us only the controls that should be reset
		switch (page) {
		case decltype(page)::new_word: new_word::reset_page(state); break;
		case decltype(page)::practice_writing: practice::writing::reset_page(state); break;
		case decltype(page)::practice_multiplechoice: break; //TODO(fran): do we wanna clear smth here?
		case decltype(page)::practice_drawing: practice::drawing::reset_page(state); break;
		default: Assert(0);
		}
	}

	void create_pages(ProcState* state) {
		load_styles();

		study_navbar::create(state);

		//---------------------Page Space----------------------: //TODO(fran): rename to page_slot?
		state->pages.page_space = CreateWindowW(page::wndclass, NULL, WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE //TODO(fran): WS_CLIPCHILDREN?
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		//IMPORTANT INFO: WS_CLIPSIBLINGS is crucial in order to make the sidebar work and not be occluded by the page, it prevents the page from drawing over the sidebar, otherwise it gets occluded
		page::set_theme(state->pages.page_space, &base_page_theme);

		study_sidebar::create(state);

		landing::create_page(state);
		new_word::create_page(state);
		show_word::create_page(state);
		practice::create_page(state);
		practice::writing::create_page(state);
		practice::multiplechoice::create_page(state);
		practice::drawing::create_page(state);
		practice::review::create_page(state);
		wordbook::create_page(state);
		wordbook_all::create_page(state);
	}

	void handle_command_event(ProcState* state, HWND child, WPARAM wparam) {
		WORD notif = HIWORD(wparam);
		if (child) {//Notifs from our childs
			switch (state->current_page) {
			case page_type::landing:
			case page_type::new_word:
			case page_type::practice:
			case page_type::show_word:
			case page_type::review_practice:
			case page_type::wordbook:
				break;
			case page_type::wordbook_all:
				wordbook_all::handle_event(state, child, notif); break;
			case page_type::practice_writing:
				practice::writing::handle_event(state, child, notif); break;
			case page_type::review_practice_writing:
				practice::review::writing::handle_event(state, child); break;
			case page_type::practice_multiplechoice:
				practice::multiplechoice::handle_event(state, child, wparam); break;
			case page_type::review_practice_multiplechoice:
				practice::review::multiplechoice::handle_event(state, child); break;
			case page_type::practice_drawing:
				practice::drawing::handle_event(state, child); break;
			case page_type::review_practice_drawing:
				practice::review::drawing::handle_event(state, child); break;
			default:
				Assert(0);
			}
		}
		else //switch (LOWORD(wparam)) { //Menu notifications
			Assert(0);
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
			state->settings = ((べんきょう::Settings*)create_nfo->lpCreateParams);
			state->current_page = page_type::landing;
			init_cpp_objects(state);
			set_state(hwnd, state);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			create_pages(state);
			set_current_page(state, page_type::landing);

			if constexpr (constexpr bool TEST_SCORE_ANIM = false; TEST_SCORE_ANIM) {
				static void (*testp)(HWND, UINT, UINT_PTR, DWORD) = [](HWND wnd, UINT, UINT_PTR id, DWORD) {
					auto state = get_state(wnd);
					flip_visibility(state->pages.landing.score_accuracy);
					flip_visibility(state->pages.landing.score_accuracy);
					SetTimer(wnd, id, 3000, testp);
				};
				SetTimer(state->wnd, 5555, 0, testp);
			}

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
			resize_page(state);
			return 0;
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				save_settings(state);

				practice::clear_practices_vector(state->pagestate.practice_review.practices);
				state->pagestate.practice_review.practices.~vector();
				
				//#free? state->pagestate.practice_writing.practice;

				practice::clear_practices_vector(state->multipagestate.temp_practices);
				state->multipagestate.temp_practices.~vector();
				
				if(state->settings->is_primary_wnd) PostQuitMessage(0);//TODO(fran): this aint gonna be enough if we ever have multiple main windows

				free(state);
				state = nullptr;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_COMMAND:
		{
			handle_command_event(state, (HWND)lparam, wparam);
			return 0;
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
		case WM_LBUTTONDOWN:
		{
			SetFocus(0);// Remove focus from whoever had it
			return 0;
		} break;
		case WM_XBUTTONDOWN:// Used for forwards and backwards navigation with mouse that have those additional buttons, TODO(fran): this seems like more of a convention, find out if there's some way to know for sure this was the user's intent when they pressed those buttons
		{
			//TODO(fran): this may be better placed in nonclient and all we do here is redirect to there
			switch (HIWORD(wparam)) {
			case XBUTTON1 /*Back button*/: SendMessage(state->wnd, WM_BACK, 0, 0); break;
			case XBUTTON2 /*Forward button*/: /*We dont allow going forward*/ break;
			default: Assert(0);
			}
			return 1;
		} break;
		case WM_MOUSEWHEEL:
		{
			return SendMessage(state->nc_parent, msg, wparam, lparam);
		} break;
		case WM_ASK_FOR_RESIZE:
		{
			resize_page(state); //TODO(fran): only process resizes every X milliseconds (later on we should get rid of this concept entirely and provide resize info to the control directly so it knows when it can send the ask_for_resize msg)
			ask_for_repaint(state); //TODO(fran): controls dont re-render otherwise, even though Im resizing them, maybe because they only change position and not size? also maybe because I dont ask the page to be resized?
			return 0;
		} break;
		case WM_PRINTCLIENT:// For some reason the base combobox (not subclassed) sends this msg, it makes no sense to me why the cb would need my client rendering
		case WM_ERASEBKGND:// We dont erase the bk here, instead we do everything on wm_paint
		case WM_GETTEXT:
		case WM_GETTEXTLENGTH:
		case WM_IME_SETCONTEXT:// Sent the first time on SetFocus // We dont want IME for the general wnd, the childs can decide
		case WM_IME_NOTIFY://for some reason you still get this even when not doing WM_IME_SETCONTEXT
		case WM_CLOSE:
		{
			return 0;
		} break;
		case WM_DELETEITEM:// Sent by the cb when I delete its items (which I need to do one by one since there's no msg to send it that can do that)
		case WM_INPUTLANGCHANGE:
		case WM_XBUTTONUP:
		{
			return 1;
		} break;
#ifdef _DEBUG
		case WM_NCHITTEST:
		case WM_SETCURSOR:
		case WM_MOUSEMOVE:
		case WM_MOUSEACTIVATE:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_PARENTNOTIFY:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
		case WM_NCPAINT:
		case WM_NCCALCSIZE:
		case WM_MOVE:
		case WM_SHOWWINDOW:
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
#endif
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

	struct pre_post_main {
		pre_post_main() { init_wndclass(wndclass, Proc); }
		~pre_post_main() { } //Classes are de-registered automatically by the os. INFO: you could also use the atexit function
	} static const PREMAIN_POSTMAIN;
}