#pragma once
#include "windows_sdk.h"
#include <windowsx.h>
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "unCap_Math.h"
#include "win32_べんきょう_db.h"

#include <vector>

namespace eyecandy {
	//TODO(fran): we should probably add a user_extra and function ::set_user_extra(), a single user setteable value, in our case for example I'd put the HWND so I can get_state() and check for things like a font or color
	//typedef void(*gridview_func_renderelement)(HDC dc, rect_i32 r, void* element);//NOTE: RECT is stupid better rect_i32

	struct Theme {
		struct {
			brush_group foreground, bk, border;
		} brushes;
		struct {
			u32 border_thickness = U32MAX;
		}dimensions;
		HFONT font = 0;
	};

	struct ProcState {
		HWND wnd;
		HWND parent;

		Theme theme;
		//gridview_func_renderelement render_element;
		sqlite3* db;//TODO(fran): we shouldnt handle how we get the elements, that should be a set_function

		HBITMAP backbuffer;
		SIZE backbuffer_dim;

		struct element {
			utf16_str text;
			v2 pos;//left top, in client coordinates
			v2 direction;
			f32 speed;//magnitude of the direction
			//HFONT font;
			f32 _rotation;//[0.0,2PI]
		};

		std::vector<element> elements;
		size_t elements_max;

		f32 t;//count of elapsed time (seconds) since last word added
		f32 t_max;
	};

	constexpr cstr wndclass[] = L"win32_wndclass_eyecandy";


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

	void render_backbuffer(ProcState* state) {
		if (state->backbuffer) {
			HDC __front_dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,__front_dc); };
			HDC backbuffer_dc = CreateCompatibleDC(__front_dc); defer{ DeleteDC(backbuffer_dc); }; //TODO(fran): we may want to create a dc on WM_NCCREATE and avoid having to recreate it every single time
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

			RECT backbuffer_rc{ 0,0,state->backbuffer_dim.cx,state->backbuffer_dim.cy };
			bool window_enabled = IsWindowEnabled(state->wnd); //TODO(fran): we need to repaint the backbuf if the window is disabled and the dis_... are !=0 and != than the non dis_... brushes

			//Paint the background
			HBRUSH bk_br = window_enabled ? state->theme.brushes.bk.normal : state->theme.brushes.bk.disabled;
			HBRUSH txt_br = window_enabled ? state->theme.brushes.foreground.normal : state->theme.brushes.foreground.disabled;
			FillRect(backbuffer_dc, &backbuffer_rc, bk_br);

			//NOTE: do _not_ paint the border here, since it should fit the real size of the wnd

