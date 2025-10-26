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