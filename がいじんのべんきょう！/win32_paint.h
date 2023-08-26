#pragma once

#include "windows_sdk.h"
#include <windowsx.h>
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "unCap_Math.h"
#include <vector>
#include "win32_Renderer.h"


//------------------"API"------------------:
// paint::set_brushes()
// paint::set_dimensions()
// paint::get_canvas()
// paint::clear_canvas()
// paint::get_stroke_count()
// paint::set_placeholder()

//--------------Extra Messages-------------:
// Sends WM_COMMAND with lparam=HWND ; wparam=0 when the user has finished making a modification to the canvas

//------------Debug Information------------:
//#define TEST_DEBUG_OBJS
//#define TEST_BACKBUFFER_USED

//TODO(fran): multilevel undo and redo feature
//TODO(fran): if the user goes outside the canvas and then tries to go back inside (with the button still pressed) we've already canceled their ability to draw and have to lift the button and click again, this shouldnt happen
//TODO(fran): change mouse img to simbolize drawing, photoshop shows a circle the size of the dot it paints

namespace paint {

	struct debug_obj {
		char msg[16];//_must_ be null terminated
		COLORREF col;
		static constexpr f32 line_sz = 2;
	};
	struct debug_pt : debug_obj {
		POINT p;
	};
	struct debug_line : debug_obj {
		POINT p1, p2;
		bool arrow;
	};
	struct debug_curve : debug_obj {
		POINT p1, p2, p3;
	};

	struct smooth_brush {//idea from https://github.com/dulnan/lazy-brush
		f32 radius;//Minimum distance between the brush's position and the mouse's for the brush to start moving
		v2 pos;
		v2 getposf() { return pos; }
		POINT getpos() { return { (int)pos.x,(int)pos.y }; }
		void setpos(f32 x, f32 y) { this->pos = { x,y }; }
		void setpos(i32 x, i32 y) { this->pos = { (f32)x,(f32)y }; }
		//returns true if the brush position changed
		bool update(POINT mouse_p) {
			bool res = false;
			v2 brush_to_mouse = { mouse_p.x - this->pos.x, mouse_p.y - this->pos.y };
			if (length_sq(brush_to_mouse) > (radius * radius)) {
				res = true;
				v2 brush_to_circle = normalize(brush_to_mouse) * radius;//TODO(fran): not sure whether this correctly connects the point and the circle
				v2 circle_to_mouse = brush_to_mouse - brush_to_circle;
				pos += circle_to_mouse;
			}
			return res;
		}
	};

	struct ProcState {
		HWND wnd;
		HWND parent;
		struct brushes {
			HBRUSH fore, bk, border, border_dis;
		}brushes;

		HDC backbuffer_dc;
		HBITMAP backbuffer;
		SIZE backbuffer_dim;
		RECT backbuffer_used;//Keeps track of the area where the user has drawn something

		HBITMAP placeholder;

		struct {
			i32 fore_thickness;
		}dimensions;

		smooth_brush smooth_pos;

		bool onMouseTracking;

		std::vector<std::vector<POINT>> strokes;//Points separated for each time the user pressed and lifted the pen up

#ifdef TEST_DEBUG_OBJS
		std::vector<debug_pt> debug_pts;
		std::vector<debug_line> debug_lines;
		std::vector<debug_curve> debug_curves;
		bool debug_redraw;
#endif
	};