			//Render the elements
			//if (state->render_element) {
			//	Assert(state->element_placements.size() == state->elements.size());
			//	for (size_t i = 0; i < state->element_placements.size(); i++)
			//		//TODO(fran): we could apply a transform so the user can render as if from {0,0} and we simply send a SIZE obj and they dont have to bother with offsetting by left and top
			//		state->render_element(backbuffer_dc, state->element_placements[i], state->elements[i]);
			//}
			{
				//auto oldbkmode = SetBkMode(backbuffer_dc, TRANSPARENT); defer{ SetBkMode(backbuffer_dc,oldbkmode); };
				//auto oldcol = SetTextColor(backbuffer_dc, ColorFromBrush(txt_br)); defer{ SetTextColor(backbuffer_dc,oldcol); };
					for (auto& e : state->elements) {
						//auto oldfont = SelectFont(backbuffer_dc, e.font); defer{ SelectFont(backbuffer_dc, oldfont); };
						//TextOutW(backbuffer_dc, (i32)e.pos.x, (i32)e.pos.y, e.text.str, (i32)e.text.sz_char() - 1);
						urender::draw_text(backbuffer_dc, e.pos, e.text, state->theme.font, txt_br, urender::txt_align::left, e._rotation);
						//TODO(fran): bold font
					}
			}
		}
	}

	void full_backbuffer_redo(ProcState* state) {
		RECT r; GetClientRect(state->wnd, &r);
		resize_backbuffer(state, {RECTW(r),RECTH(r)});
		render_backbuffer(state);
	}

	//TODO(fran): theme changing. extra: specific font re-creation for each element if the font changes
	bool _copy_theme(Theme* dst, const Theme* src) {
		bool repaint = false;
		repaint |= copy_brush_group(&dst->brushes.foreground, &src->brushes.foreground);
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);
		repaint |= copy_brush_group(&dst->brushes.border, &src->brushes.border);

		if (src->dimensions.border_thickness != U32MAX) {
			dst->dimensions.border_thickness = src->dimensions.border_thickness; repaint = true;
		}

		if (src->font) { dst->font = src->font; repaint = true; }
		return repaint;
	}

	//Set only what you need, what's not set wont be used
	//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
	void set_theme(HWND wnd, const Theme* t) {
		ProcState* state = get_state(wnd);
		if (state && t) {
			bool repaint = _copy_theme(&state->theme, t);

			//if (repaint) {
			//	render_backbuffer(state); //TODO(fran): pointless if we redraw on every frame
			//	InvalidateRect(state->wnd, NULL, TRUE); //TODO(fran): pointless if we redraw on every frame
			//}
		}
	}

	void set_db(HWND wnd, sqlite3* db) {//TODO(fran): better name
		ProcState* state = get_state(wnd);
		if (state) {
			state->db = db;
		}
	}

	//Adds new elements to the list without removing already present ones
	void add_element(ProcState* state) {
		if (state->db) {
			ProcState::element elem;
			learnt_word_elem word_elems[]{ learnt_word_elem::hiragana, learnt_word_elem::kanji };
			elem.text = べんきょう::get_random_word_element(state->db, word_elems[random_between(0u, (u32)ARRAYSIZE(word_elems) - 1)] );
			RECT r; GetClientRect(state->wnd, &r);
			//TODO(fran): place objects outside the rectangle and find valid direction based on that
			f32 pi = 3.1415926535f;
			f32 angle = random_between(0.f, 2 * pi);
#if 0
			elem.pos = { (f32)random_between(r.left,RECTW(r)), (f32)random_between(r.top,RECTH(r)) };
			elem._rotation = angle;
#else
			elem.pos = { (f32)r.left, (f32)random_between(r.top,RECTH(r)) };
			elem._rotation = angle = 0;
#endif
			elem.direction = { cosf(angle), sinf(angle) };
			elem.speed = (f32)random_between(200, 250);//TODO(fran): sensible numbers, for any resolution
			//LOGFONT lf; GetObject(state->theme.font, sizeof(lf), &lf);
			//lf.lfEscapement = (i32)((angle * 180 / pi) * 10);//TODO(fran): set lfOrientation if we use GM_ADVANCED graphics mode
			//lf.lfOrientation = lf.lfEscapement;
			//elem.font = CreateFontIndirect(&lf);
			state->elements.push_back(elem);
			//for (int i = 0; i < count; i++) state->elements.push_back(values + i);
			//full_backbuffer_redo(state); //render new elements
			//InvalidateRect(state->wnd, NULL, TRUE);//ask for WW_PAINT
		}
	}

	void clear_element(const ProcState::element& e) {
		//DeleteFont(e.font);
		free_any_str(e.text.str);
	}

	//void set_render_function(HWND wnd, gridview_func_renderelement render_func) {
	//	ProcState* state = get_state(wnd);
	//	if (state) {
	//		state->render_element = render_func;
	//		render_backbuffer(state); //re-render with new element rendering function
	//	}
	//}

	void update(ProcState* state, f32 dt) {
		state->t += dt;
		if (state->t >= state->t_max && state->elements.size() < state->elements_max) {
			add_element(state);
			state->t = 0;
		}

		for (size_t i = 0; i < state->elements.size(); i++) {
			auto& e = state->elements[i];
			e.pos += e.direction * e.speed * dt;
		}
	}

	void remove_outsiders(ProcState* state) {
		RECT r; GetClientRect(state->wnd, &r); InflateRect(&r, 3, 3);
		for (size_t i = 0; i < state->elements.size(); i++) {
			auto& e = state->elements[i];
			SIZE e_dim;
			{
				HDC dc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,dc); };
				//auto oldfont = SelectFont(dc, e.font); defer{ SelectFont(dc, oldfont); };
				auto oldfont = SelectFont(dc, state->theme.font); defer{ SelectFont(dc, oldfont); };
				GetTextExtentPoint32W(dc, e.text.str, (i32)(e.text.sz_char() - 1), &e_dim);
			}
			rect_i32 e_r{(i32)e.pos.x,(i32)e.pos.y,e_dim.cx,e_dim.cy}; //TODO(fran): take orientation into account, this is not correct, we need _not_ axis aligned rcs_overlap
			if(!rcs_overlap(r, toRECT(e_r))){
				clear_element(e);
				state->elements.erase(state->elements.begin() + i);
				i--;
			}
		}
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
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
			state->elements = decltype(state->elements)();
			state->elements_max = 50;
			state->t_max = .1f;
			state->t = state->t_max;
			
			static void (*anim_loop)(HWND, UINT, UINT_PTR, DWORD) =
				[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
				ProcState* state = get_state(hwnd);
				if (state) {
					f32 dt = 1.f / (f32)win32_get_refresh_rate_hz(state->wnd);//TODO(fran): calculate actual dt
					SetTimer(hwnd, anim_id, (u32)(dt * 1000), anim_loop);
					//NOTE: reseting the timer at the start of the loop helps a lot with keeping a consistent framerate, obviously not as good as having a proper animation loop, but better than doing settimer after everything else. Only thing im not sure is what happens, if anything, if we take soo long the next timer arrives while we're still finishing this one
					update(state,dt);
					render_backbuffer(state);
					ask_for_repaint(state);
					remove_outsiders(state);

				}
				else KillTimer(hwnd, anim_id);
			};

			SetTimer(state->wnd, 55, 0, anim_loop);

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
			Theme t; t.font = font;
			set_theme(state->wnd, &t);
			return 0;
		} break;
		case WM_GETFONT:
		{
			return (LRESULT)state->theme.font;
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
			free(state);
			set_state(hwnd, 0);
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
			int backbuf_y = 0;// (int)(state->scroll_y* (f32)state->backbuffer_dim.cy);
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
			int border_thickness = state->theme.dimensions.border_thickness;
			HBRUSH border_br = window_enabled ? state->theme.brushes.border.normal : state->theme.brushes.border.disabled;
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
		//case WM_MOUSEACTIVATE://When the user clicks on us this is 1st msg received
		//{
		//	HWND parent = (HWND)wparam;
		//	WORD hittest = LOWORD(lparam);
		//	WORD mouse_msg = HIWORD(lparam);
		//	return MA_ACTIVATE; //Activate our window and post the mouse msg
		//} break;
		case WM_LBUTTONDOWN:
		{
			//wparam = test for virtual keys pressed
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area
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
			return DefWindowProc(hwnd, msg, wparam, lparam);
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