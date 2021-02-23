#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_Renderer.h"
#include "win32_button.h"

//-----------------API-----------------:
// multibutton::set_theme() : sets the theme for the container of the buttons
// multibutton::set_button_theme() : sets the theme at the button level, for all or only a specific one
// multibutton::set_buttons() : sets the currently interactive buttons and removes any already existing ones

//-------------Usage Notes-------------:
// Favors rows before columns
// No ASCII support, always utf16

//-------------Additional Messages-------------:
//Sends WM_COMMAND to the parent when the user clicks an element wparam=(size_t)idx (of the elem); lparam=HWND (of the gridview)

//-------------Additional Styles-------------:
// multibutton::style::roundrect : Border is made of a rounded rectangle

//TODO(fran): rename to buttongroup ?
//TODO(fran): set_buttons() / set_button() for bitmap and icon
//TODO(fran): this is probably a pointless control, we should find a nicer way of creating what's basically arrays/lists of elements

namespace multibutton {
	constexpr cstr wndclass[] = TEXT("win32_wndclass_multibutton");

	//TODO(fran): the theme should probably hold the styles too
	struct Theme {
		struct {
			brush_group bk, border;
		} brushes;
		struct {
			u32 border_thickness = U32MAX;//Border of the container
			SIZE btn = { I32MAX ,I32MAX };//Size for a button, all buttons have the same size
			SIZE inbetween_pad = { I32MAX ,I32MAX };//Spacing in between two buttons
		}dimensions;
		//TODO(fran): we should add a bool preferRows; //In order to allow for both row and column preference
	};

	enum style {
		roundrect = (1 << 1),
	};

	struct ProcState {
		HWND wnd;
		HWND parent;

		Theme theme;
		button::Theme button_theme;//used for initializing new buttons, updates from all global set_button_theme() calls

		//TODO(fran): allow the user to choose button styles

		//NOTE: border_pad is automatically calculated so the buttons are perfectly centered on the wnd

		std::vector<HWND> buttons;
	};

