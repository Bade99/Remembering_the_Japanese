#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_Renderer.h"
#include <windowsx.h>

//NOTE: no ASCII support, always utf16

//------------------"Additional Styles"------------------: 
/*NOTE: you have at least the first byte free to add new styles*/
#define SO_AUTOFONTSIZE (0x00000020L) //The size of the font auto adjusts to cover the maximum wnd area possible
//IMPORTANT NOTE: 0x00000020L was the only style bit left unused by windows' static control

//TODO(fran): draw border and bk
//TODO(fran): we need a better font size calculation, for one it currently doesnt contemplate letters like "g" which go below the usual height and will usually get truncated

namespace static_oneline {
	constexpr cstr wndclass[] = L"unCap_wndclass_static_oneline";

	struct Theme {
		struct {
			brush_group foreground, bk, border;
		} brushes;
		struct {
			u32 border_thickness = U32MAX;
		}dimensions;
		HFONT font = 0;
	};

	struct ProcState {
		HWND wnd;
		HWND parent;
		Theme theme;
		LOGFONT logfont;
		s16 txt;
	};

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void ask_for_repaint(ProcState* state){ InvalidateRect(state->wnd, NULL, TRUE); }

	bool _copy_theme(ProcState* state, Theme* dst, const Theme* src) {
		bool repaint = false;
		repaint |= copy_brush_group(&dst->brushes.foreground, &src->brushes.foreground);
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);
		repaint |= copy_brush_group(&dst->brushes.border, &src->brushes.border);

		if (src->dimensions.border_thickness != U32MAX) {
			dst->dimensions.border_thickness = src->dimensions.border_thickness; repaint = true;
		}

		if (src->font) { 
			dst->font = src->font; 
			int getobjres = GetObject(dst->font, sizeof(state->logfont), &state->logfont); Assert(getobjres == sizeof(state->logfont));//cache logfont
			repaint = true; 
		}
		return repaint;
	}

	//Set only what you need, what's not set wont be used
	//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
	void set_theme(HWND wnd, const Theme* t) {
		ProcState* state = get_state(wnd);
		if (state && t) {
			bool repaint = _copy_theme(state, &state->theme, t);

			if (repaint)  ask_for_repaint(state);
		}
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{ //1st msg received
			CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;

			ProcState* st = (ProcState*)calloc(1, sizeof(ProcState));
			Assert(st);
			set_state(hwnd, st);
			st->wnd = hwnd;
			st->parent = creation_nfo->hwndParent;
			//st->default_text = 0;
			//NOTE: SS_LEFT==0, that was their way of defaulting to left, problem with that is you cant look for a SS_LEFT bit set
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
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			if (createnfo->lpszName) PostMessage(state->wnd, WM_SETTEXT, 0, (LPARAM)createnfo->lpszName);

			return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): remove once we know all the things this does (probably sends WM_PARENTNOTIFY)
		} break;
		case WM_SIZE: //4th
		{
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
		case WM_SETFONT:
		{
			HFONT font = (HFONT)wparam;
			Theme new_theme;
			new_theme.font = font;
			set_theme(state->wnd, &new_theme);
			return 0;
		} break;
		case WM_GETFONT:
		{
			return (LRESULT)state->theme.font;
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			if (state->txt.str) {
				free_any_str(state->txt.str);
				state->txt = { 0 };
			}
			free(state);
			return 0;
		}break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			//ps.rcPaint
			HDC dc = BeginPaint(state->wnd, &ps);
			bool window_enabled = IsWindowEnabled(state->wnd);
			LONG_PTR style = GetWindowLongPtr(state->wnd, GWL_STYLE); //SS_CENTER, SS_RIGHT or default to SS_LEFT

			//TODO(fran): render bk and border, flicker free
			FillRect(dc, &rc, state->theme.brushes.bk.normal);

			//TODO(fran): this should call urender::draw_text_max_coverage()
#ifdef UNCAP_GDIPLUS
			{
				Gdiplus::Graphics graphics(dc);
				graphics.SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAlias);

				Gdiplus::FontFamily   fontFamily(state->logfont.lfFaceName);

				int FontStyle_flags = Gdiplus::FontStyle::FontStyleRegular;
				//TODO(fran): we need our own font renderer, from starters the font weight has a much finer control with logfont
				if (state->logfont.lfWeight >= FW_BOLD)FontStyle_flags |= Gdiplus::FontStyle::FontStyleBold;
				if (state->logfont.lfItalic)FontStyle_flags |= Gdiplus::FontStyle::FontStyleItalic;
				if (state->logfont.lfUnderline)FontStyle_flags |= Gdiplus::FontStyle::FontStyleUnderline;
				if (state->logfont.lfStrikeOut)FontStyle_flags |= Gdiplus::FontStyle::FontStyleStrikeout;

				f32 fontsize;
				if (style & SO_AUTOFONTSIZE) fontsize = urender::appropiate_font_h(state->txt, {w,h}, &graphics, &fontFamily, FontStyle_flags, Gdiplus::UnitPixel);
				else fontsize = (f32)abs(state->logfont.lfHeight);//TODO(fran): this isnt correct, this are device units https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-logfonta
				//TODO(fran): use this guy to make sure our fontsize fits horizontally too
				//graphics.MeasureString(state->txt.str, (INT)(state->txt.sz_char() - 1),...);

				Gdiplus::Font         font(&fontFamily, fontsize, FontStyle_flags, Gdiplus::UnitPixel);

				Gdiplus::StringFormat stringFormat;
				stringFormat.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);//align vertically always

				Gdiplus::PointF		  pointF;
				Gdiplus::StringAlignment str_alignment;
				if (style & SS_CENTER) {
					pointF = { (f32)w / 2.f, (f32)h / 2.f };
					str_alignment = Gdiplus::StringAlignment::StringAlignmentCenter;
				}
				else if (style & SS_RIGHT) {
					pointF = { (f32)w, (f32)h / 2.f };
					str_alignment = Gdiplus::StringAlignment::StringAlignmentFar;//TODO(fran): im not sure if this is it
				}
				else {//SS_LEFT
					pointF = { (f32)rc.left, (f32)h / 2.f };
					str_alignment = Gdiplus::StringAlignment::StringAlignmentNear;//TODO(fran): im not sure if this is it
				}

				stringFormat.SetAlignment(str_alignment);

				COLORREF txt_color = ColorFromBrush(state->theme.brushes.foreground.normal);
				Gdiplus::SolidBrush   solidBrush(Gdiplus::Color(/*GetAValue(txt_color)*/255, GetRValue(txt_color), GetGValue(txt_color), GetBValue(txt_color)));
				graphics.DrawString(state->txt.str, (INT)(state->txt.sz_char() - 1), &font, pointF, &stringFormat, &solidBrush);
				//TODO(fran): do _not_ use gdiplus when SO_AUTOFONTSIZE isnt set, use instead gdi's TextOut which looks much better
			}
