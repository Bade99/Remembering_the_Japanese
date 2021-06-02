#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include <windowsx.h>
#include "win32_Helpers.h"
#include "win32_Renderer.h"
#include "windows_msg_mapper.h"

//TODO(fran): new class btn_text_or_img: if the text fits then draw it, otherwise render the img, great for cool resizing that allows for the same control to take different shapes but maintain all functionality

//-----------------API-----------------:
// button::set_theme()
// button::set_tooltip()

//-------------Usage Notes-------------:
//	this buttons can have text or an img, but not both at the same time
//	it's important that the parent uses WS_CLIPCHILDREN to avoid horrible flickering
//	this button follows the standard button tradition of getting the msg to send to the parent from the hMenu param of CreateWindow/Ex
//	when clicked notifies the parent through WM_COMMAND with LOWORD(wParam)= msg number specified in hMenu param of CreateWindow/Ex ; HIWORD(wParam)=0 ; lParam= HWND of the button. Just like the standard button


//-------------Additional Styles-------------:
// button::style::roundrect : Border is made of a rounded rectangle

//-------------Tooltip-------------: (NOTE: the tooltip is hidden if the user takes the mouse away from the control)
// button::default_tooltip_duration
// button::tooltip_timer_id

namespace button {
	constexpr cstr wndclass[] = TEXT("unCap_wndclass_button");
	constexpr i32 default_tooltip_duration = 3000 /*ms*/;
	constexpr u32 tooltip_timer_id = 0xf1;

	struct render_flags {
		bool isEnabled;//buttons can be enabled (accept input) or disabled (doesnt)
		bool onMouseover;//the mouse is over the button
		bool onClicked;//the button is being pushed
		//INFO: additional hidden state, the user could have pressed the button and while still holding it pressed have moved the mouse outside it, in this case onMouseover==false and onClicked==true
	};

	struct Theme {
		struct {
			brush_group foreground, bk, border;
		} brushes;
		struct {
			u32 border_thickness = U32MAX;
		}dimensions;
		HFONT font = 0;
	};

	//TODO(fran): should I pass the Theme as a param?
	typedef void(*func_render)(HWND wnd, HDC dc, rect_i32 r, render_flags flags, const Theme* theme, void* element, void* user_extra);
	typedef void(*func_free_element)(void* element, void* user_extra);
	typedef void(*func_on_click)(void* element, void* user_extra);//action to perform once the button has been clicked


	enum style {
		roundrect = (1 << 1),
	};

	struct ProcState { //NOTE: must be initialized to zero
		HWND wnd;
		HWND parent;

		union Controls {
			struct {
				HWND tooltip;
			};
			HWND all[1];//REMEMBER TO UPDATE
		}controls;

		bool onMouseOver;
		bool onLMouseClick;//The left click is being pressed on the background area
		bool OnMouseTracking; //Left click was pressed in our bar and now is still being held, the user will probably be moving the mouse around, so we want to track it to move the scrollbar

		UINT msg_to_send;

		Theme theme;

		HICON icon;
		HBITMAP bmp;

		utf16* tooltip_txt;


		void* element;
		void* user_extra;

		func_free_element free_element;
		func_render render;
		func_on_click on_click;
	};

	ProcState* get_state(HWND hwnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(hwnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); } //RedrawWindow(state->wnd, NULL, NULL, RDW_INVALIDATE);

