#pragma once
#include "windows_sdk.h"
#include <CommCtrl.h> //DefSubclassProc
#include "がいじんの_Helpers.h"
#include "unCap_Global.h"

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
		DWORD style = (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE);
		if (!(style & CBS_DROPDOWNLIST))
			break;

		RECT rc; GetClientRect(hwnd, &rc);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		BOOL ButtonState = (BOOL)SendMessageW(hwnd, CB_GETDROPPEDSTATE, 0, 0);
		if (ButtonState) {
			SetBkColor(hdc, ColorFromBrush(unCap_colors.ControlBkPush));
			FillRect(hdc, &rc, unCap_colors.ControlBkPush);
		}
		else if (state->on_mouseover) {
			SetBkColor(hdc, ColorFromBrush(unCap_colors.ControlBkMouseOver));
			FillRect(hdc, &rc, unCap_colors.ControlBkMouseOver);
		}
		else {
			SetBkColor(hdc, ColorFromBrush(unCap_colors.ControlBk));
			FillRect(hdc, &rc, unCap_colors.ControlBk);
		}

		RECT client_rec;
		GetClientRect(hwnd, &client_rec);

		HPEN pen = CreatePen(PS_SOLID, max(1, (int)((RECTHEIGHT(client_rec))*.01f)), ColorFromBrush(unCap_colors.Img)); //para el borde

		HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(HOLLOW_BRUSH));//para lo de adentro
		HPEN oldpen = (HPEN)SelectObject(hdc, pen);

		SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		//SetBkColor(hdc, bkcolor);
		SetTextColor(hdc, ColorFromBrush(unCap_colors.ControlTxt));

		//Border
		Rectangle(hdc, 0, 0, rc.right, rc.bottom);

		SelectObject(hdc, oldpen);
		DeleteObject(pen);

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
					prev_txt_clr = SetTextColor(hdc, ColorFromBrush(unCap_colors.ControlTxt_Disabled));
				}
				DrawText(hdc, buf, -1, &txt_rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				if (cuebanner) {
					SetTextColor(hdc, prev_txt_clr);
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
				urender::draw_mask(hdc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, unCap_colors.Img);//TODO(fran): parametric color
			}
		}


		SelectObject(hdc, oldbrush);

		EndPaint(hwnd, &ps);
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
	switch (msg) {
	case WM_PAINT:
	{
		PAINTSTRUCT p;
		HDC dc = BeginPaint(hwnd, &p);
		FillRect(dc,&p.rcPaint,unCap_colors.ControlTxt);
		EndPaint(hwnd, &p);
	} break;
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