#pragma once
#include "windows_sdk.h"
#include "がいじんの_Platform.h"
#include "がいじんの_Helpers.h"
#include "unCap_Renderer.h"
#include <vector>
#include <windowsx.h>

//NOTE: no ASCII support, always utf16

//TODO(fran): we really need to Assert(state) on get_state, we cant cause it will fail the first time, but we can solve it via taking WM_NCCREATE out of the switch, the question now is which one is more performant
//TODO(fran): respond on mouse move, we could put an indicator with a tooltip filled by the user
//TODO(fran): add graph generation animation

namespace graph {

	constexpr cstr wndclass[] = L"がいじんの_wndclass_graph";

#define ANIM_GRAPH_left_to_right 1

	struct ProcState {
		HWND wnd;
		HWND parent;
		struct brushes {
			//TODO(fran): we probably want multiple different drawing methods that will need different params
			HBRUSH line, bk_under_line, bk, border;//NOTE: for now we use the border color for the caret
			//HBRUSH txt_dis, bk_dis, border_dis; //disabled
		}brushes;

		LOGFONT font;
		HFONT _font;//simply cause windows msgs ask for it

		struct {
			using type = f64;
			type top;
			type bottom;
			std::vector<type> list;//we simplify to a common array type that can mostly hold anything (of the built in types)
			struct {
				size_t idx;
				size_t length;//length=0 means no points, length=1 means one point aka the point at idx, length=2 means two points aka the point at idx and the next one
				//NOTE: you can only go forward in the array, at least for now
			}range_to_show;
		}points;

		struct {
			struct {
				f32 scrollpercentage; //[0,1]
				u32 frames_cnt;//total frames
				u32 frame_idx;//current frame
			} left_to_right;

		}anim;

		HBITMAP backbuffer;//NOTE: at least for now the backbuffer always has the same dimensions as the client rect
		SIZE backbuffer_dim;
	};

	void resize_backbuffer(ProcState* state, SIZE sz) {
		RECT rc; GetClientRect(state->wnd, &rc);
		state->backbuffer_dim = { max(sz.cx, RECTW(rc)), max(sz.cy, RECTH(rc)) };
		HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
		if (state->backbuffer) DeleteBitmap(state->backbuffer);
		state->backbuffer = CreateCompatibleBitmap(dc, state->backbuffer_dim.cx, state->backbuffer_dim.cy);
	}

	void render_backbuffer(ProcState* state) {
		if (state->backbuffer) {
			HDC __front_dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,__front_dc); };
			HDC backbuffer_dc = CreateCompatibleDC(__front_dc); defer{ DeleteDC(backbuffer_dc); }; //TODO(fran): we may want to create a dc on WM_NCCREATE and avoid having to recreate it every single time
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

			RECT backbuffer_rc{ 0,0,state->backbuffer_dim.cx,state->backbuffer_dim.cy };
#if 0
			bool window_enabled = IsWindowEnabled(state->wnd); //TODO(fran): we need to repaint the backbuf if the window is disabled and the dis_... are !=0 and != than the non dis_... brushes
#endif

			//Paint the background
#if 0
			HBRUSH bk_br = window_enabled ? state->brushes.bk : state->brushes.bk_dis;
#else
			HBRUSH bk_br = state->brushes.bk;
#endif
			FillRect(backbuffer_dc, &backbuffer_rc, bk_br);

			//NOTE: do _not_ paint the border here, since it should fit the real size of the wnd