	void set_element(HWND wnd, void* element) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (state->free_element && state->element) state->free_element(state->element,state->user_extra);
			state->element = element;
			ask_for_repaint(state);
		}
	}

	void set_user_extra(HWND wnd, void* user_extra) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->user_extra = user_extra;
			//TODO(fran): redraw?
		}
	}

	void set_function_free_elements(HWND wnd, func_free_element func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->free_element = func;
		}
	}

	void set_function_render(HWND wnd, func_render func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->render = func;
			ask_for_repaint(state);
		}
	}

	void set_function_on_click(HWND wnd, func_on_click func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->on_click = func;
		}
	}

	//NOTE: text must be null terminated
	//if txt == 0 the tooltip is removed
	bool set_tooltip(HWND wnd, const utf16* txt) {
		//NOTE: no ASCII support
		bool res = false;

		ProcState* state = get_state(wnd);
		if (state) {
			res = true;
			if(txt) {
				//store new tooltip text
				state->tooltip_txt = (decltype(state->tooltip_txt))malloc( (wcslen(txt)+1) * sizeof(*txt));
				wcscpy(state->tooltip_txt, txt);
			}
			else {
				//remove tooltip text
				if (state->tooltip_txt) { free(state->tooltip_txt); state->tooltip_txt = 0; }
			}
		}
		return res;
	}

	bool _copy_theme(Theme* dst, const Theme* src) {
		bool repaint = false;
		repaint |= copy_brush_group(&dst->brushes.foreground, &src->brushes.foreground);
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);
		repaint |= copy_brush_group(&dst->brushes.border, &src->brushes.border);

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
			bool repaint = _copy_theme(&state->theme, t);

			if (repaint)  ask_for_repaint(state);
		}
	}

	void paint(ProcState* state, HDC dc) {
		RECT rc; GetClientRect(state->wnd, &rc);
		int w = RECTW(rc), h = RECTH(rc);
		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
		bool enabled = IsWindowEnabled(state->wnd);
		HFONT font = state->theme.font;
		//TODO(fran): Check that we are going to paint something new
		HBRUSH border_br = enabled ? state->theme.brushes.border.normal : state->theme.brushes.border.disabled;
		HBRUSH txt_br = enabled ? state->theme.brushes.foreground.normal : state->theme.brushes.foreground.disabled;
		HBRUSH bk_br;
		if (enabled) {
			if (state->onMouseOver && state->onLMouseClick) bk_br = state->theme.brushes.bk.clicked;
			else if (state->onMouseOver || state->OnMouseTracking || (GetFocus() == state->wnd)) bk_br = state->theme.brushes.bk.mouseover;
			else bk_br = state->theme.brushes.bk.normal;
		}
		else bk_br = state->theme.brushes.bk.disabled;
		SetBkColor(dc, ColorFromBrush(bk_br));
		HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc, oldbr); };

		{
			//Border and Bk
			HPEN pen = CreatePen(PS_SOLID, state->theme.dimensions.border_thickness, ColorFromBrush(border_br)); defer{ DeletePen(pen); };//INFO: pens with zero thickness get 1px thickness, so we gotta manually avoid using the pen
			HPEN oldpen = SelectPen(dc, pen); defer{ SelectPen(dc,oldpen); };

			if (style & style::roundrect) {
				i32 extent = min(w, h);
				bool is_small = extent < 50;
				i32 roundedness = max(1, (i32)roundf((f32)extent * .2f));
				if (is_small) {
					HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
					RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, roundedness, roundedness);
				}
				else {
#if 0
					//Bk
					urender::RoundRectangleFill(dc, bk_br, rc, roundedness);
					//Border
					urender::RoundRectangleBorder(dc, border_br, rc, roundedness, (f32)border_thickness);
#elif 0
					//Bk
					urender::RoundRectangleFill(dc, bk_br, rc, roundedness);
					//Border
					urender::RoundRectangleBorder(dc, border_br, rc, roundedness, (f32)border_thickness);
					urender::RoundRectangleCornerFill(dc, bk_br, rc, roundedness);//TODO(fran): if this didnt cut a bit of the line it would be semi acceptable
#elif 1

					//TODO(fran): for some reason the border of the rounded rectangle start to look worse once you mouseover the button, we're probably screwing the border by filling the bk somehow

					//Bk
#if 0
					urender::RoundRectangleFill(dc, bk_br, rc, roundedness);
#else
					urender::RoundRectangleFill_smooth(dc, bk_br, rc, roundedness, (f32)state->theme.dimensions.border_thickness);
#endif
					//Border
					if (state->theme.dimensions.border_thickness != 0) {
#if 0
						//Border Arcs
						urender::RoundRectangleArcs(dc, border_br, rc, roundedness, (f32)state->theme.dimensions.border_thickness);
						//Border Lines
						urender::RoundRectangleLines(dc, border_br, rc, roundedness, (f32)state->theme.dimensions.border_thickness + 1);
						//TODO(fran): this looks a little better, but still not quite (especially the bottom) and is super hacky
#else
						urender::RoundRectangleBorder_smooth(dc, border_br, rc, roundedness, (f32)state->theme.dimensions.border_thickness);
#endif
					}
#else
					HDC highresdc = CreateCompatibleDC(dc); defer{ DeleteDC(highresdc); };
					HBRUSH oldbr = SelectBrush(highresdc, bk_br); defer{ SelectBrush(highresdc,oldbr); };
					i32 scale_factor = 2;
					HPEN pen = CreatePen(PS_SOLID, border_thickness * scale_factor, ColorFromBrush(border_br)); defer{ DeletePen(pen); };
					HPEN oldpen = SelectPen(highresdc, pen); defer{ SelectPen(highresdc,oldpen); };
					HBITMAP highresbmp = CreateCompatibleBitmap(dc, w * scale_factor, h * scale_factor); defer{ DeleteBitmap(highresbmp); };
					HBITMAP oldbmp = SelectBitmap(highresdc, highresbmp); defer{ SelectBitmap(highresdc, oldbmp); };
					RoundRect(highresdc, 1, 1, w* scale_factor - 2, h* scale_factor - 2, roundedness* scale_factor * 2, roundedness* scale_factor * 2);
					int oldstretchmode = SetStretchBltMode(dc, HALFTONE); defer{ SetStretchBltMode(dc, oldstretchmode); };
					SetBrushOrgEx(dc, 0, 0, nullptr);
					StretchBlt(dc, rc.left, rc.top, w, h, highresdc, 0, 0, h* scale_factor, w* scale_factor, SRCCOPY);

					//TODO(fran): problems with this option: very expensive, weird glitches on the top, impossible to do transparency since we gotta blit the entire bmp instead of just the roundrect (maybe we can still do transparency via SetWindowRgn() )
#endif 
				}
			}
			else {
				//Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom); //uses pen for border and brush for bk
				//Bk
				FillRect(dc, &rc, bk_br);
				//Border
				FillRectBorder(dc, rc, state->theme.dimensions.border_thickness, bk_br, BORDERALL);
			}
		}

		if (style & BS_ICON) {//Here will go buttons that only have an icon

			HICON icon = state->icon;
			//NOTE: we assume all icons to be squares 1:1
			MYICON_INFO iconnfo = MyGetIconInfo(icon);
			int max_sz = roundNdown(iconnfo.nWidth, (int)((float)min(RECTWIDTH(rc), RECTHEIGHT(rc)) * .8f)); //HACK: instead use png + gdi+ + color matrices
			if (!max_sz) {
				if ((iconnfo.nWidth % 2) == 0) max_sz = iconnfo.nWidth / 2;
				else max_sz = iconnfo.nWidth; //More HACKs
			}
			int icon_h = max_sz;
			int icon_w = icon_h;
			int icon_y = (RECTH(rc) - icon_h) / 2;
			int icon_x = (RECTW(rc) - icon_w) / 2;
#if 0
			urender::draw_icon(dc, icon_x, icon_y, icon_w, icon_h, icon, 0, 0, iconnfo.nWidth, iconnfo.nHeight);
#else
			DrawIconEx(dc, icon_x, icon_y, icon, icon_w, icon_h, 0, 0, DI_NORMAL);
#endif
			//TODO(fran): the actual problem with icons is the shitty software im using to generate the layers, it generates lots of erroneous pixel colors on the smaller layers. NOTE: now im not too sure about this, gimp renders the layers correctly and without all the broken stuff visual studio's viewer shows, so it may just be that gdi+ (used by urender::draw_icon) sucks as usual which wouldnt really be a surprise
		}
		else if (style & BS_BITMAP) {
			BITMAP bitmap; GetObject(state->bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {
				//TODO(fran):unify rect calculation with icon
				int max_sz = roundNdown(bitmap.bmWidth, (int)((float)min(RECTWIDTH(rc), RECTHEIGHT(rc)) * .8f)); //HACK: instead use png + gdi+ + color matrices
				if (!max_sz) {
					if ((bitmap.bmWidth % 2) == 0) max_sz = bitmap.bmWidth / 2;
					else max_sz = bitmap.bmWidth; //More HACKs
				}

				int bmp_height = max_sz;
				int bmp_width = bmp_height;
				int bmp_align_height = (RECTHEIGHT(rc) - bmp_height) / 2;
				int bmp_align_width = (RECTWIDTH(rc) - bmp_width) / 2;
				urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, state->bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, txt_br);

			}
		}
		else { //Here will go buttons that only have text
			HFONT oldfont = SelectFont(dc, font); defer{ SelectFont(dc,oldfont); };
			SetTextColor(dc, ColorFromBrush(txt_br));
			TCHAR Text[40];
			int len = (int)SendMessage(state->wnd, WM_GETTEXT, ARRAYSIZE(Text), (LPARAM)Text);

			// Calculate vertical position for the item string so that it will be vertically centered
			SIZE txt_sz; GetTextExtentPoint32(dc, Text, len, &txt_sz);
			int yPos = (rc.bottom + rc.top - txt_sz.cy) / 2;

			SetTextAlign(dc, TA_CENTER);
			int xPos = (rc.right - rc.left) / 2;
			TextOut(dc, xPos, yPos, Text, len);
		}
	}

	void notify_parent(ProcState* state) {
		if (state->on_click) state->on_click(state->element, state->user_extra);
		else PostMessage(state->parent, WM_COMMAND, (WPARAM)MAKELONG(state->msg_to_send, 0), (LPARAM)state->wnd);
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//static int __cnt; printf("%d:BUTTON:%s\n", __cnt++, msgToString(msg));
		ProcState* state = get_state(hwnd);
		//Assert(state); //NOTE: cannot check thanks to the grandeur of windows' msgs before WM_NCCREATE
		switch (msg) {
		case WM_DESIRED_SIZE:
		{
			SIZE* min = (decltype(min))wparam;
			SIZE* max = (decltype(max))lparam;

			DWORD style = (DWORD)GetWindowLongPtr(state->wnd, GWL_STYLE);
			HFONT font = state->theme.font;
			if (style & BS_ICON) {
				MYICON_INFO iconnfo = MyGetIconInfo(state->icon);
				min->cx = minimum((int)max->cx,(int)(iconnfo.nWidth * 1.5f));
				min->cy = minimum((int)max->cy,(int)(iconnfo.nHeight * 1.5f));

				*max = *min;//TODO(fran): dont I do rescaling on imgs?
			}
			else if (style & BS_BITMAP) {
				BITMAP bitmap; GetObject(state->bmp, sizeof(bitmap), &bitmap);

				min->cx = minimum(max->cx, (long)(bitmap.bmWidth * 1.5f));
				min->cy = minimum(max->cy, (long)(bitmap.bmHeight * 1.5f));

				*max = *min;
			}
			else { //we got text
				HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
				SelectFont(dc, font);
				TEXTMETRIC tm; GetTextMetrics(dc, &tm);

				TCHAR Text[64]; //TODO(fran): this could not be more stupid, I should have the text, at least with a str
				int len = (int)SendMessage(state->wnd, WM_GETTEXT, ARRAYSIZE(Text), (LPARAM)Text);

				GetTextExtentPoint32(dc, Text, len, min);
				min->cx = (int)((float)min->cx * 1.2f);
				min->cy = (int)((float)min->cy * 1.2f);

				*max = *min;
			}

			return 2;
		} break;
		case BCM_GETIDEALSIZE:
		{
			SIZE* sz = (SIZE*)lparam;//NOTE: all sizes are relative to the entire button, not just the img or text
			DWORD style = (DWORD)GetWindowLongPtr(state->wnd, GWL_STYLE);
			HFONT font = state->theme.font;
			if (sz->cx) { //calculate cy based on cx
				if (style & BS_ICON || style & BS_BITMAP) {
					sz->cy = sz->cx; //we always assume that imgs are square
				}
				else { //we got text
					HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
					SelectFont(dc, font);
					TEXTMETRIC tm; GetTextMetrics(dc, &tm);
					sz->cy = (int)((float)tm.tmHeight * 1.2f);
				}
			}
			else { //calculate cx and cy
				if (style & BS_ICON) {
					MYICON_INFO iconnfo = MyGetIconInfo(state->icon);
					sz->cx = iconnfo.nWidth;
					sz->cy = iconnfo.nHeight;
				}
				else if (style & BS_BITMAP) {
					BITMAP bitmap; GetObject(state->bmp, sizeof(bitmap), &bitmap);
					sz->cx = bitmap.bmWidth;
					sz->cy = bitmap.bmHeight;
				}
				else { //we got text
					HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
					SelectFont(dc, font);
					TEXTMETRIC tm; GetTextMetrics(dc, &tm);

					TCHAR Text[40]; //TODO(fran): this could not be more stupid, I should have the text, at least with a str
					int len = (int)SendMessage(state->wnd, WM_GETTEXT, ARRAYSIZE(Text), (LPARAM)Text);

					GetTextExtentPoint32(dc, Text, len, sz);
					sz->cx = (int)((float)sz->cx * 1.2f);
					sz->cy = (int)((float)sz->cy * 1.2f);
				}
			}
			return TRUE;
		} break;
		case BM_GETIMAGE:
		{
			if (wparam == IMAGE_BITMAP) return (LRESULT)state->bmp;
			else if (wparam == IMAGE_ICON) return (LRESULT)state->icon;
			return 0;
		}
		case BM_SETIMAGE://TODO: in this call you decide whether to show both img and txt, only txt or only img, see doc
		{
			if (wparam == IMAGE_BITMAP) {
				HBITMAP old = state->bmp;
				state->bmp = (HBITMAP)lparam;
				return (LRESULT)old;
			}
			else if (wparam == IMAGE_ICON) {
				HICON old = state->icon;
				state->icon = (HICON)lparam;
				return (LRESULT)old;
			}
			return 0;
		} break;
		case WM_CANCELMODE:
		{
			//We got canceled by the system, not really sure what it means but doc says we should cancel everything mouse capture related, so stop tracking
			if (state->OnMouseTracking) {
				ReleaseCapture();//stop capturing the mouse
				state->OnMouseTracking = false;
			}
			state->onLMouseClick = false;
			state->onMouseOver = false;
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_NCDESTROY:
		{
			//we are getting killed
			//doc says: This message frees any memory internally allocated for the window.
			return DefWindowProc(hwnd, msg, wparam, lparam);//Probably does nothing
		} break;
		case WM_GETTEXT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_CAPTURECHANGED:
		{
			//We lost mouse capture
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_LBUTTONUP:
		{
			if (state->OnMouseTracking) {
				if (state->onMouseOver) notify_parent(state);
				ReleaseCapture();//stop capturing the mouse
				state->OnMouseTracking = false;
			}
			state->onLMouseClick = false;

			return 0;
		} break;
		case WM_LBUTTONDOWN:
		{
			//Left click is down
			if (state->onMouseOver) {
				//Click happened inside the button, so we want to capture the mouse movement in case the user starts moving the mouse trying to scroll

				//TODO(fran): maybe first check that we arent already tracking (!state->OnMouseTrackingSb)
				SetCapture(state->wnd);//We want to keep capturing the mouse while the user is still pressing some button, even if the mouse leaves our client area
				state->OnMouseTracking = true;
				state->onLMouseClick = true;
			}
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_MOUSELEAVE:
		{
			//we asked TrackMouseEvent for this so we can update when the mouse leaves our client area, which we dont get notified about otherwise and is needed, for example when the user hovers on top the button and then hovers outside the client area
			state->onMouseOver = false;
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_MOUSEMOVE:
		{
			//After WM_NCHITTEST and WM_SETCURSOR we finally get that the mouse has moved
			//Sent to the window where the cursor is, unless someone else is explicitly capturing it, in which case this gets sent to them
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//client coords, relative to upper-left corner

			//Store previous state
			bool prev_onMouseOver = state->onMouseOver;

			RECT rc; GetClientRect(state->wnd, &rc);
			if (test_pt_rc(mouse, rc)) {//Mouse is inside the button
				state->onMouseOver = true;
			}
			else {//Mouse is outside the button
				state->onMouseOver = false;
			}

			bool state_change = prev_onMouseOver != state->onMouseOver;
			if (state_change) {
				ask_for_repaint(state);
				TRACKMOUSEEVENT track;
				track.cbSize = sizeof(track);
				track.hwndTrack = state->wnd;
				track.dwFlags = TME_LEAVE;
				TrackMouseEvent(&track);
			}

			return 0;
		} break;
		case WM_MOUSEACTIVATE:
		{
			//Sent to us when we're still an inactive window and we get a mouse press
			//TODO(fran): we could also ask our parent (wparam) what it wants to do with us
			return MA_ACTIVATE; //Activate our window and post the mouse msg
		} break;
		case WM_SETCURSOR:
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
		case WM_NCHITTEST:
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
		}
		case WM_ERASEBKGND:
		{
			//You receive this msg if you didnt specify hbrBackground  when you registered the class, now it's up to you to draw the background
			HDC dc = (HDC)wparam;
			//TODO(fran): look at https://docs.microsoft.com/en-us/windows/win32/gdi/drawing-a-custom-window-background and SetMapModek, allows for transforms

			return 0; //If you return 0 then on WM_PAINT fErase will be true, aka paint the background there
		} break;
		case WM_NCPAINT:
		{
			//Paint non client area, we shouldnt have any
			HDC hdc = GetDCEx(hwnd, (HRGN)wparam, DCX_WINDOW | DCX_USESTYLE);
			ReleaseDC(hwnd, hdc);
			return 0; //we process this message, for now
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS* p = (WINDOWPOS*)lparam; //new window pos, size, etc
			return DefWindowProc(hwnd, msg, wparam, lparam); //TODO(fran): if we handle this msg ourselves instead of calling DefWindowProc we wont need to handle WM_SIZE and WM_MOVE since they wont be sent, also it says it's more efficient https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-windowposchanged
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			//Someone calls SetWindowPos with the new values, here you can apply modifications over those
			WINDOWPOS* p = (WINDOWPOS*)lparam;
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SHOWWINDOW: //On startup I received this cause of WS_VISIBLE flag
		{
			//Sent when window is about to be hidden or shown, doesnt let it clear if we are in charge of that or it's going to happen no matter what we do
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE: //Sent on startup after WM_SIZE, although possibly sent by DefWindowProc after I let it process WM_SIZE, not sure
		{
			//This msg is received _after_ the window was moved
			//Here you can obtain x and y of your window's client area
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		case WM_SIZE: {
			//NOTE: neat, here you resize your render target, if I had one or cared to resize windows' https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCCREATE: { //1st msg received
			ProcState* st = (ProcState*)calloc(1, sizeof(ProcState));
			Assert(st);
			set_state(hwnd, st);
			CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;
			st->parent = creation_nfo->hwndParent;
			st->wnd = hwnd;
			st->msg_to_send = (UINT)(UINT_PTR)creation_nfo->hMenu;
			if (creation_nfo->lpszName)SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)creation_nfo->lpszName);
			return TRUE; //continue creation, this msg seems kind of more of a user setup place, strange considering there's also WM_CREATE
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
		case WM_SETTEXT:
		{
			//This function is insane, it actually does painting on it's own without telling nobody, so we need a way to kill that
			//I think there are a couple approaches that work, I took this one which works since windows 95 (from what the article says)
			//Many thanks for yet another hack http://www.catch22.net/tuts/win32/custom-titlebar
			LONG_PTR  dwStyle = GetWindowLongPtr(state->wnd, GWL_STYLE);
			// turn off WS_VISIBLE
			SetWindowLongPtr(state->wnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);

			// perform the default action, minus painting
			LRESULT ret = DefWindowProc(state->wnd, msg, wparam, lparam);

			// turn on WS_VISIBLE
			SetWindowLongPtr(state->wnd, GWL_STYLE, dwStyle);

			// perform custom painting, aka dont and do what should be done, repaint, it's really not that expensive for our case, we barely call WM_SETTEXT, and it can be optimized out later
			ask_for_repaint(state);
			
			return ret;
		} break;
		case WM_PAINT:
		{
			//IMPORTANT TODO(fran): fix rendering bugs, sometimes part of the borders dont get drawn, I think I found the bug, border br doesnt adapt like bk, we have push, mouseover, etc, the border is just one so it's probably creating inconsistency problems

			PAINTSTRUCT ps; //TODO(fran): we arent using the rectangle from the ps, I think we should for performance
			HDC dc = BeginPaint(state->wnd, &ps); defer{ EndPaint(state->wnd, &ps); };

			if (state->render) {
				RECT rc; GetClientRect(state->wnd, &rc);
				render_flags flags;

				flags.onClicked = state->onMouseOver && state->onLMouseClick;
				flags.onMouseover = state->onMouseOver || state->OnMouseTracking || (GetFocus() == state->wnd);
				flags.isEnabled = IsWindowEnabled(state->wnd);
				
				state->render(state->wnd, dc, to_rect_i32(rc), flags,&state->theme, state->element, state->user_extra);
			}
			else paint(state, dc);

			return 0;
		} break;
		case WM_DESTROY:
		{
			if (state->free_element && state->element) state->free_element(state->element, state->user_extra);
			if (state->tooltip_txt) set_tooltip(state->wnd, 0);
			free(state);
		}break;
		case WM_STYLECHANGING:
		{
			// SetWindowLong... related, we can check the proposed new styles and change them
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_STYLECHANGED:
		{
			// Notifies that the style was changed, you cant do nothing here
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFONT: {
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
		case WM_GETICON:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_CREATE:
		{

			//Create our tooltip
			state->controls.tooltip = CreateWindowEx(WS_EX_TOPMOST/*make sure it can be seen in any wnd config*/, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,//INFO(fran): TTS_BALLOON doesnt work on windows10, you need a registry change https://superuser.com/questions/1349414/app-wont-show-balloon-tip-notifications-in-windows-10 simply amazing
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				state->wnd, NULL, NULL, NULL);
			Assert(state->controls.tooltip);

			TOOLINFO toolInfo{ sizeof(toolInfo) };
			toolInfo.hwnd = state->wnd;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS /*IMPORTANT TODO(fran): if we dont ask for TTF_SUBCLASS then we're in charge of notifying the tooltip on the events it needs, like WM_MOUSEMOVE,etc this is one way we can prevent the tooltip from working if the user hasnt requested it to*/; /*| TTF_ABSOLUTE | TTF_TRACK*/ /*allows to show the tooltip when we please*/; //TODO(fran): TTF_TRANSPARENT might be useful, mouse msgs that go to the tooltip are sent to the parent aka us
			toolInfo.uId = (UINT_PTR)state->wnd;
			toolInfo.rect = { 0 };
			//toolInfo.hinst = GetModuleHandle(NULL);
			toolInfo.lpszText = LPSTR_TEXTCALLBACK;//Tooltip control sends us TTN_GETDISPINFO msg so we can tell it what to render
			toolInfo.lParam = 0;
			toolInfo.lpReserved = 0;
			BOOL addtool_res = (BOOL)SendMessage(state->controls.tooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
			//TODO(fran): we dont want the tooltip to be always enabled, only when the user asks for it to be
			//TODO(fran): hide tooltip if we get hidden
			Assert(addtool_res);

			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		case WM_IME_SETCONTEXT: //When we get keyboard focus for the first time this gets sent
		{
			return 0; //We dont want IME for a button
		} break;
		case WM_INPUTLANGCHANGE:
		{
			return 0;
		} break;
		case WM_SETFOCUS: //Button has WS_TABSTOP style and we got keyboard focus thanks to that
		{
			//TODO(fran): repaint and show as if the user is hovering over the button
			ask_for_repaint(state);
			return 0;
		}
		case WM_KEYUP:
		{
			return 0;
		} break;
		case WM_KEYDOWN:
		{
			//Nothing for us here
			return 0;
		} break;
		case WM_CHAR:
		{
			//Here we check for enter and tab
			TCHAR c = (TCHAR)wparam;
			switch (c) {
			case VK_TAB://Tab
			{
				SetFocus(GetNextDlgTabItem(GetParent(state->wnd), state->wnd, FALSE));
				//INFO: GetNextDlgTabItem also check that the new item is visible and not disabled
			}break;
			case VK_RETURN://Received when the user presses the "enter" key //Carriage Return aka \r
			{
				//SendMessage(state->wnd, WM_LBUTTONDOWN, ); //TODO(fran): repaint and show as if the user clicked the button
				notify_parent(state);

			}break;
			}
			return 0;
		} break;
		case WM_KILLFOCUS:
		{
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_ENABLE:
		{
			BOOL enable = (BOOL)wparam;
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_UAHDESTROYWINDOW:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
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
				{
					NMTTDISPINFO* nfo = (decltype(nfo))lparam;
					nfo->szText[0] = 0;
					nfo->lpszText = (state->tooltip_txt) ? state->tooltip_txt : 0;//NOTE: tooltip will use our memory, TODO(fran): this could be dangerous, that mem can get deleted if the user changes the tooltip text
					return 0;
				} break;
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
		case WM_SETTOOLTIPTEXT:
		{
			cstr* txt = (decltype(txt))lparam;
			return set_tooltip(state->wnd, txt);
		} break;
		case WM_MOUSEWHEEL:
		{
			//no reason for needing mousewheel input
			return DefWindowProc(hwnd, msg, wparam, lparam);//propagates the msg to the parent
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
	};
	static const pre_post_main PREMAIN_POSTMAIN;
}