#pragma once
#include "windows_sdk.h"
#include "がいじんの_Platform.h"
#include "がいじんの_Helpers.h"
#include "unCap_Renderer.h"
#include "unCap_Math.h"

//------------------"API"------------------:
#define score_base_msg_addr (WM_USER + 4000)
#define SC_SETSCORE (score_base_msg_addr+1) /*wparam = float ; lparam = unused*/ /*score should be normalized between 0.0 and 1.0*/ /*IMPORTANT: use f32_to_WPARAM() from the "Helpers" file, casting (eg. (WPARAM).7f) wont work*/

namespace score {

#define ANIM_SCORE_ring 1

	
	struct ProcState {
		HWND wnd;
		HWND nc_parent;//NOTE: we dont probably care about interacting with the parent, this is just visual candy
		f32 score;
		HFONT font;

		//HBITMAP score; //TODO(fran): we probably dont want to be redrawing the score bitmap on each resize
		//bool redraw_score;
		//RECT last_redraw;//Stores RECT size for the last time a redraw happened

		struct {
			HBRUSH bk;
			HBRUSH ring_bk;
			HBRUSH ringfull;
			HBRUSH ringempty;
			HBRUSH inner_circle;
		} brushes;

		struct {
			struct {
				f32 score_start;
				f32 score;//total score, last frame will present this value, in between frames will interpolate from score_start, also it must match ProcState->score or the animation will stop (this will be important when multithreading)
				u32 frames_cnt;//total frames
				u32 frame_idx;//current frame
			} ring;

		}anim;
	};

