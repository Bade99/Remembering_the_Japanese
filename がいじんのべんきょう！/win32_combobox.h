#pragma once
#include "windows_sdk.h"
#include <windowsx.h>
#include <CommCtrl.h> //DefSubclassProc
#include "win32_Helpers.h"
#include "unCap_Global.h"

//-------------Additional Styles-------------:
#define CBS_ROUNDRECT 0x8000L //Border is made of a rounded rectangle instead of just straight lines //TODO(fran): not convinced with the name

struct ComboProcState {
	HWND wnd;
	HBITMAP dropdown; //NOTE: we dont handle releasing the HBITMAP sent to us
	bool on_mouseover;
};

#define CB_SETDROPDOWNIMG (WM_USER+1) //Only accepts monochrome bitmaps (.bmp), wparam=HBITMAP ; lparam=0
#define CB_GETDROPDOWNIMG (WM_USER+2) //wparam=0 ; lparam=0 ; returns HBITMAP

//BUGs:
//-when the user opens the combobox there's a small window of time where the text shown on top will get changed if the mouse got over some item of the list. To replicate: press combobox and quickly move the mouse straight down

LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {

	//printf(msgToString(msg)); printf("\n");

	//INFO: we require GetWindowLongPtr at "position" GWLP_USERDATA to be left for us to use
	//NOTE: Im pretty sure that "position" 0 is already in use
	ComboProcState* state = (ComboProcState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (!state) {
		ComboProcState* st = (ComboProcState*)calloc(1, sizeof(ComboProcState));
		st->wnd = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)st);
		state = st;
	}

	switch (msg)
	{
	case WM_NCDESTROY: //TODO(fran): better way to cleanup our state, we could have problems with state being referenced after WM_NCDESTROY and RemoveSubclass taking us out without being able to free our state, we need a way to find out when we are being removed
	{
		if (state) free(state);
		return DefSubclassProc(hwnd, msg, wparam, lParam);
	}
	case CB_GETDROPDOWNIMG:
	{
		return (LRESULT)state->dropdown;
	} break;
	case CB_SETDROPDOWNIMG:
	{
		state->dropdown = (HBITMAP)wparam;
		return 0;
	} break;
	case CB_SETCURSEL:
	{
		LONG_PTR  dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		// turn off WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);

		// perform the default action, minus painting
		LRESULT ret = DefSubclassProc(hwnd, msg, wparam, lParam); //defproc for setcursel DOES DRAWING, we gotta do the good ol' trick of invisibility, also it seems that it stores a paint request instead, cause after I make it visible again it asks for wm_paint, as it should have in the first place

		// turn on WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle);

		//Notify parent (once again something that should be the default but isnt)
		SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), CBN_SELCHANGE), (LPARAM)hwnd);
		return ret;
	}
	case WM_NCPAINT:
	{
		return 0;
	} break;
	case WM_MOUSEHOVER:
	{
		//force button to repaint and specify hover or not
		if (state->on_mouseover) break;
		state->on_mouseover = true;
		InvalidateRect(hwnd, 0, TRUE);
		return DefSubclassProc(hwnd, msg, wparam, lParam);
	}
	case WM_MOUSELEAVE:
	{
		state->on_mouseover = false;
		InvalidateRect(hwnd, 0, TRUE);
		return DefSubclassProc(hwnd, msg, wparam, lParam);
	}
	case WM_MOUSEMOVE:
	{
		//TODO(fran): We are tracking the mouse every single time it moves, kind of suspect solution
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 1;
		tme.hwndTrack = hwnd;

		TrackMouseEvent(&tme);
		return DefSubclassProc(hwnd, msg, wparam, lParam);
	}
	//case CBN_DROPDOWN://lets us now that the list is about to be show, therefore the user clicked us
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hwnd, &ps); defer{ EndPaint(hwnd, &ps); };

		RECT rc; GetClientRect(hwnd, &rc);
		int w = RECTW(rc), h = RECTH(rc);
		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE); Assert(style & CBS_DROPDOWNLIST);
		int border_thickness_pen = 0;//means 1px when creating pens
		int border_thickness = 1;

		BOOL ButtonState = (BOOL)SendMessageW(hwnd, CB_GETDROPPEDSTATE, 0, 0);
		HBRUSH bk_br, border_br = unCap_colors.Img;
		if (ButtonState) {
			bk_br = unCap_colors.ControlBkPush;
		}
		else if (state->on_mouseover) {
			bk_br = unCap_colors.ControlBkMouseOver;
		}
		else {
			bk_br = unCap_colors.ControlBk;
		}
		SetBkColor(dc, ColorFromBrush(bk_br));

		//TODO(fran): this code is structured horribly

		HPEN pen = CreatePen(PS_SOLID, border_thickness_pen, ColorFromBrush(border_br));

		HBRUSH oldbrush = SelectBrush(dc, (HBRUSH)GetStockObject(HOLLOW_BRUSH)); defer{ SelectObject(dc, oldbrush); };
		HPEN oldpen = SelectPen(dc, pen);

		SelectFont(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		//SetBkColor(hdc, bkcolor);
		SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt));

		//Border and Bk
		{
			HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
			if (style & CBS_ROUNDRECT) {
				i32 extent = min(w, h);
				bool is_small = extent < 50;
				i32 roundedness = max(1, (i32)roundf((f32)extent * .2f));
				if (is_small) {
					RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, roundedness, roundedness);
				}
				else {
					//Bk
					urender::RoundRectangleFill(dc, bk_br, rc, roundedness);
					//Border
					urender::RoundRectangleBorder(dc, border_br, rc, roundedness, (f32)border_thickness);
				}
			}
			else {
				Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom); //uses pen for border and brush for bk
			}
		}

		SelectObject(dc, oldpen);
		DeletePen(pen);

		/*
		if (GetFocus() == hwnd)
		{
			//INFO: with this we know when the control has been pressed
			RECT temp = rc;
			InflateRect(&temp, -2, -2);
			DrawFocusRect(hdc, &temp);
		}
		*/
		int DISTANCE_TO_SIDE = 5;
		{
			cstr* buf=0;
			bool cuebanner = false;

			int index = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
			if (index >= 0)
			{
				int buflen = (int)SendMessage(hwnd, CB_GETLBTEXTLEN, index, 0);
				buf = (decltype(buf))malloc((buflen + 1)*sizeof(*buf));
				SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
			} 
			else if( index == CB_ERR){//No item has been selected yet
				buf = (decltype(buf))malloc((100) * sizeof(*buf));
				//TODO(fran): I think that CB_GETCUEBANNER is the same as WM_GETTEXT
				cuebanner = SendMessage(hwnd, CB_GETCUEBANNER, (WPARAM)buf, 100);//INFO: the only way to know it there's a cue banner is by the return value, it also doesnt seem like you can retrieve the required size of the buffer
				if (!cuebanner) { free(buf); buf = 0; }
			}
			if (buf) {
				RECT txt_rc = rc;
				txt_rc.left += DISTANCE_TO_SIDE;
				COLORREF prev_txt_clr;
				if (cuebanner) {//TODO(fran): this double if is horrible
					prev_txt_clr = SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt_Disabled));
				}
				DrawText(dc, buf, -1, &txt_rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				if (cuebanner) {
					SetTextColor(dc, prev_txt_clr);
				}
				free(buf);
			}
		}

		if (state->dropdown) {//TODO(fran): flicker free
			HBITMAP bmp = state->dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				int max_sz = roundNdown(bitmap.bmWidth, (int)((float)RECTHEIGHT(rc)*.6f)); //HACK: instead use png + gdi+ + color matrices
				if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

				int bmp_height = max_sz;
				int bmp_width = bmp_height;
				int bmp_align_width = RECTWIDTH(rc) - bmp_width - DISTANCE_TO_SIDE;
				int bmp_align_height = (RECTHEIGHT(rc) - bmp_height) / 2;
				urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, unCap_colors.Img);//TODO(fran): parametric color
			}
		}
		
		return 0;
	} break;
	case WM_ERASEBKGND:
	{
		return 1;
	} break;
	//case WM_NCDESTROY:
	//{
	//	RemoveWindowSubclass(hwnd, this->ComboProc, uIdSubclass);
	//	break;
	//}

	}

	return DefSubclassProc(hwnd, msg, wparam, lParam);
}

