#pragma once
#include "windows_sdk.h"
#include <windowsx.h>
#include "windows_msg_mapper.h"
#include <vector>
#include <Shlwapi.h>
#include "win32_Helpers.h"
#include "unCap_Reflection.h"
#include "LANGUAGE_MANAGER.h"
#include "win32_new_msgs.h"

//TODO(fran): show password button
//TODO(fran): ballon tips, probably handmade since windows doesnt allow it anymore, the indicator leaves it much clearer what the tip is referring to in cases where there's many controls next to each other
//TODO(fran): caret stops blinking after a few seconds of being shown, this seems to be a windows """feature""", we may need to set a timer to re-blink the caret every so often while we have keyboard focus
//TODO(fran): paint/handle my own IME window https://docs.microsoft.com/en-us/windows/win32/intl/ime-window-class
//TODO(fran): since WM_SETTEXT doesnt notify by default we should change WM_SETTEXT_NO_NOTIFY to WM_SETTEXT_NOTIFY and reverse the current roles
//TODO(fran): IDEA for multiline with wrap around text, keep a list of wrap around points, be it a new line, word break or word that doesnt fit on its line, then we can go straight from user clicking the wnd to the correspoding line by looking how many lines does the mouse go and input that into the wrap around list to get to that line's first char idx
//TODO(fran): on WM_STYLECHANGING check for password changes, that'd need a full recalculation
//TODO(fran): on a WM_STYLECHANGING we should check if the alignment has changed and recalc/redraw every char, NOTE: I dont think windows' controls bother with this since it's not too common of a use case

//NOTE: this took two days to fully implement, certainly a hard control but not as much as it's made to believe, obviously im just doing single line but extrapolating to multiline isnt much harder now a single line works "all right"

//-------------------"API"--------------------:
// edit_oneline::set_theme()
//edit_oneline::maintain_placerholder_when_focussed() : hide or keep showing the placeholder text when the user clicks over the wnd

//-------------"API" (Additional Messages)-------------:
#define editoneline_base_msg_addr (WM_USER + 1500)

#define EM_SETINVALIDCHARS (editoneline_base_msg_addr+2) /*characters that you dont want the edit control to accept either trough WM_CHAR or WM_PASTE*/ /*lparam=unused ; wparam=pointer to null terminated cstring, each char will be added as invalid*/ /*INFO: overrides the previously invalid characters that were set before*/ /*TODO(fran): what to do on pasting? reject the entire string or just ignore those chars*/
#define WM_SETTEXT_NO_NOTIFY (editoneline_base_msg_addr+3) /*wparam=unused ; lparam=null terminated utf16* */


//-------------"Default Notifications"-------------:
//EN_CHANGE
//EN_KILLFOCUS

//-------------"Non-standard Notifications"-------------:
//Notification msgs, sent to the parent through WM_COMMAND with LOWORD(wparam)=control identifier ; HIWORD(wparam)=msg code ; lparam=HWND of the control
//IMPORTANT: this are sent _only_ if you have set an identifier for the control (hMenu on CreateWindow)
#define _editoneline_notif_base_msg 0x0104
#define EN_ENTER (_editoneline_notif_base_msg+1) /*User has pressed the enter key*/
#define EN_ESCAPE (_editoneline_notif_base_msg+2) /*User has pressed the escape key*/


//-------------Additional Styles-------------:
#define ES_ROUNDRECT 0x0200L //Border is made of a rounded rectangle instead of just straight lines //TODO(fran): not convinced with the name


//-------------Tooltip-------------:
#define EDITONELINE_default_tooltip_duration 3000 /*ms*/
#define EDITONELINE_tooltip_timer_id 0xf1


//---------Differences with default oneline edit control---------:
//·EN_CHANGE is sent when there is a modification to the text, of any type, and it's sent immediately, not after rendering



namespace edit_oneline{

	constexpr cstr wndclass[] = L"unCap_wndclass_edit_oneline";

	constexpr cstr password_char = sizeof(password_char) > 1 ? _t('●') : _t('*');

	struct char_sel {//TODO(fran): we should be using size_t and solve the -1 issue by simply checking for size_t_max
		using type = size_t;
		type anchor;//Eg ABC		anchor=1	anchor is between A and B
		type cursor;//Eg ABC		cursor=1	cursor is between A and B
		//First character of the selection
		type x_min() {
			type res = (anchor < cursor) ? anchor : cursor;
			return res;
		}
		//First character beyond the selection
		type x_max() {
			type res = (anchor > cursor) ? anchor : cursor;
			return res;
		}
		type sel_width() {
			type res = distance(anchor, cursor);
			return res;
		}
		b32 has_selection() { return (b32)sel_width(); }
		//void set_both(int new_val) { anchor = cursor = new_val; }

		//Example:
		//			ABCD	EM_SETSEL(1,1)	cursor is placed in between A and B
		//			ABCD	EM_SETSEL(1,2)	selection covers the letter B, cursor is placed between B and C

		//TODO(fran): not sure whether I should store y position, it's actually more of a UI thing, but it may be necessary for things like scrolling to xth char_sel
	};

	struct Theme {
		struct {
			brush_group foreground, bk, border, selection; //NOTE: for now we use the border color for the caret color
		} brushes;
		struct {
			u32 border_thickness = U32MAX;
		}dimensions;
		HFONT font = 0;
	};

	struct ProcState {
		HWND wnd;
		HWND parent;
		u32 identifier;

		Theme theme;

		u32 char_max_sz;//doesnt include terminating null, also we wont have terminating null
		
		char_sel char_cur_sel;//this is in "character" coordinates, zero-based
		struct caretnfo {
			HBITMAP bmp;
			SIZE dim;
			POINT pos;//client coords, it's also top down so this is the _top_ of the caret
		}caret;

		str char_text;//much simpler to work with and debug
		std::vector<int> char_dims;//NOTE: specifies, for each character, its width
		//TODO(fran): it's probably better to use signed numbers in case the text overflows ths size of the control
		int char_pad_x;//NOTE: specifies x offset from which characters start being placed on the screen, relative to the client area. For a left aligned control this will be offset from the left, for right aligned it'll be offset from the right, and for center alignment it'll be the left most position from where chars will be drawn

		cstr placeholder[100]; //NOTE: uses txt_dis brush for rendering
		bool maintain_placerholder_on_focus;//Hide placeholder text when the user clicks over it
		
		cstr invalid_chars[100];

		union EditOnelineControls {
			struct {
				HWND tooltip;
			};
			HWND all[1];//REMEMBER TO UPDATE
		}controls;

		bool OnMouseTracking;//true when capturing the mouse while the user remains with left click down

		HGLOBAL clipboard_handle;

