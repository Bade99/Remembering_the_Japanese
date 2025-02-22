#pragma once
#include "windows_sdk.h"
#include <CommCtrl.h> //DefSubclassProc
#include "win32_Helpers.h"
#include "win32_Global.h"
#include "windows_msg_mapper.h"

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

//TODO(fran): for comboboxes with WS_TABSTOP style it might be good to open the listbox when we ge keyboard focus

LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {

	//static int __cnt; printf("%d:COMBO:%s\n", __cnt++, msgToString(msg));

	//INFO: we require GetWindowLongPtr at "position" GWLP_USERDATA to be left for us to use
	//NOTE: Im pretty sure that "position" 0 is already in use
	ComboProcState* state = (ComboProcState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (!state) {
		ComboProcState* st = (ComboProcState*)calloc(1, sizeof(ComboProcState));
		st->wnd = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)st);
		state = st;
	}

	const int DISTANCE_TO_SIDE = 5; //aka padding

	//IMPORTANT: the user must free() the returned buffer if it is != null
	struct _GetComboText_res {cstr* buf = 0; bool is_cuebanner = false; };
	static auto GetComboText = [](HWND hwnd) {
		_GetComboText_res res;
		//TODO(fran): return text length
		int index = ComboBox_GetCurSel(hwnd);
		if (index >= 0)
		{
			int buflen = ComboBox_GetLBTextLen(hwnd, index);
			res.buf = (decltype(res.buf))malloc((buflen + 1) * sizeof(*res.buf));
			ComboBox_GetLBText(hwnd, index, res.buf);
		}
		else if (index == CB_ERR) {//No item has been selected yet
			int max_buf_sz = 200;
			res.buf = (decltype(res.buf))malloc((max_buf_sz) * sizeof(*res.buf));
			//TODO(fran): I think that CB_GETCUEBANNER is the same as WM_GETTEXT
			res.is_cuebanner = ComboBox_GetCueBannerText(hwnd, res.buf, max_buf_sz);//INFO: the only way to know if there's a cue banner is by the return value, it also doesnt seem like you can retrieve the required size of the buffer
			if (!res.is_cuebanner) { free(res.buf); res.buf = 0; }
		}
		return res;
	};

	struct _GetBitmapPlacement_res { int bmp_align_width, bmp_align_height, bmp_width, bmp_height; };
	static auto GetBitmapPlacement = [](ComboProcState* state, RECT rc, int DISTANCE_TO_SIDE) {
		_GetBitmapPlacement_res res{0};
		if (state->dropdown) {//TODO(fran): flicker free
			HBITMAP bmp = state->dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				int max_sz = roundNdown(bitmap.bmWidth, (int)((float)RECTHEIGHT(rc) * .6f)); //HACK: instead use png + gdi+ + color matrices
				if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

				res.bmp_height = max_sz;
				res.bmp_width = res.bmp_height;
				res.bmp_align_width = RECTWIDTH(rc) - res.bmp_width - DISTANCE_TO_SIDE;
				res.bmp_align_height = (RECTHEIGHT(rc) - res.bmp_height) / 2;
			}
		}
		return res;
	};

	switch (msg)
	{
#if 0
	case WM_COMMAND:
	{
		//Im pretty sure the listbox sends a wm_command msg to the combobox when a selection is made
		/*char a[100];
		GetClassNameA((HWND)lparam, a, ARRAYSIZE(a));//ComboLBox
		printf("%s\n",a);*/
		COMBOBOXINFO nfo{sizeof(nfo)}; GetComboBoxInfo(state->wnd, &nfo);
		if (lparam && (HWND)lparam == nfo.hwndList) {
			//printf("COMBO LISTBOX ID    %d\n",LOWORD(wparam));//1000
			//printf("COMBO LISTBOX NOTIF %d\n",HIWORD(wparam));//1
			return 0;//TODO(fran): this actually not even close to enough, the listbox always sends the same notif code, the combobox manually checks against a stored 'last_idx' value to see if the selection changed and only then notify the parent, also it closes the listbox from here
		}
	} break;
	case WM_KEYDOWN:
	{
		u8 vk = (decltype(vk))wparam;
		if (vk == VK_RETURN) return 0;
	} break;
#endif
	case WM_NCDESTROY: //TODO(fran): better way to cleanup our state, we could have problems with state being referenced after WM_NCDESTROY and RemoveSubclass taking us out without being able to free our state, we need a way to find out when we are being removed
	{
		if (state) free(state);
		return DefSubclassProc(hwnd, msg, wparam, lparam);
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
		LRESULT ret = DefSubclassProc(hwnd, msg, wparam, lparam); //defproc for setcursel DOES DRAWING, we gotta do the good ol' trick of invisibility, also it seems that it stores a paint request instead, cause after I make it visible again it asks for wm_paint, as it should have in the first place

		// turn WS_VISIBLE back on
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
		return DefSubclassProc(hwnd, msg, wparam, lparam);
	}
	case WM_MOUSELEAVE:
	{
		state->on_mouseover = false;
		InvalidateRect(hwnd, 0, TRUE);
		return DefSubclassProc(hwnd, msg, wparam, lparam);
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
		return DefSubclassProc(hwnd, msg, wparam, lparam);
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

		BOOL ButtonState = ComboBox_GetDroppedState(hwnd);
		HBRUSH bk_br, border_br = global::colors.Img;
		if (ButtonState) {
			bk_br = global::colors.ControlBkPush;
		}
		else if (state->on_mouseover) {
			bk_br = global::colors.ControlBkMouseOver;
		}
		else {
			bk_br = global::colors.ControlBk;
		}
		SetBkColor(dc, ColorFromBrush(bk_br));

		//TODO(fran): this code is structured horribly

		HPEN pen = CreatePen(PS_SOLID, border_thickness_pen, ColorFromBrush(border_br));

		HBRUSH oldbrush = SelectBrush(dc, (HBRUSH)GetStockObject(HOLLOW_BRUSH)); defer{ SelectObject(dc, oldbrush); };
		HPEN oldpen = SelectPen(dc, pen);

		SelectFont(dc, GetWindowFont(hwnd));
		//SetBkColor(hdc, bkcolor);
		SetTextColor(dc, ColorFromBrush(global::colors.ControlTxt));

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
		{
			auto [buf, is_cuebanner] = GetComboText(hwnd);

			if (buf) {
				RECT txt_rc = rc;
				txt_rc.left += DISTANCE_TO_SIDE;
				COLORREF prev_txt_clr;
				if (is_cuebanner) {//TODO(fran): this double if is horrible
					prev_txt_clr = SetTextColor(dc, ColorFromBrush(global::colors.ControlTxt_Disabled));
				}
				DrawText(dc, buf, -1, &txt_rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				if (is_cuebanner) {
					SetTextColor(dc, prev_txt_clr);
				}
				free(buf);
			}
		}

		if (state->dropdown) {//TODO(fran): flicker free
			HBITMAP bmp = state->dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				auto [bmp_align_width, bmp_align_height, bmp_width, bmp_height] = GetBitmapPlacement(state, rc, DISTANCE_TO_SIDE);

				urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, global::colors.Img);//TODO(fran): parametric color
			}
		}
		
		return 0;
	} break;
	case WM_ERASEBKGND:
	{
		return 1;
	} break;
	case WM_CHAR:
	{
		//Here we check for tab
		TCHAR c = (TCHAR)wparam;
		switch (c) {
		case VK_TAB://Tab
		{
			SetFocus(GetNextDlgTabItem(GetParent(state->wnd), state->wnd, FALSE));
			//INFO: GetNextDlgTabItem also check that the new item is visible and not disabled
			//TODO(fran): return 0; ?
		}break;
		}
	} break;
	//case WM_NCDESTROY:
	//{
	//	RemoveWindowSubclass(hwnd, this->ComboProc, uIdSubclass);
	//	break;
	//}
	case WM_MOUSEWHEEL:
	{
		//no reason for processing mousewheel input, some comboboxes change their index when being scrolled, I hate that behaviour since the user always ends up doing that by accident when trying to scroll the page
		return DefWindowProc(hwnd, msg, wparam, lparam);//propagates the msg to the parent
	} break;
	case WM_DESIRED_SIZE: 
	{
		SIZE* min = (decltype(min))wparam;
		SIZE* max = (decltype(max))lparam;

		auto [buf, cuebanner] = GetComboText(hwnd);

		RECT rc; GetClientRect(hwnd, &rc);
		rc.right = rc.left + max->cx;
		rc.bottom = rc.top + max->cy;
		auto [bmp_align_width, bmp_align_height, bmp_width, bmp_height] = GetBitmapPlacement(state, rc, DISTANCE_TO_SIDE);

		desired_size ret;
		long cx;

		if (buf) {
			HDC dc = GetDC(hwnd); defer{ ReleaseDC(hwnd,dc); };
			SIZE sz; GetTextExtentPoint32(dc, buf, (int)cstr_len(buf), &sz);
			free(buf);

			cx = sz.cx;
			ret = desired_size::fixed;
		}
		else {
			SIZE sz = avg_str_dim(GetWindowFont(hwnd), 10);

			cx = sz.cx;
			ret = desired_size::flexible;
		}

		cx +=  DISTANCE_TO_SIDE * 3 + bmp_width;

		//TODO(fran): actually what I should do here is get the size of the biggest item and just return that, like in cb_insertstring, and I could avoid using cb_setdroppedwidth entirely

		max->cx = minimum(max->cx, cx);

		return ret;
	} break;
	case CB_INSERTSTRING: //TODO(fran): maybe we should only calculate the width of the listbox before it is shown, but idk which event to capture to find out when the lb is about to be shown
	{
		auto ret = DefSubclassProc(hwnd, msg, wparam, lparam);

		HDC dc = GetDC(hwnd); defer{ ReleaseDC(hwnd,dc); };
		HFONT font = GetWindowFont(hwnd);
		HFONT oldfont = SelectFont(dc, font); defer{ SelectFont(dc, oldfont); };

		int cnt = ComboBox_GetCount(hwnd);

		int listbox_width = 0;

		for (int i = 0; i < cnt; i++) {
			cstr txt[200];
			
			int txt_len = ComboBox_GetLBTextLen(hwnd, i);
			
			if (ARRAYSIZE(txt) < txt_len + 1) continue;

			auto res = ComboBox_GetLBText(hwnd, i, txt);
			
			if (res == CB_ERR) continue;

			SIZE sz; GetTextExtentPoint32(dc, txt, txt_len, &sz);

			int w = sz.cx + DISTANCE_TO_SIDE * 2;

			if (w > listbox_width)
				listbox_width = w;
		}
		ComboBox_SetDroppedWidth(hwnd, listbox_width);

		return ret;
	} break;
	case WM_COMMAND:
	{
		auto ret = DefSubclassProc(hwnd, msg, wparam, lparam);

		switch (HIWORD(wparam)) {
		case CBN_SELCHANGE:
		{
			//Selected item has changed

			//We may need to update the size of the combobox to fit the new item in the editbox
			AskForResize(GetParent(hwnd));

		} break;
		}

		return ret;
	} break;

	}

	return DefSubclassProc(hwnd, msg, wparam, lparam);
}

#if 0
//INFO: only use for CBS_DROPDOWN comboboxes
LRESULT CALLBACK TESTComboProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//{ static int msg_count; printf("TEST Combo: %d : ", msg_count++); printf(msgToString(msg)); printf("\n"); }
	switch (msg) {
	case WM_PAINT:
	{
		PAINTSTRUCT p;
		HDC dc = BeginPaint(hwnd, &p);
		FillRect(dc,&p.rcPaint,global::colors.ControlTxt);
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
#endif