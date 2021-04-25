#pragma once
#include <windef.h>

//Messages that, like window's default messages, can and should if possible be implemented by any type of HWND

#define extramsgs_base_msg_addr (WM_USER + 5000)

#define WM_SETDEFAULTTEXT (extramsgs_base_msg_addr+1) 
// wparam = unused
// lparam = pointer to null terminated cstr
// returns TRUE if the text was set, and FALSE otherwise

#define WM_SETTOOLTIPTEXT (extramsgs_base_msg_addr+2)
// wparam = unused
// lparam = pointer to null terminated cstr
// returns TRUE if the text was set, and FALSE otherwise

#define WM_DESIRED_SIZE (extramsgs_base_msg_addr+3) 
// wparam = pointer to SIZE with maximum bounds, should be modified to represent the minimum SIZE the control can take
// lparam = pointer to SIZE with maximum bounds, should be modified to represent the maximum SIZE the control can take
// returns:
	// 0 : dont care
	// 1 : flexible : the two SIZE params represent the lower and upper bound but also allow for values in between
	// 2 : fixed    : the two SIZE params represent the small and big size and dont allow for values in between

enum desired_size : unsigned int { dontcare = 0, flexible, fixed };
desired_size GetWindowDesiredSize(HWND wnd, SIZE* min, SIZE* max) {
	return (desired_size)SendMessage(wnd, WM_DESIRED_SIZE, (WPARAM)min, (LPARAM)max);
}

struct _desired_size { SIZE min, max; desired_size flexibility; };
_desired_size GetWindowDesiredSize(HWND wnd, SIZE min, SIZE max) {
	_desired_size res;
	res.flexibility = (desired_size)SendMessage(wnd, WM_DESIRED_SIZE, (WPARAM)&min, (LPARAM)&max);
	res.min = min;
	res.max = max;
	return res;
}