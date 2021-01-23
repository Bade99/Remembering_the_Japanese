#pragma once
#include "windows_sdk.h"
#include "がいじんの_Platform.h"
#include "unCap_Serialization.h"
#include "sqlite3.h"
#include <string>
#include "がいじんの_Helpers.h"
#include "unCap_button.h"
#include "LANGUAGE_MANAGER.h"

struct べんきょうSettings {

#define foreach_べんきょうSettings_member(op) \
		op(RECT, rc,200,200,600,800 ) \

	foreach_べんきょうSettings_member(_generate_member);

	sqlite3* db;

	_generate_default_struct_serialize(foreach_べんきょうSettings_member);

	_generate_default_struct_deserialize(foreach_べんきょうSettings_member);

};
_add_struct_to_serialization_namespace(べんきょうSettings)

//This window will comprise of 4 main separate ones, as if they were in a tab control, in the sense only one will be shown at any single time, and there wont be any relationships between them

//TODO(fran): should I use the hiragana column as primary key?

constexpr char べんきょう_table_words[] = "words";
constexpr char べんきょう_table_words_structure[] = 
	"hiragana			TEXT UNIQUE NOT NULL COLLATE NOCASE," //hiragana or katakana
	"kanji				TEXT COLLATE NOCASE,"
	"translation		TEXT NOT NULL COLLATE NOCASE," //TODO(fran): this should actually be a list, so we need a separate table
	"type				TEXT NOT NULL COLLATE NOCASE," //noun,verb,... TODO(fran): could use an enum and reference to another table
	"creation_date		TEXT," //ISO8601 string ("YYYY-MM-DD HH:MM:SS.SSS")
	"last_shown_date	INTEGER" //Unix Time, we dont care to show this value, it's simply for sorting
;//INFO: a column ROWID is automatically created and serves the function of "id INTEGER PRIMARY KEY" and AUTOINCREMENT, _but_ AUTOINCREMENT as in MySQL or others (simply incrementing on every insert), on the other hand the keyword AUTOINCREMENT in sqlite incurrs an extra cost because it also checks that the value hasnt already been used for a deleted row (we dont care for this in this table)
//INFO: the last line cant have a "," REMEMBER to put it or take it off


struct べんきょうProcState {
	HWND wnd;
	HWND nc_parent;
	べんきょうSettings* settings;

	enum page { //This wnd will be managed in pages instead of launching separate windows for different tasks, like a browser
		landing,
		new_word,
		practice,
		search,
		show_word,
	} current_page;

	struct {

		union landingpage_controls {
			using type = HWND;
			struct {
				type button_new;
				type button_practice;
				type button_search;
			}list; //INFO: unfortunately in order to make 'all' auto-update we need to give a name to this struct
			type all[sizeof(list)/sizeof(type)]; //NOTE: make sure you understand structure padding before implementing this, also this should be re-tested if trying with different compilers or alignment
			//static_assert(sizeof(((landingpage_controls*)0)->all) == sizeof(s), "Element count for 'all' does not match, update the number!");
		} landingpage;

		union new_word_controls {
			using type = HWND;
			struct {
				type edit_kanji;
				type edit_hiragana;//or katakana
				type edit_translation;
				type combo_type;//verb,noun,...
				//TODO(fran): here you should be able to add more than one translation
				type button_save;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} new_word;

		union practice_controls {
			using type = HWND;
			struct {
				type edit_question;
				type edit_answer;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} practice;

		union search_controls {
			using type = HWND;
			struct {
				type edit_search;
				//and a list for the results
			}list;
			type all[sizeof(list) / sizeof(type)];
		} search;

		union show_word_controls {
			using type = HWND;
			struct {
				type edit_kanji;
				type edit_hiragana;//or katakana
				type edit_translation;
				type combo_type;//verb,noun,...
				//TODO(fran): here you should be able to add more than one translation
				type button_modify;
			}list;
			type all[sizeof(list) / sizeof(type)];
		} show_word;

	}controls;
};

namespace べんきょう { //INFO: Im trying namespaces to see if this is better than having every function with the name of the wndclass
	using ProcState = べんきょうProcState;

	constexpr cstr wndclass[] = L"がいじんの_wndclass_べんきょう";

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	void startup(ProcState* state) {
		using namespace std::string_literals;
		std::string create_work_table = "CREATE TABLE IF NOT EXISTS "s + べんきょう_table_words + "("s + べんきょう_table_words_structure+ ");"s; //INFO: the param that requests this expects utf8
		char* create_errmsg;
		sqlite3_exec(state->settings->db, create_work_table.c_str(), 0, 0, &create_errmsg);
		sqlite_exec_runtime_assert(create_errmsg);
	}

