#pragma once
#include "windows_sdk.h"
#include <CommCtrl.h> //DefSubclassProc

//This is a window subclass to be used with anything that functions with WM_SETTEXT, it's objective is to work as a quick notification. The window remains hidden until its text is updated via WM_SETTEXT, at that point the window is shown for the lenght of time specified in NOTIFY_SETTEXTDURATION before being hidden again awaiting for the next WM_SETTEXT

#define NOTIFY_SETTEXTDURATION (WM_USER+7600) //Sets the lenght of time the window will be shown after each text update. wParam=(UINT)duration in milliseconds ; lParam = unused

void Notify_SetTextDuration(HWND wnd, UINT duration) {
	SendMessage(wnd, NOTIFY_SETTEXTDURATION, duration, 0);
}

/// <summary>
/// Shows and hides text on a control that receives WM_SETTEXT messages
/// <para>You can set the duration of text on the control by sending</para>
/// <para>UNCAP_SETTEXTDURATION message with wParam=(UINT)duration in milliseconds</para>
/// <para>When the desired time has elapsed the control is hidden</para>
/// <para>If WM_SETTEXT is sent with an empty string then the control is hidden</para>
/// <para>If UNCAP_SETTEXTDURATION is sent with wParam=0 then the text will not be hidden</para>
/// </summary>
/// <param name="uIdSubclass">Not used</param>
/// <param name="dwRefData">Not used</param>
LRESULT CALLBACK NotifyProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {
	//INFO: for this procedure and its hwnds we are going to try the SetProp method for storing info in the hwnd
	TCHAR text_duration[] = TEXT("notify_textduration");
	constexpr int notify_timer_id = 55;
	switch (Msg)
	{
	case NOTIFY_SETTEXTDURATION:
	{
		SetProp(hwnd, text_duration, (HANDLE)wParam);
		return TRUE;
	}
	case WM_TIMER: {
		KillTimer(hwnd, notify_timer_id);//Stop timer, otherwise it keeps sending messages
		ShowWindow(hwnd, SW_HIDE);
		break;
	}
	case WM_SETTEXT:
	{
		WCHAR* text = (WCHAR*)lParam;

		if (text[0] != L'\0') { //We have new text to process

			//Calculate minimum size of the control for the new text. TODO(fran): should we do this or just say that the control is as long as it can be?

			//...

			//Start Timer for text removal: this messages will be shown for a little period of time before the control is hidden again
			UINT msgDuration = (UINT)(UINT_PTR)GetProp(hwnd, text_duration);	//Retrieve the user defined duration of text on screen
																	//If this value was not set then the timer is not used and the text remains on screen
			if (msgDuration)
				SetTimer(hwnd, notify_timer_id, msgDuration, NULL); //By always setting the second param to 1 we are going to override any existing timer

			ShowWindow(hwnd, SW_SHOW); //Show the control with its new text
		}
		else { //We want to hide the control and clear whatever text is in it
			ShowWindow(hwnd, SW_HIDE);
		}

		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	case WM_DESTROY:
	{
		//Cleanup
		RemoveProp(hwnd, text_duration);
		return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	default: return DefSubclassProc(hwnd, Msg, wParam, lParam);
	}
	return 0;
}