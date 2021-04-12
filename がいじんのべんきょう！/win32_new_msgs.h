#pragma once

//Messages that, like window's default messages, can and should if possible be implemented by any type of HWND

#define extramsgs_base_msg_addr (WM_USER + 5000)

#define WM_SETDEFAULTTEXT (extramsgs_base_msg_addr+1) /*wparam=unused ; lparam=pointer to null terminated cstr*/ /*returns TRUE if the text was set, and FALSE otherwise*/
#define WM_SETTOOLTIPTEXT (extramsgs_base_msg_addr+2) /*wparam=unused ; lparam=pointer to null terminated cstr*/ /*returns TRUE if the text was set, and FALSE otherwise*/
