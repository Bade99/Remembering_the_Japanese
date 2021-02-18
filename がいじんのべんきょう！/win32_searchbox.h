#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_Renderer.h"
#include "win32_edit_oneline.h"
#include "win32_listbox.h"
#include <windowsx.h>
#include <CommCtrl.h>//SetWindowSubclass
#include "win32_new_msgs.h"

//NOTE: no ASCII support, always utf16

//------------------"API"------------------:
//searchbox::wndclass identifies the window class to be used when calling CreateWindow()
//searchbox::set_brushes() to set the HBRUSHes used by the searchbox, its editbox and listbox
//searchbox::set_function_free_elements() to free the content of N elements in the listbox, this is called when we need to clear the listbox, for example when it's hidden or when the search options change
//searchbox::set_function_retrieve_search_options()
//searchbox::set_function_perform_search()
//searchbox::set_function_show_element_on_editbox()
//searchbox::set_function_render_listbox_element()
//searchbox::set_user_extra() extra user-defined data to be sent on each function call

//-------------"API" (Additional Messages)-------------:
#define searchbox_base_msg_addr (WM_USER + 3300)
#define SRBM_CLKOUTSIDE (searchbox_base_msg_addr + 20) //do not use, internal msg


//IMPORTANT: instead of having what's written on the editbox duplicated on the first row of the listbox we'll implement ESC (escape) as a means of getting back to the word you were writing, TODO(fran): for this to work we need to store here the last written input by the user, and restore it on EN_ESCAPE or whatever

//------------------"Additional Styles"------------------:
#define SRB_ROUNDRECT 0x0001 //Control has rounded corners

//TODO(fran): add to editbox the EN_DOWN notif when the user presses the down key, that signifies us that they want the listbox open and a search performed on what's currently on the editbox (eg if they wrote smth and then went to a different app, when they come back the listbox wont show, there are two options here, show the listbox directly when the editbox gets clicked, or wait for the down arrow msg, or some other input). Also now that I think about it, normally the selection on the searchbox doesnt redirect you directly to your destination, a search is performed at the point and the items shown on a non floating listbox, but that really depends, eg google sends you to a page if it can, otherwise loads a different page with the non floating listbox
//TODO(fran): small BUG, when the user click on an item from the listbox we lose keyboard focus and therefore the nonclient renders itself as inactive for a couple of frames before coming back to active
//TODO(fran): I dont see an easy way of hiding the listbox if the user starts clicking outside of it but inside our main wnd, we would need some way of detecting a click or some change, right now neither the listbox, nor the editbox, nor us get anything when the user clicks outside

namespace searchbox {
	
	constexpr cstr wndclass[] = L"がいじんの_wndclass_searchbox";

	//NOTE: trying a different approach, the control is the active part, and the user is passive. May be a bad idea, we'll find out
	typedef void(*searchbox_func_free_elements)(ptr<void*> elements, void* user_extra);
	typedef ptr<void*>(*searchbox_func_retrieve_search_options)(utf16_str user_input, void* user_extra);//you're provided with the string written on the editbox and are expected to return an array of elements to show on the listbox
	typedef void(*searchbox_func_perform_search)(void* element, bool is_element, void* user_extra);//the user has selected something to be searched. If is_element == true then element points to your element data, if == false then it points to a utf16_str with the content of the editbox
	typedef void(*searchbox_func_show_element_on_editbox)(HWND editbox, void* element, void* user_extra);//you're given one of your elements and tasked with SendMessage(WM_SETTEXT_NO_NOTIFY) to the provided editbox hwnd (this is used when the user scrolls with the arrow keys along the options on the listbox, you can also decide _not_ to update the editbox if you dont feel like it, though it's not really supported right now, what happens could not be what the user expects)

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

		void* user_extra;

		searchbox_func_free_elements free_elements;
		searchbox_func_retrieve_search_options retrieve_search_options;
		searchbox_func_perform_search perform_search;
		searchbox_func_show_element_on_editbox show_element_on_editbox;

		struct{
			HHOOK hookmouseclick;
		}impl;
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

