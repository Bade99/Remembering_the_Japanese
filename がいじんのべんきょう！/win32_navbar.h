#pragma once
#include "win32_Platform.h"
#include "windows_sdk.h"
#include "windows_extra_msgs.h"
#include <windowsx.h>
#include "win32_Helpers.h"
#include "unCap_Math.h"

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
		struct {
			u8 spacing = U8MAX;
		}dimensions;
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

			public:
			~Controls();//NOTE: this two lines are simply to remove a compiler warning about it not being able to access destructor, which is probably necessary since im using vectors which need destructors
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

	struct __copy_theme { bool repaint, resize; };
	__copy_theme _copy_theme(Theme* dst, const Theme* src) {
		bool repaint = false, resize = false;
		repaint |= copy_brush_group(&dst->brushes.bk, &src->brushes.bk);

		if (src->dimensions.spacing != U8MAX) {
			dst->dimensions.spacing = src->dimensions.spacing; repaint = true; resize = true;
		}

		return { repaint, resize };
	}

	//Set only what you need, what's not set wont be used
	//NOTE: the caller takes care of destruction of the theme's objects, if any, like the HBRUSHes, needs it
	void set_theme(HWND wnd, const Theme* t) {
		ProcState* state = get_state(wnd);
		if (state && t) {
			auto [repaint,resize] = _copy_theme(&state->theme, t);

			if (repaint)  ask_for_repaint(state);
			if (resize)  ask_for_resize(state);
		}
	}

	enum class attach_point : u32 {left=0,center,right};
	//NOTE: Already attached childs are automatically removed and attached to their new place
	//NOTE: a negative index means add the window at the end of the list
	void attach(HWND wnd, HWND child, attach_point attachment, i32 index) {
		ProcState* state = get_state(wnd);
		if (state) {
			Assert((u32)attachment < ARRAYSIZE(state->controls.all));
			//TODO(fran): should check the child is actually a child of this wnd?

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

	//TODO(fran): get this out of here
	SIZE operator+(const SIZE& lhs, const SIZE& rhs) {
		SIZE res;
		res.cx = lhs.cx + rhs.cx;
		res.cy = lhs.cy + rhs.cy;
		return res;
	}
	SIZE& operator+=(SIZE& lhs, const SIZE& rhs) {
		lhs = lhs + rhs;
		return lhs;
	}

	void resize_controls(ProcState* state) {
		//TODO(fran): take into account spacing, or simply have the user insert HWND spacers. also should we add spacing on the leftmost and rightmost part of the wnd?
		struct minmaxsz { SIZE min, max; };
		enum desired_size : u32{dontcare=0,flexible,fixed};
		struct mmds { SIZE min, max; desired_size flexibility; };

		constexpr int region_cnt = ARRAYSIZE(state->controls.all);

		SIZE total_bounds[region_cnt]{ 0 };//left center right

		RECT rc; GetClientRect(state->wnd, &rc);
		i32 w_max = RECTW(rc);
		i32 h_max = RECTH(rc);
		//TODO(fran): should I add an early out? if(!w_max || !h_max) return;

		std::vector<mmds> bounds[region_cnt];
		for (int i = 0; i < ARRAYSIZE(bounds); i++) bounds[i].reserve(state->controls.all[i].size());

		std::vector<SIZE> final_bounds[region_cnt];
		for (int i = 0; i < ARRAYSIZE(final_bounds); i++) final_bounds[i].resize(state->controls.all[i].size());

		//Retrieve all desired sizes
		for (int i = 0; i < region_cnt; i++) {
			auto& v = state->controls.all[i];
			for(size_t j = 0; j < v.size(); j++){
				mmds b; b.max = { w_max,h_max }; b.min = b.max;
				b.flexibility = (decltype(b.flexibility))SendMessage(v[j], WM_DESIRED_SIZE, (WPARAM)&b.min, (LPARAM)&b.max);
				if (b.flexibility != flexible && b.flexibility != fixed) {
					b.flexibility = flexible;
					//TODO(fran): assign it the smallest minimum and maximum out of all the others
					b.max.cx = w_max / (region_cnt *3);
					b.min.cx = 0;
				}
				bounds[i].push_back(b);//TODO(fran): could do resize instead of reserve and use the obj inside the vector directly instead of having it outside and then making a copy

				total_bounds[i] += b.max;
			}
		}
		
		SIZE tot{0}; for (auto s : total_bounds) tot += s;
		if (tot.cx <= w_max) { //Great, we can place everyone just how they want to
			
			for (int i = 0; i < ARRAYSIZE(bounds); i++)
				for (int j = 0; j < bounds[i].size(); j++)
					final_bounds[i][j] = bounds[i][j].max;

		}
		else { //Resizing is necessary

			//First try by only resizing the ones that are flexible
			int size_to_clear = tot.cx - w_max;

			int flexible_potential=0;
			//TODO(fran): #speed create array with ptrs to all the flexible wnds
			for (auto& v : bounds) for (auto& b : v) if (b.flexibility == flexible) flexible_potential += distance(b.min.cx, b.max.cx);

			if (flexible_potential >= size_to_clear) { //We can fit everyone by simply resizing the flexible windows

				while (size_to_clear > 0) {//TODO(fran): im sure there's some edge case where this never ends
					std::vector<mmds*> flexibles;
					for (auto& v : bounds) for (auto& b : v) if (b.flexibility == flexible && (b.max.cx - b.min.cx) > 0) flexibles.push_back(&b);
					int per_wnd_reduction = (int)ceilf( (f32)size_to_clear / (f32)flexibles.size());//TODO(fran): may need safe_ratio0() //TODO(fran): if size_to_clear is less than flexibles.size() then per_wnd_reduction will be 0 and we loop forever, I added the ceil to fix that but that probably introduces the case where the possible extra +1 the ceil adds is more than the flexible potential. See if this assumption is true or not

					for (auto f : flexibles) {
						int reduction = minimum(per_wnd_reduction, distance(f->max.cx, f->min.cy));
						f->max.cx -= reduction;
						size_to_clear -= reduction;
					}
				}

				//store final values
				for (int i = 0; i < ARRAYSIZE(bounds); i++)
					for (int j = 0; j < bounds[i].size(); j++)
						final_bounds[i][j] = bounds[i][j].max;
			}
			else { // We need more space, all fixed windows get downgraded to their small size, and then we try with flexible windows again
#if 0
				ZeroMemory(total_bounds, sizeof(total_bounds));
#else
				for (auto& b : total_bounds) b = { 0, 0 };
				
#endif

				for (int i = 0; i < ARRAYSIZE(bounds); i++) {
					for (int j = 0; j < bounds[i].size(); j++) {
						if (bounds[i][j].flexibility == fixed) {
							final_bounds[i][j] = bounds[i][j].min; //load final values for fixed windows

							total_bounds[i] += final_bounds[i][j];
						}
						else total_bounds[i] += bounds[i][j].max;
					}
				}

				SIZE tot{0}; for (auto s : total_bounds) tot += s;
				if (tot.cx <= w_max) {//with fixed windows set to small and flexible ones to max everyone fits

					//load final size for flexible windows
					for (int i = 0; i < ARRAYSIZE(bounds); i++)
						for (int j = 0; j < bounds[i].size(); j++)
							if (bounds[i][j].flexibility == flexible) final_bounds[i][j] = bounds[i][j].max;

				}
				else {//Resize flexible windows, no matter the result we apply resizing after that

					//TODO(fran): join this with the previous flexible wnd resizing code
					while (size_to_clear > 0) {//TODO(fran): im sure there's some edge case where this never ends
						std::vector<mmds*> flexibles;
						for (auto& v : bounds) for (auto& b : v) if (b.flexibility == flexible && (b.max.cx - b.min.cx) > 0) flexibles.push_back(&b);
						if (flexibles.empty()) break;
						int per_wnd_reduction = size_to_clear / (int)flexibles.size();//TODO(fran): may need safe_ratio0()

						for (auto f : flexibles) {
							int reduction = minimum(per_wnd_reduction, distance(f->max.cx, f->min.cy));
							f->max.cx -= reduction;
							size_to_clear -= reduction;
						}
					}

					//store final values
					for (int i = 0; i < ARRAYSIZE(bounds); i++)
						for (int j = 0; j < bounds[i].size(); j++)
							final_bounds[i][j] = bounds[i][j].max;

				}

			}
		}

		//Apply resizing
		//We try to maintain the left-center-right layout, but we allow moving the center and right sections beyond what they should when we cant fit everything
		//NOTE: since we currently are a horizontal navbar we also apply vertical centering
		int start_x = rc.left; // == 0

		auto group_resize = [&](int i) {
			for (int j = 0; j < final_bounds[i].size(); j++) {
				auto sz = final_bounds[i][j];
				rect_i32 r;
				r.left = start_x;
				r.w = sz.cx;
				r.top = (h_max - sz.cy) / 2;
				r.h = sz.cy;

				start_x = r.right();
				MoveWindow(state->controls.all[i][j], r.left, r.top, r.w, r.h, FALSE);
			}
		};

		for (int i = 0; i < ARRAYSIZE(final_bounds);i++) {
			if (i == 0) { //left alignment:go from start_x onwards
				
				group_resize(i);
			}
			else if (i < (ARRAYSIZE(final_bounds) - 1)) { //center alignment: if possible center all the controls relative to the common center
				
				Assert(i == 1);//TODO(fran): if I ever actually need more than three alignment points then we need to choose the correct center for each group

				int tot_sz = 0; for (auto& sz : final_bounds[i]) tot_sz += sz.cx;

				int center = w_max / 2;

				start_x = maximum(start_x, center - tot_sz/2);

				group_resize(i);
			}
			else { //right alignment: if possible line it up so the last wnd is exactly at the end of the navbar
				int tot_sz = 0; for (auto& sz : final_bounds[i]) tot_sz += sz.cx;

				start_x = maximum(start_x, (int)rc.right - tot_sz);

				group_resize(i);
			}
		}
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//printf("NAVBAR:%s\n", msgToString(msg));
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
		case WM_CREATE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;


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
			//TODO(fran): maybe I should ask for a resize
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_ENABLE:
		{
			BOOL enable = (BOOL)wparam;
			ask_for_repaint(state);
			return 0;
		} break;


		case WM_PARENTNOTIFY:
		{
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