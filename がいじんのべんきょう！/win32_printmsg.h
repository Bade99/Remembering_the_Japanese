#pragma once
#include "windows_sdk.h"
#include "windows_msg_mapper.h"
#include <CommCtrl.h>

//--------------Philosophy--------------:
//Simple message logger for any window, say you dont have access to the wndproc but want to know which msgs are getting sent/received, sometimes spy++ can save you, sometimes it cant, that's why this exists, a very quick one-line-setup logger

//--------------Usage--------------:
//SetWindowSubclass(someone's hwnd, PrintMsgProc, 0, (DWORD_PTR)"Combo List"); 
	//The last paramater must be an ASCII string and will be printed along with the rest of the info for each msg


LRESULT CALLBACK PrintMsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
	{ static int msg_count; printf("%s: %d : %s\n", dwRefData? (char*)dwRefData : "_", msg_count++, msgToString(msg)); }
	return DefSubclassProc(hwnd, msg, wparam, lparam);
}