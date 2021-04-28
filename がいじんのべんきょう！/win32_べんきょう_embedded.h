#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_static_oneline.h"
#include "win32_べんきょう_db.h"

namespace embedded {
	namespace show_word_reduced {
		//---------------Creation Example---------------:
		// CreateWindowW(embedded::show_word_reduced::wndclass, 0, WS_CHILD | embedded::show_word_reduced::style::roundrect, 0, 0, 0, 0, parent, 0, 0, 0);
		//---------------API---------------:
		// set_theme()
		// set_word()

		constexpr cstr wndclass[] = L"がいじんの_wndclass_embedded_showwordreduced";
		struct Theme {
			struct {
				brush_group txt, bk, border;
			} brushes;
			struct {
				u32 border_thickness = U32MAX;
			}dimensions;
			HFONT font = 0;
		};
		enum style {
			roundrect = (1 << 0),

		};
		struct ProcState {
			HWND wnd;
			HWND parent;
			Theme theme;
			union {
				using type = HWND;
				struct { type hiragana, kanji, meaning, mnemonic, lexical_category; };
				type all[5];
			private: void _() { static_assert(sizeof(all) == sizeof(*this), "Come update the array to the correct count!"); }
			} controls;
		};
		ProcState* get_state(HWND wnd) { ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0); return state; }
		void set_state(HWND wnd, ProcState* state) { SetWindowLongPtr(wnd, 0, (LONG_PTR)state); }
		void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, TRUE); }
		void update_controls_theme(ProcState* state) {
			for (auto& c : state->controls.all) {
				static_oneline::set_brushes(c, true, state->theme.brushes.txt.normal, state->theme.brushes.bk.normal, state->theme.brushes.border.normal, state->theme.brushes.txt.disabled, state->theme.brushes.bk.disabled, state->theme.brushes.border.disabled);
				static_oneline::set_dimensions(c, state->theme.dimensions.border_thickness);
				SendMessage(c, WM_SETFONT, (WPARAM)state->theme.font, TRUE);
			}
		}
		//Set only what you need, what's not set wont be used
		//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
		void set_theme(HWND wnd, const Theme* t) {
			ProcState* state = get_state(wnd);
			if (state && t) {
				bool repaint = false;
				repaint |= copy_brush_group(&state->theme.brushes.txt, &t->brushes.txt);
				repaint |= copy_brush_group(&state->theme.brushes.bk, &t->brushes.bk);
				repaint |= copy_brush_group(&state->theme.brushes.border, &t->brushes.border);

				if (t->dimensions.border_thickness != U32MAX) {
					state->theme.dimensions.border_thickness = t->dimensions.border_thickness; repaint = true;
				}

				if (t->font) { state->theme.font = t->font; repaint = true; }

				if (repaint) { update_controls_theme(state); ask_for_repaint(state); }
			}
		}
		void resize_controls(ProcState* state) {
			RECT r; GetClientRect(state->wnd, &r);
			int w = RECTWIDTH(r);
			int h = RECTHEIGHT(r);
			int w_pad = (int)((float)w * .05f);//TODO(fran): hard limit for max padding
			int h_pad = (int)((float)h * .05f);
			int max_w = w - w_pad * 2;
			int start_y = 0;

			auto& controls = state->controls;

			//Hiragana, kanji and lexical category on the left, meaning, notes, mnemonic on the right
			int cell_w = (max_w - w_pad / 2) / 2;
			auto grid = create_grid_2x2(30, cell_w, start_y, w_pad / 2, h_pad / 2, max_w, w);
			//TODO(fran): I think 4x1 config is better cause the wnd isnt expected to be very wide

			rect_i32 hiragana = grid[0][0];
			rect_i32 kanji = grid[0][1];
			rect_i32 meaning = grid[1][0];
			rect_i32 mnemonic = grid[1][1];

			//TODO(fran): do I wanna put the lexical_category? it actually feels kinda pointless

			rect_i32 bottom_most_control = mnemonic;

			int used_h = bottom_most_control.bottom();// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls

			MyMoveWindow_offset(controls.hiragana, hiragana, FALSE);
			MyMoveWindow_offset(controls.kanji, kanji, FALSE);
			MyMoveWindow_offset(controls.meaning, meaning, FALSE);
			MyMoveWindow_offset(controls.mnemonic, mnemonic, FALSE);
		}
		void show_controls(ProcState* state, bool show) {
			for (auto& c : state->controls.all) ShowWindow(c, show ? SW_SHOW : SW_HIDE);
		}
		//NOTE: a copy of the word is kept internally
		void set_word(HWND wnd, learnt_word16* word) {
			ProcState* state = get_state(wnd);
			if (state && word) {
				//TODO(fran): we could resize_controls in case we resize by the lenght of the strings
				SendMessage(state->controls.hiragana, WM_SETTEXT, 0, (LPARAM)word->attributes.hiragana.str);
				SendMessage(state->controls.kanji, WM_SETTEXT, 0, (LPARAM)word->attributes.kanji.str);
				SendMessage(state->controls.meaning, WM_SETTEXT, 0, (LPARAM)word->attributes.meaning.str);
				SendMessage(state->controls.lexical_category, WM_SETTEXT, 0, (LPARAM)word->attributes.lexical_category.str);
				SendMessage(state->controls.mnemonic, WM_SETTEXT, 0, (LPARAM)word->attributes.mnemonic.str);
				//TODO(fran): ask_for_repaint(state) ?
			}
		}
		LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			ProcState* state = get_state(hwnd);
			switch (msg) {
				//TODO?(fran): WM_MOUSEWHEEL, we might no be able to fit all the controls
			case WM_NCCREATE: {
				CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;
				ProcState* st = (ProcState*)calloc(1, sizeof(ProcState)); Assert(st);
				set_state(hwnd, st);
				st->wnd = hwnd;
				st->parent = creation_nfo->hwndParent;
				return TRUE; //continue creation
			} break;
			case WM_NCCALCSIZE: {
				if (wparam) {
					NCCALCSIZE_PARAMS* calcsz = (NCCALCSIZE_PARAMS*)lparam;
					return 0;
				}
				else {
					RECT* client_rc = (RECT*)lparam;//TODO(fran): make client_rc cover the full window area
					return 0;
				}
			} break;
			case WM_CREATE: {
				CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;
				for (auto& c : state->controls.all) {
					c = CreateWindowW(static_oneline::wndclass, 0, WS_CHILD | SS_CENTERIMAGE | SS_LEFT | SO_AUTOFONTSIZE
						, 0, 0, 0, 0, state->wnd, 0, 0, 0);
				}
				show_controls(state, true);

				return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): remove once we know all the things this does
			} break;
			case WM_SETFONT: {
				HFONT font = (HFONT)wparam;
				Theme new_theme;
				new_theme.font = font;
				set_theme(state->wnd, &new_theme);
				return 0;
			} break;
			case WM_SHOWWINDOW: {
				bool show = wparam;
				//show_controls(state,show); //TODO(fran): if we ever need controls that start hidden
				return DefWindowProc(hwnd, msg, wparam, lparam);
			} break;
			case WM_SIZE: {
				resize_controls(state);
				return DefWindowProc(hwnd, msg, wparam, lparam);
			} break;
			case WM_NCDESTROY: {
				free(state);
				return 0;
			}break;
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rc; GetClientRect(state->wnd, &rc);
				int w = RECTW(rc), h = RECTH(rc);
				u16 radius = (u16)(min(w, h) * .05f);
				//ps.rcPaint
				HDC dc = BeginPaint(state->wnd, &ps);
				bool window_enabled = IsWindowEnabled(state->wnd);
				LONG_PTR style = GetWindowLongPtr(state->wnd, GWL_STYLE);
				HBRUSH bkbr = window_enabled ? state->theme.brushes.bk.normal : state->theme.brushes.bk.disabled;
				HBRUSH borderbr = window_enabled ? state->theme.brushes.border.normal : state->theme.brushes.border.disabled;
				//TODO(fran): maybe fillrect alpha is needed to allow for transparency?
				//Bk
				if (bkbr) {

					if (style & style::roundrect) {
						urender::RoundRectangleFill(dc, bkbr, rc, radius);
					}
					else {
						FillRect(dc, &rc, bkbr);
					}
				}
				//Border
				if (borderbr) {

					if (style & style::roundrect) {
						urender::RoundRectangleBorder(dc, borderbr, rc, radius, (f32)state->theme.dimensions.border_thickness);
					}
					else {
						FillRectBorder(dc, rc, state->theme.dimensions.border_thickness, borderbr, BORDERALL);
					}
				}

				EndPaint(hwnd, &ps);
				return 0;
			} break;
			case WM_MOVE:
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
			case WM_DESTROY:
			case WM_SETCURSOR:
			case WM_MOUSEMOVE:
			{
				return DefWindowProc(hwnd, msg, wparam, lparam);
			} break;
			case WM_NCPAINT:
			case WM_ERASEBKGND:
			case WM_LBUTTONDOWN://TODO(fran): we may want to SetFocus(0);
			case WM_RBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_IME_SETCONTEXT:
			case WM_GETTEXT:
			case WM_GETTEXTLENGTH:
			case WM_PARENTNOTIFY:
			{
				return 0;
			} break;
			case WM_SETTEXT:
			{
				return 1;
			}
			case WM_NCHITTEST: {
				POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Screen coords, relative to upper-left corner
				RECT rw; GetWindowRect(state->wnd, &rw);
				LRESULT hittest = HTNOWHERE;
				if (test_pt_rc(mouse, rw))hittest = HTCLIENT;
				return hittest;
			} break;
			case WM_MOUSEACTIVATE: {
				return MA_ACTIVATE;
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
			pre_post_main() { init_wndclass(GetModuleHandleW(NULL)); }
			~pre_post_main() { }
		}static const PREMAIN_POSTMAIN;
	}
}