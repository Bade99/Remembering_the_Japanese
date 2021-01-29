#pragma once
#include "windows_sdk.h"
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"

constexpr cstr app_name[] = L"がいじんのべんきょう！";

union UNCAP_COLORS {//TODO(fran): HBRUSH Border
	struct {
		//TODO(fran): add _disabled for at least txt,bk,border
#define foreach_color(op) \
		op(HBRUSH,ControlBk,CreateSolidBrush(RGB(40, 41, 35))) \
		op(HBRUSH,ControlBkPush,CreateSolidBrush(RGB(0, 110, 200))) \
		op(HBRUSH,ControlBkMouseOver,CreateSolidBrush(RGB(0, 120, 215))) \
		op(HBRUSH,ControlTxt,CreateSolidBrush(RGB(248, 248, 242))) \
		op(HBRUSH,ControlTxt_Inactive,CreateSolidBrush(RGB(208, 208, 202))) \
		op(HBRUSH,ControlMsg,CreateSolidBrush(RGB(248, 230, 0))) \
		op(HBRUSH,Scrollbar,CreateSolidBrush(RGB(148, 148, 142))) \
		op(HBRUSH,ScrollbarMouseOver,CreateSolidBrush(RGB(188, 188, 182))) \
		op(HBRUSH,ScrollbarBk,CreateSolidBrush(RGB(50, 51, 45))) \
		op(HBRUSH,Img,CreateSolidBrush(RGB(228, 228, 222))) \
		op(HBRUSH,Img_Inactive,CreateSolidBrush(RGB(198, 198, 192))) \
		op(HBRUSH,CaptionBk,CreateSolidBrush(RGB(20, 21, 15))) \
		op(HBRUSH,CaptionBk_Inactive,CreateSolidBrush(RGB(60, 61, 65))) \
		op(HBRUSH,ControlBk_Disabled,CreateSolidBrush(RGB(35, 36, 30))) \
		op(HBRUSH,ControlTxt_Disabled,CreateSolidBrush(RGB(128, 128, 122))) \
		op(HBRUSH,Img_Disabled,CreateSolidBrush(RGB(98, 98, 92))) \
		op(HBRUSH,Search_Bk,CreateSolidBrush(RGB(30, 31, 25))) \
		op(HBRUSH,Search_BkPush,CreateSolidBrush(RGB(0, 120, 210))) \
		op(HBRUSH,Search_BkMouseOver,CreateSolidBrush(RGB(0, 130, 225))) \
		op(HBRUSH,Search_Txt,CreateSolidBrush(RGB(238, 238, 232))) \
		op(HBRUSH,Search_Edit_Bk,CreateSolidBrush(RGB(60, 61, 65))) \
		op(HBRUSH,Search_Edit_Txt,CreateSolidBrush(RGB(248, 248, 242))) \
		op(HBRUSH,Search_BkSelected,CreateSolidBrush(RGB(60, 61, 55))) \

		foreach_color(_generate_member_no_default_init);

	};
	HBRUSH all[0 + foreach_color(_generate_count)];

	_generate_default_struct_serialize(foreach_color);

	_generate_default_struct_deserialize(foreach_color);
};

static void default_colors_if_not_set(UNCAP_COLORS* c) {
#define _default_initialize(type, name,value) if(!c->name) c->name = value;
	foreach_color(_default_initialize);
#undef _default_initialize
}

extern UNCAP_COLORS unCap_colors;

union UNCAP_FONTS {
	struct {
		HFONT General;
		HFONT Menu;
	};
	HFONT all[2];//REMEMBER to update

	//NOTE: this checks for the lower bound, if all is bigger than the elements the assert wont trigger, I havent yet found a way to do the correct check, which would be against the struct, without having to put a name to it and thus destroying the whole point of it
private: void _(){ static_assert(sizeof(all) == sizeof(*this), "Come here and update the array to the correct element count!"); }
};

extern UNCAP_FONTS unCap_fonts;

//TODO(fran): now we could do the same we do with langs and colors with the bitmaps, though we really need some sort of version checking, say the user wants to change the bitmaps, they wont be able to if we write them to disk each time
//	one solution can be to write the version say on the first 16 characters, eg 0000000000000001, with that we can check, and also we could add a no_update flag, eg 00000dont_update so the user can forget about getting their imgs overridden
union UNCAP_BMPS { //1bpp 16x16 bitmaps
	struct {
		HBITMAP close;
		HBITMAP maximize;
		HBITMAP minimize;
		HBITMAP arrow_right;
		HBITMAP tick;
		HBITMAP dropdown;
		HBITMAP circle;
		HBITMAP bin;
	};
	HBITMAP all[8];//REMEMBER to update

private: void _() { static_assert(sizeof(all) == sizeof(*this), "Come here and update the array to the correct element count!"); }
};

extern UNCAP_BMPS unCap_bmps;