	constexpr cstr wndclass[] = L"win32_wndclass_paint";


	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {//NOTE: only used on creation
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, 0, true); }

	void set_dimensions(HWND wnd, i32 brush_thickness) {
		ProcState* state = get_state(wnd);
		if (state) {
			state->dimensions.fore_thickness = brush_thickness;
			state->smooth_pos.radius = (f32)brush_thickness;
		}
	}

	//NOTE: the caller takes care of deleting the brushes, we dont do it
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH fore, HBRUSH bk, HBRUSH border, HBRUSH border_dis) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (fore)state->brushes.fore = fore;
			if (bk)state->brushes.bk = bk;
			if (border)state->brushes.border = border;
			if (border_dis)state->brushes.border_dis = border_dis;
			if (repaint) {
				//TODO(fran): if the bk were changed we'd have to re-render with the new bk, quite a complicated operation
				ask_for_repaint(state);//in case of border change
			}
		}
	}

	//Stores a copy of the bmp and renders it until something is drawn on the canvas
	//Bitmap should be 32bpp
	//If bmp == nullptr then the placeholder is removed and the canvas cleared
	void set_placeholder(HWND wnd, HBITMAP bmp) {
		ProcState* state = get_state(wnd);
		if (state) {
			//TODO(fran): maybe we should use a different control that's simply an img viewer
			if (bmp) {
				if (state->placeholder) {
					DeleteBitmap(state->placeholder);
					state->placeholder = 0;
				}
				state->placeholder = (HBITMAP)CopyImage(bmp, IMAGE_BITMAP, 0, 0, 0);//TODO(fran): see LR_CREATEDIBSECTION
				Assert(state->placeholder);
				if(state->strokes.size() == 0) ask_for_repaint(state);
			}
			else {
				if (state->placeholder) {
					DeleteBitmap(state->placeholder);
					state->placeholder = 0;
					ask_for_repaint(state);//TODO(fran): idk if we want to previously clear the canvas
				}
			}
		}
	}

	struct canvas {
		HBITMAP bmp;
		SIZE dim;
		RECT rc_used;//region where the user drew something
	};
	//NOTE: 'canvas' is the actual canvas used, do _not_ modify it
	//NOTE: if you call the function while on our WM_COMMAND notification it's guaranteed the bitmap is not selected onto any dc
	//NOTE: the borders are not drawn onto the canvas
	canvas get_canvas(HWND wnd) {
		ProcState* state = get_state(wnd);
		canvas res{ 0 };
		if (state) {
			res.bmp = state->backbuffer;
			res.dim = state->backbuffer_dim;
			res.rc_used = state->backbuffer_used;
		}
		return res;
	}

	size_t get_stroke_count(HWND wnd) {
		ProcState* state = get_state(wnd);
		size_t res = 0;
		if (state) {
			res = state->strokes.size();
		}
		return res;
	}

	void backbuffer_clear(ProcState* state) {
		if (state->backbuffer) {
			HDC backbuffer_dc = state->backbuffer_dc;
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

			RECT backbuffer_rc{ 0,0,state->backbuffer_dim.cx,state->backbuffer_dim.cy };

			//Fill the background
			HBRUSH bk_br = state->brushes.bk;
			FillRect(backbuffer_dc, &backbuffer_rc, bk_br);

			//NOTE: do _not_ paint the border here, since it should fit the real size of the wnd
			state->backbuffer_used = { 0,0,0,0 };

			state->strokes.clear();//TODO(fran): no idea if I need to manually clear each containing vector
#ifdef TEST_DEBUG_OBJS
			state->debug_lines.clear();
			state->debug_pts.clear();
			state->debug_curves.clear();
#endif
		}
	}

	void clear_canvas(HWND wnd) {
		ProcState* state = get_state(wnd);
		if (state) {
			backbuffer_clear(state);
			ask_for_repaint(state);
		}
	}

	void backbuffer_resize(ProcState* state) {
		RECT rc; GetClientRect(state->wnd, &rc);
		state->backbuffer_dim = { RECTW(rc), RECTH(rc) };
#if 0
		if (state->backbuffer) DeleteBitmap(state->backbuffer);
		HDC targetdc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,targetdc); };
		state->backbuffer = CreateCompatibleBitmap(targetdc, state->backbuffer_dim.cx, state->backbuffer_dim.cy);//IMPORTANT: you cannot use createcompbitmap with the backbuffer cause it'll either have an invalid hbitmap or a monochrome one
		backbuffer_clear(state);
		//TODO(fran): instead of clearing fit the current contents of the backbuffer into the new one, we'll need to know in which directions we're being resized and the prev and new rc sizes
