#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_Global.h"
#include "unCap_Math.h"
#include <deque>

namespace page {
	constexpr cstr wndclass[] = TEXT("win32_wndclass_page");

	struct Theme {
		struct {
			brush_group bk, border;//NOTE: the page itself renders nothing but the background
		} brushes;
		struct {
			u32 border_thickness = U32MAX;
		}dimensions;
	};

	struct ProcState {
		HWND wnd;
		HWND parent;
		Theme theme;

		bool does_scrolling;

		struct {
			b32 active; //currently performing animation
			int current_frame; //starts at
			f32 dt;
			int total_frames;
			i64 _time_between; //internal
			f32 time_between_last_two_scroll_events; //ms
		} scroll_anim;
		std::deque<int> scroll_tasks;

		f32 scroll;//vertical scrolling of page //@int NOTE(fran): given the limitations of MoveWindow we must interpret this value as an integer (cast), the float precision is only used for animations

		//anim //@old @delete
		int scroll_frame; //@delete
		f32 scroll_dt;
		f32 scroll_v;
		f32 scroll_a;
		bool scroll_on_anim; //@delete
	};

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void ask_for_repaint(ProcState* state) {
		InvalidateRect(state->wnd, 0, TRUE); //ask for WW_PAINT
	}

	//void set_dimensions(HWND wnd, size_t border_thickness = ((size_t)-1)) {
	//	ProcState* state = get_state(wnd);
	//	size_t res;
	//	if (state) {
	//		if (border_thickness != ((size_t)-1) && state->border_thickness != border_thickness) {
	//			state->border_thickness = border_thickness;
	//			ask_for_repaint(state);
	//		}
	//	}
	//}

	bool _copy_theme(Theme* dst, const Theme* src) {
		bool repaint = false;
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);
		repaint |= copy_brush_group(&dst->brushes.border, &src->brushes.border);

		if (src->dimensions.border_thickness != U32MAX) {
			dst->dimensions.border_thickness = src->dimensions.border_thickness; repaint = true;
		}

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

	//NOTE: the caller takes care of deleting the brushes, we dont do it
	//void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk, HBRUSH border, HBRUSH bk_disabled, HBRUSH border_disabled) {
	//	ProcState* state = get_state(wnd);
	//	if (state) {
	//		if (bk)state->brushes.bk = bk;
	//		if (border)state->brushes.border = border;
	//		if (bk_disabled)state->brushes.bk_dis = bk_disabled;
	//		if (border_disabled)state->brushes.border_dis = border_disabled;
	//		if (repaint) {
	//			ask_for_repaint(state);
	//		}
	//	}
	//}

	void set_scrolling(HWND wnd, bool does_scrolling) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->does_scrolling = does_scrolling;//TODO(fran): reset current scrolling?
		}
	}

#if 0
	void smooth_scroll(ProcState* state, int increment) {
		static const int total_frames = 100;
		state->scroll_frame = 0;

		state->scroll_a = (f32)increment * 10;
		state->scroll_dt = 1.f / (f32)win32_get_refresh_rate_hz(state->wnd);//duration of each frame
		static u32 scroll_ms = (u32)(state->scroll_dt * 1000.f);


		//NOTEs about this amazing find: 1st lambda must be static, 2nd auto cannot be used, you must declare the function type
		//thanks https://stackoverflow.com/questions/2067988/recursive-lambda-functions-in-c11
		static void (*scroll_anim)(HWND, UINT, UINT_PTR, DWORD) =
			[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
			ProcState* state = get_state(hwnd);
			if (state) {
				state->scroll_a += -8.5f * state->scroll_v;//drag

				f32 dy = .5f * state->scroll_a * squared(state->scroll_dt) + state->scroll_v * state->scroll_dt;
				state->scroll_v += state->scroll_a * state->scroll_dt;//TODO: where does this go? //NOTE: Casey put it here, it had it before calculating pos_delta
				dy *= 100;

				state->scroll += dy;
				RECT r; GetWindowRect(state->wnd, &r); MapWindowPoints(0, state->parent, (POINT*)&r, 2);
				MoveWindow(state->wnd, r.left, r.top + (int)dy, RECTW(r), RECTH(r), TRUE);//TODO(fran): store f32 y position (PROBLEM with this idea is then I have a different value from the real one, and if someone else moves us then we'll cancel that move on the next scroll, it may be better to live with this imprecise scrolling for now)
				if (state->scroll_frame++ < total_frames) {
					SetTimer(state->wnd, anim_id, scroll_ms, scroll_anim); state->scroll_a = 0;
				}
				else { state->scroll_on_anim = false; KillTimer(state->wnd, anim_id); }
			}
		};
		if (state->scroll_on_anim) {

		}
		else {
			state->scroll_on_anim = true;
			SetTimer(state->wnd, 1111, 0, scroll_anim);
		}
	}
