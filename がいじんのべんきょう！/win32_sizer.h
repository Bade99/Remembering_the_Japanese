#pragma once
#include "windows_sdk.h"
#include "win32_Helpers.h"

#include <vector>
#include <algorithm>

//Vertical resizer: 
//	Has two jobs:
//		- mover: place one window on top of another
//		- sizer: adjust its windows' width to equal the resizer's; and adjust the height if not all can fit


//TODO(fran): lets not create another window, lets simply create a resizer struct that we call whenever we need to resize, also when adding/removing controls from it it will auto resize. it will take charge in moving the objects, it will use getclientrect(or getwindowrect?) to determine the correct size for that wnd

struct sizer {
	//calculates bottom-right point of the bottommost wnd if a resize were to be made
	virtual POINT get_bottom(rect_i32 rc) = 0;
	//returns bottom-right point of the bottommost wnd resized
	virtual POINT resize(rect_i32 rc) = 0;
};

//single (simple) sizer
struct ssizer : sizer {
	HWND wnd;
	ssizer(HWND _wnd) : wnd(_wnd) {}

#define _ssizer_resize(do_resize) \
	POINT xy = {rc.x,rc.y}; \
	int w = rc.w, h = rc.h; \
	if constexpr(do_resize) MoveWindow(wnd, xy.x, xy.y, w, h, FALSE); \
	xy.y += h; \
	xy.x = rc.right(); \
	return xy; \

	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		_ssizer_resize(false);
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		_ssizer_resize(true);
	}
};

//vertical pad sizer (affects y but not x)
struct vpsizer : sizer {
	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		return { rc.left,rc.bottom() };
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		return get_bottom(rc);
	}
};

//horizontal pad sizer (affects x but not y)
struct hpsizer : sizer {
	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		return { rc.right(),rc.top };
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		return get_bottom(rc);
	}
};

//vertical sizer
struct vsizer : sizer{
	//rect_i32 rc;
	//NOTE: int value represents new height of the wnd
	std::vector<std::pair<sizer*, i32>> wnds;

	vsizer(std::initializer_list<std::pair<sizer*, i32>> l) : wnds(l)/*, rc{0}*/ { /*if (l.size()) resize();*/ }

	//vsizer& set_dimensions(rect_i32 _rc) { rc = _rc; return *this; }
	//void add_wnd(HWND wnd) { wnds.push_back(wnd); resize(); }//TODO(fran): adding at index, and adding _after_ some already present HWND
	//void remove_wnd(HWND wnd) { wnds.erase(std::remove(wnds.begin(), wnds.end(), wnd), wnds.end()); resize(); }

#define _vsizer_resize(do_resize) \
	POINT xy = {rc.x,rc.y}; \
	for (auto wnd : wnds) { \
		int w = rc.w, h = wnd.second; \
		rect_i32 newr{xy.x,xy.y,w,h}; \
		if constexpr(do_resize) wnd.first->resize(newr); \
		xy.y += h; \
	} \
	xy.x = rc.right(); \
	return xy; \

	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		_vsizer_resize(false);
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		_vsizer_resize(true);
	}
};

//horizontal sizer
struct hsizer : sizer {
	//rect_i32 rc;
	//NOTE: int value represents new width of the wnd
	std::vector<std::pair<sizer*, i32>> wnds;

	hsizer(std::initializer_list<std::pair<sizer*, i32>> l) : wnds(l)/*, rc{0}*/ { /*if (l.size()) resize();*/ }

	//vsizer& set_dimensions(rect_i32 _rc) { rc = _rc; return *this; }
	//void add_wnd(HWND wnd) { wnds.push_back(wnd); resize(); }//TODO(fran): adding at index, and adding _after_ some already present HWND
	//void remove_wnd(HWND wnd) { wnds.erase(std::remove(wnds.begin(), wnds.end(), wnd), wnds.end()); resize(); }

#define _hsizer_resize(do_resize) \
	POINT res{rc.x,rc.y}; \
	POINT xy = {rc.x,rc.y}; \
	for (auto wnd : wnds) { \
		int w = wnd.second, h = rc.h; \
		rect_i32 newr{xy.x,xy.y,w,h}; \
		if constexpr(do_resize) res.y = maximum(res.y, wnd.first->resize(newr).y); \
		else res.y = maximum(res.y, wnd.first->get_bottom(newr).y); \
		xy.x += w; \
		res.x = xy.x; \
	} \
	return res; \

	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		_hsizer_resize(false);//TODO(fran): returning the maximum instead of the value given to us should be a user setteable thing, that can be defined at object creation time (or close to it)
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		_hsizer_resize(true);
	}
};