#else
		//TODO(fran): check the oldbackbuffer is valid, otherwise we simply wanna execute the code in #if 0 (maybe we can get away with setting state->backbuffer_used to 0 at the start and run this code for both paths)
		//Draw old backbuffer into the new one
		HDC targetdc = GetDC(state->wnd); defer{ ReleaseDC(state->wnd,targetdc); };

		HDC oldbackbufferdc = CreateCompatibleDC(targetdc); defer{ DeleteDC(oldbackbufferdc); };
		HBITMAP oldbackbuffer = state->backbuffer; defer{ DeleteBitmap(oldbackbuffer); };
		
		HDC newbackbuffer_dc = state->backbuffer_dc;
		HBITMAP newbackbuffer = CreateCompatibleBitmap(targetdc, state->backbuffer_dim.cx, state->backbuffer_dim.cy);//IMPORTANT: you cannot use createcompbitmap with the backbufferdc cause it'll either have an invalid hbitmap or a monochrome one

		state->backbuffer = newbackbuffer;//store new backbuffer

		BITMAP oldbackbuffernfo; GetObject(oldbackbuffer, sizeof(oldbackbuffernfo), &oldbackbuffernfo);
		auto oldbmp = SelectBitmap(oldbackbufferdc, oldbackbuffer); defer{ SelectBitmap(oldbackbufferdc,oldbmp); };

		auto oldbmp2 = SelectBitmap(newbackbuffer_dc, newbackbuffer); defer{ SelectBitmap(newbackbuffer_dc,oldbmp2); };

		//Fill the background
		HBRUSH bk_br = state->brushes.bk;
		RECT newbackbuffer_rc{ 0,0,state->backbuffer_dim.cx,state->backbuffer_dim.cy };
		FillRect(newbackbuffer_dc, &newbackbuffer_rc, bk_br);

		//Insert the old backbuffer
		rect_i32 oldbackbuffer_rc;
		//center the old backbuffer into the new one
		oldbackbuffer_rc.left = (RECTW(newbackbuffer_rc) - oldbackbuffernfo.bmWidth) / 2;
		oldbackbuffer_rc.top = (RECTH(newbackbuffer_rc) - oldbackbuffernfo.bmHeight) / 2;
		//crop by the used part of the backbuffer (we dont wanna draw what doesnt have any content)
		oldbackbuffer_rc.left += state->backbuffer_used.left;
		oldbackbuffer_rc.top += state->backbuffer_used.top;
		oldbackbuffer_rc.w = RECTW(state->backbuffer_used);
		oldbackbuffer_rc.h = RECTH(state->backbuffer_used);

		//render
		BitBlt(newbackbuffer_dc, oldbackbuffer_rc.x, oldbackbuffer_rc.y, oldbackbuffer_rc.w, oldbackbuffer_rc.h, oldbackbufferdc, state->backbuffer_used.left, state->backbuffer_used.top, SRCCOPY);

		//Update backbuffer_used coordinates
		state->backbuffer_used = to_RECT(oldbackbuffer_rc);
		//clamp to new backbuffer size
		state->backbuffer_used.left =   clamp(0, state->backbuffer_used.left,   RECTW(newbackbuffer_rc) - 1);
		state->backbuffer_used.right =  clamp(0, state->backbuffer_used.right,  RECTW(newbackbuffer_rc) - 1);
		state->backbuffer_used.top =    clamp(0, state->backbuffer_used.top,    RECTH(newbackbuffer_rc) - 1);
		state->backbuffer_used.bottom = clamp(0, state->backbuffer_used.bottom, RECTH(newbackbuffer_rc) - 1);
		
		//TODO(fran): BUG: we dont update the position of the points in the strokes, that's gonna be quite slow, we can ignore it for now since we no longer use them

