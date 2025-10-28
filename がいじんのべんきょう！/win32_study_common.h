#pragma once
//TODO(fran): we probably want this things inside the 'study' namespace

HBRUSH brush_for(learnt_word_elem type) {
	HBRUSH res{ 0 };//NOTE: compiler cant know this will always be initialized so I gotta zero it
	switch (type) {
	case decltype(type)::hiragana: res = global::colors.hiragana; break;
	case decltype(type)::kanji: res = global::colors.kanji; break;
	case decltype(type)::meaning: res = global::colors.meaning; break;
	default: Assert(0);
	}
	return res;
};

int draw_bitmap_1bpp(HBITMAP bmp, HDC dc, rect_i32 r, int x_pad, HBRUSH color = global::colors.Img) {
	//TODO(fran): flicker free
	BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
	Assert(bitmap.bmBitsPixel == 1);
	int max_sz = roundNdown(bitmap.bmWidth, (int)((f32)r.h * .6f)); //HACK: instead use png + gdi+ + color matrices
	if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

	int bmp_height = max_sz;
	int bmp_width = bmp_height;
	int bmp_align_width = r.left + r.w - bmp_width - x_pad;
	int bmp_align_height = r.top + (r.h - bmp_height) / 2;
	urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, color);

	return bmp_align_width;
}

template <typename T>
constexpr multiflag<T> get_filled_multiflag() { return (1u << (get_last_bit_set_position_slow((u32)T::_last_bit) + 1)) - 1u; }

template <typename T>
constexpr u32 get_enumflag_element_count() { return (u32)popcnt64(get_filled_multiflag<T>()); }

/// <summary>
/// Centers the elements in the page, or switches to normal alignment if the page is too small to center the elements into
/// </summary>
/// <param name="h">Height of the page</param>
/// <param name="h_pad">Standard vertical padding</param>
/// <param name="used_h">Total height used by the page's controls</param>
/// <param name="y">Initial y position of the controls</param>
/// <param name="y">Place to store the true final used height</param>
void get_page_elements_centering(i32 h, i32 h_pad, i32 used_h, i32* y, i32* final_h) {
	if (used_h <= h) {
		//Vertically center the whole of the controls
		*y = (h - used_h) / 2;
	}
	else {
		//If the page is too small do not do vertical centering, just add some padding on the top and bottom
		*y = h_pad; 
		*final_h = used_h + *y * 2;
	}
}