//INFO: only use for CBS_DROPDOWN comboboxes
LRESULT CALLBACK TESTComboProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//{ static int msg_count; printf("TEST Combo: %d : ", msg_count++); printf(msgToString(msg)); printf("\n"); }
	switch (msg) {
	case WM_PAINT:
	{
		PAINTSTRUCT p;
		HDC dc = BeginPaint(hwnd, &p);
		FillRect(dc,&p.rcPaint,unCap_colors.ControlTxt);
		EndPaint(hwnd, &p);
	} break;
#if 1 /*Remove button for opening listbox*/
	case WM_SIZE:
	{
		LRESULT res = DefSubclassProc(hwnd, msg, wparam, lparam);
		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(hwnd, &info);
		RECT rc; GetClientRect(hwnd,&rc);
		InflateRect(&rc, -1, -1);//Allow for a little border, for now
		//MoveWindow(info.hwndItem, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), FALSE);
		MoveWindow(info.hwndItem, rc.left, info.rcItem.top, RECTWIDTH(rc), RECTHEIGHT(info.rcItem), FALSE);
		//NOTE: doing this I think I may have found the reason for their control not fully accepting the dimensions you want, their edit control doesnt have vertical centering, so the text wouldnt look center in the middle of the control, if this is the reason it's as pathetic as can be

		//TODO(fran): now the edit control is correctly centered but we have too much of the bk seeping through

		return res;
	} break;
#endif
	case WM_NCPAINT:
	{
		//return 0;
		return DefSubclassProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_ERASEBKGND:
	{
		return 0;
	} break;
	case CB_SETCURSEL:
	{
		LONG_PTR  dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		// turn off WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);

		// perform the default action, minus painting
		LRESULT ret = DefSubclassProc(hwnd, msg, wparam, lparam); //defproc for setcursel DOES DRAWING, we gotta do the good ol' trick of invisibility, also it seems that it stores a paint request instead, cause after I make it visible again it asks for wm_paint, as it should have in the first place

		// turn on WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle);

		//Notify parent (once again something that should be the default but isnt)
		SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), CBN_SELCHANGE), (LPARAM)hwnd);
		return ret;
	} break;
	default: return DefSubclassProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

void COMBOBOX_clear_list_items(HWND cb) {
	int item_cnt = (int)SendMessage(cb, CB_GETCOUNT, 0, 0);
	for (int i = 0; i < item_cnt; i++) {
		SendMessageW(cb, CB_DELETESTRING, 0, 0);
	}
}

LRESULT CALLBACK PrintMsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
	{ static int msg_count; printf("%s: %d : ", (char*)dwRefData, msg_count++); printf(msgToString(msg)); printf("\n"); }
	return DefSubclassProc(hwnd, msg, wparam, lparam);
}