#endif
	}

	//Returns the smallest rectangle that completely covers both rectangles
	RECT combine_rc(RECT r1, RECT r2) {
		RECT res;
		res.left = min(r1.left, r2.left);
		res.top = min(r1.top, r2.top);
		res.bottom = max(r1.bottom, r2.bottom);
		res.right = max(r1.right, r2.right);
		return res;
	}

	//TODO(fran): change p to v2 to have floating point precision
	void backbuffer_draw_point(ProcState* state, v2 p, f32 thickness) {
		if (state->backbuffer) {
			HDC backbuffer_dc = state->backbuffer_dc;
			auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };
			RECT circle;
#if 0
			{
				auto rectCenterWH = [](POINT center, u32 w, u32 h) -> RECT {
					RECT res;
					res.left = center.x - w / 2;
					res.top = center.y - h / 2;
					res.right = res.left + w;
					res.bottom = res.top + h;
					return res;
				};
				circle = rectCenterWH({ (i32)p.x,(i32)p.y }, (i32)thickness, (i32)thickness);
				HPEN pen = CreatePen(PS_SOLID, 0, ColorFromBrush(state->brushes.fore)); defer{ DeletePen(pen); };
				HBRUSH oldbr = SelectBrush(backbuffer_dc, state->brushes.fore); defer{ SelectBrush(backbuffer_dc,oldbr); };
				HPEN oldpen = SelectPen(backbuffer_dc, pen); defer{ SelectPen(backbuffer_dc,oldpen); };
				Ellipse(backbuffer_dc, circle.left, circle.top, circle.right, circle.bottom);
				//TODO(fran): stop drawing points and start connecting lines (is this possible or sensible thing to do?)
			}
#else 
			{
				auto rectCenterWH = [](v2 center, f32 w, f32 h) -> Gdiplus::RectF {
					Gdiplus::RectF res;
					res.X = center.x - w / 2;
					res.Y = center.y - h / 2;
					res.Width = w;
					res.Height = h;
					return res;
				};
				auto circlef = rectCenterWH(p, thickness, thickness);
				Gdiplus::Graphics g(backbuffer_dc);
				g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
				g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
				g.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
				g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
				COLORREF c = ColorFromBrush(state->brushes.fore);
				Gdiplus::Color col(GetRValue(c), GetGValue(c), GetBValue(c));
				auto br = Gdiplus::SolidBrush(col);
				g.FillEllipse(&br, circlef);

				circle.left = (i32)floor(circlef.X);
				circle.top = (i32)floor(circlef.Y);
				circle.right = (i32)ceil(circlef.X+ circlef.Width);
				circle.bottom = (i32)ceil(circlef.Y+ circlef.Height);
				//TODO(fran): clamp circle, maybe some value can go to -1
			}
#endif

			RECT bused = state->backbuffer_used; //TODO(fran): maybe I should default state->backbuffer_used to the max and min values, idk
			if (!bused.left && !bused.top && !bused.right && !bused.bottom) bused = circle;
			state->backbuffer_used = combine_rc(bused, circle);
		}
	}

	i32 get_line_thickness(HWND wnd) {
		i32 res = 0;
		ProcState* state = get_state(wnd);
		if (state) {
			res = state->dimensions.fore_thickness;
		}
		return res;
	}

	const std::vector<std::vector<POINT>>* get_all_points(HWND wnd) {
		std::vector<std::vector<POINT>>* res = nullptr;
		ProcState* state = get_state(wnd);
		if (state) {
			res = &state->strokes;
		}
		return res;
	}

	void add_debug_pt(HWND wnd, debug_pt p) {
#ifdef TEST_DEBUG_OBJS
		ProcState* state = get_state(wnd);
		if (state) {
			state->debug_pts.push_back(p);
			ask_for_repaint(state);
		}
#endif
	}
	void add_debug_line(HWND wnd, debug_line l) {
#ifdef TEST_DEBUG_OBJS
		ProcState* state = get_state(wnd);
		if (state) {
			state->debug_lines.push_back(l);
			ask_for_repaint(state);
		}
#endif
	}
	void add_debug_curve(HWND wnd, debug_curve c) {
#ifdef TEST_DEBUG_OBJS
		ProcState* state = get_state(wnd);
		if (state) {
			state->debug_curves.push_back(c);
			ask_for_repaint(state);
		}
#endif
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
			HDC targetdc = GetDC(st->wnd); defer{ ReleaseDC(st->wnd,targetdc); };
			st->backbuffer_dc = CreateCompatibleDC(targetdc);
			st->strokes = decltype(st->strokes)();
#ifdef TEST_DEBUG_OBJS
			st->debug_pts = decltype(st->debug_pts)();
			st->debug_lines = decltype(st->debug_lines)();
			st->debug_curves = decltype(st->debug_curves)();
#endif
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
			backbuffer_resize(state);
			ask_for_repaint(state);
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
			if (state->placeholder) {
				DeleteBitmap(state->placeholder);
				state->placeholder = 0;
			}
			DeleteDC(state->backbuffer_dc); state->backbuffer_dc = 0;
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
			Assert(state->backbuffer_dim.cx == w);
			bool window_enabled = IsWindowEnabled(state->wnd); //TODO(fran): we need to repaint the backbuf if the window is disabled and the dis_... are !=0 and != than the non dis_... brushes

			//Border
			int border_thickness = 1; //TODO(fran): allow the user to specify border thickness
			HBRUSH border_br = window_enabled ? state->brushes.border : state->brushes.border_dis;
			FillRectBorder(dc, rc, border_thickness, border_br, BORDERALL);

			HDC backbuffer_dc = state->backbuffer_dc;

			rect_i32 canvas;
			canvas.x = border_thickness;
			canvas.y = border_thickness;
			canvas.w = w - border_thickness * 2;
			canvas.h = h - border_thickness * 2;

			if (state->strokes.size() > 0 || state->placeholder==nullptr) {
				auto oldbmp = SelectBitmap(backbuffer_dc, state->backbuffer); defer{ SelectBitmap(backbuffer_dc,oldbmp); };

				i32 backbuf_x = canvas.x, backbuf_y = canvas.y;
				BitBlt(dc, canvas.x, canvas.y, canvas.w, canvas.h, backbuffer_dc, backbuf_x, backbuf_y, SRCCOPY);

				{
#define TEST_BACKBUFFER_USED
#if defined(TEST_BACKBUFFER_USED) && defined(_DEBUG)
					HBRUSH _br = CreateSolidBrush(RGB(255, 0, 0)); defer{ DeleteBrush(_br); };
					FillRectBorder(dc, state->backbuffer_used, 1, _br, BORDERALL);
#endif
				}

				{
#ifdef TEST_DEBUG_OBJS
					if (state->debug_redraw) {
						Gdiplus::Graphics g(dc);
						g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
						g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
						g.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
						g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
						for (auto& o : state->debug_pts) {
							i32 point_sz = (i32)(o.line_sz * 1.5f);
							Gdiplus::Rect r; r.X = o.p.x; r.Y = o.p.y; r.Width = point_sz; r.Height = point_sz;
							Gdiplus::Color c; c.SetFromCOLORREF(o.col);
							auto b = Gdiplus::SolidBrush(c);
							g.FillRectangle(&b, r);
							HDC dc = g.GetHDC(); defer{ g.ReleaseHDC(dc); };
							urender::draw_text_debug(dc, o.p, o.msg, 13, o.col, urender::txt_align::left);
						}
						for (auto& o : state->debug_lines) {
							Gdiplus::Color c; c.SetFromCOLORREF(o.col);
							auto p = Gdiplus::Pen(c, o.line_sz);
							g.DrawLine(&p, o.p1.x, o.p1.y, o.p2.x, o.p2.y);
							if (o.arrow) {

								i32 dx = o.p2.x - o.p1.x;
								i32 dy = o.p2.y - o.p1.y;
								v2 backwards_dir{ (f32)dx, (f32)dy }; backwards_dir = normalize(backwards_dir); backwards_dir *= -1;

								f32 arrow_sz = o.line_sz * 2.5f;
								v2 right = { (f32)o.p2.x, (f32)o.p2.y };
								v2 back = right + backwards_dir * arrow_sz;
								v2 up_dir = perp(backwards_dir);
								v2 top = back + up_dir * arrow_sz;
								v2 bottom = back - up_dir * arrow_sz;

								Gdiplus::GraphicsPath path;
								path.AddLine(top.x, top.y, bottom.x, bottom.y);
								path.AddLine(bottom.x, bottom.y, right.x, right.y);
								path.AddLine(right.x, right.y, top.x, top.y);
								path.CloseFigure();
								auto b = Gdiplus::SolidBrush(c);
								g.FillPath(&b, &path);
							}

							HDC dc = g.GetHDC(); defer{ g.ReleaseHDC(dc); };
							urender::draw_text_debug(dc, o.p1, o.msg, 13, o.col, urender::txt_align::left);
						}
						for (auto& o : state->debug_curves) {
							Gdiplus::Color c; c.SetFromCOLORREF(o.col);
							auto p = Gdiplus::Pen(c, o.line_sz);

							g.DrawCurve(&p, (Gdiplus::Point*) & o.p1, 3);

							HDC dc = g.GetHDC(); defer{ g.ReleaseHDC(dc); };
							urender::draw_text_debug(dc, o.p1, o.msg, 13, o.col, urender::txt_align::left);
						}

						state->debug_redraw = false;
					}
#endif
				}
			}
			else { //If there is nothing on the backbuffer and we have a valid placeholder
				
				RECT bk_rc = rectWH(canvas.left, canvas.top, canvas.w, canvas.h);
				HBRUSH bk_br = state->brushes.bk;

				//Background
				//TODO(fran): clipping (when a placeholder exists)
				FillRect(dc, &bk_rc, bk_br);
				
				//Placeholder
				BITMAP bmnfo; GetObject(state->placeholder, sizeof(bmnfo), &bmnfo);
				int placeholder_w = bmnfo.bmWidth;
				int placeholder_h = bmnfo.bmHeight;
				int placeholder_x = (canvas.w - placeholder_w) / 2;
				int placeholder_y = (canvas.h - placeholder_h) / 2;
				//TODO(fran): we may want to implement scaling when the canvas is smaller than the placeholder
				auto oldbmp = SelectBitmap(backbuffer_dc, state->placeholder); defer{ SelectBitmap(backbuffer_dc,oldbmp); };
				BitBlt(dc, placeholder_x, placeholder_y, placeholder_w, placeholder_h, backbuffer_dc, 0, 0, SRCCOPY);
			}

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
			bool window_enabled = IsWindowEnabled(state->wnd);
			if (window_enabled) {
				state->smooth_pos.setpos((i32)mouse.x, (i32)mouse.y);
				backbuffer_draw_point(state, state->smooth_pos.getposf(), (f32)state->dimensions.fore_thickness);
				ask_for_repaint(state);

				TRACKMOUSEEVENT track;
				track.cbSize = sizeof(track);
				track.dwFlags = TME_LEAVE;
				track.hwndTrack = state->wnd;
				TrackMouseEvent(&track);//TODO(fran): look at SetCapture()
				state->onMouseTracking = true;

				state->strokes.push_back(std::vector<POINT>());
				state->strokes[state->strokes.size() - 1].push_back(state->smooth_pos.getpos());//TODO(fran): may be better to store floating point
			}
			return 0;
		} break;
		case WM_MOUSEMOVE /*WM_MOUSEFIRST*/://When the mouse goes over us this is 3rd msg received
		{
			//wparam = test for virtual keys pressed
			POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Client coords, relative to upper-left corner of client area
			if (state->onMouseTracking) {
				if (state->smooth_pos.update(mouse)) {
					backbuffer_draw_point(state, state->smooth_pos.getposf(), (f32)state->dimensions.fore_thickness);
					ask_for_repaint(state);

					state->strokes[state->strokes.size() - 1].push_back(state->smooth_pos.getpos());//TODO(fran): may be better to store floating point
				}
			}

			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONUP:
		case WM_MOUSELEAVE:
		{
			TRACKMOUSEEVENT track;
			track.cbSize = sizeof(track);
			track.dwFlags = TME_CANCEL;
			track.hwndTrack = state->wnd;
			TrackMouseEvent(&track);
			if (state->onMouseTracking) {
				state->onMouseTracking = false;
				SendMessage(state->parent, WM_COMMAND, 0, (LPARAM)state->wnd);
			}
#ifdef TEST_DEBUG_OBJS
			state->debug_redraw = true;
#endif
		} break;
		case WM_CANCELMODE:
		{
			//We got canceled by the system, not really sure what it means but docs say we should cancel everything mouse capture related, so stop tracking
			//also sent by EnableWindow when disabling us
			SendMessage(state->wnd, WM_MOUSELEAVE, 0, 0);//TODO(fran): idk whether to send mouseleave or lbuttonup, or do nothing
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
		case WM_ENABLE:
		{
			BOOL enable = (BOOL)wparam;
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_MOUSEWHEEL:
		{
			//for now there's no reason for needing mousewheel input
			return DefWindowProc(hwnd, msg, wparam, lparam);//propagates the msg to the parent
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
	} static const PREMAIN_POSTMAIN;
}