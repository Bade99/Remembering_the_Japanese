#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_Renderer.h"
#include "win32_edit_oneline.h"
#include "win32_listbox.h"
#include <windowsx.h>

//NOTE: no ASCII support, always utf16

//------------------"API"------------------:
//searchbox::wndclass identifies the window class to be used when calling CreateWindow()
//searchbox::set_brushes() to set the HBRUSHes used by the searchbox, its editbox and listbox


//------------------"Additional Styles"------------------:
#define SRB_ROUNDRECT 0x0001 //Control has rounded corners

//TODO(fran): add to editbox the EN_DOWN notif when the user presses the down key, that signifies us that they want the listbox open and a search performed on what's currently on the editbox (eg if they wrote smth and then went to a different app, when they come back the listbox wont show, there are two options here, show the listbox directly when the editbox gets clicked, or wait for the down arrow msg, or some other input). Also now that I think about it, normally the selection on the searchbox doesnt redirect you directly to your destination, a search is performed at the point and the items shown on a non floating listbox, but that really depends, eg google sends you to a page if it can, otherwise loads a different page with the non floating listbox

namespace searchbox {
	
	constexpr cstr wndclass[] = L"がいじんの_wndclass_searchbox";

	struct ProcState {
		HWND wnd;
		HWND parent;
		struct brushes {
			HBRUSH bk, img;
			HBRUSH bk_disabled, img_disabled;
		}brushes;
		HFONT font;//simply cause windows msgs ask for it

		struct {
			HWND editbox;
			HWND listbox;
			//HWND button; //TODO(fran): add as an extra style
		}controls;
	};
	//TODO(fran): use state->brushes.img to render an img icon of a magnifying glass


	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	//NOTE: the caller takes care of deleting the brushes, we dont do it
	//Any null HBRUSH param is ignored and the current one remains
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH txt, HBRUSH bk, HBRUSH border, HBRUSH img, HBRUSH txt_disabled, HBRUSH bk_disabled, HBRUSH border_disabled, HBRUSH img_disabled) {
		ProcState* state = get_state(wnd);
		if (state) {
			edit_oneline::set_brushes(state->controls.editbox, repaint, txt, bk, border, txt_disabled, bk_disabled, border_disabled);

			//TODO(fran): set listbox brushes (though we want to allow the user to choose the render function)

			if (bk)state->brushes.bk = bk;
			if (bk_disabled)state->brushes.bk_disabled = bk_disabled;

			if (img)state->brushes.img = img;
			if (img_disabled)state->brushes.img_disabled = img_disabled;

			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
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
			return TRUE; //continue creation
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;
			LONG_PTR style = GetWindowLongPtr(state->wnd, GWL_STYLE);

			//TODO(fran): settext is redirected to the editbox
			if (createnfo->lpszName) PostMessage(state->wnd, WM_SETTEXT, 0, (LPARAM)createnfo->lpszName);

			DWORD editbox_extrastyles= ES_LEFT;
			if (style & SRB_ROUNDRECT) editbox_extrastyles |= ES_ROUNDRECT;

			state->controls.editbox = CreateWindowW(edit_oneline::wndclass, 0, WS_CHILD | WS_VISIBLE | WS_TABSTOP | editbox_extrastyles
				, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);

			state->controls.listbox = CreateWindowEx(WS_EX_TOOLWINDOW /*| WS_EX_TOPMOST*/,listbox::wndclass, 0, WS_POPUP
				, 0, 0, 0, 0, NULL, 0, NULL, NULL); 
			//TODO(fran): create our own floating listbox
			//TODO(fran): try with ComboLBox (though that probably animates and at that point it's faster to create something from scratch)

			//TODO(fran): another idea is to have a huge open listbox that fills itself row by row, could look nice for this project since the search page is completely empty with lots of space, but really not reusable anywhere else, you always want a small search bar and a big over-the-window/floating listbox with results

			return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): remove once we know all the things this does
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
		case WM_SIZE: //4th, sent _after_ the wnd has been resized
		{
			//TODO(fran): do I resize the listbox here, or only when I want to show it?
			RECT r; GetClientRect(state->wnd, &r);
			i32 w = RECTW(r), h =RECTH(r);
			MoveWindow(state->controls.editbox, 0, 0, w, h, FALSE);
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
			BOOL redraw = LOWORD(lparam);
			state->font = font;
			SendMessage(state->controls.editbox, msg, wparam, lparam);
			SendMessage(state->controls.listbox, msg, wparam, lparam);
			if (redraw) RedrawWindow(state->wnd, NULL, NULL, RDW_INVALIDATE);
			return 0;
		} break;
		case WM_GETFONT:
		{
			return (LRESULT)state->font;
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			if (state) {
				DestroyWindow(state->controls.listbox);//NOTE: since this isnt a child I dont think it gets automatically destroyed
				free(state);
			}
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
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			//ps.rcPaint
			HDC dc = BeginPaint(state->wnd, &ps);
			bool window_enabled = IsWindowEnabled(state->wnd);
			LONG_PTR style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			//TODO(fran): maybe draw a user defined image (on the right if ES_LEFT or ES_CENTER, on the left otherwise)
			EndPaint(hwnd, &ps);
			return 0;
		} break;
		case WM_PARENTNOTIFY:
		{
			WORD event = LOWORD(wparam);
			Assert(event == WM_CREATE || event == WM_DESTROY || event == WM_LBUTTONDOWN || event == WM_MBUTTONDOWN || event == WM_RBUTTONDOWN || event == WM_XBUTTONDOWN);
			return 0;
		} break;
		case WM_COMMAND:
		{
			HWND child = (HWND)lparam;
			if (child) {//Notifs from our childs
				WORD notif = HIWORD(wparam);
				if (child == state->controls.editbox) {
					switch (notif) {
					case EN_ENTER:
					{
						//TODO(fran): it's not that easy to know what to tell the user, we have to differentiate between "the user chose an element from the listbox" and "the user wrote something", also the user could first move to some element of the listbox but afterwards write something more (well actually the problem is mine, it's easy to differentiate, a click is obviously for listbox, and enter is listbox if it's visible (since the value is linked with the editbox's) and editbox if not visible aka listbox has no elements). My problem comes when there's the same writing for different words and then I need to filter with more info, specifically the info that the listbox's elements have (since I plan to add it there). Well actually this is bullshit, it's very simple, we need two cases, if the listbox is not visible then what's in the editbox counts and it's just that info, _but_ if the listbox is visible then what count is that element's data (since the editbox is, in this case, simply showing part of the info the listbox provided).
						//A different idea would be the google/internet-search-engine searchbar approach, when the user selects an existing listbox item we go directly to it, otherwise we show possible results on a non floating listbox below the searchbar
					} break;
					case EN_ESCAPE:
					{
						Assert(0);//TODO
					} break;
					default: Assert(0);
					}
				}
				else if (child == state->controls.listbox) {

				}
				else {
					Assert(0);
				}
			}
			else {
				//Menu notifications
				Assert(0);
			}
			return 0;
		} break;
		//Msgs redirected to editbox
		case WM_SETDEFAULTTEXT:
		case WM_SETTEXT:
		case WM_GETTEXT:
		case WM_GETTEXTLENGTH:
		{
			return SendMessage(state->controls.editbox, msg, wparam,lparam);
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