			//Render the points
			if (state->points.range_to_show.length && state->points.range_to_show.idx < state->points.list.size()) {
				int w = state->backbuffer_dim.cx, h = state->backbuffer_dim.cy;
				//we got a valid starting index and at least one point
				size_t pt_count = min(distance(state->points.range_to_show.idx, state->points.list.size()), state->points.range_to_show.length);
				POINT* points = (decltype(points))malloc(sizeof(*points) * (pt_count + 2)); defer{ free(points); };

				f32 x_pad = safe_ratio0((f32)w, (f32)(pt_count - 1));
				f32 curr_x = 0;
				for (size_t i = 0; i < pt_count; i++) {
					points[i].x = (int)round(curr_x);
					curr_x += x_pad;
					auto y_percent = distance(state->points.list[state->points.range_to_show.idx + i], state->points.bottom) / distance(state->points.top, state->points.bottom); //TODO(fran): this probably wraps incorrectly for values that go below bottom

					//NOTE: now we have a problem, y axis goes down
					//HACK:
					y_percent = 1 - y_percent;
					//one solution would be to render onto a separate buffer that has y going up, and then simply render to the screen with bitblt

					points[i].y = (i32)(y_percent * h);
				}

				//Draw below the line:
				//Complete the polygon
				{
					points[pt_count] = { points[pt_count - 1].x,h };
					points[pt_count + 1] = { points[0].x,h };
					HPEN pen = CreatePen(PS_SOLID, 0, ColorFromBrush(state->brushes.bk_under_line));
					HPEN oldpen = SelectPen(backbuffer_dc, pen); defer{ SelectPen(backbuffer_dc,oldpen); };
					HBRUSH oldbr = SelectBrush(backbuffer_dc, state->brushes.bk_under_line); defer{ SelectBrush(backbuffer_dc,oldbr); };
					Polygon(backbuffer_dc, points, (int)(pt_count + 2));//draw
				}

				constexpr int line_thickness = 1;

				HPEN pen = CreatePen(PS_SOLID, line_thickness, ColorFromBrush(state->brushes.line));
				HPEN oldpen = SelectPen(backbuffer_dc, pen); defer{ SelectPen(backbuffer_dc,oldpen); };

				MoveToEx(backbuffer_dc, points[0].x, points[0].y, nullptr);
				if (pt_count == 1) {
					//We need to invent an extra point at the end otherwise we dont have a second point to connect the line
					POINT end{ w,points[0].y };
					PolylineTo(backbuffer_dc, &end, (DWORD)pt_count);
				}
				else {
					PolylineTo(backbuffer_dc, &points[1], (DWORD)(pt_count - 1));
					//NOTE: there's also Polyline which doesnt depend on MoveTo but doesnt update the current position, that pos could be useful for "append drawing"
				}
				//TODO(fran): add style to allow for PolyBezierTo and handle WM_STYLECHANGE to update on realtime too
			}
		}
	}

	void full_backbuffer_redo(ProcState* state) {
#if 0
		SIZE sz = calc_element_placements(state);
#else
		RECT r; GetClientRect(state->wnd, &r);
		SIZE sz{RECTW(r),RECTH(r)};
#endif
		resize_backbuffer(state, sz);
		render_backbuffer(state);
	}



	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	template<typename T>
	void set_points(HWND wnd, T* values, size_t count) {
		ProcState* state = get_state(wnd);
		if (state) {
			static_assert(std::is_convertible<T, decltype(state->points)::type>::value, "The type of *values cannot be trivially converted to f64");
			state->points.list.clear();//delete old points
			for (int i = 0; i < count; i++) state->points.list.push_back((decltype(state->points.list)::value_type)values[i]);

			full_backbuffer_redo(state);

			InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): animation? or does it simply work, may it does
		}
	}

	template<typename T>
	void add_points(HWND wnd, T* values, size_t count) {
		ProcState* state = get_state(wnd);
		if (state) {
			static_assert(std::is_convertible<T, decltype(state->points)::type>::value, "The type of *values cannot be trivially converted to f64");
			for (int i = 0; i < count; i++) state->points.list.push_back((decltype(state->points.list)::value_type)values[i]);

			full_backbuffer_redo(state);
			
			InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): here we'd want to do the animation from the end of the current position instead of from the start, I guess

		}
	}

