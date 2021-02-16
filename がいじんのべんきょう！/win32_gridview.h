#pragma once
#include "windows_sdk.h"
#include <windowsx.h>
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "unCap_Math.h"
#include <vector>

//------------------"API"------------------:
//gridview::set_brushes() to set the background and border brushes
//gridview::set_dimensions() to set all values related to element placement and size
//gridview::get_dimensions()
//gridview::set_elements() to remove existing elements and add new ones
//gridview::add_elements() to add new elements without removing existing ones
//gridview::set_render_function() to set the function used for rendering an element
//gridview::get_row_cnt() get the current number of rows necessary to display all the elements
//gridview::get_dim_for_...() calculates the required dimensions of the wnd based on different parameters
//gridview::get_elem_cnt() current number of elements

//--------------Extra Messages-------------:
//Sends WM_COMMAND to the parent when the user clicks an element wparam=void* (elem data); lparam=HWND (ctrl wnd)

//NOTE: elements' position and sizing is calculated on resize, no calculation of such sort must ever happen on wm_paint
//NOTE: we will implement scrolling, though I'd really like for a scrollbar to not exist, simply scrolling via scrollbar
//NOTE: we will not at this stage support moving items, the grid is fixed, one advantage of this is that we could pre render the entire view and when the time comes for scrolling we simply offset the y position of that pre-rendered image

//TODO(fran): we could add styles for left alignment and right alignment, apart from the defualt centering

//TODO(fran): add debug_render_elementoutlines so the user can make sure they're drawing inside the required square, the function will draw a red square around the rectangle of each element

namespace gridview {
	//TODO(fran): we should probably add a user_extra and function ::set_user_extra(), a single user setteable value, in our case for example I'd put the HWND so I can get_state() and check for things like a font or color
	typedef void(*gridview_func_renderelement)(HDC dc,rect_i32 r,void* element);//NOTE: RECT is stupid better rect_i32

	struct ProcState {
		HWND wnd;
		HWND parent;
		struct brushes {//NOTE: the gridview itself renders nothing but the background, the individual elements are always drawn by... a function? it'd be nice to simply tell the user give me some function to do drawing, much simpler and less obtrusive than having the drawing code in the middle of other msgs
			HBRUSH bk, border;
			HBRUSH bk_dis, border_dis; //disabled
		}brushes;

		gridview_func_renderelement render_element;

		HBITMAP backbuffer;
		SIZE backbuffer_dim;

		SIZE border_pad;
		SIZE inbetween_pad;//in between two elements

		SIZE element_dim;//NOTE: all elements are of the same size

		size_t row_cnt;//value requesteable by the user

		std::vector<void*> elements;
		std::vector<rect_i32> element_placements;

		f32 scroll_y;//[0,1]
	};
	//NOTE: User seteable: border_pad.cy , element_dim , inbetween_pad

	constexpr cstr wndclass[] = L"unCap_wndclass_gridview";


	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	//NOTE: "sz" should be the already precalculated size to cover all the render elements, and maxing out in its w component by the size of the window rectangle (since we allow vertical but not horizontal scrolling)
	void resize_backbuffer(ProcState* state, SIZE sz) {
		RECT rc; GetClientRect(state->wnd, &rc);
		state->backbuffer_dim = { max(sz.cx, RECTW(rc)), max(sz.cy, RECTH(rc)) };
		HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
		if (state->backbuffer) DeleteBitmap(state->backbuffer);
		state->backbuffer = CreateCompatibleBitmap(dc, state->backbuffer_dim.cx, state->backbuffer_dim.cy);
	}