//horizontal right aligned sizer, wnds will be placed starting from the right
struct hrsizer : sizer {
	//rect_i32 rc;
	//NOTE: int value represents new width of the wnd
	std::vector<std::pair<sizer*, i32>> wnds;

	hrsizer(std::initializer_list<std::pair<sizer*, i32>> l) : wnds(l)/*, rc{0}*/ { /*if (l.size()) resize();*/ }

	//vsizer& set_dimensions(rect_i32 _rc) { rc = _rc; return *this; }
	//void add_wnd(HWND wnd) { wnds.push_back(wnd); resize(); }//TODO(fran): adding at index, and adding _after_ some already present HWND
	//void remove_wnd(HWND wnd) { wnds.erase(std::remove(wnds.begin(), wnds.end(), wnd), wnds.end()); resize(); }

#define _hrsizer_resize(do_resize) \
	POINT xy = {rc.right(),rc.y}; \
	for (auto wnd : wnds) { \
		int w = wnd.second, h = rc.h; \
		rect_i32 newr{xy.x-w,xy.y,w,h}; \
		if constexpr(do_resize) wnd.first->resize(newr); \
		xy.x -= w; \
	} \
	xy.x = rc.right(); \
	xy.y = rc.bottom(); \
	return xy; \

	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		_hrsizer_resize(false);
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		_hrsizer_resize(true);
	}
};

//horizontal center aligned sizer, wnds will be placed centered relative to the requested rc
struct hcsizer : sizer {
	//rect_i32 rc;
	//NOTE: int value represents new width of the wnd
	std::vector<std::pair<sizer*, i32>> wnds;

	hcsizer(std::initializer_list<std::pair<sizer*, i32>> l) : wnds(l){}

	//void add_wnd(HWND wnd) { wnds.push_back(wnd); resize(); }//TODO(fran): adding at index, and adding _after_ some already present HWND
	//void remove_wnd(HWND wnd) { wnds.erase(std::remove(wnds.begin(), wnds.end(), wnd), wnds.end()); resize(); }

//TODO(fran): we can optimize the !do_resize path by returning right after the first for loop
#define _hcsizer_resize(do_resize) \
	i32 total_w=0; \
	for (auto wnd : wnds) { \
		int w = wnd.second, h = rc.h; \
		rect_i32 newr{0,0,w,h}; \
		total_w += wnd.first->get_bottom(newr).x; \
	} \
	POINT xy = {rc.x + (rc.w-total_w)/2,rc.y}; \
	for (auto wnd : wnds) { \
		int w = wnd.second, h = rc.h; \
		rect_i32 newr{ xy.x,xy.y,w,h }; \
		if constexpr (do_resize) wnd.first->resize(newr); \
		xy.x += w; \
	} \
	xy.y = rc.bottom(); \
	return xy; \

	//calculates bottom-right point of the bottommost wnd if a resize was made
	POINT get_bottom(rect_i32 rc) override {
		_hcsizer_resize(false);
	}
	//returns bottom-right point of the bottommost wnd resized
	POINT resize(rect_i32 rc) override {
		_hcsizer_resize(true);
	}
};

//TODO(fran): hcsizer and vcsizer (center aligned resizers, all its controls will be placed centered relative to the requested rc dimensions)

//TODO(fran): we need two extra msgs to be sendeable to any HWND, WM_H_FOR_W (we provide a height you tell us how wide you need to be) and WM_W_FOR_H (we provide a width you tell us how high you need to be)

//TODO(fran): I need sizers to be able to contain sizers inside, for this we need to move to them actually also being HWNDs, or we could go the other way, put single HWNDs into sizers