#include <limits> //std::numeric_limits
#undef max //NOTE: in order to be able to use std::numeric_limits<>::max()
	struct graph_dimensions {
		
		decltype(ProcState::points)::type top_point = std::numeric_limits<decltype(ProcState::points)::type>::max();
		decltype(ProcState::points)::type bottom_point = std::numeric_limits<decltype(ProcState::points)::type>::max();
		struct {
			size_t idx;
			size_t length;
		}viewable_points_range{ std::numeric_limits<size_t>::max() ,std::numeric_limits<size_t>::max() };

	template<typename T>
		graph_dimensions& set_top_point(T x) { 
			static_assert(std::is_convertible<T, decltype(ProcState::points)::type>::value, "The type cannot be trivially converted to f64");
			top_point = (decltype(top_point))x; return *this;
		}
	template<typename T>
		graph_dimensions& set_bottom_point(T x) { 
			static_assert(std::is_convertible<T, decltype(ProcState::points)::type>::value, "The type cannot be trivially converted to f64");
			bottom_point = (decltype(bottom_point))x; return *this;
		}
		//length=0 -> no points, length=1 -> 1 point (the point at idx), length=2 -> 2 points (point at idx and the next one)
		graph_dimensions& set_viewable_points_range(size_t idx, size_t length) { viewable_points_range.idx = idx; viewable_points_range.length = length; return *this; }
	};

	void set_dimensions(HWND wnd, graph_dimensions dims) {
		ProcState* state = get_state(wnd);
		if (state) {
			bool redo_backbuffer = false;
			if (dims.top_point != std::numeric_limits<decltype(dims.top_point)>::max() && dims.top_point != state->points.top) {
				state->points.top = dims.top_point;
				redo_backbuffer = true;
			}
			if (dims.bottom_point != std::numeric_limits<decltype(dims.bottom_point)>::max() && dims.bottom_point != state->points.bottom) {
				state->points.bottom = dims.bottom_point;
				state->points.bottom = (decltype(state->points)::type)dims.bottom_point;
				redo_backbuffer = true;
			}
			if (dims.viewable_points_range.idx != std::numeric_limits<size_t>::max() && dims.viewable_points_range.idx != state->points.range_to_show.idx){
				state->points.range_to_show.idx = dims.viewable_points_range.idx;
				redo_backbuffer = true;
			}
			if (dims.viewable_points_range.length != std::numeric_limits<size_t>::max() && dims.viewable_points_range.length != state->points.range_to_show.length) {
				state->points.range_to_show.length = dims.viewable_points_range.length;
				redo_backbuffer = true;
			}

			if (redo_backbuffer) {
				full_backbuffer_redo(state);
				InvalidateRect(state->wnd, NULL, TRUE);//TODO(fran): animation
			}
		}
	}