	constexpr cstr wndclass[] = L"がいじんの_wndclass_score";

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	//NOTE: any NULL HBRUSH remains unchanged
	//NOTE: interpolation happens between ringfull and ringempty, using the score percentage to get the color
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk, HBRUSH ring_bk, HBRUSH ringfull, HBRUSH ringempty, HBRUSH inner_circle) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (ring_bk) state->brushes.ring_bk = ring_bk;
			if (ringfull) state->brushes.ringfull = ringfull;
			if (ringempty) state->brushes.ringempty = ringempty;
			if (inner_circle) state->brushes.inner_circle = inner_circle;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	//void resize(ProcState* state){
		//left as a TODO if we want to reduce bitmap redraws
	//	InvalidateRect(state->wnd, NULL, TRUE);
	//}

	void play_anim(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
		ProcState* state = get_state(hwnd);
		bool next_frame = false;

		switch (anim_id) {
		case ANIM_SCORE_ring:
		{
			auto& anim = state->anim.ring;
			anim.frame_idx++;
			if (anim.frame_idx < anim.frames_cnt) next_frame = true;

			f32 dt = (f32)anim.frame_idx / (f32)anim.frames_cnt;

			f32 anim_score = lerp(anim.score_start, dt, anim.score);

			//HACK: (we cant match score this way)
			PostMessage(state->wnd, SC_SETSCORE, f32_to_WPARAM(anim_score), 0);

		} break;
		}

		if (next_frame) {
			SetTimer(state->wnd, anim_id, max((u32)1, (u32)((1.f/(f32)win32_get_refresh_rate_hz(state->wnd))*1000)), play_anim);
		}
		else {
			KillTimer(state->wnd, anim_id);
		}
	}

	void start_anim(ProcState* state, u64 anim_id) {
		switch (anim_id) {
		case ANIM_SCORE_ring:
		{
			state->anim.ring.score_start = 0.f;
			state->anim.ring.score = state->score;
			u32 refresh = win32_get_refresh_rate_hz(state->wnd);
			state->anim.ring.frames_cnt = refresh / 2;
			state->anim.ring.frame_idx = 0;
		} break;
		}
		SetTimer(state->wnd, anim_id, USER_TIMER_MINIMUM, play_anim);
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			decltype(state) st = (decltype(st))calloc(1, sizeof(decltype(*st)));
			Assert(st);
			st->nc_parent = create_nfo->hwndParent;
			st->wnd = hwnd;
			set_state(hwnd, st);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			return 0;
		} break;
		case WM_SIZE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				free(state);
				state = nullptr;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCCALCSIZE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SHOWWINDOW:
		{
			BOOL show = wparam;
			if (show) {
				//Start animation
				start_anim(state, ANIM_SCORE_ring);
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_GETTEXTLENGTH:
		{
			return 0;
		} break;
		case WM_IME_SETCONTEXT://sent the first time on SetFocus
		{
			return 0; //We dont want IME
		} break;
		case WM_IME_NOTIFY://for some reason you still get this even when not doing WM_IME_SETCONTEXT
		{
			return 0;
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCPAINT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case SC_SETSCORE:
		{
			f32 new_score = f32_from_WPARAM(wparam);
			state->score = clamp01(new_score);
			//TODO(fran): it may be good to check whether new_score == old_score to avoid re-rendering
			InvalidateRect(state->wnd, NULL, TRUE);
			return 0;
		} break;
		case WM_ERASEBKGND:
		{
#if 0
			LRESULT res;
			if (state->brushes.bk) {
				HDC dc = (HDC)wparam;
				RECT r; GetClientRect(state->wnd, &r);
#if 0
				FillRect(dc, &r, state->brushes.bk);
#else
				FillRectAlpha(dc, &r, state->brushes.bk, 255); //TODO(fran): this function probably needs to premultiply the alpha, which it currently does not
#endif
				res = 1;//We erased the bk
			}
			else res = DefWindowProc(hwnd, msg, wparam, lparam);
			return res;
#else
			return 0;//WM_PAINT will take care of everything
#endif
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(state->wnd, &ps);

			RECT rc;  GetClientRect(state->wnd, &rc);
			int score_x, score_y, score_w, score_h;
			score_w = score_h = min(RECTW(rc), RECTH(rc));
			score_x = (RECTW(rc) - score_w) / 2;
			score_y = (RECTH(rc) - score_h) / 2;

			auto color_to_v4 = [](COLORREF col, u8 alpha) {
				//NOTE: we separate alpha since colorref doesnt really care or set alpha usually
				v4 res;
				res.r = (f32)GetRValue(col) / 255.f;
				res.g = (f32)GetGValue(col) / 255.f;
				res.b = (f32)GetBValue(col) / 255.f;
				res.a = (f32)alpha / 255.f;//TODO(fran): decide what to do with alpha, the user should have control over it
				return res;
			};

			v4 ringfull = color_to_v4(ColorFromBrush(state->brushes.ringfull), 255);
			v4 ringempty = color_to_v4(ColorFromBrush(state->brushes.ringempty), 255);
			v4 ring_color = lerp(ringempty, state->score, ringfull);//NOTE: lerping should only be done on aldready premultiplied values, since we're using 255 we're ok for now
			v4 ring_bk = color_to_v4(ColorFromBrush(state->brushes.ring_bk), 255);
			v4 inner_circle = color_to_v4(ColorFromBrush(state->brushes.inner_circle), 255);
			v4 bk = color_to_v4(ColorFromBrush(state->brushes.bk), 255);

			urender::draw_ring(dc, score_x, score_y, score_w, score_h, state->score, ring_color, ring_bk, inner_circle, bk);

			EndPaint(hwnd, &ps);
			return 0;
		} break;
		case WM_NCHITTEST:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETCURSOR:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEMOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEACTIVATE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PARENTNOTIFY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_KILLFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFONT:
		{
			HFONT new_font = (HFONT)wparam;
			BOOL redraw = LOWORD(lparam);
			state->font = new_font;
			if(redraw)InvalidateRect(state->wnd, NULL, TRUE);
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

	void init_wndclass(HINSTANCE inst) { //INFO: now that we use pre_post_main we cant depend on anything that isnt calculated at compile time
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = Proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(ProcState*);
		wcex.hInstance = inst;
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = 0; //TODO(fran): unCap_colors.ControlBk hasnt been deserialized by the point pre_post_main gets executed!
		wcex.lpszMenuName = 0;
		wcex.lpszClassName = wndclass;
		wcex.hIconSm = 0;

		ATOM class_atom = RegisterClassExW(&wcex);
		Assert(class_atom);
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