#endif

			EndPaint(hwnd, &ps);
			return 0;
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
		case WM_GETTEXT://the specified char count must include null terminator, since windows' defaults to force writing it to you
		{
			LRESULT res;
			int char_cnt_with_null = max((int)wparam, 0);//Includes null char
			int char_text_cnt_with_null = (int)(state->txt.sz_char());
			if (char_cnt_with_null > char_text_cnt_with_null) char_cnt_with_null = char_text_cnt_with_null;
			cstr* buf = (cstr*)lparam;
			if (buf) {//should I check?
				StrCpyN(buf, state->txt.str, char_cnt_with_null);
				if (char_cnt_with_null < char_text_cnt_with_null) buf[char_cnt_with_null - 1] = (cstr)0;
				res = char_cnt_with_null - 1;
			}
			else res = 0;
			return res;
		} break;
		case WM_GETTEXTLENGTH://does not include null terminator
		{
			int res;
			if (state->txt.sz_char()) res = (int)(state->txt.sz_char() - 1);
			else res = 0;
			return res;
		} break;
		case WM_SETTEXT:
		{
			//TODO(fran): we should check the text doesnt have any \n
			BOOL res = FALSE;
			cstr* buf = (cstr*)lparam;//null terminated

			cstr empty = 0;//INFO: compiler doesnt allow you to set it to L''
			if (!buf) buf = &empty;

			size_t char_sz = cstr_len(buf);//not including null terminator

			size_t char_sz_with_null = char_sz + 1;

			if (state->txt.str)free_any_str(state->txt.str);
			state->txt = (utf16_str)alloc_any_str(char_sz_with_null * sizeof(*state->txt.str));

			StrCpyNW(state->txt.str, buf, (int)char_sz_with_null);

			res = TRUE;
			ask_for_repaint(state);
			return res;
		} break;
		case WM_MOUSEWHEEL:
		{
			//no reason for needing mousewheel input
			return DefWindowProc(hwnd, msg, wparam, lparam);//propagates the msg to the parent
		} break;
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		{
			return SendMessage(state->parent,msg,wparam,lparam);
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