		bool hide_IME_wnd;
	};


	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }

	bool _copy_theme(Theme* dst, const Theme* src) {
		bool repaint = false;
		repaint |= copy_brush_group(&dst->brushes.foreground, &src->brushes.foreground);
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);
		repaint |= copy_brush_group(&dst->brushes.border, &src->brushes.border);
		repaint |= copy_brush_group(&dst->brushes.selection, &src->brushes.selection);

		if (src->dimensions.border_thickness != U32MAX) {
			dst->dimensions.border_thickness = src->dimensions.border_thickness; repaint = true;
		}

		if (src->font) { dst->font = src->font; repaint = true; }
		return repaint;
	}

	//Set only what you need, what's not set wont be used
	//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
	void set_theme(HWND wnd, const Theme* t) {
		ProcState* state = get_state(wnd);
		if (state && t) {
			bool repaint = _copy_theme(&state->theme, t);//TODO(fran): the entire char_dims vector needs to be recalculated on font changes

			if (repaint)  ask_for_repaint(state);
		}
	}

	SIZE calc_caret_dim(ProcState* state) {
		SIZE res;
		HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
		HFONT oldfont = SelectFont(dc, state->theme.font); defer{ SelectFont(dc, oldfont); };
		TEXTMETRIC tm;
		GetTextMetrics(dc, &tm);
		int avg_height = tm.tmHeight;
		res.cx = 1;
		res.cy = avg_height;
		return res;
	}

	int calc_line_height_px(ProcState* state) {
		int res;
		HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
		HFONT oldfont = SelectFont(dc, state->theme.font); defer{ SelectFont(dc, oldfont); };
		TEXTMETRIC tm;
		GetTextMetrics(dc, &tm);
		res = tm.tmHeight;
		return res;
	}

	SIZE calc_char_dim(ProcState* state, cstr c) {
		Assert(c != _t('\t'));

		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
		if (style & ES_PASSWORD) c = password_char;

		SIZE res;
		HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
		HFONT oldfont = SelectFont(dc, state->theme.font); defer{ SelectFont(dc, oldfont); };
		//UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };
		//SetTextAlign(dc, TA_LEFT);
		GetTextExtentPoint32(dc, &c, 1, &res);
		return res;
	}

	SIZE calc_text_dim(ProcState* state) {
		SIZE res;
		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
		if (style & ES_PASSWORD) {
			res = calc_char_dim(state, 0);
			res.cx *= (LONG)state->char_text.length();
		}
		else {
			HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
			HFONT oldfont = SelectFont(dc, state->theme.font); defer{ SelectFont(dc, oldfont); };
			//UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };
			//SetTextAlign(dc, TA_LEFT);
			GetTextExtentPoint32(dc, state->char_text.c_str(), (int)state->char_text.length(), &res);
		}

		return res;
	}

	POINT calc_caret_p(ProcState* state) {
		Assert(safe_subtract0(state->char_cur_sel.cursor, (size_t)1) <= state->char_dims.size());
		POINT res = { state->char_pad_x, 0 };
		for (size_t i = 0; i < state->char_cur_sel.cursor; i++) {
			res.x += state->char_dims[i];
			//TODO(fran): probably wrong
		}
		//NOTE: we are single line so we always want the text to be vertically centered, same goes for the caret
		RECT rc; GetClientRect(state->wnd, &rc);
		int wnd_h = RECTHEIGHT(rc);
		int caret_h = state->caret.dim.cy;
		res.y = (wnd_h - state->caret.dim.cy) / 2;
		return res;
	}


	void remove_selection(ProcState* state, size_t x_min, size_t x_max) {
		if (!state->char_text.empty()) {
			x_min = clamp((size_t)0, x_min, state->char_text.length());
			x_max = clamp((size_t)0, x_max, state->char_text.length());

			state->char_text.erase(x_min, distance(x_min,x_max));
			state->char_dims.erase(state->char_dims.begin() + x_min, state->char_dims.begin() + x_max);

			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			if (style & ES_CENTER) {
				//When the text is centered its pad_x needs to be recalculated on every character addition/removal
				RECT rc; GetClientRect(state->wnd, &rc);
				state->char_pad_x = (RECTWIDTH(rc) - calc_text_dim(state).cx) / 2;
			}

			SendMessage(state->wnd, EM_SETSEL, x_min, x_min);
			//state->char_cur_sel.anchor = state->char_cur_sel.cursor = x_min;
		}
	}

	//Removes the current text selection and updates the selection values
	void remove_selection(ProcState* state) {
		remove_selection(state, state->char_cur_sel.x_min(), state->char_cur_sel.x_max());
	}

	void insert_character(ProcState* state, utf16_str s, size_t x_min, size_t x_max) {
		x_min = clamp((size_t)0, x_min, state->char_text.length());
		x_max = clamp((size_t)0, x_max, state->char_text.length());
		char_sel sel{ x_min, x_max };

		if (safe_subtract0(state->char_text.length(), sel.sel_width()) < state->char_max_sz) {

			//remove existing selection
			if (sel.has_selection()) remove_selection(state, x_min, x_max);

			//insert new character
			state->char_text.insert(state->char_cur_sel.cursor, s.str);

			for (size_t i = 0; i < s.sz_char() - 1; i++)
				state->char_dims.insert(state->char_dims.begin() + state->char_cur_sel.cursor + i, calc_char_dim(state, s[i]).cx);

			size_t anchor = state->char_cur_sel.cursor + safe_subtract0(s.sz_char(), (size_t)1);
			size_t cursor = anchor;

			SendMessage(state->wnd, EM_SETSEL, anchor, cursor);
		}
	}

	//inserts character replacing the current selection
	void insert_character(ProcState* state, utf16 c) {//TODO(fran): optimized version for single character insertion
		utf16 txt[2];
		txt[0] = c;
		txt[1] = 0;

		insert_character(state, to_utf16_str(txt), state->char_cur_sel.x_min(), state->char_cur_sel.x_max());
	}

	//inserts string replacing the current selection
	void insert_character(ProcState* state, const utf16* s) {
		if (s && *s)
			insert_character(state, to_utf16_str(const_cast<utf16*>(s)), state->char_cur_sel.x_min(), state->char_cur_sel.x_max());
	}

	bool _settext(ProcState* state, cstr* buf /*null terminated*/) {
		bool res = false;
		cstr empty = 0;//INFO: compiler doesnt allow you to set it to L''
		if (!buf) buf = &empty; //NOTE: this is the standard default behaviour
		size_t char_sz = cstr_len(buf);//not including null terminator
		if (char_sz <= (size_t)state->char_max_sz) {
			SendMessage(state->wnd, EM_SETSEL, 0, -1);
			remove_selection(state);
			insert_character(state, buf);

			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			if (style & ES_CENTER) {
				//Recalc pad_x
				RECT rc; GetClientRect(state->wnd, &rc);
				state->char_pad_x = (RECTWIDTH(rc) - calc_text_dim(state).cx) / 2;

			}
			state->caret.pos = calc_caret_p(state);
			SetCaretPos(state->caret.pos);

			res = true;
			InvalidateRect(state->wnd, NULL, TRUE);
		}
		return res;
	}

	//NOTE: pasting from the clipboard establishes a couple of invariants: lines end with \r\n, there's a null terminator, we gotta parse it carefully cause who knows whats inside
	bool paste_from_clipboard(ProcState* state, const cstr* txt) { //returns true if it could paste something
	bool res = false;
	size_t char_sz = cstr_len(txt);//does not include null terminator
	if ((size_t)state->char_max_sz >= state->char_text.length() + char_sz) {

	}
	else {
		char_sz -= ((state->char_text.length() + char_sz) - (size_t)state->char_max_sz);
	}
	if (char_sz > 0) {
		//TODO(fran): remove invalid chars (we dont know what could be inside)

		remove_selection(state);//NOTE or TODO(fran): not necessary, insert_character already has to do it, but it's easier to read and understand the steps needed
		insert_character(state, txt);

		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
		if (style & ES_CENTER) {
			//Recalc pad_x
			RECT rc; GetClientRect(state->wnd, &rc);
			state->char_pad_x = (RECTWIDTH(rc) - calc_text_dim(state).cx) / 2;

		}
		state->caret.pos = calc_caret_p(state);
		SetCaretPos(state->caret.pos);
		res = true;
	}
	return res;
	}

	void set_composition_pos(ProcState* state)//TODO(fran): this must set the other stuff too
	{
		HIMC imc = ImmGetContext(state->wnd);
		if (imc != NULL) {
			defer{ ImmReleaseContext(state->wnd, imc); };
			COMPOSITIONFORM cf;
			//NOTE immgetcompositionwindow fails
	#if 0 //IME window no longer draws the unnecessary border with resizing and button, it is placed at cf.ptCurrentPos and extends down after each character
			cf.dwStyle = CFS_POINT;

			if (GetFocus() == state->wnd) {
				//NOTE: coords are relative to your window area/nc area, _not_ screen coords
				cf.ptCurrentPos.x = state->caret.pos.x;
				cf.ptCurrentPos.y = state->caret.pos.y + state->caret.dim.cy;
			}
			else {
				cf.ptCurrentPos.x = 0;
				cf.ptCurrentPos.y = 0;
			}
	#else //IME window no longer draws the unnecessary border with resizing and button, it is placed at cf.ptCurrentPos and has a max size of cf.rcArea in the x axis, the y axis can extend a lot longer, basically it does what it wants with y
			if (!state->hide_IME_wnd) {
				cf.dwStyle = CFS_RECT;

				//TODO(fran): should I place the IME in line with the caret or below so the user can see what's already written in that line?
				if (GetFocus() == state->wnd) {
					cf.ptCurrentPos.x = state->caret.pos.x;
					cf.ptCurrentPos.y = state->caret.pos.y + state->caret.dim.cy;
					//TODO(fran): programatically set a good x axis size
					cf.rcArea = { state->caret.pos.x , state->caret.pos.y + state->caret.dim.cy,state->caret.pos.x + 100,state->caret.pos.y + state->caret.dim.cy + 100 };
				}
				else {
					cf.rcArea = { 0,0,0,0 };
					cf.ptCurrentPos.x = 0;
					cf.ptCurrentPos.y = 0;
				}
			}
			else {
				cf.dwStyle = CFS_RECT | CFS_FORCE_POSITION;
				cf.ptCurrentPos.x = state->caret.pos.x;//INFO: it does _not_ respect x that goes beyond the control
				cf.ptCurrentPos.y = state->caret.pos.y + state->caret.dim.cy;//INFO: it does _not_ respect y that goes beyond the control
				//TODO(fran): programatically set a good x axis size
				cf.rcArea = rectWH(cf.ptCurrentPos.x, cf.ptCurrentPos.y, 1, 1);//INFO: it does _not_ respect w & h, if w or h is too small the window isnt shown, otherwise it's shown in one unique size
				#if 0 //this doesnt work, candidate window still shows up
				CANDIDATEFORM cnf;
				cnf.dwIndex = 0;
				cnf.dwStyle = CFS_EXCLUDE;
				cnf.ptCurrentPos = { GetSystemMetrics(SM_CXVIRTUALSCREEN) + 1,GetSystemMetrics(SM_CYVIRTUALSCREEN) + 1 };
				cnf.rcArea = rectWH(GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), GetSystemMetrics(SM_CXVIRTUALSCREEN)+500, GetSystemMetrics(SM_CYVIRTUALSCREEN) + 500);
				ImmSetCandidateWindow(imc, &cnf); //this one seems to be the list!
				#endif
				//ShowWindow(ImmGetDefaultIMEWnd(state->wnd),SW_HIDE); //maybe I can hide the window? noup
				//ImmSetStatusWindowPos //no idea what this is
			}
	#endif
			BOOL res = ImmSetCompositionWindow(imc, &cf); Assert(res);
		}
	}

	void set_IME_wnd(HWND wnd, bool hidden) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->hide_IME_wnd = hidden;
		}
	}

	void set_composition_font(ProcState* state)//TODO(fran): this must set the other stuff too
	{
		HIMC imc = ImmGetContext(state->wnd);
		if (imc != NULL) {
			defer{ ImmReleaseContext(state->wnd, imc); };
			LOGFONT lf;
			int getobjres = GetObject(state->theme.font, sizeof(lf), &lf); Assert(getobjres == sizeof(lf));

			BOOL setres = ImmSetCompositionFont(imc, &lf); Assert(setres);
		}
	}

	namespace ETP {
		enum ETP {//EditOneline_tooltip_placement
			left = (1 << 1),
			top = (1 << 2),
			right = (1 << 3),
			bottom = (1 << 4),
			//current_char = (1 << 5), //instead of placement in relation to the control wnd it will be done relative to a character
		};
	}
	//TODO(fran): positioning relative to a character offset, eg {3,5} 3rd char of 5th row place on top or left or...
	void show_tip(HWND wnd, const cstr* msg, int duration_ms, u32 ETP_flags) {
		ProcState* state = get_state(wnd); Assert(state);
		TOOLINFO toolInfo{ sizeof(toolInfo) };
		toolInfo.hwnd = state->wnd;
		toolInfo.uId = (UINT_PTR)state->wnd;
		toolInfo.hinst = GetModuleHandle(NULL);
		toolInfo.lpszText = const_cast<cstr*>(msg);
		SendMessage(state->controls.tooltip, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolInfo);
		SendMessage(state->controls.tooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolInfo);
		POINT tooltip_p{0,0};
		RECT tooltip_r;
		tooltip_r.left = 0;
		tooltip_r.right = 50;
		tooltip_r.top = 0;
		tooltip_r.bottom = calc_line_height_px(state);//The tooltip uses the same font, so we know the height of its text area
		SendMessage(state->controls.tooltip, TTM_ADJUSTRECT, TRUE, (LPARAM)&tooltip_r);//convert text area to tooltip full area

		RECT rc; GetClientRect(state->wnd, &rc);

		if (ETP_flags & ETP::left) {
			tooltip_p.x = 0;
		}

		if (ETP_flags & ETP::right) {
			tooltip_p.x = RECTWIDTH(rc);
		}

		if (ETP_flags & ETP::top) {
			tooltip_p.y = -RECTHEIGHT(tooltip_r);
		}

		if (ETP_flags & ETP::bottom) {
			tooltip_p.y = RECTHEIGHT(rc);
		}

		ClientToScreen(state->wnd, &tooltip_p);
		SendMessage(state->controls.tooltip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(tooltip_p.x, tooltip_p.y));
		SetTimer(state->wnd, EDITONELINE_tooltip_timer_id, duration_ms, NULL);
	}

	void notify_parent(ProcState* state, WORD notif_code) {
		PostMessage(state->parent, WM_COMMAND, MAKELONG(state->identifier, notif_code), (LPARAM)state->wnd);
	}

	bool show_placeholder(ProcState* state) {
		bool res = *state->placeholder && (GetFocus() != state->wnd || state->maintain_placerholder_on_focus) && (state->char_text.length() == 0);
		return res;
	}

	void maintain_placerholder_when_focussed(HWND wnd, bool maintain) {//TODO(fran): idk if this should be standardized and put in the wparam of WM_SETDEFAULTTEXT, it seems kind of annoying especially since this is something you probably only want to set once, different from the text which you may want to change more often
		ProcState* state = get_state(wnd);
		if (state) {
			state->maintain_placerholder_on_focus = maintain;
			ask_for_repaint(state);//TODO(fran): only ask for repaint when it's actually necessary
		}
	}

	void recalculate_caret(ProcState* state) {
		POINT oldcaretp = state->caret.pos;
		state->caret.pos = calc_caret_p(state);
		if (oldcaretp != state->caret.pos) SetCaretPos(state->caret.pos);
	}

	struct _string_traversal { size_t p; bool reached_limit; };//NOTE: string::npos seems worse since it doesnt give you a valid last pos

	_string_traversal skip_whitespace(utf16_str s, size_t start_p, int direction) {
		size_t last_valid_i;
		for (size_t i = start_p; i < s.sz_char(); i += direction) {
			last_valid_i = i;
			if (!iswspace(s[i])) {
				return { i,false };
			}
		}
		return { last_valid_i,true };
	}

	//goes to first character in word or punctuation group (skips whitespaces)
	//TODO(fran): what to do if we're already at the start of a group?
	_string_traversal goto_start_of_group(utf16_str s, size_t start_p, int direction) {
		Assert(direction < 0);

		auto [x, reached_limit] = skip_whitespace(s, start_p, direction);
		if (reached_limit) return { x, reached_limit };

		bool is_punct = iswpunct(s[x]);//can either be punctuation or alphanumeric

		//go to the start of the previous word or punctuation 'group'
		size_t last_valid_j;
		for (size_t j = x; j < s.sz_char(); j += direction) {
			if ((is_punct ? !iswpunct(s[j]) : !iswalnum(s[j]))) return { j + 1,false };
			last_valid_j = j;
		}
		return { last_valid_j,true };
	}

	//goes one past the last character in word or punctuation group (skips whitespaces)
	//TODO(fran): what to do if we're already at the end of a group?
	_string_traversal goto_end_of_group(utf16_str s, size_t start_p, int direction) {
		Assert(direction > 0);

		auto [x, reached_limit] = skip_whitespace(s, start_p, direction);
		if (reached_limit) return { x, reached_limit };

		bool is_punct = iswpunct(s[x]);//can either be punctuation or alphanumeric

		//go to the end of the word or punctuation 'group'
		size_t last_valid_j;
		for (size_t j = x; j < s.sz_char(); j += direction) {
			if ((is_punct ? !iswpunct(s[j]) : !iswalnum(s[j]))) return { j,false };
			last_valid_j = j;
		}
		return { last_valid_j,true };
	}

	//finds different points where to stop when traversing a string, used for Ctrl+Left/Right Arrows keycombo
	size_t find_stopper(utf16_str s, size_t start_p, int direction/*should be +1 or -1*/) {
		Assert(direction);
		//TODO(fran): handle overflow

		start_p = clamp((decltype(start_p))0, start_p, s.sz_char());

		if (iswspace(s[start_p]) || iswcntrl(s[start_p])) {//we're on a whitespace
			//find first non whitespace in the direction
			size_t last_valid_i;
			for (size_t i = start_p; i < s.sz_char(); i += direction) {
				last_valid_i = i;
				if (!iswspace(s[i]) && !iswcntrl(s[i])) {
					//found a non whitespace char, now go to the beginning/end of that new thing

					bool is_punct = iswpunct(s[i]);//can either be punctuation or alphanumeric

					size_t last_valid_j;
					for (size_t j = i; j < s.sz_char(); j += direction) {
						if ((is_punct ? !iswpunct(s[j]) : !iswalnum(s[j]))) return direction>=0 ? j : j+1;
						last_valid_j = j;
					}

					return last_valid_j;
				}
			}
			return last_valid_i;
		}
		else {
			
			if (iswpunct(s[start_p])) {//we're on a punctuation mark
				//TODO(fran):it seems like everybody does smth different with punctuation, look at visual studio & sublime for examples, so just find what I feel works best

				if (size_t i = start_p; direction < 0 && !iswpunct(s[--i])) {//if going left and we're on the first character of the punctuation group

					//go to first character in previous word or punctuation group
					auto [x,_] = goto_start_of_group(s, i, direction);
					return x;
				}
				else {
					//find first non punctuation in the direction
					for (size_t i = start_p; i < s.sz_char(); i += direction) {
						if (!iswpunct(s[i])) {
							return i;
						}
						//TODO(fran): same fix as in whitespace
					}
				}
			}
			else {//we're on a word
				//find first non word in the direction 
				//TODO(fran): what about langs that dont usually separate words, like japanese? looks pretty hard since you'd actually have to comprehend the text to understand where to cut each word

				if (direction > 0) {//if going right
					for (size_t i = start_p; i < s.sz_char(); i += direction) {
						if (!iswalnum(s[i])) {//find first character not in the current word
							return i;
						}
						//TODO(fran): same fix as in whitespace
					}
				}
				else {//if going left

					if (size_t i = start_p; i > 0 && !iswalnum(s[--i])) {//if we're at the first character of the word

						//we go one back (--i)

						if (iswpunct(s[i])) {//if we're on punctuation
							//go to the start of the punctuation 'group' //TODO(fran): make this things into separate functions for reuse
							size_t last_valid_j;
							for (size_t j = i; j < s.sz_char(); j += direction) {
								if (!iswpunct(s[j])) return j + 1;
								last_valid_j = j;
							}
							return last_valid_j;
						}
						else {//else we're on a whitespace
							//skip all whitespaces and go to the start of the previous word or punctuation 'group'
							auto [x,_] = goto_start_of_group(s, i, direction);
							return x;
						}

					}
					else {//else find first character of the word
						
						size_t last_valid_i;//TODO(fran): initialize?
						for (size_t i = start_p; i < s.sz_char(); i += direction) {
							last_valid_i = i;
							if (!iswalnum(s[i])) {//find first character not in the current word
								return i+1;
							}
						}
						return last_valid_i;//if for example we get to the start of the string then stop there
					}
				}
			}


		}

		return start_p;
	}
	size_t find_next_stopper(utf16_str s, size_t start_p) {
		return find_stopper(s, start_p, +1);
	}
	size_t find_prev_stopper(utf16_str s, size_t start_p) {
		return find_stopper(s, start_p, -1);
	}

	//Positive direction moves cursor/selection to the right, negative to the left
	void move_selection(ProcState* state, int direction, bool shift_is_down, bool ctrl_is_down) {
		size_t anchor, cursor;

		if (shift_is_down && ctrl_is_down) {
			anchor = state->char_cur_sel.anchor;
			cursor = find_stopper(to_utf16_str(state->char_text), state->char_cur_sel.cursor, clamp(-1, direction, +1));
		}
		else if (shift_is_down) {
			//User is adding one extra character to the selection
			anchor = state->char_cur_sel.anchor;
			cursor = clamp((char_sel::type)0, state->char_cur_sel.cursor + direction, state->char_text.length());
		}
		else if (ctrl_is_down) {
			anchor = cursor = find_stopper(to_utf16_str(state->char_text),state->char_cur_sel.cursor,clamp(-1,direction,+1));
		}
		else {
			if (state->char_cur_sel.has_selection()) anchor = cursor = ((direction>=0) ? state->char_cur_sel.x_max() : state->char_cur_sel.x_min());
			else anchor = cursor = clamp((char_sel::type)0, state->char_cur_sel.cursor + direction, state->char_text.length());
		}

		SendMessage(state->wnd, EM_SETSEL, anchor, cursor);
	}

	size_t point_to_char(ProcState* state, POINT mouse/*client coords*/) {
		size_t res=0;
		f32 x = (decltype(x))state->char_pad_x;
		int i = 0;
		for (; i < state->char_dims.size(); i++) {
			f32 d = (f32)state->char_dims[i] / 2.f;
			x += d;
			if ((f32)mouse.x < x) { res = i; break; }
			x += d;
		}
		if (i == state->char_dims.size()) res = i;//if the mouse is rightmost of the last character then simply clip to there
		return res;
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//printf(msgToString(msg)); printf("\n");

		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{ //1st msg received
			CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;

			Assert(!(creation_nfo->style & ES_MULTILINE));
			Assert(!(creation_nfo->style & ES_RIGHT));//TODO(fran)

			ProcState* st = (ProcState*)calloc(1, sizeof(ProcState));
			Assert(st);
			set_state(hwnd, st);
			st->wnd = hwnd;
			st->parent = creation_nfo->hwndParent;
			st->identifier = (u32)(UINT_PTR)creation_nfo->hMenu;
			st->char_max_sz = 32767;//default established by windows
			*st->placeholder = 0;
			//NOTE: ES_LEFT==0, that was their way of defaulting to left
			if (creation_nfo->style & ES_CENTER) {
				//NOTE: ES_CENTER needs the pad to be recalculated all the time

				st->char_pad_x = abs(creation_nfo->cx / 2);//HACK //TODO(fran): this should be calculated on resize since the first size of the wnd is probably not the one that's gonna be used
			}
			else {//we are either left or right
				st->char_pad_x = 3;
			}
			st->char_text = str();//REMEMBER: this c++ guys dont like being calloc-ed, they need their constructor, or, in this case, someone else's, otherwise they are badly initialized
			st->char_dims = std::vector<int>();
			return TRUE; //continue creation
		} break;
		case WM_NCCALCSIZE: { //2nd msg received https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-nccalcsize
			if (wparam) {
				//Indicate part of current client area that is valid
				NCCALCSIZE_PARAMS* calcsz = (NCCALCSIZE_PARAMS*)lparam;
				return 0; //causes the client area to resize to the size of the window, including the window frame
			}
			else {
				RECT* client_rc = (RECT*)lparam;
				//TODO(fran): make client_rc cover the full window area
				return 0;
			}
		} break;
		case WM_CREATE://3rd msg received
		{
			//Create our tooltip
			state->controls.tooltip = CreateWindowEx(WS_EX_TOPMOST/*make sure it can be seen in any wnd config*/, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,//INFO(fran): TTS_BALLOON doesnt work on windows10, you need a registry change https://superuser.com/questions/1349414/app-wont-show-balloon-tip-notifications-in-windows-10 simply amazing
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				state->wnd, NULL, NULL, NULL);
			Assert(state->controls.tooltip);

			TOOLINFO toolInfo { sizeof(toolInfo) };
			toolInfo.hwnd = state->wnd;
			toolInfo.uFlags = TTF_IDISHWND | TTF_ABSOLUTE | TTF_TRACK /*allows to show the tooltip when we please*/; //TODO(fran): TTF_TRANSPARENT might be useful, mouse msgs that go to the tooltip are sent to the parent aka us
			toolInfo.uId = (UINT_PTR)state->wnd;
			toolInfo.rect = { 0 };
			toolInfo.hinst = GetModuleHandle(NULL);
			toolInfo.lpszText = 0;
			toolInfo.lParam = 0;
			toolInfo.lpReserved = 0;
			BOOL addtool_res = (BOOL)SendMessage(state->controls.tooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
			Assert(addtool_res);

			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SIZE: {//4th, strange, I though this was sent only if you didnt handle windowposchanging (or a similar one)
			//NOTE: neat, here you resize your render target, if I had one or cared to resize windows' https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size
			//This msg is received _after_ the window was resized
			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			if (style & ES_CENTER) {
				//Recalc pad_x
				RECT rc; GetClientRect(state->wnd, &rc);
				state->char_pad_x = (RECTWIDTH(rc) - calc_text_dim(state).cx) / 2;

			}
			state->caret.pos = calc_caret_p(state);
			//SetCaretPos(state->caret.pos);//TODO(fran): should I be setting the caret? maybe only if on focus
			

			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE: //5th. Sent on startup after WM_SIZE, although possibly sent by DefWindowProc after I let it process WM_SIZE, not sure
		{
			//This msg is received _after_ the window was moved
			//Here you can obtain x and y of your window's client area
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SHOWWINDOW: //6th. On startup I received this cause of WS_VISIBLE flag
		{
			//Sent when window is about to be hidden or shown, doesnt let it clear if we are in charge of that or it's going to happen no matter what we do
			BOOL show = (BOOL)wparam;
			if (!show) {
				//We're going to be hidden, we must also hide our tooltips
				SendMessage(state->wnd, WM_TIMER, EDITONELINE_tooltip_timer_id, 0); //TODO(fran): #robustness, we should know which timers are active to only disable those, also replace this crude SendMessage with a hide_tooltips()
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCPAINT://7th
		{
			//Paint non client area, we shouldnt have any
			HDC hdc = GetDCEx(hwnd, (HRGN)wparam, DCX_WINDOW | DCX_USESTYLE);
			ReleaseDC(hwnd, hdc);
			return 0; //we process this message, for now
		} break;
		case WM_ERASEBKGND://8th
		{
			//You receive this msg if you didnt specify hbrBackground  when you registered the class, now it's up to you to draw the background
			HDC dc = (HDC)wparam;
			//TODO(fran): look at https://docs.microsoft.com/en-us/windows/win32/gdi/drawing-a-custom-window-background and SetMapModek, allows for transforms

			return 0; //If you return 0 then on WM_PAINT fErase will be true, aka paint the background there
		} break;
		case EM_LIMITTEXT://Set text limit in _characters_ ; does NOT include the terminating null
			//wparam = unsigned int with max char count ; lparam=0
		{
			//TODO(fran): docs says this shouldnt affect text already inside the control nor text set via WM_SETTEXT, not sure I agree with that
			state->char_max_sz = (u32)wparam;
			return 0;
		} break;
		case WM_SETFONT:
		{
			//TODO(fran): should I make a copy of the font? it seems like windows just tells the user to delete the font only after they deleted the control so probably I dont need a copy
			HFONT font = (HFONT)wparam;

			Theme new_theme;
			new_theme.font = font;
			set_theme(state->wnd, &new_theme);

			return 0;
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			//NOTE: the brushes are deleted by whoever created them
			if (state->caret.bmp) {
				DeleteBitmap(state->caret.bmp);
				state->caret.bmp = 0;
			}
			state->char_dims.~vector();
			state->char_text.~basic_string();
			free(state);
			return 0;
		}break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			//ps.rcPaint
			HDC dc = BeginPaint(state->wnd, &ps);
			HBRUSH bk_br, txt_br, border_br, selection_br;
			u32 border_thickness = state->theme.dimensions.border_thickness;
			bool show_placeholder = *state->placeholder && (GetFocus() != state->wnd || state->maintain_placerholder_on_focus) && (state->char_text.length()==0);
			if (IsWindowEnabled(state->wnd)) {
				bk_br = state->theme.brushes.bk.normal;
				txt_br = state->theme.brushes.foreground.normal;
				border_br = state->theme.brushes.border.normal;
				selection_br = state->theme.brushes.selection.normal;
			}
			else {
				bk_br = state->theme.brushes.bk.disabled;
				txt_br = state->theme.brushes.foreground.disabled;
				border_br = state->theme.brushes.border.disabled;
				selection_br = state->theme.brushes.selection.disabled;
			}
			if (show_placeholder) {
				txt_br = state->theme.brushes.foreground.disabled;
			}

			if (style & ES_ROUNDRECT) {
	#if 1
				//Border and Bk
				HPEN pen = CreatePen(PS_SOLID, border_thickness, ColorFromBrush(border_thickness ? border_br : bk_br)); defer{ DeletePen(pen); };
				HPEN oldpen = SelectPen(dc, pen); defer{ SelectPen(dc,oldpen); };
				HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
				i32 roundedness = max(1, (i32)roundf((f32)min(w, h) * .2f));
				RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, roundedness, roundedness);
	#else
				//Bk
				FillRect(dc, &rc, bk_br);
				//Border
				urender::RoundRectangleBorder(dc, border_br, rc, 5, 1);
	#endif
			} 
			else {
				//Border
				FillRectBorder(dc, rc, border_thickness, border_thickness ? border_br : bk_br, BORDERLEFT | BORDERTOP | BORDERRIGHT | BORDERBOTTOM);//HACK: instead of not rendering the border Im simply making it be the same color as the bk, I may want to change that (a border of 0 size still gets rendered as 1px, which means there will be overdraw with the bk rendering)

				//Bk
				//Clip the drawing region to avoid overriding the border
				HRGN restoreRegion = CreateRectRgn(0, 0, 0, 0); if (GetClipRgn(dc, restoreRegion) != 1) { DeleteObject(restoreRegion); restoreRegion = NULL; } defer{ SelectClipRgn(dc, restoreRegion); if (restoreRegion != NULL) DeleteObject(restoreRegion); };
				{
					RECT left = rectNpxL(rc, border_thickness);
					RECT top = rectNpxT(rc, border_thickness);
					RECT right = rectNpxR(rc, border_thickness);
					RECT bottom = rectNpxB(rc, border_thickness);
					ExcludeClipRect(dc, left.left, left.top, left.right, left.bottom);
					ExcludeClipRect(dc, top.left, top.top, top.right, top.bottom);
					ExcludeClipRect(dc, right.left, right.top, right.right, right.bottom);
					ExcludeClipRect(dc, bottom.left, bottom.top, bottom.right, bottom.bottom);
					//TODO(fran): this aint too clever, it'd be easier to set the clipping region to the deflated rect
				}
				RECT bk_rc = rc; InflateRect(&bk_rc, -(i32)border_thickness, -(i32)border_thickness);
				FillRect(dc, &bk_rc, bk_br);//TODO(fran): we need to clip this where the text was already drawn, this will be the last thing we paint

			}

			int text_y;//top of the text
			{
				HFONT oldfont = SelectFont(dc, state->theme.font); defer{ SelectFont(dc, oldfont); };
				UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };

				COLORREF oldtxtcol = SetTextColor(dc, ColorFromBrush(txt_br)); defer{ SetTextColor(dc, oldtxtcol); };
				COLORREF oldbkcol = SetBkColor(dc, ColorFromBrush(bk_br)); defer{ SetBkColor(dc, oldbkcol); };

				TEXTMETRIC tm; GetTextMetrics(dc, &tm);
				// Calculate vertical position for the string so that it will be vertically centered
				// We are single line so we want vertical alignment always
				int yPos = (rc.bottom + rc.top - tm.tmHeight) / 2;
				int xPos;

				text_y = yPos;

				LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
				//ES_LEFT ES_CENTER ES_RIGHT
				//TODO(fran): store char positions in the vector
				if (style & ES_CENTER) {
					SetTextAlign(dc, TA_CENTER);
					xPos = (rc.right - rc.left) / 2;
				}
				else if (style & ES_RIGHT) {
					SetTextAlign(dc, TA_RIGHT);
					xPos = rc.right - state->char_pad_x;
				}
				else /*ES_LEFT*/ {//NOTE: ES_LEFT==0, that was their way of defaulting to left
					SetTextAlign(dc, TA_LEFT);
					xPos = rc.left + state->char_pad_x;
				}

				if (show_placeholder) {
					TextOut(dc, xPos, yPos, state->placeholder, (int)cstr_len(state->placeholder));
				}
				else if (style & ES_PASSWORD) {
					//TODO(fran): what's faster, full allocation or for loop drawing one by one
					cstr* pass_text = (cstr*)malloc(state->char_text.length() * sizeof(cstr));
					for (size_t i = 0; i < state->char_text.length(); i++)pass_text[i] = password_char;

					TextOut(dc, xPos, yPos, pass_text, (int)state->char_text.length());
				}
				else {
					TextOut(dc, xPos, yPos, state->char_text.c_str(), (int)state->char_text.length());
				}
			}

			{ //Render Selection
				if (state->char_cur_sel.has_selection()) {
					//We use the height of the caret (for now) as the height of the selection
					RECT selection;
					selection.top = text_y;
					selection.bottom = selection.top + state->caret.dim.cy;
					selection.left = state->char_pad_x;
					for (size_t i = 0; i < state->char_cur_sel.x_min(); i++)//TODO(fran): make into a function
						selection.left += state->char_dims[i];
					selection.right = selection.left;
					for (size_t i = state->char_cur_sel.x_min(); i < state->char_cur_sel.x_max(); i++)
						selection.right += state->char_dims[i];
					COLORREF sel_col = ColorFromBrush(selection_br);
					urender::FillRectAlpha(dc, selection, GetRValue(sel_col), GetGValue(sel_col), GetBValue(sel_col), 128);
					//TODO(fran): instead of painting over the text with alpha we need to change the bk color of the selected section when rendering the text, otherwise the text color gets too opaqued and looks bad
				}
			}

			EndPaint(hwnd, &ps);
			return 0;
		} break;
		case WM_ENABLE:
		{//Here we get the new enabled state
			BOOL now_enabled = (BOOL)wparam;
			InvalidateRect(state->wnd, NULL, TRUE);
			//TODO(fran): Hide caret
			return 0;
		} break;
		case WM_CANCELMODE:
		{
			//Stop mouse capture, and similar input related things
			ReleaseCapture();
			state->OnMouseTracking = false;
			//Sent, for example, when the window gets disabled
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCHITTEST://When the mouse goes over us this is 1st msg received
		{
			//Received when the mouse goes over the window, on mouse press or release, and on WindowFromPoint

			// Get the point coordinates for the hit test.
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Screen coords, relative to upper-left corner

			// Get the window rectangle.
			RECT rw; GetWindowRect(state->wnd, &rw);

			LRESULT hittest = HTNOWHERE;

			// Determine if the point is inside the window
			if (test_pt_rc(mouse, rw))hittest = HTCLIENT;

			return hittest;
		} break;
		case WM_SETCURSOR://When the mouse goes over us this is 2nd msg received
		{
			//DefWindowProc passes this to its parent to see if it wants to change the cursor settings, we'll make a decision, setting the mouse cursor, and halting proccessing so it stays like that
			//Sent after getting the result of WM_NCHITTEST, mouse is inside our window and mouse input is not being captured

			/* https://docs.microsoft.com/en-us/windows/win32/learnwin32/setting-the-cursor-image
				if we pass WM_SETCURSOR to DefWindowProc, the function uses the following algorithm to set the cursor image:
				1. If the window has a parent, forward the WM_SETCURSOR message to the parent to handle.
				2. Otherwise, if the window has a class cursor, set the cursor to the class cursor.
				3. If there is no class cursor, set the cursor to the arrow cursor.
			*/
			//NOTE: I think this is good enough for now
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEMOVE /*WM_MOUSEFIRST*/://When the mouse goes over us this is 3rd msg received
		{
			//wparam = test for virtual keys pressed
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area

			if (state->OnMouseTracking) {
				//user is trying to make a selection
				size_t cursor = point_to_char(state, mouse);
				SendMessage(state->wnd, EM_SETSEL, state->char_cur_sel.anchor, cursor);
			}

			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEACTIVATE://When the user clicks on us this is 1st msg received
		{
			HWND parent = (HWND)wparam;
			WORD hittest = LOWORD(lparam);
			WORD mouse_msg = HIWORD(lparam);
			SetFocus(state->wnd);//set keyboard focus to us
			return MA_ACTIVATE; //Activate our window and post the mouse msg
		} break;
		case WM_LBUTTONDOWN://When the user clicks on us this is 2nd msg received (possibly, maybe wm_setcursor again)
		{
			//wparam = test for virtual keys pressed
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area

			SetCapture(state->wnd);//We want to keep capturing the mouse while the user is still pressing some button, even if the mouse leaves our client area
			state->OnMouseTracking = true;

			size_t cursor = point_to_char(state, mouse);
			SendMessage(state->wnd, EM_SETSEL, cursor, cursor);
			
			return 0;
		} break;
		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			state->OnMouseTracking = false;
		} break;
		case WM_SETFOCUS://SetFocus -> WM_IME_SETCONTEXT -> WM_SETFOCUS
		{
			//We gained keyboard focus
			SIZE caret_dim = calc_caret_dim(state);
			if (caret_dim.cx != state->caret.dim.cx || caret_dim.cy != state->caret.dim.cy) {
				if (state->caret.bmp) {
					DeleteBitmap(state->caret.bmp);
					state->caret.bmp = 0;
				}
				//TODO(fran): we should probably check the bpp of our control's img
				int pitch = caret_dim.cx * 4/*bytes per pixel*/;//NOTE: windows expects word aligned bmps, 32bits are always word aligned
				int sz = caret_dim.cy * pitch;
				u32* mem = (u32*)malloc(sz); defer{ free(mem); };
				COLORREF color = ColorFromBrush(state->theme.brushes.foreground.disabled);

				//IMPORTANT: changing caret color is gonna be a pain, docs: To display a solid caret (NULL in hBitmap), the system inverts every pixel in the rectangle; to display a gray caret ((HBITMAP)1 in hBitmap), the system inverts every other pixel; to display a bitmap caret, the system inverts only the white bits of the bitmap.
				//Aka we either calculate how to arrive at the color we want from the caret's bit flipping (will invert bits with 1) or we give up, we should do our own caret
				//NOTE: since the caret is really fucked up we'll average the pixel color so we at least get grays
				//TODO(fran): decide what to do, the other CreateCaret options work as bad or worse so we have to go with this, do we create a separate brush so the user can mix and match till they find a color that blends well with the bk?
				u8 gray = ((u16)(u8)(color >> 16) + (u16)(u8)(color >> 8) + (u16)(u8)color) / 3;
				color = RGB(gray, gray, gray);

				int px_count = sz / 4;
				for (int i = 0; i < px_count; i++) mem[i] = color;
				state->caret.bmp = CreateBitmap(caret_dim.cx, caret_dim.cy, 1, 32, (void*)mem);
				state->caret.dim = caret_dim;
			}
			BOOL caret_res = CreateCaret(state->wnd, state->caret.bmp, 0, 0);
			Assert(caret_res);
			state->caret.pos = calc_caret_p(state);
			SetCaretPos(state->caret.pos);
			BOOL showcaret_res = ShowCaret(state->wnd); //TODO(fran): the docs seem to imply you display the caret here but I dont wanna draw outside wm_paint
			Assert(showcaret_res);

			//Check in case we are showing the default text, when we get keyboard focus that text should disappear
			bool show_placeholder = *state->placeholder && (state->char_text.length() == 0);//this one doesnt check GetFocus since it's always gonna be us
			if (show_placeholder) InvalidateRect(state->wnd, NULL, TRUE);

			return 0;
		} break;
		case WM_KILLFOCUS:
		{
			//We lost keyboard focus
			//TODO(fran): docs say we should destroy the caret now
			DestroyCaret();
			//Also says to not display/activate a window here cause we can lock the thread
			bool show_placeholder = *state->placeholder && (state->char_text.length() == 0);//this one doesnt check GetFocus since it's never gonna be us
			if(show_placeholder) InvalidateRect(state->wnd, NULL, TRUE);

			PostMessage(GetParent(state->wnd), WM_COMMAND, MAKELONG(state->identifier, EN_KILLFOCUS), (LPARAM)state->wnd);

			return 0;
		} break;
		case WM_KEYDOWN://When the user presses a non-system key this is the 1st msg we receive
		{
			//Notifications:
			bool en_change = false;

			//TODO(fran): here we process things like VK_HOME VK_NEXT VK_LEFT VK_RIGHT VK_DELETE
			//NOTE: there are some keys that dont get sent to WM_CHAR, we gotta handle them here, also there are others that get sent to both, TODO(fran): it'd be nice to have all the key things in only one place
			//NOTE: for things like _shortcuts_ you wanna handle them here cause on WM_CHAR things like Ctrl+V get translated to something else
			//		also you want to use the uppercase version of the letter, eg case _t('V'):
			char vk = (char)wparam;
			bool ctrl_is_down = HIBYTE(GetKeyState(VK_CONTROL));
			bool shift_is_down = HIBYTE(GetKeyState(VK_SHIFT));
			//printf("%c : %d\n", vk, (i32)vk);
			switch (vk) {
			case VK_HOME://Home
			{
				size_t anchor, cursor;

				if (shift_is_down && ctrl_is_down) {//Make a selection to the start of the text
					anchor = state->char_cur_sel.anchor;
					cursor = 0;
				}
				else if (shift_is_down) {//Make a selection to the start of the line
					anchor = state->char_cur_sel.anchor;
					cursor = 0;
				}
				else if (ctrl_is_down) {//Remove selection and Go to the start of the text
					anchor = cursor = 0;
				}
				else anchor = cursor = 0;//Remove selection and Go to the start of the line (since we're singleline go to start of text)

				SendMessage(state->wnd, EM_SETSEL, anchor, cursor);
			} break;
			case VK_END://End
			{
				size_t anchor, cursor;

				if (shift_is_down && ctrl_is_down) {//Make a selection to the end of the text
					anchor = state->char_cur_sel.anchor;
					cursor = state->char_text.length();
				}
				else if (shift_is_down) {//Make a selection to the end of the line
					anchor = state->char_cur_sel.anchor;
					cursor = state->char_text.length();
				}
				else if (ctrl_is_down) {//Remove selection and Go to the end of the text
					anchor = cursor = state->char_text.length();
				}
				else anchor = cursor = state->char_text.length();//Remove selection and Go to the end of the line (since we're singleline go to start of text)

				SendMessage(state->wnd, EM_SETSEL, anchor, cursor);
			} break;
			case VK_LEFT://Left arrow
			{
				move_selection(state, -1, shift_is_down, ctrl_is_down);
			} break;
			case VK_RIGHT://Right arrow
			{
				move_selection(state, +1, shift_is_down, ctrl_is_down);

			} break;
			case VK_DELETE://What in spanish is the "Supr" key (delete character ahead of you)
			{
				if (!state->char_text.empty()) {
					if (state->char_cur_sel.has_selection())remove_selection(state);
					else {
						if (ctrl_is_down && shift_is_down) {//delete everything til end of the line
							remove_selection(state, state->char_cur_sel.cursor, state->char_text.length());
						}
						else if (ctrl_is_down) {//delete everything up to the next stopper
							remove_selection(state, state->char_cur_sel.cursor, find_stopper(to_utf16_str(state->char_text),state->char_cur_sel.cursor,+1));
						}
						else if (shift_is_down) {//save whole line to clipboard and then delete it
							SendMessage(state->wnd, EM_SETSEL, 0, -1);
							SendMessage(state->wnd, WM_COPY, 0, 0);
							remove_selection(state);
						}
						else remove_selection(state, state->char_cur_sel.cursor, state->char_cur_sel.cursor + 1);//delete character in front of the cursor
					}
					en_change = true;
				}
			} break;
			case VK_BACK://Backspace
			{
				if (!state->char_text.empty()) {
					if (state->char_cur_sel.has_selection())remove_selection(state);
					else {

						if(ctrl_is_down && shift_is_down) remove_selection(state, 0, state->char_cur_sel.cursor); //Remove every character from cursor to line start
						else if(ctrl_is_down) remove_selection(state, find_stopper(to_utf16_str(state->char_text), state->char_cur_sel.cursor, -1), state->char_cur_sel.cursor);
						else remove_selection(state, state->char_cur_sel.cursor - 1, state->char_cur_sel.cursor);
					}
					en_change = true;
				}
			} break;
			case _t('v'):
			case _t('V'):
			{
				if (ctrl_is_down) {
					SendMessage(state->wnd, WM_PASTE, 0, 0);
				}
			} break;
			case _t('c'):
			case _t('C'):
			{
				if (ctrl_is_down) {
					SendMessage(state->wnd, WM_COPY, 0, 0);
				}
			} break;
			case _t('x'):
			case _t('X'):
			{
				if (ctrl_is_down) {
					SendMessage(state->wnd, WM_CUT, 0, 0);
				}
			} break;
			}


			InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): dont invalidate everything, NOTE: also on each wm_paint the cursor will stop so we should add here a bool repaint; to avoid calling InvalidateRect when it isnt needed
			if (en_change) notify_parent(state, EN_CHANGE); //There was a change in the text
			return 0;
		}break;
		case WM_CUT:
		{
			SendMessage(state->wnd, WM_COPY, 0, 0);
			remove_selection(state);
		} break;
		case WM_COPY:
		{
#ifdef UNICODE
			UINT format = CF_UNICODETEXT;
#else
			UINT format = CF_TEXT;
#endif
			//Copy text from current selection to clipboard
			if (state->char_cur_sel.has_selection()) {
				if (OpenClipboard(state->wnd)) {
					defer{ CloseClipboard(); };
					HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (state->char_cur_sel.sel_width() + 1) * sizeof(state->char_text[0])); Assert(mem);//TODO(fran): runtime_assert ?

					{
						void* txt = GlobalLock(mem); Assert(txt); defer{ GlobalUnlock(mem); };

						memcpy(txt, state->char_text.c_str() + state->char_cur_sel.x_min(), state->char_cur_sel.sel_width() * sizeof(state->char_text[0]));//copy selection
						((decltype(&state->char_text[0]))txt)[state->char_cur_sel.sel_width() + 1] = 0;//null terminate
					}
					
					EmptyClipboard();
					auto setclipret = SetClipboardData(format, mem);

					if (!setclipret) GlobalFree(mem);//free mem if for some reason we fail to set the clipboard with our data
					else state->clipboard_handle = mem;//store handle so we can free it on WM_DESTROYCLIPBOARD
				}
			}

		} break;
		case WM_PASTE:
		{
			//Notifications:
			bool en_change = false;

			//TODO(fran): pasting onto the selected region
			//TODO(fran): if no unicode is available we could get the ansi and convert it, if it is available. //NOTE: docs say the format is converted automatically to the one you ask for
	#ifdef UNICODE
			UINT format = CF_UNICODETEXT;
	#else
			UINT format = CF_TEXT;
	#endif
			if (IsClipboardFormatAvailable(format)) {//NOTE: lines end with \r\n, has null terminator
				if (OpenClipboard(state->wnd)) {
					defer{ CloseClipboard(); };
					HGLOBAL clipboard = GetClipboardData(format);
					if (clipboard) {
						cstr* clipboardtxt = (cstr*)GlobalLock(clipboard);
						if (clipboardtxt)
						{
							defer{ GlobalUnlock(clipboard); };
							bool paste_res = paste_from_clipboard(state, clipboardtxt); //TODO(fran): this should be separated into two fns, a general paste fn and first a sanitizer for anything strange that may be in the clipboard txt
							en_change = paste_res;
						}

					}
				}
			}
			if (en_change) notify_parent(state, EN_CHANGE); //There was a change in the text
		} break;
		case WM_CHAR://When the user presses a non-system key this is the 2nd msg we receive
		{//NOTE: a WM_KEYDOWN msg was translated by TranslateMessage() into WM_CHAR
			//Notifications:
			bool en_change = false;

			TCHAR c = (TCHAR)wparam;
			bool ctrl_is_down = HIBYTE(GetKeyState(VK_CONTROL));
			//lparam = flags
			switch (c) { //https://docs.microsoft.com/en-us/windows/win32/menurc/using-carets
			case 127://Ctrl + Backspace
			{
				/*if (!state->char_text.empty()) {
					if (state->char_cur_sel.has_selection())remove_selection(state);
					else remove_selection(state, find_stopper(to_utf16_str(state->char_text), state->char_cur_sel.cursor, -1), state->char_cur_sel.cursor);

					en_change = true;
				}*/
				//do nothing, we already handled it on WM_KEYDOWN
			} break;
			case VK_BACK://Backspace (for some reason it gets sent as WM_CHAR even though it can be handled in WM_KEYDOWN)
			{
				//if (!state->char_text.empty()) {
				//	if (state->char_cur_sel.has_selection())remove_selection(state);
				//	else remove_selection(state, state->char_cur_sel.cursor - 1, state->char_cur_sel.cursor);

				//	en_change = true;
				//}
				//do nothing, we already handled it on WM_KEYDOWN
			}break;
			case VK_TAB://Tab
			{
				LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
				if (style & WS_TABSTOP) {//TODO(fran): I think we should specify a style that specifically says on tab pressed change to next control, since this style is just to say I want that to happen to me
					SetFocus(GetNextDlgTabItem(GetParent(state->wnd), state->wnd, FALSE));
				}
				//We dont handle tabs for now
			}break;
			case VK_RETURN://Received when the user presses the "enter" key //Carriage Return aka \r
			{
				//NOTE: I wonder, it doesnt seem to send us \r\n so is that something that is manually done by the control?
				//We dont handle "enter" for now
				//if(state->identifier) //TODO(fran): should I only send notifs if I have an identifier? what do the defaults do?
					PostMessage(GetParent(state->wnd), WM_COMMAND, MAKELONG(state->identifier, EN_ENTER), (LPARAM)state->wnd);
			}break;
			case VK_ESCAPE://Escape
			{
				//TODO(fran): should we do something?
				PostMessage(GetParent(state->wnd), WM_COMMAND, MAKELONG(state->identifier, EN_ESCAPE), (LPARAM)state->wnd);
			}break;
			case 0x0A://Linefeed, aka \n
			{
				//I havent found which key triggers this
				printf("WM_CHAR = linefeed\n");
			}break;
			default:
			{
				//TODO(fran): filter more values
				if (c <= (TCHAR)0x14) break;//control chars
				if (c <= (TCHAR)0x1f) break;//IME

				//TODO(fran): maybe this should be the first check for any WM_CHAR?
				if (contains_char(c,state->invalid_chars)) {
					//display tooltip //TODO(fran): make this into a reusable function
					std::wstring tooltip_msg = (RS(10) + L" " + state->invalid_chars);
					show_tip(state->wnd, tooltip_msg.c_str(), EDITONELINE_default_tooltip_duration, ETP::left | ETP::top);
					return 0;
				}

				//We have some normal character
				//TODO(fran): what happens with surrogate pairs? I dont even know what they are -> READ
				if (safe_subtract0(state->char_text.length(), state->char_cur_sel.sel_width()) < state->char_max_sz) {
					insert_character(state, c);
					
					LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);//TODO(fran): again, which function is gonna take the responsibility of doing this?
					if (style & ES_CENTER) {
						//Recalc pad_x
						RECT rc; GetClientRect(state->wnd, &rc);
						state->char_pad_x = (RECTWIDTH(rc) - calc_text_dim(state).cx) / 2;
					}
					state->caret.pos = calc_caret_p(state);
					SetCaretPos(state->caret.pos);

					//wprintf(L"%s\n", state->char_text.c_str());
					en_change = true;
				}

			}break;
			}
			InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): dont invalidate everything
			if (en_change) notify_parent(state, EN_CHANGE); //There was a change in the text
			return 0;
		} break;
		case WM_TIMER:
		{
			WPARAM timerID = wparam;
			switch (timerID) {
			case EDITONELINE_tooltip_timer_id:
			{
				KillTimer(state->wnd, timerID);
				TOOLINFO toolInfo{ sizeof(toolInfo) };
				toolInfo.hwnd = state->wnd;
				toolInfo.uId = (UINT_PTR)state->wnd;
				SendMessage(state->controls.tooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);
				return 0;
			} break;
			default: return DefWindowProc(hwnd, msg, wparam, lparam);
			}
		} break;
		case WM_KEYUP:
		{
			//TODO(fran): smth to do here?
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_GETTEXT://the specified char count must include null terminator, since windows' defaults to force writing it to you
		{
			LRESULT res;
			int char_cnt_with_null = max((int)wparam, 0);//Includes null char
			int char_text_cnt_with_null = (int)(state->char_text.length() + 1);
			if (char_cnt_with_null > char_text_cnt_with_null) char_cnt_with_null = char_text_cnt_with_null;
			cstr* buf = (cstr*)lparam;
			if (buf) {//should I check?
				StrCpyN(buf, state->char_text.c_str(), char_cnt_with_null);
				if (char_cnt_with_null < char_text_cnt_with_null) buf[char_cnt_with_null - 1] = (cstr)0;
				res = char_cnt_with_null - 1;
			}
			else res = 0;
			return res;
		} break;
		case WM_GETTEXTLENGTH://does not include null terminator
		{
			return state->char_text.length();
		} break;
		case WM_SETTEXT:
		{
			//Notifications:
			bool en_change = false;

			cstr* buf = (cstr*)lparam;//null terminated
		
			BOOL res = edit_oneline::_settext(state, buf);

			en_change = res;
			if (en_change) notify_parent(state, EN_CHANGE); //There was a change in the text

			return res;
		}break;
		case WM_SETTEXT_NO_NOTIFY:
		{
			cstr* buf = (cstr*)lparam;//null terminated

			BOOL res = edit_oneline::_settext(state, buf);

			return res;
		} break;
		case WM_SYSKEYDOWN://1st msg received after the user presses F10 or Alt+some key
		{
			//TODO(fran): notify the parent?
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SYSKEYUP:
		{
			//TODO(fran): notify the parent?
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_IME_SETCONTEXT://Sent after SetFocus to us
		{//TODO(fran): lots to discover here
			BOOL is_active = (BOOL)wparam;
			//lparam = display options for IME window
			u32 disp_flags = (u32)lparam;
			//NOTE: if we draw the composition window we have to modify some of the lparam values before calling defwindowproc or ImmIsUIMessage

			//NOTE: here you can hide the default IME window and all its candidate windows

			//TODO(fran): with this I get to hide the candidate window & in set_composition_pos I can "hide" the composition window, only problem left is the up and down arrow keys are still bound to the IME window and interact with the now hidden candidate window, I need to block the up&down arrows from getting to the IME and use them myself, or Create my own IME window that's more configurable
			//TODO(fran): maybe I can intercept the "new candidate request" through WM_IME_NOTIFY or somewhere else, stop it from getting to the IME window and transforming it into a WM_LBUTTONDOWN with up or down arrow as key
#if 0
			if (state->hide_IME_wnd) {
				lparam = 0;
			}
#endif

			//If we have created an IME window, call ImmIsUIMessage. Otherwise pass this message to DefWindowProc
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}break;
		case WM_IME_NOTIFY:
		{
			//Notifies about changes to the IME window
			//TODO(fran): process this msgs once we manage the ime window
			u32 command = (u32)wparam;
			//lparam = command specific data
			#if 0
			const char* notif;
			switch (command) {
			case IMN_CHANGECANDIDATE:	   notif = "IMN_CHANGECANDIDATE"; break;
			case IMN_CLOSECANDIDATE:	   notif = "IMN_CLOSECANDIDATE"; break;
			case IMN_CLOSESTATUSWINDOW:	   notif = "IMN_CLOSESTATUSWINDOW"; break;
			case IMN_GUIDELINE:			   notif = "IMN_GUIDELINE"; break;
			case IMN_OPENCANDIDATE:		   notif = "IMN_OPENCANDIDATE"; break;
			case IMN_OPENSTATUSWINDOW:	   notif = "IMN_OPENSTATUSWINDOW"; break;
			case IMN_SETCANDIDATEPOS:	   notif = "IMN_SETCANDIDATEPOS"; break;
			case IMN_SETCOMPOSITIONFONT:   notif = "IMN_SETCOMPOSITIONFONT"; break;
			case IMN_SETCOMPOSITIONWINDOW: notif = "IMN_SETCOMPOSITIONWINDOW"; break;
			case IMN_SETCONVERSIONMODE:	   notif = "IMN_SETCONVERSIONMODE"; break;
			case IMN_SETOPENSTATUS:		   notif = "IMN_SETOPENSTATUS"; break;
			case IMN_SETSENTENCEMODE:	   notif = "IMN_SETSENTENCEMODE"; break;
			case IMN_SETSTATUSWINDOWPOS:   notif = "IMN_SETSTATUSWINDOWPOS"; break;
			case 0xf:					   notif = "HIDDEN IME NOTIF?! 0xf"; break;//probably IME_SETCOMPOSITION or smth like that, happens when you press a key
			case 0x10d:					   notif = "HIDDEN IME NOTIF?! 0x10d"; break;
			case 0x10e:					   notif = "HIDDEN IME NOTIF?! 0x10e"; break;//probably IME_CANCEL or smth like that, happens when the ime window is closed eg by pressing escape
			default: notif = 0; Assert(0);
			}
			printf("WM_IME_NOTIFY: %s\n", notif);
			#endif
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_IME_REQUEST://After Alt+Shift to change the keyboard (and some WM_IMENOTIFY) we receive this msg
		{
			#if 0
			const char* req;
			switch (wparam) {
			case IMR_CANDIDATEWINDOW:			req = "IMR_CANDIDATEWINDOW"; break;
			case IMR_COMPOSITIONFONT:			req = "IMR_COMPOSITIONFONT"; break;
			case IMR_COMPOSITIONWINDOW:			req = "IMR_COMPOSITIONWINDOW"; break;
			case IMR_CONFIRMRECONVERTSTRING:	req = "IMR_CONFIRMRECONVERTSTRING"; break;
			case IMR_DOCUMENTFEED:				req = "IMR_DOCUMENTFEED"; break;
			case IMR_QUERYCHARPOSITION:			req = "IMR_QUERYCHARPOSITION"; break;
			case IMR_RECONVERTSTRING:			req = "IMR_RECONVERTSTRING"; break;
			default:req = 0; Assert(0);
			}
			printf("WM_IME_REQUEST: %s\n", req);
			#endif
			
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_INPUTLANGCHANGE://After Alt+Shift to change the keyboard (and some WM_IMENOTIFY) and WM_IME_REQUEST we receive this msg
		{
			//wparam = charset of the new locale
			//lparam = input locale identifier... wtf
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_IME_STARTCOMPOSITION://On japanese keyboard, after we press a key to start writing this is 1st msg received
		{
			//doc:Sent immediately before the IME generates the composition string as a result of a keystroke
			//This is a notification to an IME window to open its composition window. An application should process this message if it displays composition characters itself.
			//If an application has created an IME window, it should pass this message to that window.The DefWindowProc function processes the message by passing it to the default IME window.
			set_composition_pos(state);
			set_composition_font(state);//TODO(fran): should I place this somewhere else?

			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_IME_COMPOSITION://On japanese keyboard, after we press a key to start writing this is 2nd msg received
		{//doc: sent when IME changes composition status cause of keystroke
			//wparam = DBCS char for latest change to composition string, TODO(fran): find out about DBCS

			//TODO(fran): idk which is the best place to handle IME changes, there's also WM_IME_NOTIFY and WM_IME_REQUEST(probably not in this one)

			auto defret = DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): this will generate WM_CHAR msgs that I dont really want, I can retrieve the composition string right from here, I shouldnt call DefWindowProc
			bool en_change = false;

			//TODO(fran): find a way to know when the IME was accepted, we dont really want to receive the accepted text as WM_CHAR messages since we already have the text written into the control
			if (lparam == 0) {
				//IME was cancelled, delete whatever was written with it
				if (state->char_cur_sel.has_selection()) {
					remove_selection(state);
					en_change = true;
				}
			}
			else {//TODO(fran): check lparam, we may not always want to update the text depending on the code it has
				HIMC imc = ImmGetContext(state->wnd);
				if (imc != NULL) {
					defer{ ImmReleaseContext(state->wnd, imc); };
					//INFO: ImmGetCompositionStringW: https://cpp.hotexamples.com/examples/-/-/ImmGetCompositionStringW/cpp-immgetcompositionstringw-function-examples.html
					int szbytes = ImmGetCompositionStringW(imc, GCS_COMPSTR, 0, 0);//excluding null terminator
					if (szbytes > 0) {//otherwise gotta handle possible errors
						utf16* txt;
						//szbytes += (1 * sizeof(*txt));//include null terminator
						txt = (decltype(txt))malloc(szbytes + sizeof(*txt)); defer{ free(txt); };

						auto len = ImmGetCompositionStringW(imc, GCS_COMPSTR, txt, szbytes) / sizeof(*txt);
						txt[len] = 0;//ImmGetCompositionStringW does _not_ write the null terminator

						insert_character(state, txt);


						LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);//TODO(fran): again, which function is gonna take the responsibility of doing this?
						if (style & ES_CENTER) {
							//Recalc pad_x
							RECT rc; GetClientRect(state->wnd, &rc);
							state->char_pad_x = (RECTWIDTH(rc) - calc_text_dim(state).cx) / 2;
						}
						state->caret.pos = calc_caret_p(state);
						SetCaretPos(state->caret.pos);

						en_change = true;

						SendMessage(state->wnd, EM_SETSEL, safe_subtract0(state->char_cur_sel.cursor, len), state->char_cur_sel.cursor);
					}
				}
			}
			if (en_change) notify_parent(state, EN_CHANGE); //There was a change in the text
			return defret;
		} break;
		case WM_IME_CHAR://WM_CHAR from the IME window, this are generated once the user has pressed enter on the IME window, so more than one char will probably be coming
		{
			//UNICODE: wparam=utf16 char ; ANSI: DBCS char
			//lparam= the same flags as WM_CHAR
	#ifndef UNICODE
			Assert(0);//TODO(fran): DBCS
	#endif 
			PostMessage(state->wnd, WM_CHAR, wparam, lparam);

			return 0;//docs dont mention return value so I guess it dont matter
		} break;
		case WM_IME_ENDCOMPOSITION://After the chars are sent from the IME window it hides/destroys itself (idk)
		{
			//TODO: Handle once we process our own IME
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		//case WM_IME_CONTROL: //NOTE: I feel like this should be received by the wndproc of the IME, I dont think I can get DefWndProc to send it there for me
		//{
		//	//Used to manually send some msg to the IME window, in my case I use it so DefWindowProc sends my msg to it IME default wnd
		//	return DefWindowProc(hwnd, msg, wparam, lparam);
		//}break;
		case WM_SETDEFAULTTEXT:
		{
			cstr* text = (cstr*)lparam;
		
			memcpy_s(state->placeholder, sizeof(state->placeholder), text, (cstr_len(text) + 1) * sizeof(*text));
			//TODO(fran): check whether we need to redraw
			return 1;
		} break;
		case EM_SETINVALIDCHARS:
		{
			cstr* chars = (cstr*)lparam;
			memcpy_s(state->invalid_chars, sizeof(state->invalid_chars), chars, (cstr_len(chars) + 1) * sizeof(*chars));
		} break;
		case WM_NOTIFYFORMAT://1st msg sent by our tooltip
		{
			switch (lparam) {
			case NF_QUERY: return sizeof(cstr) > 1 ? NFR_UNICODE : NFR_ANSI; //a child of ours has asked us
			case NF_REQUERY: return SendMessage((HWND)wparam/*parent*/, WM_NOTIFYFORMAT, (WPARAM)state->wnd, NF_QUERY); //NOTE: the return value is the new notify format, right now we dont do notifications so we dont care, but this could be a TODO(fran)
			}
			return 0;
		} break;
		case WM_QUERYUISTATE://Strange msg, it wants to know the UI state for a window, I assume that means the tooltip is asking us how IT should look?
		{
			return UISF_ACTIVE | UISF_HIDEACCEL | UISF_HIDEFOCUS; //render as active wnd, dont show keyboard accelerators nor focus indicators
		} break;
		case WM_NOTIFY:
		{
			NMHDR* msg_info = (NMHDR*)lparam;
			if (msg_info->hwndFrom == state->controls.tooltip) {
				switch (msg_info->code) {
				case NM_CUSTOMDRAW:
				{
					NMTTCUSTOMDRAW* cd = (NMTTCUSTOMDRAW*)lparam;
					//INFO: cd->uDrawFlags = flags for DrawText
					switch (cd->nmcd.dwDrawStage) {
						//TODO(fran): probably case CDDS_PREERASE: for SetBkColor for the background
					case CDDS_PREPAINT: 
					{
						return CDRF_NOTIFYITEMDRAW;//TODO(fran): I think im lacking something here, we never get CDDS_ITEMPREPAINT, it's possible the msgs are not sent cause it uses a visual style, in which case it doesnt care about you, we would have to remove it with setwindowtheme, and since there wont be any style we'll have to draw it completely ourselves I guess
					} break;
					case CDDS_ITEMPREPAINT://Setup before painting an item
					{
						SelectFont(cd->nmcd.hdc, state->theme.font);
						SetTextColor(cd->nmcd.hdc, ColorFromBrush(state->theme.brushes.foreground.normal));
						SetBkColor(cd->nmcd.hdc, ColorFromBrush(state->theme.brushes.bk.normal));
						return CDRF_NEWFONT;
					} break;
					default: return CDRF_DODEFAULT;
					}
				} break;
				case TTN_GETDISPINFO:
					Assert(0);
				case TTN_LINKCLICK:
					Assert(0);
				case TTN_POP://Tooltip is about to be hidden
					return 0;
				case TTN_SHOW://Tooltip is about to be shown
					return 0;//here we can change the tooltip's position
				}
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_GETFONT:
		{
			return (LRESULT)state->theme.font;
		} break;
		case WM_DEADCHAR://Very interesting, this is the way that TranslateMessage notifies about things like a tilde being pressed, since this keys arent expected to be directly made into characters on the screen
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PARENTNOTIFY: //Msgs from our potential childs
		{
			return 0;//what the childs do is completely opaque to us
		} break;
		case WM_COMMAND:
		{
			LRESULT res;
			HWND child = (HWND)lparam;
			if (child) {//Notifs from our childs
				//NOTE: for now childs are completely opaque, we simply work as "pasamanos", TODO(fran): we shouldnt have to do this, what we need is some sort of observer, aka something happens and the control can send the msg directly to a place where it'll be answered, having this parent child relationships is not enough, for example in cases like this were there are lots of things that can be appended to edit controls, searchbar, button, scrollbar, ...
				res = SendMessage(state->parent, WM_COMMAND, wparam, lparam);
			}
			else {//otherwise it's a notif from an accelerator or menu
				Assert(0);
			}
			return res;
		} break;
		case EM_SETSEL:
		{
			size_t _start = (size_t)wparam;
			size_t _end = (size_t)lparam;
			//int anchor = _start; //TODO(fran): store the anchor, useful for extending selection eg when the user clicks while pressing shift

			if (_start == (size_t)-1 || (i32)_start == (i32)-1) {//TODO(fran): find out the best check for this since -1 usually gets automapped to i32 and thus will be different from (size_t)-1 (I think)
				//Remove current selection
				if (state->char_cur_sel.anchor != state->char_cur_sel.cursor) {
					state->char_cur_sel.anchor = state->char_cur_sel.cursor;
					ask_for_repaint(state);
				}
			}
			else {
				size_t end_max = (size_t)state->char_text.length();
				if (_end == (size_t)-1 || (i32)_end == (i32)-1) {
					_end = end_max; //Set _end to one past the last valid char
				}

				_start = clamp((size_t)0, _start, end_max);
				_end = clamp((size_t)0, _end, end_max);


				if (state->char_cur_sel.anchor != _start || state->char_cur_sel.cursor != _end) {
					state->char_cur_sel.anchor = _start;
					state->char_cur_sel.cursor = _end;
					ask_for_repaint(state);
				}
			}

			recalculate_caret(state);//TODO(fran): only do it if cursor changed

			return 0;
		} break;
		case EM_GETSEL://TODO(fran): EM_GETSEL_EX (to be able to request for size_t (possibly 64bit) values)
		{
			DWORD* start = (decltype(start))wparam;
			DWORD* end = (decltype(end))lparam;

			if (start) *start = (DWORD)state->char_cur_sel.x_min();
			if (end) *end = (DWORD)state->char_cur_sel.x_max();

			return -1;//TODO(fran): support for 16bit, should return zero-based value with the starting position of the selection in the LOWORD and the position of the first TCHAR after the last selected TCHAR in the HIWORD. If either of these values exceeds 65535 then the return value is -1.
		} break;
		case WM_CAPTURECHANGED://We're losing mouse capture
		{
			state->OnMouseTracking = false;
			return 0;
		} break;
		case WM_DESTROYCLIPBOARD:
		{
			GlobalFree(state->clipboard_handle);//TODO(fran): should I zero clipboard_handle?
			//TODO(fran): this and storing the clipboard_handle are actually pointless, windows now owns and knows how to free our clipboard data so we should actually _not_ give it our handle when we OpenClipboard() and this extra work should solve itself
			return 0;
		} break;
		case WM_RENDERALLFORMATS:
		{
			//When our application is about to be closed windows requests that we "render" all the clipboard formats that we have previously set
			//Problem is this is called when you use Delayed Rendering, by calling SetClipboardData with a null hMem parameter, we never use that. So I can only assume two things: either this is a windows bug or TODO(fran): we got a bug
			//Extra info on 'rendering': https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard?redirectedfrom=MSDN#_win32_Copying_Information_to_the_Clipboard
			return 0;
		} break;

		default:
		{

	#ifdef _DEBUG
			if (msg >= 0xC000 && msg <= 0xFFFF) {//String messages for use by applications  
				TCHAR arr[256];
				int res = GetClipboardFormatName(msg, arr, 256);
				cstr_printf(arr); printf("\n");
				//After Alt+Shift to change the keyboard (and some WM_IMENOTIFY) we receive "MSIMEQueryPosition"
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}

			Assert(0);
	#else 
			return DefWindowProc(hwnd, msg, wparam, lparam);
	#endif
		} break;
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
		cl.hCursor = LoadCursor(nullptr, IDC_IBEAM);
		cl.hbrBackground = NULL;
		cl.lpszMenuName = NULL;
		cl.lpszClassName = wndclass;
		cl.hIconSm = NULL;

		ATOM class_atom = RegisterClassExW(&cl);
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