#else
	void smooth_scroll(ProcState* state, int increment) {
		
		//TODO(fran): the scrolling is now pretty good & responsive, only thing missing is a bit of smoothing across large distances (speeding up at the beginning and slowing down close to the end, possibly based on the frecuency of new scroll events?)
		state->scroll_anim.time_between_last_two_scroll_events = (f32)EndCounter(state->scroll_anim._time_between); //TODO(fran): it may be good to calculate the 2nd derivative of this, aka the delta between the last two times between two scroll events
		if (state->scroll_anim.time_between_last_two_scroll_events > .200f /*ms expressed in seconds*/)
			state->scroll_anim.time_between_last_two_scroll_events = 0;
		//printf("time between last two scroll events: %f ms\n", (f32)EndCounter(state->scroll_anim._time_between));
		state->scroll_anim._time_between = StartCounter();

		if (state->scroll_tasks.size() && (signum(state->scroll_tasks.back()) != signum(increment))) {
			//IMPORTANT(fran): I think the Timer runs on the same thread, otherwise race conditions accessing & modifying scroll_tasks can crash the app
			auto last = state->scroll_tasks.back();
			state->scroll_tasks.clear();
			state->scroll_tasks.push_back(last);
		}
		state->scroll_tasks.push_back(increment);

		
		static void (*scroll_timeout)(HWND, UINT, UINT_PTR, DWORD) =
			[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
			ProcState* state = get_state(hwnd);
			if (state) {
				if (state->scroll_tasks.size()) {
					auto last = state->scroll_tasks.back();
					state->scroll_tasks.clear();
					state->scroll_tasks.push_back(last);
				}
			}
		};

		//If no new scroll events happen after X ms then stop scrolling, the user is no longer scrolling the mouse wheel and wants scrolling to stop
		SetTimer(state->wnd, 2222, 125/*ms*/, scroll_timeout);

		static auto setup_scroll_anim = [](ProcState* state, TIMERPROC scroll_animation) {
			state->scroll_anim.active = true;

			//auto anim_duration = (50.f + state->scroll_anim.time_between_last_two_scroll_events) / state->scroll_tasks.size(); //ms
			auto anim_duration = 50.f / state->scroll_tasks.size() + state->scroll_anim.time_between_last_two_scroll_events; //ms //TODO(fran): the gradual slowdown effect works for short event chains, long ones, like the ones I can create with my free scrolling mouse still stop dead with no ease out
			state->scroll_anim.dt = 1.f / (f32)win32_get_refresh_rate_hz(state->wnd);//duration of each frame in seconds
			state->scroll_anim.total_frames = (i32)maximum(anim_duration / (state->scroll_anim.dt * 1000.f), 1.f);
			state->scroll_anim.current_frame = 1;
			SetTimer(state->wnd, 1111, (u32)(state->scroll_anim.dt * 1000), scroll_animation);
		};

		static void (*scroll_anim)(HWND, UINT, UINT_PTR, DWORD) =
			[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
			ProcState* state = get_state(hwnd);
			if (state) {

				RECT r; GetWindowRect(state->wnd, &r); MapWindowPoints(0, state->parent, (POINT*)&r, 2);

				int original_wnd_y = r.top - (int)state->scroll;

				f32 dy = (f32)state->scroll_tasks[0] / (f32)state->scroll_anim.total_frames;
				state->scroll += dy;

				MoveWindow(state->wnd, r.left, (int)(original_wnd_y+state->scroll), RECTW(r), RECTH(r), TRUE);

				if (state->scroll_anim.current_frame++ <= state->scroll_anim.total_frames) {
					SetTimer(state->wnd, anim_id, (u32)(state->scroll_anim.dt*1000), scroll_anim);
				}
				else {
					state->scroll_tasks.pop_front();

					if (state->scroll_tasks.empty()) {
						state->scroll_anim.active = false; 
						KillTimer(state->wnd, anim_id); 
					}
					else {
						setup_scroll_anim(state, scroll_anim);
					}
				}
			}
		};

		if (!state->scroll_anim.active) {
			setup_scroll_anim(state, scroll_anim);
		}
	}