//TODO(fran): I should define NOMINMAX and crate my own min max _functions_
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

	//template<typename T>
	////NOTE: in an xy graph this would be y_max
	//void set_top_point(HWND wnd, T value) {
	//	ProcState* state = get_state(wnd);
	//	if (state) {
	//		static_assert(std::is_convertible<T, decltype(state->points)::type>::value, "The type of value cannot be trivially converted to f64");
	//		state->points.top = (decltype(state->points)::type)value;
	//		InvalidateRect(state->wnd, NULL, TRUE);
	//	}
	//}

	//template<typename T>
	////NOTE: in an xy graph this would be y_min
	//void set_bottom_point(HWND wnd, T value) {
	//	ProcState* state = get_state(wnd);
	//	if (state) {
	//		static_assert(std::is_convertible<T, decltype(state->points)::type>::value, "The type of value cannot be trivially converted to f64");
	//		state->points.bottom = (decltype(state->points)::type)value;
	//		InvalidateRect(state->wnd, NULL, TRUE);
	//	}
	//}

	//length=0 means no points, length=1 means one point aka the point at idx, length=2 means two points aka the point at idx and the next one
	//void set_viewable_points_range(HWND wnd, size_t idx, size_t length) {
	//	ProcState* state = get_state(wnd);
	//	if (state) {
	//		state->points.range_to_show.idx = idx;
	//		state->points.range_to_show.length = length;
	//		InvalidateRect(state->wnd, NULL, TRUE);
	//	}
	//}

	//NOTE: we simply use the brushes, we dont handle or delete them
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH line, HBRUSH bk_under_line, HBRUSH bk, HBRUSH border) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (line)state->brushes.line = line;
			if (bk_under_line)state->brushes.bk_under_line = bk_under_line;
			if (bk)state->brushes.bk = bk;
			if (border)state->brushes.border = border;
			if (repaint) {
				render_backbuffer(state); //re-render with new colors
				InvalidateRect(state->wnd, NULL, TRUE); //TODO(fran): animate
			}
		}
	}

	void play_anim(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
		ProcState* state = get_state(hwnd);
		bool next_frame = false;

		switch (anim_id) {
		case ANIM_GRAPH_left_to_right:
		{
			auto& anim = state->anim.left_to_right;
			anim.frame_idx++;
			if (anim.frame_idx < anim.frames_cnt) next_frame = true;

			f32 dt = (f32)anim.frame_idx / (f32)anim.frames_cnt;

			anim.scrollpercentage = dt;

			InvalidateRect(state->wnd, 0, TRUE);

		} break;
		}

		if (next_frame) {
			SetTimer(state->wnd, anim_id, max((u32)1, (u32)((1.f / (f32)win32_get_refresh_rate_hz(state->wnd)) * 1000)), play_anim);
		}
		else {
			KillTimer(state->wnd, anim_id);
		}
	}

	void start_anim(ProcState* state, u64 anim_id) {
		switch (anim_id) {
		case ANIM_GRAPH_left_to_right:
		{
			u32 refresh = win32_get_refresh_rate_hz(state->wnd);
			state->anim.left_to_right.frames_cnt = refresh / 2;
			state->anim.left_to_right.frame_idx = 0;
		} break;
		}
		SetTimer(state->wnd, anim_id, USER_TIMER_MINIMUM, play_anim);
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
			st->points.list = decltype(st->points.list)();//INFO: c++ objects often need manual initialization, 0 aint good enough for them
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
			full_backbuffer_redo(state);
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
		case WM_SHOWWINDOW://6th. On startup you receive this cause of WS_VISIBLE flag
		{
			//Sent when window is about to be hidden or shown, doesnt let it clear if we are in charge of that or it's going to happen no matter what we do
			BOOL show = (BOOL)wparam;
			if (show) {
				//Start animation
				start_anim(state, ANIM_GRAPH_left_to_right);
			}
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
			int getobjres = GetObject(font, sizeof(state->font), &state->font); Assert(getobjres == sizeof(state->font));
			state->_font = font;
			if (redraw) RedrawWindow(state->wnd, NULL, NULL, RDW_INVALIDATE);
			return 0;
		} break;
		case WM_GETFONT:
		{
			return (LRESULT)state->_font;
		} break;
		case WM_DESTROY:
		{
			state->points.list.~vector();
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			free(state);
			set_state(hwnd, 0);//TODO(fran): I dont know if I should do this always
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

			//TODO(fran): identify over which point the user is hovering at and notify our parent

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
		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_GETTEXTLENGTH:
		{
			return 0;
		} break;
		case WM_SETTEXT:
		{
			return TRUE;//we "set" the text
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			//ps.rcPaint
			HDC dc = BeginPaint(state->wnd, &ps);

			//Background
			FillRect(dc, &rc, state->brushes.bk);

			//Backbuffer copy
			Assert(state->backbuffer_dim.cx == w && state->backbuffer_dim.cy == h);
			Assert(state->backbuffer);
			int backbuf_x = 0;
			int backbuf_y = 0;
			int backbuf_w = (int)((f32)state->backbuffer_dim.cx * state->anim.left_to_right.scrollpercentage);
			int backbuf_h = state->backbuffer_dim.cy;

			HDC backbuffer_dc = CreateCompatibleDC(dc); defer{ DeleteDC(backbuffer_dc); };
			//TODO(fran): we definitely want to create a dc on WM_NCCREATE and avoid having to recreate it every single time
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

			BitBlt(dc, 0, 0, backbuf_w, backbuf_h, backbuffer_dc, backbuf_x, backbuf_y, SRCCOPY);

			//Border
			//TODO(fran): we should allow the user to specify border thickness
			bool window_enabled = IsWindowEnabled(state->wnd); //TODO(fran): we need to repaint the backbuf if the window is disabled and the dis_... are !=0 and != than the non dis_... brushes
			int border_thickness = 1;
#if 0
			HBRUSH border_br = window_enabled ? state->brushes.border : state->brushes.border_dis;
#else
			HBRUSH border_br = state->brushes.border;
#endif
			FillRectBorder(dc, rc, border_thickness, border_br, BORDERALL);

			EndPaint(hwnd, &ps);
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