	ProcState* get_state(HWND hwnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(hwnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void resize_controls(ProcState* state) {
		RECT rc; GetClientRect(state->wnd, &rc);
		int h = RECTH(rc);
		int w = max(state->theme.dimensions.btn.cx, RECTW(rc));//NOTE: make w at least be able to hold one element
		//NOTE: we dont care bout height, we can go as low as we want and simply allow scrolling

		//TODO(fran) prioritize rows
		int row_cnt = clamp(1, safe_ratio1((i32)(h-(state->buttons.size()-1)*state->theme.dimensions.inbetween_pad.cy), state->theme.dimensions.btn.cy), (i32)state->buttons.size());

		int elems_per_row = (i32)ceilf(safe_ratio1((f32)state->buttons.size(),(f32)row_cnt));

		SIZE border_pad;
		//NOTE: elements are centered with relation to the wnd
		border_pad.cx = (w - (elems_per_row * state->theme.dimensions.btn.cx + (elems_per_row - 1) * state->theme.dimensions.inbetween_pad.cx)) / 2;//border for left and right
		border_pad.cy = (h - (row_cnt *state->theme.dimensions.btn.cy + (row_cnt-1) * state->theme.dimensions.inbetween_pad.cy)) / 2;


		//TODO(fran): perfect centering for the last row, which will most probably have less elements
		size_t elem_cnt = state->buttons.size();
		rect_i32 elem_rc;
		elem_rc.xy = { 0,border_pad.cy };
		elem_rc.wh = { state->theme.dimensions.btn.cx, state->theme.dimensions.btn.cy };
		for (int y = 0, i = 0; y < row_cnt; y++) {
			elem_rc.x = border_pad.cx;
			for (int x = 0; x < elems_per_row && elem_cnt; x++) {
				MoveWindow(state->buttons[i++], elem_rc.x, elem_rc.y, elem_rc.w, elem_rc.h, TRUE);
				elem_rc.x += state->theme.dimensions.btn.cx + state->theme.dimensions.inbetween_pad.cx;
				elem_cnt--;
			}
			elem_rc.y += state->theme.dimensions.btn.cy + state->theme.dimensions.inbetween_pad.cy;
		}
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }

	//Set only what you need, what's not set wont be used
	//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
	void set_theme(HWND wnd, const Theme* t) {
		ProcState* state = get_state(wnd);
		if (state && t) {
			bool repaint = false, resize = false;
			repaint |= copy_brush_group(&state->theme.brushes.bk, &t->brushes.bk);
			repaint |= copy_brush_group(&state->theme.brushes.border, &t->brushes.border);

			if (t->dimensions.border_thickness != U32MAX) {
				state->theme.dimensions.border_thickness = t->dimensions.border_thickness; repaint = true;
			}

			if (t->dimensions.btn.cx != INT32_MAX && t->dimensions.btn.cx != state->theme.dimensions.btn.cx) {
				state->theme.dimensions.btn.cx = t->dimensions.btn.cx; resize = true;
			}
			if (t->dimensions.btn.cy != INT32_MAX && t->dimensions.btn.cy != state->theme.dimensions.btn.cy) {
				state->theme.dimensions.btn.cy = t->dimensions.btn.cy; resize = true;
			}

			if (t->dimensions.inbetween_pad.cx != INT32_MAX && t->dimensions.inbetween_pad.cx != state->theme.dimensions.inbetween_pad.cx) {
				state->theme.dimensions.inbetween_pad.cx = t->dimensions.inbetween_pad.cx; resize = true;
			}
			if (t->dimensions.inbetween_pad.cy != INT32_MAX && t->dimensions.inbetween_pad.cy != state->theme.dimensions.inbetween_pad.cy) {
				state->theme.dimensions.inbetween_pad.cy = t->dimensions.inbetween_pad.cy; resize = true;
			}

			if (resize) resize_controls(state);
			if (repaint)  ask_for_repaint(state);
		}
	}

	//Set the theme for all buttons (set only what you need, what's not set wont be used)
	//IMPORTANT: Buttons added after this call will also use the new theme
	void set_button_theme(HWND wnd, const button::Theme* t) {
		ProcState* state = get_state(wnd);
		if (state && t) {
			for (auto& b : state->buttons)button::set_theme(b, t);
			button::_copy_theme(&state->button_theme, t);//semi HACK: we should standardise this functionality for every control
		}
	}

	//Set the theme for a specific button (set only what you need, what's not set wont be used)
	//'idx' is zero-based
	void set_button_theme(HWND wnd, const button::Theme* t, size_t idx) {
		ProcState* state = get_state(wnd);
		if (state && t && idx < state->buttons.size()) {
			button::set_theme(state->buttons[idx], t);
		}
	}

	void remove_buttons(ProcState* state) {
		for (auto& b : state->buttons) DestroyWindow(b);//TODO(fran): why does this send a WM_UAHDESTROYWINDOW msg to the button?
		state->buttons.clear();
	}

	//NOTE: a separate copy of the strings in 'buttons' is kept internally
	void set_buttons(HWND wnd, ptr<utf16*> buttons) {
		ProcState* state = get_state(wnd);
		if (state) {
			remove_buttons(state);
			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			DWORD style_button_txt = WS_CHILD | WS_TABSTOP | WS_VISIBLE | (style & style::roundrect ? button::style::roundrect : 0);
			for (auto& b_str : buttons) {
				HWND btn = CreateWindowW(button::wndclass, b_str, style_button_txt
					, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
				button::set_theme(btn, &state->button_theme);
				state->buttons.push_back(btn);
			}
			resize_controls(state);
		}
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE: { //1st msg received
			CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;

			ProcState* st = (ProcState*)calloc(1, sizeof(ProcState));
			Assert(st);
			set_state(hwnd, st);
			st->wnd = hwnd;
			st->parent = creation_nfo->hwndParent;

			st->buttons = decltype(st->buttons)();

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
		case WM_CREATE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): remove once we know all the things this does
		}
		case WM_SIZE: //4th
		{
			resize_controls(state);
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE: //5th, This msg is received _after_ the window was moved
		{
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
		case WM_SHOWWINDOW: //6th. On startup you receive this cause of WS_VISIBLE flag
		{
			//Sent when window is about to be hidden or shown, doesnt let it clear if we are in charge of that or it's going to happen no matter what we do
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCPAINT://7th
		{
			//Paint non client area, we shouldnt have any
			//HDC hdc = GetDCEx(hwnd, (HRGN)wparam, DCX_WINDOW | DCX_USESTYLE);
			//ReleaseDC(hwnd, hdc);
			return 0; //we process this message, for now
		} break;
		case WM_ERASEBKGND://8th, you receive this msg if you didnt specify hbrBackground when you registered the class, now it's up to you to draw the background
		{
			HDC dc = (HDC)wparam;
			return 0; //If you return 0 then on WM_PAINT fErase will be true, aka paint the background there
		} break;
		case WM_SETFONT: {
			HFONT font = (HFONT)wparam;
			button::Theme new_theme;
			new_theme.font = font;
			set_button_theme(state->wnd, &new_theme);
			return 0;
		} break;
		case WM_GETFONT:
		{
			return (LRESULT)state->button_theme.font;
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			state->buttons.~vector();
			free(state);
			return 0;
		}break;
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
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEACTIVATE://When the user clicks on us this is 1st msg received
		{
			HWND parent = (HWND)wparam;
			WORD hittest = LOWORD(lparam);
			WORD mouse_msg = HIWORD(lparam);
			return MA_ACTIVATE; //Activate our window and post the mouse msg
		} break;
		case WM_LBUTTONDOWN:
		{
			//wparam = test for virtual keys pressed
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area
			return 0;
		} break;
		case WM_LBUTTONUP:
		{
			return 0;
		} break;
		case WM_IME_SETCONTEXT://Sent after SetFocus to us
		{
			BOOL is_active = (BOOL)wparam;
			u32 disp_flags = (u32)lparam;
			return 0; //We dont want IME
		}break;
		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_GETTEXTLENGTH://does not include null terminator
		{
			return 0;
		} break;
		case WM_SETTEXT:
		{
			return TRUE;
		} break;
		case WM_ENABLE:
		{
			BOOL enable = (BOOL)wparam;
			InvalidateRect(state->wnd, NULL, TRUE);
			//TODO(fran): idk if I also need to disable my buttons or that's done automatically
			return 0;
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps; //TODO(fran): we arent using the rectangle from the ps, I think we should for performance
			HDC dc = BeginPaint(state->wnd, &ps);
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			bool enabled = IsWindowEnabled(state->wnd);

			HBRUSH bk_br = enabled ? state->theme.brushes.bk.normal : state->theme.brushes.bk.disabled;
			HBRUSH border_br = state->theme.dimensions.border_thickness ? (enabled ? state->theme.brushes.border.normal : state->theme.brushes.border.disabled) : bk_br;//INFO: pens with zero thickness get 1px thickness, so we gotta manually avoid using painting borders with pens
			
			if (style & style::roundrect) {
				i32 extent = min(w, h);
				bool is_small = extent < 50;
				i32 roundedness = max(1, (i32)roundf((f32)extent * .2f));
				if (is_small) {
					HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
					HPEN pen = CreatePen(PS_SOLID, state->theme.dimensions.border_thickness, ColorFromBrush(border_br)); defer{ DeletePen(pen); };
					HPEN oldpen = SelectPen(dc, pen); defer{ SelectPen(dc,oldpen); };
					//Bk and Border
					RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, roundedness, roundedness);
				}
				else {
					//Bk
					urender::RoundRectangleFill(dc, bk_br, rc, roundedness);
					//Border
					if (state->theme.dimensions.border_thickness != 0) {
						//Border Arcs
						urender::RoundRectangleArcs(dc, border_br, rc, roundedness, (f32)state->theme.dimensions.border_thickness);
						//Border Lines
						urender::RoundRectangleLines(dc, border_br, rc, roundedness, (f32)state->theme.dimensions.border_thickness + 1);
						//TODO(fran): this looks a little better, but still not quite (especially the bottom) and is super hacky
					}
				}
			}
			else {
				//TODO(fran): I could change to Rectangle() now I manually "hide" the border
				//Bk
				FillRect(dc, &rc, bk_br);
				//Border
				FillRectBorder(dc, rc, state->theme.dimensions.border_thickness, border_br, BORDERALL);
			}

			EndPaint(state->wnd, &ps);
			return 0;
		} break;
		case WM_PARENTNOTIFY:
		{
			return 0;
		} break;
		case WM_COMMAND:
		{
			HWND child = (HWND)lparam;
			Assert(std::find(state->buttons.begin(), state->buttons.end(), child) != state->buttons.end());
			size_t idx = 0;
			for (; idx < state->buttons.size(); idx++) {
				if (child == state->buttons[idx]) break;
			}
			//TODO(fran): we should actually check that the idx is valid
			SendMessage(state->parent, WM_COMMAND, (WPARAM)idx, (LPARAM)state->wnd);
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
		pre_post_main() {
			init_wndclass(GetModuleHandleW(NULL));
		}
		~pre_post_main() { //INFO: you can also use the atexit function
			//Classes are de-registered automatically by the os
		}
	} static const PREMAIN_POSTMAIN;
}