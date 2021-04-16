#pragma once
#include "win32_Platform.h"
#include "windows_sdk.h"
#include <windowsx.h>
#include "win32_Helpers.h"

#include <vector>

//-----------------API-----------------:
// navbar::set_theme()
// navbar::attach()
// navbar::detach()


//-----------------Usage-----------------:
// Every control added to the navbar must be able to answer to WM_DESIRED_SIZE in order to provide maximum and minimum bounds for proper resizing

//INFO: this is a _horizontal_ navbar //TODO(fran): add vertical support, should be pretty easy, only thing that would need changing is resize_controls()

//TODO(fran): for the navbar to work better and more seamless it'f be good to provide buttons that take function pointers to handle click events in a more decentralized way, the best would be for the user to create a button, set everything they need for it and then send it off to us with the only request to be how to position it, this way the button is completely unaware of the existance of the navbar, no stupid message forwarding from the button to the nav and then to the main wnd, stupid, pointless and bug prone

//TODO(fran): look at ways for the user to attach to this hwnd, the simple method would be that the user simply does CreateWindow with us as parent and then notifies us that for 'x' HWND they want it placed in a certain way and size.
//Following on this we could specify three placement possibilities, left, right or center

namespace navbar {
	constexpr cstr wndclass[] = TEXT("win32_wndclass_navbar");

	struct Theme {
		struct {
			brush_group bk;
		} brushes;
	};

	struct ProcState { //NOTE: must be initialized to zero
		HWND wnd;
		HWND parent;

		union Controls {
			using type = std::vector<HWND>;//TODO(fran): maybe using set or map is better, though my main need is straight end to end iteration and idk if anything is as fast as a vector there
			struct {
				type left, center, right;
			};
			type all[3];
		}controls;

		Theme theme;

	};

	ProcState* get_state(HWND hwnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(hwnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }

	void ask_for_resize(ProcState* state) { PostMessage(state->wnd, WM_SIZE, 0, 0); }

	bool _copy_theme(Theme* dst, const Theme* src) {
		bool repaint = false;
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);
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

	enum class attach_point : u32 {left=1,center,right};
	//NOTE: Already attached childs are automatically removed and attached to their new place
	//NOTE: a negative index means add the window at the end of the list
	void attach(HWND wnd, HWND child, attach_point attachment, i32 index) {
		ProcState* state = get_state(wnd);
		if (state) {
			Assert((u32)attachment < ARRAYSIZE(state->controls.all));

			for (auto& c : state->controls.all) c.erase(std::remove(c.begin(), c.end(), child), c.end());
			
			if (index < 0) state->controls.all[(u32)attachment].push_back(child);
			else Assert(0);//TODO(fran): insertion at any point

			ask_for_resize(state);
		}
	}

	void detach(HWND wnd, HWND child) {
		ProcState* state = get_state(wnd);
		if (state) {
			for (auto& c : state->controls.all) c.erase(std::remove(c.begin(), c.end(), child), c.end());

			ask_for_resize(state);
		}
	}

	void resize_controls(ProcState* state) {
		Assert(0);
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//printf(msgToString(msg)); printf("\n");
		ProcState* state = get_state(hwnd);

		switch (msg) {
		case WM_NCCREATE: { //1st msg received
			ProcState* st = (decltype(st))calloc(1, sizeof(*st));
			Assert(st);
			set_state(hwnd, st);
			CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;
			st->parent = creation_nfo->hwndParent;
			st->wnd = hwnd;
			for(auto& c : st->controls.all) c = decltype(st->controls)::type();//TODO(fran): nicer initialization
			return TRUE; //continue creation, this msg seems kind of more of a user setup place, strange considering there's also WM_CREATE
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			if (state) {
				for (auto& c : state->controls.all) c.~vector();
				
				free(state);
			}
			return 0;
		}break;


		case WM_NCHITTEST: //Received when the mouse goes over the window, on mouse press or release, and on WindowFromPoint
		{
			// Get the point coordinates for the hit test.
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Screen coords, relative to upper-left corner

			// Get the window rectangle.
			RECT rw; GetWindowRect(state->wnd, &rw);

			LRESULT hittest = HTNOWHERE;

			// Determine if the point is inside the window
			if (test_pt_rc(mouse, rw))hittest = HTCLIENT;

			return hittest;
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
		case WM_MOUSEMOVE:
		{
			//After WM_NCHITTEST and WM_SETCURSOR we finally get that the mouse has moved
			//Sent to the window where the cursor is, unless someone else is explicitly capturing it, in which case this gets sent to them
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//client coords, relative to upper-left corner

			return 0;
		} break;
		case WM_MOUSEACTIVATE:
		{
			//Sent to us when we're still an inactive window and we get a mouse press
			//TODO(fran): we could also ask our parent (wparam) what it wants to do with us
			return MA_ACTIVATE; //Activate our window and post the mouse msg
		} break;
		case WM_LBUTTONDOWN:
		{
			return 0;
		} break;
		case WM_LBUTTONUP:
		{
			return 0;
		} break;
		case WM_RBUTTONDOWN:
		{
			return 0;
		} break;
		case WM_RBUTTONUP:
		{
			return 0;
		} break;
		case WM_MOUSEWHEEL:
		{
			return 0;
		} break;


		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_SETTEXT:
		{
			return 0;
		} break;
		case WM_SETFONT: {
			return 0;
		} break;
		case WM_GETFONT:
		{
			return 0;
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
		case WM_MOVE: //Sent on startup after WM_SIZE, although possibly sent by DefWindowProc after I let it process WM_SIZE, not sure
		{
			//This msg is received _after_ the window was moved
			//Here you can obtain x and y of your window's client area
			return 0;
		} break;
		case WM_SIZE: {
			resize_controls(state);
			//return DefWindowProc(hwnd, msg, wparam, lparam);
			return 0;
		} break;


		case WM_PAINT:
		{
			PAINTSTRUCT ps; //TODO(fran): we arent using the rectangle from the ps, I think we should for performance
			HDC dc = BeginPaint(state->wnd, &ps); defer{ EndPaint(state->wnd, &ps); };
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
			bool enabled = IsWindowEnabled(state->wnd);
			HBRUSH bk_br = enabled ? state->theme.brushes.bk.normal : state->theme.brushes.bk.disabled;

			FillRect(dc, &rc, bk_br);

		} break;
		case WM_ERASEBKGND:
		{
			//You receive this msg if you didnt specify hbrBackground when you registered the class, now it's up to you to draw the background
			HDC dc = (HDC)wparam;

			return 0; //If you return 0 then on WM_PAINT fErase will be true, aka paint the background there
		} break;
		case WM_NCPAINT:
		{
			//Paint non client area, we shouldnt have any
			//HDC hdc = GetDCEx(hwnd, (HRGN)wparam, DCX_WINDOW | DCX_USESTYLE);
			//ReleaseDC(hwnd, hdc);
			return 0; //we process this message, for now
		} break;


		case WM_SHOWWINDOW: //On startup I received this cause of WS_VISIBLE flag
		{
			//Sent when window is about to be hidden or shown, doesnt let it clear if we are in charge of that or it's going to happen no matter what we do
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_ENABLE:
		{
			BOOL enable = (BOOL)wparam;
			ask_for_repaint(state);
			return 0;
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


	void init_wndclass(HINSTANCE instance) {//TODO(fran): pretty sure the hinstance is pointless for this case
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