	void set_user_extra(HWND wnd, void* user_extra) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->user_extra = user_extra;
			listbox::set_user_extra(state->controls.listbox, user_extra);
			//TODO(fran): should I redraw?
		}
	}

	void set_function_free_elements(HWND wnd, searchbox_func_free_elements func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->free_elements = func;
			//TODO(fran): should I redraw?
		}
	}

	void set_function_retrieve_search_options(HWND wnd, searchbox_func_retrieve_search_options func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->retrieve_search_options = func;
			//TODO(fran): should I redraw?
		}
	}

	void set_function_perform_search(HWND wnd, searchbox_func_perform_search func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->perform_search = func;
			//TODO(fran): should I redraw?
		}
	}

	void set_function_show_element_on_editbox(HWND wnd, searchbox_func_show_element_on_editbox func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->show_element_on_editbox = func;
			//TODO(fran): should I redraw?
		}
	}

	void set_function_render_listbox_element(HWND wnd, listbox::listbox_func_renderelement func) {
		ProcState* state = get_state(wnd);
		if (state) {
			listbox::set_render_function(state->controls.listbox, func);
		}
	}

	//IMPORTANT (multiwnd/multithreading dubious): uses a static object to store the hwnd, since we use it to hide listboxes, and only one is active at any single time this shouldnt be a problem, but you never know
	HWND __hookmouseclick_store_hwnd(HWND _wnd = (HWND)INT32_MIN) {
		static HWND wnd{ 0 };
		if (_wnd != (HWND)INT32_MIN) {
			wnd = _wnd;
		}
		return wnd;
	}

	LRESULT CALLBACK hookmouseclick(int code, WPARAM wparam, LPARAM lparam) {
		//printf("MOUSECLICKHOOK:0x%08x\n", wparam);
		if (code == HC_ACTION) {//TODO(fran): WH_MOUSE also includes a HC_NOREMOVE code
			switch (wparam) {
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_NCLBUTTONDOWN:
			case WM_NCRBUTTONDOWN:
			{
				HWND wnd = __hookmouseclick_store_hwnd();
				ProcState* state = get_state(wnd);
				if (state) {
					//check if the mouse is inside our controls
#if 0
					POINT mouse = ((MSLLHOOKSTRUCT*)lparam)->pt;//TODO(fran): _per-monitor-aware_ screen coordinates (idk I that changes something)
#else
					POINT mouse = ((MOUSEHOOKSTRUCT*)lparam)->pt;//screen coordinates
#endif
					RECT lbrc; GetWindowRect(state->controls.listbox, &lbrc);
					RECT edrc; GetWindowRect(state->controls.editbox, &edrc);
					if (!test_pt_rc(mouse, lbrc) && !test_pt_rc(mouse, edrc)) {
						//Mouse clicked outside our control -> hide the listbox
						PostMessage(state->wnd, SRBM_CLKOUTSIDE, 0, 0);
					}

				}
			}break;
			}
		}
		return CallNextHookEx(0, code, wparam, lparam);
	}

	void show_listbox(ProcState* state, bool show) {
		if (show) {
			RECT rw;  GetWindowRect(state->wnd, &rw);
			i32 w = RECTW(rw), h = RECTH(rw) * (i32)listbox::get_element_cnt(state->controls.listbox);
#if 0
			//REMEMBER: SetWindowPos activates the window no matter how many times you tell it not to if you're doing many things like in this case
			SetWindowPos(state->controls.listbox, HWND_NOTOPMOST, rw.left, rw.bottom, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE);
#else
			SetWindowPos(state->controls.listbox, HWND_NOTOPMOST/*TODO(fran): we may want HWND_TOP or HWND_TOPMOST*/, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOSIZE);
			MoveWindow(state->controls.listbox, rw.left, rw.bottom, w, h, TRUE);
			ShowWindow(state->controls.listbox, SW_SHOWNA);//TODO(fran): make sure it's on top of the z order, at least on top of us
#endif
			//We need to know when the user clicks outside of our editbox/listbox to be able to hide the listbox
			if (!state->impl.hookmouseclick) {
				//TODO(fran): maybe this should be standar implementation in the listbox
				__hookmouseclick_store_hwnd(state->wnd);
				state->impl.hookmouseclick = SetWindowsHookEx(WH_MOUSE, hookmouseclick, 0, GetCurrentThreadId()); //attach the hook
				//state->impl.hookmouseclick = SetWindowsHookEx(WH_MOUSE_LL, hookmouseclick, 0, GetCurrentThreadId()); //TODO(fran): I wanted to use this one but apparently low level hooks cannot be run on debug threads, the docs recommend using raw input instead //https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse
				Assert(state->impl.hookmouseclick);
			}

		}
		else {
			printf("SEARCHBOX:HIDE LISTBOX\n");
			ShowWindow(state->controls.listbox, SW_HIDE);

			UnhookWindowsHookEx(state->impl.hookmouseclick); // remove the hook
			state->impl.hookmouseclick = 0;
		}
	}

	//TODO(fran): instead of working as a pasamanos it'd be best to provide an attach_listbox() function with a full configured listbox that abides by our listbox specification
	void set_listbox_dimensions(HWND wnd, listbox::dimensions dims) {
		ProcState* state = get_state(wnd);
		if (state) {
			listbox::set_dimensions(state->controls.listbox, dims);
		}
	}