#endif

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//static int __cnt; printf("%d:PAGE:%s\n", __cnt++, msgToString(msg));
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{ //1st msg received
			CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;

			ProcState* state = (ProcState*)calloc(1, sizeof(ProcState));
			Assert(state);
			set_state(hwnd, state);
			state->wnd = hwnd;
			state->parent = creation_nfo->hwndParent;
			state->scroll_tasks = decltype(state->scroll_tasks)();
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

			return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): remove once we know all the things this does
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
			BOOL redraw = LOWORD(lparam);
			return 0;//TODO(fran): sendmessage(parent), sendmessage(children) ? obviously the problem is idk who asked for it, there could be some unfortunate unexpected infinite loops (or undesired behaviour, what if the parent tells us and we tell it and change it's font), this is why it'd be better to have each page be its own wnclass, we'd need to implement some glue code in order to connect and communicate with all the pages
		} break;
		case WM_GETFONT:
		{
			return 0;//TODO(fran): sendmessage(parent) ?
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			state->scroll_tasks.~deque();
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
		case WM_GETTEXT:
		{
			return 0;//TODO(fran): what to do
		} break;
		case WM_GETTEXTLENGTH:
		{
			return 0;//TODO(fran): what to do
		} break;
		case WM_SETTEXT:
		{
			//TODO(fran): what to do
			return 1;//we "set" the text
		} break;
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			return SendMessage(state->parent, msg, wparam, lparam);
		} break;
		case WM_PAINT://NOTE: we could also do this on WM_ERASEBACKGROUND and return defwindowproc on WM_PAINT
		{
			PAINTSTRUCT ps;
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			HDC dc = BeginPaint(state->wnd, &ps);
			bool window_enabled = IsWindowEnabled(state->wnd);

			//Bk
			HBRUSH bk_br = window_enabled ? state->theme.brushes.bk.normal : state->theme.brushes.bk.disabled;
			FillRect(dc, &rc, bk_br);

			//Border
			int border_thickness = (int)state->theme.dimensions.border_thickness;
			HBRUSH border_br = window_enabled ? state->theme.brushes.border.normal : state->theme.brushes.border.disabled;
			FillRectBorder(dc, rc, border_thickness, border_br, BORDERALL);

			EndPaint(hwnd, &ps);
			return 0;
		} break;
		case WM_COMMAND:
		{
			HWND child = (HWND)lparam;
			if (child) return SendMessage(state->parent, msg, wparam, lparam);//Any msg from our childs goes up to our parent
			else Assert(0); //No msg from menus nor accelerators should come here
		} break;
		case WM_MOUSEWHEEL:
		{
			if (state->does_scrolling) {
				short zDelta = (short)(((float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA) /** 3.f*/);
				int dy = avg_str_dim(global::fonts.General, 1).cy;
				//printf("zDelta %d ; height %d\n", zDelta, dy);
				
				//TODO(fran): SystemParametersInfo(SPI_GETWHEELSCROLLLINES) to scroll based on windows mouse scroll sensitivity the user set?

				int step = zDelta * 3 * dy;
#if 0 //possibly better solution
				UINT flags = MAKELONG(SW_SCROLLCHILDREN | SW_SMOOTHSCROLL, 200);
				ScrollWindowEx(state->wnd, 0, step, nullptr, nullptr, nullptr, nullptr, flags);
#elif 0 //handmade solution (no WM_PRINT and friends)
				smooth_scroll(state, step);
#else
				smooth_scroll(state, step);
#endif
				/*
				if (zDelta >= 0)
					for (int i = 0; i < zDelta; i++)
						SendMessage(state->wnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0); //TODO(fran): use ScrollWindowEx ?
				else
					for (int i = 0; i > zDelta; i--)
						SendMessage(state->wnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
				*/
				return 0;
			}
			else return DefWindowProc(hwnd, msg, wparam, lparam);//propagates msg to the parent
		} break;
		case WM_PARENTNOTIFY:
		{
			return 0;//We dont care to send this msgs from our childs up the chain, at least for now
		} break;

		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		{
			return SendMessage(state->parent, msg, wparam, lparam);//let the parent handle it
		} break;

		case WM_IME_SETCONTEXT: //When we get keyboard focus for the first time this gets sent
		{
			return 0; //We dont want IME
		} break;
		case WM_SETFOCUS: //Triggered, for example, when the user clicks and generates a WM_XBUTTONDOWN
		{
			return 0;//TODO(fran): not sure what to do with this, set focus back to whoever had it, set focus to our parent, do nothing?
		}
		case WM_KILLFOCUS:
		{
			return 0;
		} break;

		case WM_DELETEITEM:
		{
			return 1;//we "processed" this msg
			//TODO(fran): this shouldnt be getting here, I think this is because of the windows default comboboxes I use in some places that send this msg up to their parent when we change langs and the cb elements are cleared
		} break;
		case WM_ASK_FOR_RESIZE:
		{
			AskForResize(state->parent); //TODO(fran): the page could get the resize info on startup and manage this without bothering the parent
			return 0;
		} break;

		default:
#ifdef _DEBUG
		Assert(0);
#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
		}
		return 0;//TODO(fran): or sendmessage(parent)
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