	SIZE calc_element_placements(ProcState* state) {
		SIZE res{0,0};
		RECT rc; GetClientRect(state->wnd, &rc);
		int w = max(state->element_dim.cx, RECTW(rc));//NOTE: make w at least be able to hold one element
		//NOTE: we dont care bout height, we can go as low as we want and simply allow scrolling
		int elems_per_row = min((int)state->elements.size(), safe_ratio1(w,state->element_dim.cx));//NOTE: and then you floor() the result, but since this are integers and always positive the division already performs flooring
		//NOTE: I use safe_ratio1 instead of the correct safe_ratio0 just to avoid having other divisions fail as well, since the loop inst gonna run this values are meaningless anyway

		//Correction factor for elems_per_row
		{
			for (int i=0;i<10 /*safeguard*/;i++) {
				int pad_cnt = elems_per_row - 1; //TODO(fran): check what happens when there's only one elem per row
				if ((elems_per_row * state->element_dim.cx + pad_cnt * state->inbetween_pad.cx) <= w) break;
				else elems_per_row--;
			}
		}

		//NOTE: elements are centered with relation to the wnd
		state->border_pad.cx = (w - (elems_per_row * state->element_dim.cx + (elems_per_row - 1) * state->inbetween_pad.cx))/2;//border for left and right

		int row_cnt = (int)ceilf((f32)state->elements.size() / (f32)elems_per_row);

		state->row_cnt = row_cnt;

		//TODO(fran): perfect centering for the last row, which will most probably have less elements
		size_t elem_cnt = state->elements.size();
		state->element_placements.clear();
		rect_i32 elem_rc;
		elem_rc.xy = { 0,state->border_pad.cy };
		elem_rc.wh = { state->element_dim.cx, state->element_dim.cy };
		for (int y = 0; y < row_cnt; y++) {
			elem_rc.x = state->border_pad.cx;
			for (int x = 0; x < elems_per_row && elem_cnt; x++) {
				state->element_placements.push_back(elem_rc);
				elem_rc.x+= state->element_dim.cx + state->inbetween_pad.cx;
				elem_cnt--;
			}
			elem_rc.y += state->element_dim.cy + state->inbetween_pad.cy;
		}
		res.cx = w;//we always cover the entire width
		//TODO(fran): not sure if I can replace this with elem_rc
		if (!state->element_placements.empty()) 
			res.cy = state->element_placements.back().bottom() /*last elem y*/ + state->border_pad.cy /* plus a little give so it doesnt feel so cramped*/;
		return res;
	}

	void render_backbuffer(ProcState* state) {
		if (state->backbuffer) {
			HDC __front_dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,__front_dc); };
			HDC backbuffer_dc = CreateCompatibleDC(__front_dc); defer{ DeleteDC(backbuffer_dc); }; //TODO(fran): we may want to create a dc on WM_NCCREATE and avoid having to recreate it every single time
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

			RECT backbuffer_rc{ 0,0,state->backbuffer_dim.cx,state->backbuffer_dim.cy };
			bool window_enabled = IsWindowEnabled(state->wnd); //TODO(fran): we need to repaint the backbuf if the window is disabled and the dis_... are !=0 and != than the non dis_... brushes

			//Paint the background
			HBRUSH bk_br = window_enabled ? state->brushes.bk : state->brushes.bk_dis;
			FillRect(backbuffer_dc, &backbuffer_rc, bk_br);

			//NOTE: do _not_ paint the border here, since it should fit the real size of the wnd