#define EN_UP (EN_ESCAPE+50)
#define EN_DOWN (EN_ESCAPE+51)
	//TODO(fran): this functionality could be added to the default edit control
	LRESULT CALLBACK EditNotifyProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
		HWND parent = GetParent(hwnd);
		switch (msg) {
		case WM_KEYDOWN:
		{
			char vk = (char)wparam;
			switch (vk) {
			case VK_UP:
			{
				PostMessage(parent, WM_COMMAND, MAKELONG(0, EN_UP), (LPARAM)hwnd);
			} break;
			case VK_DOWN:
			{
				PostMessage(parent, WM_COMMAND, MAKELONG(0, EN_DOWN), (LPARAM)hwnd);
			} break;
			}
		} break;
		}

		return DefSubclassProc(hwnd, msg, wparam, lParam);
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
			SetWindowSubclass(state->controls.editbox, EditNotifyProc,0,0 );//TODO(fran): idk if calling RemoveWindowSubclass on WM_DESTROY is necessary

			state->controls.listbox = CreateWindowEx(WS_EX_TOOLWINDOW /*| WS_EX_TOPMOST*/,listbox::wndclass, 0, WS_POPUP
				, 0, 0, 0, 0, NULL, 0, NULL, NULL); 
			listbox::set_parent(state->controls.listbox, state->wnd);
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
					case EN_CHANGE:
					{
						//The user has modified the editbox -> retrieve search options based on the content of the editbox
						utf16_str txt; _get_edit_str(state->controls.editbox, txt); defer{ if(txt.sz) free_any_str(txt.str); };
						//TODO(fran): check what happens if the editbox is empty -> malloc(0) is implementation dependant
						
						//Clear previous search options
						if (state->free_elements)
							state->free_elements(listbox::get_all_elements(state->controls.listbox)/*HACK*/, state->user_extra);
						listbox::remove_all_elements(state->controls.listbox);//NOTE: this is inefficient since in the case of set_elements() we'll basically perform two repaints on the listbox

						bool show_lb = false;

						if (txt.sz_char() > 1 /*not null nor just the null terminator*/) {
							//Get search options
							ptr<void*> search_options{0}; defer{ search_options.free(); };
							if(state->retrieve_search_options) 
								search_options = state->retrieve_search_options(txt, state->user_extra);

							if (search_options.cnt) {
								//Set new elements (auto removes existing ones), show the listbox
								listbox::set_elements(state->controls.listbox, search_options.mem, search_options.cnt);
								show_lb = true;
							}
							else {
								//No search options, hide the listbox
								show_lb = false;
							}
						}
						else {
							//The editbox is empty, hide the listbox
							show_lb = false;
						}

						show_listbox(state, show_lb);

					} break;
					case EN_ENTER:
					{
						//TODO(fran): it's not that easy to know what to tell the user, we have to differentiate between "the user chose an element from the listbox" and "the user wrote something", also the user could first move to some element of the listbox but afterwards write something more (well actually the problem is mine, it's easy to differentiate, a click is obviously for listbox, and enter is listbox if it's visible (since the value is linked with the editbox's) and editbox if not visible aka listbox has no elements). My problem comes when there's the same writing for different words and then I need to filter with more info, specifically the info that the listbox's elements have (since I plan to add it there). Well actually this is bullshit, it's very simple, we need two cases, if the listbox is not visible then what's in the editbox counts and it's just that info, _but_ if the listbox is visible then what count is that element's data (since the editbox is, in this case, simply showing part of the info the listbox provided).
						//A different idea would be the google/internet-search-engine searchbar approach, when the user selects an existing listbox item we go directly to it, otherwise we show possible results on a non floating listbox below the searchbar

						//SOLUTION: the listbox's selection is cleared each time the user writes smth, that means we have a way of differentiating between enter for listbox and enter for editbox, if a valid selection is present in the listbox then it's a listbox enter, otherwise it's an editbox one

						size_t lb_sel = listbox::get_cur_sel(state->controls.listbox);
						size_t lb_cnt = listbox::get_element_cnt(state->controls.listbox);
						if (lb_sel < lb_cnt) {
							//Valid listbox selection, use the listbox item
							if (state->perform_search)
								state->perform_search(listbox::get_element_at(state->controls.listbox, lb_sel), true, state->user_extra);
						}
						else {
							//Invalid listbox selection, use the editbox's text
							utf16_str txt; _get_edit_str(state->controls.editbox, txt); defer{ if (txt.sz) free_any_str(txt.str); };
							if (state->perform_search)
								state->perform_search(&txt, false, state->user_extra);
						}

						//Clear search options
						if (state->free_elements)
							state->free_elements(listbox::get_all_elements(state->controls.listbox)/*HACK*/, state->user_extra);
						listbox::remove_all_elements(state->controls.listbox);
						show_listbox(state, false);


					} break;
					case EN_ESCAPE:
					{
						show_listbox(state, false);
						if (state->free_elements)
							state->free_elements(listbox::get_all_elements(state->controls.listbox)/*HACK*/, state->user_extra);
						listbox::remove_all_elements(state->controls.listbox);
						//TODO(fran): restore the content of the editbox (WM_SETTEXT_NONOTIF)
					} break;
					case EN_UP:
					{
						listbox::sel_up(state->controls.listbox);
					} break;
					case EN_DOWN:
					{
						listbox::sel_down(state->controls.listbox);
					} break;
					case EN_KILLFOCUS:
					{
						//NOTE: since killfocus arrives _before_ LBN_CLK, we need to check who is the one with focus now, if it's the listbox cause the user clicked it then we are okay, otherwise we gotta clear it
						if (state->controls.listbox != GetFocus()) {
							//Clear search options
							if (state->free_elements)
								state->free_elements(listbox::get_all_elements(state->controls.listbox)/*HACK*/, state->user_extra);
							listbox::remove_all_elements(state->controls.listbox);
							show_listbox(state, false);
						}
					} break;
					default: Assert(0);
					}
				}
				else if (child == state->controls.listbox) {
					switch (notif) {
					case LBN_CLK:
					{
						if (state->perform_search)
							state->perform_search(listbox::get_clicked_element(state->controls.listbox), true, state->user_extra);

						//Clear search options
						if (state->free_elements)
							state->free_elements(listbox::get_all_elements(state->controls.listbox)/*HACK*/, state->user_extra);
						listbox::remove_all_elements(state->controls.listbox);
						show_listbox(state, false);

					} break;
					case LBN_SELCHANGE://The user is moving up and down the elements of the listbox
					{
						size_t lb_sel = listbox::get_cur_sel(state->controls.listbox);
						size_t lb_cnt = listbox::get_element_cnt(state->controls.listbox);
						if (lb_sel < lb_cnt) {
							//Valid listbox selection
							if (state->show_element_on_editbox)
								state->show_element_on_editbox(state->controls.editbox,listbox::get_element_at(state->controls.listbox, lb_sel), state->user_extra);
							//TODO(fran): save what the user currently wrote to be able to restore it later
						}
						else {
							//Invalid listbox selection
						}
					} break;
					default: Assert(0);
					}
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
		case SRBM_CLKOUTSIDE://The user clicked outside our listbox/editbox -> hide the listbox
		{
			//Clear search options
			if (state->free_elements)
				state->free_elements(listbox::get_all_elements(state->controls.listbox)/*HACK*/, state->user_extra);
			listbox::remove_all_elements(state->controls.listbox);
			show_listbox(state, false);
		} break;
		case CB_SETCUEBANNER:
		{
			return SendMessage(state->controls.editbox, WM_SETDEFAULTTEXT, wparam, lparam);
		} break;
		//case CB_GETCUEBANNER://TODO(fran): I dont have a WM_GETDEFAULTTEXT
		//{
		//	//NOTE: there is more than one stupid thing about this msg, first the paramaters are REVERSED compared to WM_GETTEXT, and second there's no CB_GETCUEBANNERLENGTH!
		//	return SendMessage(state->controls.editbox, WM_GETDEFAULTTEXT, lparam, wparam);
		//} break;
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