	void add_controls(ProcState* state) {
		//---------------------Landing page----------------------:
		state->controls.landingpage.list.button_new = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.landingpage.list.button_new, 4);
		//TODO(fran): more brushes, fore_push,... , border_mouseover,...
		UNCAPBTN_set_brushes(state->controls.landingpage.list.button_new, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		state->controls.landingpage.list.button_practice = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.landingpage.list.button_new, 5);
		//TODO(fran): more brushes, fore_push,... , border_mouseover,...
		UNCAPBTN_set_brushes(state->controls.landingpage.list.button_practice, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		state->controls.landingpage.list.button_search = CreateWindowW(unCap_wndclass_button, NULL, WS_CHILD | WS_TABSTOP
			, 0, 0, 0, 0, state->wnd, 0, NULL, NULL);
		AWT(state->controls.landingpage.list.button_new, 6);
		//TODO(fran): more brushes, fore_push,... , border_mouseover,...
		UNCAPBTN_set_brushes(state->controls.landingpage.list.button_search, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

		for (auto ctl : state->controls.landingpage.all)
			SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);

		//TODO(fran): create the other "tabs"

	}

	void resize_controls(ProcState* state) {
		//TODO(fran): should I resize everyone or just the ones from the current page, I feel like just resizing the necessary ones is better and when there's a page change we call resize there just in case
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTWIDTH(r);
		int h = RECTHEIGHT(r);
		int w_pad = (int)((float)w * .05f);
		int h_pad = (int)((float)h * .05f);
		
#define _MyMoveWindow(wnd,xywh,repaint) MoveWindow(wnd, xywh##_x, xywh##_y, xywh##_w, xywh##_h, FALSE);

		switch (state->current_page) {
		case ProcState::page::landing: 
		{
			//One button on top of the other vertically, all buttons must be squares
			//TODO(fran): we could, at least to try how it looks, to check whether the wnd is longer on w or h and place the controls vertically or horizontally next to each other
			int wnd_cnt = ARRAYSIZE(state->controls.landingpage.all);
			int pad_cnt = wnd_cnt - 1 + 2; //+2 for bottom and top, wnd_cnt-1 to put a pad in between each control

			int max_h = h - pad_cnt * h_pad;
			int max_w = w - 2 * w_pad;
			int wnd_h = clamp(0, max_h / wnd_cnt, max_w);

			int button_new_x = w_pad;
			int button_new_y = h_pad;
			int button_new_w = wnd_h;
			int button_new_h = wnd_h;

			int button_practice_x = button_new_x;
			int button_practice_y = button_new_y + button_new_h + h_pad;
			int button_practice_w= wnd_h;
			int button_practice_h= wnd_h;

			int button_search_x = button_new_x;
			int button_search_y = button_practice_y + button_practice_h + h_pad;
			int button_search_w= wnd_h;
			int button_search_h= wnd_h;

			_MyMoveWindow(state->controls.landingpage.list.button_new, button_new,FALSE);
			_MyMoveWindow(state->controls.landingpage.list.button_practice, button_practice,FALSE);
			_MyMoveWindow(state->controls.landingpage.list.button_search, button_search,FALSE);
		} break;
		case ProcState::page::new_word:
		{
			state->controls.new_word.all;
		} break;
		case ProcState::page::practice: 
		{
			state->controls.practice.all;
		} break;
		case ProcState::page::search: 
		{
			state->controls.search.all;
		} break;
		case ProcState::page::show_word: 
		{
			state->controls.show_word.all;
		} break;
		}
	}

	void save_settings(ProcState* state) {
		RECT rc; GetWindowRect(state->wnd, &rc);
		state->settings->rc = rc;
	}

	void show_page(ProcState* state, ProcState::page p, u32 ShowWindow_cmd /*SW_SHOW,...*/) {
		switch (p) {
		case ProcState::page::landing: for (auto ctl : state->controls.landingpage.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::new_word: for (auto ctl : state->controls.new_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::practice: for (auto ctl : state->controls.practice.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::search: for (auto ctl : state->controls.search.all) ShowWindow(ctl, ShowWindow_cmd); break;
		case ProcState::page::show_word: for (auto ctl : state->controls.show_word.all) ShowWindow(ctl, ShowWindow_cmd); break;
		}
	}

	void set_current_page(ProcState* state, ProcState::page new_page) {
		show_page(state, state->current_page,SW_HIDE);
		state->current_page = new_page;
		resize_controls(state);
		show_page(state, state->current_page,SW_SHOW);
	}

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			decltype(state) st = (decltype(st))calloc(1, sizeof(decltype(*st)));
			Assert(st);
			st->nc_parent = GetParent(hwnd);
			st->wnd = hwnd;
			st->settings = ((べんきょうSettings*)create_nfo->lpCreateParams);
			st->current_page = ProcState::page::landing;
			set_state(hwnd, st);//TODO(fran): now I wonder why I append everything with the name of the wnd when I can simply use function overloading, even for get_state since all my functions do the same I can simple have one
			startup(st);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			add_controls(state);

			set_current_page(state, ProcState::page::landing);//TODO(fran): page:: would be nicer than ProcState::page::

			return 0;
		} break;
		case WM_SIZE:
		{
			LRESULT res = DefWindowProc(hwnd, msg, wparam, lparam);
			resize_controls(state);
			return res;
		} break;
		case WM_CLOSE:
		{
			return 0;
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				save_settings(state);
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
		case WM_ERASEBKGND:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): or return 1, idk
		} break;
		case WM_PAINT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
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
		default:
#ifdef _DEBUG
			Assert(0);
#else 
			return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
		}
		return 0;
	}

	void init_wndclass(HINSTANCE inst) {
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = Proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(ProcState*);
		wcex.hInstance = inst;
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = unCap_colors.ControlBk;
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
