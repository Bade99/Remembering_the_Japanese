#pragma once
#include "windows_sdk.h"
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"

namespace global {

	constexpr cstr app_name[] = L"がいじんのべんきょう！";

	union _colors {//TODO(fran): HBRUSH Border
		struct {
			//TODO(fran): add _disabled for at least txt,bk,border
#define foreach_color(op) \
		op(HBRUSH,ControlBk,CreateSolidBrush(RGB(40, 41, 35))) \
		op(HBRUSH,ControlBk_Light,CreateSolidBrush(RGB(70, 71, 65))) \
		op(HBRUSH,ControlBk_Dark,CreateSolidBrush(RGB(20, 21, 25))) \
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
		op(HBRUSH,Score_RingBk,CreateSolidBrush(RGB(20, 21, 35))) \
		op(HBRUSH,Score_RingFull,CreateSolidBrush(RGB(20, 243, 35))) \
		op(HBRUSH,Score_RingEmpty,CreateSolidBrush(RGB(243, 21, 35))) \
		op(HBRUSH,Score_InnerCircle,CreateSolidBrush(RGB(50, 51, 55))) \
		op(HBRUSH,Graph_Line,CreateSolidBrush(RGB(99, 176, 214))) \
		op(HBRUSH,Graph_BkUnderLine,CreateSolidBrush(RGB(65, 103, 120))) \
		op(HBRUSH,Graph_Bk,CreateSolidBrush(RGB(20, 21, 30))) \
		op(HBRUSH,Graph_Border,CreateSolidBrush(RGB(40, 41, 35))) \
		op(HBRUSH,hiragana,CreateSolidBrush(RGB(144, 115, 153))) \
		op(HBRUSH,kanji,CreateSolidBrush(RGB(208, 125, 0))) \
		op(HBRUSH,translation,CreateSolidBrush(RGB(192, 99, 108))) \
		op(HBRUSH,Bk_right_answer,CreateSolidBrush(RGB(23, 206, 44))) \
		op(HBRUSH,BkMouseover_right_answer,CreateSolidBrush(RGB(13, 186, 24))) \
		op(HBRUSH,BkPush_right_answer,CreateSolidBrush(RGB(3, 166, 4))) \
		op(HBRUSH,Bk_wrong_answer,CreateSolidBrush(RGB(206, 23, 44))) \
		op(HBRUSH,BkMouseover_wrong_answer,CreateSolidBrush(RGB(186, 13, 24))) \
		op(HBRUSH,BkPush_wrong_answer,CreateSolidBrush(RGB(166, 3, 4))) \
		op(HBRUSH,Selection,CreateSolidBrush(RGB(0, 120, 215))) \
		op(HBRUSH,Selection_Disabled,CreateSolidBrush(RGB(200, 200, 200))) \
		op(HBRUSH,Accent,CreateSolidBrush(RGB(218, 125, 0))) \

		//TODO(fran): not sure which color to choose for kanji, I use black on my studies but that wont be too visible while we keep the gray bk

			foreach_color(_generate_member_no_default_init);

		};
		HBRUSH all[0 + foreach_color(_generate_count)];

		_generate_default_struct_serialize(foreach_color);

		_generate_default_struct_deserialize(foreach_color);
	}static colors;

	static void default_colors_if_not_set(_colors* c) {
#define _default_initialize(type, name,value) if(!c->name) c->name = value;
		foreach_color(_default_initialize);
#undef _default_initialize
	}

	union _fonts {
		struct {
			HFONT General;
			HFONT Menu;
			HFONT Caption;
		};
		HFONT all[3];//REMEMBER to update

		//NOTE: this checks for the lower bound, if all is bigger than the elements the assert wont trigger, I havent yet found a way to do the correct check, which would be against the struct, without having to put a name to it and thus destroying the whole point of it
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Come here and update the array to the correct element count!"); }
	}static fonts;


	//TODO(fran): now we could do the same we do with langs and colors with the bitmaps, though we really need some sort of version checking, say the user wants to change the bitmaps, they wont be able to if we write them to disk each time
//	one solution can be to write the version say on the first 16 characters, eg 0000000000000001, with that we can check, and also we could add a no_update flag, eg 00000dont_update so the user can forget about getting their imgs overridden
	union _bmps { //1bpp 16x16 bitmaps
		struct {
			HBITMAP close;
			HBITMAP maximize;
			HBITMAP minimize;
			HBITMAP arrow_right;
			HBITMAP tick;
			HBITMAP dropdown;
			HBITMAP circle;
			HBITMAP bin;
			HBITMAP arrowLine_left;
			HBITMAP arrowSimple_right;
			HBITMAP eye;
		};
		HBITMAP all[11];//REMEMBER to update

	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Come here and update the array to the correct element count!"); }
	}static bmps;
}