			//Render the elements
			if (state->render_element) {
				Assert(state->element_placements.size() == state->elements.size());
				for (size_t i = 0; i < state->element_placements.size(); i++)
					//TODO(fran): we could apply a transform so the user can render as if from {0,0} and we simply send a SIZE obj and they dont have to bother with offsetting by left and top
					state->render_element(backbuffer_dc, state->element_placements[i], state->elements[i]);
			}
		}
	}

	void full_backbuffer_redo(ProcState* state) {
		SIZE sz = calc_element_placements(state);
		resize_backbuffer(state, sz);
		render_backbuffer(state);
	}

	//NOTE: since c++ default values for functions is very limited we have to do this for multiple params
	struct element_dimensions {
		int border_pad_y = INT32_MAX;
		SIZE element_dim = { INT32_MAX,INT32_MAX };
		SIZE inbetween_pad = { INT32_MAX,INT32_MAX };
		//element_dimensions() : border_pad_y(INT32_MAX), element_dim{ INT32_MAX, INT32_MAX }, inbetween_pad{ INT32_MAX,INT32_MAX } {};
		element_dimensions& set_border_pad_y(int x) { border_pad_y = x; return *this; }
		element_dimensions& set_element_dim(SIZE x) { element_dim = x; return *this; }
		element_dimensions& set_inbetween_pad(SIZE x) { inbetween_pad = x; return *this; }
	};

	/// <param name="border_pad_y">offset from the top of the wnd from where to start placing elements</param>
	/// <param name="element_dim">element width and heigth, all elements have the same dimensions</param>
	/// <param name="inbetween_pad">padding between each element</param>
	void set_dimensions(HWND wnd, element_dimensions dims) {
		ProcState* state = get_state(wnd);
		if (state) {
			bool redo_backbuffer = false;
			if (dims.border_pad_y != INT32_MAX && dims.border_pad_y != state->border_pad.cy) {
				state->border_pad.cy = dims.border_pad_y;
				redo_backbuffer = true;
			}
			if (dims.element_dim != SIZE{ INT32_MAX, INT32_MAX } && dims.element_dim != state->element_dim) {
				state->element_dim = dims.element_dim;
				redo_backbuffer = true;
			}
			if (dims.inbetween_pad != SIZE{ INT32_MAX, INT32_MAX } && dims.inbetween_pad != state->inbetween_pad) {
				state->inbetween_pad = dims.inbetween_pad;
				redo_backbuffer = true;
			}
			if (redo_backbuffer) {
				full_backbuffer_redo(state);
			}
		}
	}

	size_t get_row_cnt(HWND wnd) {
		ProcState* state = get_state(wnd);
		size_t res;
		if (state) {
			res = state->row_cnt;
		}
		return res;
	}

	element_dimensions get_dimensions(HWND wnd) {
		ProcState* state = get_state(wnd);
		element_dimensions res{0};
		if (state) {
			res.border_pad_y = state->border_pad.cy;
			res.element_dim = state->element_dim;
			res.inbetween_pad = state->inbetween_pad;
		}
		return res;
	}

	SIZE get_dim_for_elemcnt_elemperrow(HWND wnd, size_t elem_cnt, size_t elems_per_row) {
		SIZE res{0};
		ProcState* state = get_state(wnd);
		if (state) {
			i32 rows = (i32)ceilf((f32)elem_cnt / (f32)elems_per_row);
			res.cx = (i32)elems_per_row * state->element_dim.cx + (i32)(elems_per_row - 1) * state->inbetween_pad.cx + 2 * max(3,state->border_pad.cx)/*HACK*/;
			res.cy = rows * state->element_dim.cy + (rows - 1) * state->inbetween_pad.cy + 2 * state->border_pad.cy;
		}
		return res;
	}

	SIZE get_dim_for_elemcnt_w(HWND wnd, size_t elem_cnt, size_t w) {
		SIZE res{ 0 };
		ProcState* state = get_state(wnd);
		if (state) {
			int elems_per_row = min((int)elem_cnt, safe_ratio1((i32)w, state->element_dim.cx));
			//Correction factor for elems_per_row
			{
				for (int i = 0; i < 10 /*safeguard*/; i++) {
					int pad_cnt = elems_per_row - 1;
					if ((elems_per_row * state->element_dim.cx + pad_cnt * state->inbetween_pad.cx) <= w) break;
					else elems_per_row--;
				}
			}
			i32 border_padx = ((i32)w - (elems_per_row * state->element_dim.cx + (elems_per_row - 1) * state->inbetween_pad.cx)) / 2;

			i32 row_cnt = (i32)ceilf((f32)elem_cnt / (f32)elems_per_row);

			res.cx = elems_per_row * state->element_dim.cx + (elems_per_row - 1) * state->inbetween_pad.cx + 2 * max(3, border_padx)/*HACK*/;
			res.cy = row_cnt * state->element_dim.cy + (row_cnt - 1) * state->inbetween_pad.cy + 2 * state->border_pad.cy;
		}
		return res;
	}

	size_t get_elem_cnt(HWND wnd) {
		size_t res = 0;
		ProcState* state = get_state(wnd);
		if (state) res = state->elements.size();
		return res;
	}

	//NOTE: the caller takes care of deleting the brushes, we dont do it
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk, HBRUSH border, HBRUSH bk_disabled, HBRUSH border_disabled) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk)state->brushes.bk = bk;
			if (border)state->brushes.border = border;
			if (bk_disabled)state->brushes.bk_dis = bk_disabled;
			if (border_disabled)state->brushes.border_dis = border_disabled;
			if (repaint) {
				render_backbuffer(state); //re-render with new colors
				InvalidateRect(state->wnd, NULL, TRUE); //ask for WW_PAINT
			}
		}
	}

	//TODO(fran): do add_elements as a batch operation, add_elements(void* elems, size_t cnt), that way we avoid pointless recalculation if we did it one by one

	//Removes any elements already present and sets the new ones
	void set_elements(HWND wnd, void** values, size_t count) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->elements.clear();//delete old points
			for (int i = 0; i < count; i++) state->elements.push_back(values[i]);
			full_backbuffer_redo(state); //render new elements
			InvalidateRect(state->wnd, NULL, TRUE);//ask for WW_PAINT
		}
	}

	//Adds new elements to the list without removing already present ones
	void add_elements(HWND wnd, void** values, size_t count) {
		ProcState* state = get_state(wnd);
		if (state) {
			for (int i = 0; i < count; i++) state->elements.push_back(values + i);
			full_backbuffer_redo(state); //render new elements
			InvalidateRect(state->wnd, NULL, TRUE);//ask for WW_PAINT
		}
	}

	void set_render_function(HWND wnd, gridview_func_renderelement render_func) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->render_element = render_func;
			render_backbuffer(state); //re-render with new element rendering function
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
			st->border_pad = { 3,3 };
			st->inbetween_pad = { 3,3 };
			st->elements = decltype(st->elements)();
			st->element_placements = decltype(st->element_placements)();
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
			//TODO(fran): this same operations will need to be done on any recalculation really, changes in borders, inbetween, elem cnt can all change the size of the backbuffer
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
			return 0;
		} break;
		case WM_GETFONT:
		{
			return 0;
		} break;
		case WM_DESTROY:
		{
			//TODO(fran): should I notify the parent if I have some elements? probably not
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY://Last msg. Sent _after_ WM_DESTROY
		{
			if (state->backbuffer) {
				DeleteBitmap(state->backbuffer);
				state->backbuffer = 0;
			}
			state->elements.~vector();
			state->element_placements.~vector();
			free(state);
			return 0;
		}break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc; GetClientRect(state->wnd, &rc);
			int w = RECTW(rc), h = RECTH(rc);
			HDC dc = BeginPaint(state->wnd, &ps);
			//TODO(fran): offset by scrollpercetage and render the pre-rendered img
			int backbuf_x = 0;
			int backbuf_y = (int)(state->scroll_y * (f32)state->backbuffer_dim.cy);
			Assert(state->backbuffer_dim.cx == w);
			int backbuf_w = w;
			int backbuf_h = h;
			
			HDC backbuffer_dc = CreateCompatibleDC(dc); defer{ DeleteDC(backbuffer_dc); }; 
			//TODO(fran): we definitely want to create a dc on WM_NCCREATE and avoid having to recreate it every single time
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

			BitBlt(dc, 0, 0, w, h, backbuffer_dc, backbuf_x, backbuf_y, SRCCOPY);

			//Paint the border
			//TODO(fran): we should allow the user to specify border thickness
			bool window_enabled = IsWindowEnabled(state->wnd); //TODO(fran): we need to repaint the backbuf if the window is disabled and the dis_... are !=0 and != than the non dis_... brushes
			int border_thickness = 1;
			HBRUSH border_br = window_enabled ? state->brushes.border : state->brushes.border_dis;
			FillRectBorder(dc, rc, border_thickness, border_br, BORDERALL);

			EndPaint(hwnd, &ps);
			return 0;
		} break;
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

			//TODO(fran): maybe we should change the mouse arrow to the hand when the user hovers on top of an element

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
			//TODO(fran): for now we wont have a responsible layout when the mouse goes over an element, we may want to add that
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
			//TODO(fran): here we'll use our precalculated data to find out which element the user pressed and notify the parent, or even better we could add a msg WM_NOTIFYTO and allow the user to chose whoever they want notified

			//TODO(fran): take into account scrolling
			for (int i = 0; i < state->element_placements.size(); i++) {
				if (test_pt_rc(mouse, state->element_placements[i])) {
					SendMessage(state->parent, WM_COMMAND, (WPARAM)state->elements[i], (LPARAM)state->wnd);//TODO(fran): PostMessage?
					//TODO(fran): there's no good win32 msg that is a notif that allows you to sent some extra info, you got WM_COMMAND, WM_NOTIFY and WM_PARENTNOTIFY each with its limitations, we may want to make our own msg that could work for all controls
				}
			}

			return 0;
		} break;
		case WM_LBUTTONUP:
		{
			//TODO(fran): do we want to react on button up or down?
			return 0;
		} break;
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
			return 1;//we "set" the text
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