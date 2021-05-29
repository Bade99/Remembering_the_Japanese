#pragma once
#include "windows_sdk.h"
#include "win32_Helpers.h"
#include "win32_Global.h"
#include "LANGUAGE_MANAGER.h"
#include <CommCtrl.h> //DefSubclassProc

namespace edit_oneline {
	namespace show_on_hover{//TODO(fran): this actually applies to any window, not only editors
		struct ProcState {
			bool on_mouseover;
		};
		LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR str_id) {

			//static int __cnt; printf("%d:EDIT_ONELINE__ON_HOVER:%s\n", __cnt++, msgToString(msg));

			//INFO: we require GetWindowLongPtr at "position" GWLP_USERDATA to be left for us to use
			//NOTE: Im pretty sure that "position" 0 is already in use
			ProcState* state = (ProcState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (!state) {
				ProcState* st = (ProcState*)calloc(1, sizeof(ProcState));
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)st);
				state = st;
			}

			switch (msg)
			{
			case WM_MOUSELEAVE:
			{
				state->on_mouseover = false;
				InvalidateRect(hwnd, 0, TRUE);
				return DefSubclassProc(hwnd, msg, wparam, lparam);//TODO(fran): can I know whether I should return this or not? probably not
			}
			case WM_MOUSEMOVE:
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;

				TrackMouseEvent(&tme);
				bool old_on_mouseover = state->on_mouseover;
				state->on_mouseover = true;
				if(old_on_mouseover != state->on_mouseover) InvalidateRect(hwnd, 0, TRUE);

				return DefSubclassProc(hwnd, msg, wparam, lparam);
			}
			//case CBN_DROPDOWN://lets us now that the list is about to be show, therefore the user clicked us
			case WM_PAINT:
			{
				if(state->on_mouseover) return DefSubclassProc(hwnd, msg, wparam, lparam);
				else {
					PAINTSTRUCT ps;
					HDC dc = BeginPaint(hwnd, &ps); defer{ EndPaint(hwnd, &ps); };

					RECT rc; GetClientRect(hwnd, &rc);
					int w = RECTW(rc), h = RECTH(rc);

					HBRUSH bk_br = global::colors.ControlBk_Dark;
					HBRUSH txt_br = global::colors.ControlTxt;

					//Bk
					{
						FillRect(dc, &rc, bk_br);
					}

					SelectFont(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
					SetBkColor(dc, ColorFromBrush(bk_br));
					SetTextColor(dc, ColorFromBrush(txt_br));
					str txt = L"[" + RS((u32)str_id) + L"]";
					DrawText(dc, txt.c_str(), (int)txt.length(), &rc, DT_EDITCONTROL | DT_CENTER | DT_VCENTER | DT_SINGLELINE);//TODO(fran): DT_CENTER or DT_LEFT?

					return 0;
				}
			} break;
			case WM_ERASEBKGND:
			{
				if(state->on_mouseover) return DefSubclassProc(hwnd, msg, wparam, lparam);
				else return 1;
			} break;
			}

			return DefSubclassProc(hwnd, msg, wparam, lparam);
		}
	}
}