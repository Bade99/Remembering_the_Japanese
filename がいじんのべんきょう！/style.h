#pragma once
//TODO: maybe this variables could go inside global::styles
constexpr DWORD style_button_txt = WS_CHILD | WS_TABSTOP | button::style::roundrect;
constexpr DWORD style_button_bmp = WS_CHILD | WS_TABSTOP | button::style::roundrect | BS_BITMAP;
constexpr DWORD style_button_icon = WS_CHILD | WS_TABSTOP | button::style::roundrect | BS_ICON;
static button::Theme base_btn_theme;
static button::Theme img_btn_theme;
static button::Theme accent_btn_theme;
static static_oneline::Theme base_static_theme;
static static_oneline::Theme kanji_static_theme;
static navbar::Theme nav_theme;
static navbar::Theme sidebar_theme;
static button::Theme navbar_btn_theme;
static button::Theme navbar_img_btn_theme;
static button::Theme dark_btn_theme;
static button::Theme dark_nonclickable_btn_theme;
static embedded::show_word_reduced::Theme eswr_theme;
static embedded::show_word_disambiguation::Theme eswd_theme;
static edit_oneline::Theme base_editoneline_theme;
static edit_oneline::Theme hiragana_editoneline_theme;
static edit_oneline::Theme kanji_editoneline_theme;
static edit_oneline::Theme meaning_editoneline_theme;
static page::Theme base_page_theme;

void load_styles() {
	base_btn_theme = []()->auto {
		button::Theme base_btn_theme;
		base_btn_theme.dimensions.border_thickness = 1;
		base_btn_theme.brushes.bk.normal = global::colors.ControlBk;
		base_btn_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
		base_btn_theme.brushes.bk.clicked = global::colors.ControlBkPush;
		base_btn_theme.brushes.bk.mouseover = global::colors.ControlBkMouseOver;
		base_btn_theme.brushes.foreground.normal = global::colors.ControlTxt;
		base_btn_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
		base_btn_theme.brushes.border.normal = global::colors.Img;//TODO(fran): global::colors.ControlBorder
		//TODO(fran): use the extra brushes, fore_push,... , border_mouseover,...
		return base_btn_theme;
		}();

	img_btn_theme = [&]()->auto {
		auto img_btn_theme = base_btn_theme;
		img_btn_theme.brushes.foreground.normal = global::colors.Img;
		return img_btn_theme;
		}();

	accent_btn_theme = [&]()->auto {
		auto accent_btn_theme = base_btn_theme;
		accent_btn_theme.brushes.foreground.normal = global::colors.Accent;
		accent_btn_theme.brushes.border.normal = global::colors.Accent;
		return accent_btn_theme;
		}();

	base_static_theme = [&]()->auto {
		static_oneline::Theme base_static_theme;
		base_static_theme.brushes.foreground.normal = global::colors.ControlTxt;
		base_static_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
		base_static_theme.brushes.bk.normal = global::colors.ControlBk;
		base_static_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
		return base_static_theme;
		}();

	kanji_static_theme = [&]()->auto {
		auto kanji_static_theme = base_static_theme;
		kanji_static_theme.brushes.foreground.normal = brush_for(learnt_word_elem::kanji);
		return kanji_static_theme;
		}();

	nav_theme = [&]()->auto {
		navbar::Theme nav_theme;
		nav_theme.brushes.bk.normal = global::colors.ControlBk_Disabled;//TODO(fran): darker color than bk
		nav_theme.dimensions.spacing = 3;
		nav_theme.dimensions.is_vertical = false;
		return nav_theme;
		}();

	sidebar_theme = [&]()->auto {
		auto sidebar_theme = nav_theme;
		sidebar_theme.dimensions.spacing = 0;
		sidebar_theme.dimensions.is_vertical = true;
		return sidebar_theme;
		}();

	navbar_btn_theme = [&]()->auto {
		auto navbar_btn_theme = base_btn_theme;
		navbar_btn_theme.brushes.bk.normal = nav_theme.brushes.bk.normal;
		navbar_btn_theme.brushes.border = navbar_btn_theme.brushes.bk;
		return navbar_btn_theme;
		}();

	navbar_img_btn_theme = [&]()->auto {
		auto navbar_img_btn_theme = img_btn_theme;
		navbar_img_btn_theme.brushes.bk.normal = nav_theme.brushes.bk.normal;
		navbar_img_btn_theme.brushes.border = navbar_img_btn_theme.brushes.bk;
		return navbar_img_btn_theme;
		}();

	dark_btn_theme = [&]()->auto {
		auto dark_btn_theme = base_btn_theme;
		dark_btn_theme.brushes.bk.normal = global::colors.ControlBk_Dark;
		return dark_btn_theme;
		}();

	dark_nonclickable_btn_theme = [&]()->auto {
		auto dark_nonclickable_btn_theme = dark_btn_theme;
		for (auto& b : dark_nonclickable_btn_theme.brushes.bk.all) b = dark_nonclickable_btn_theme.brushes.bk.normal;
		for (auto& b : dark_nonclickable_btn_theme.brushes.border.all) b = dark_nonclickable_btn_theme.brushes.border.normal;
		return dark_nonclickable_btn_theme;
		}();

	eswr_theme = [&]()->auto {
		embedded::show_word_reduced::Theme eswr_theme;
		eswr_theme.font = global::fonts.General;
		eswr_theme.dimensions.border_thickness = 1;
		eswr_theme.brushes.bk.normal = global::colors.ControlBk;
		eswr_theme.brushes.txt.normal = global::colors.ControlTxt;
		eswr_theme.brushes.border.normal = global::colors.ControlTxt;
		return eswr_theme;
		}();

	eswd_theme = [&]()->auto {
		embedded::show_word_disambiguation::Theme eswd_theme;
		eswd_theme.font = global::fonts.General;
		eswd_theme.dimensions.border_thickness = 1;
		eswd_theme.brushes.bk.normal = global::colors.ControlBk;
		eswd_theme.brushes.txt.normal = global::colors.ControlTxt;
		eswd_theme.brushes.border.normal = global::colors.ControlTxt;
		return eswd_theme;
		}();

	base_editoneline_theme = [&]()->auto {
		edit_oneline::Theme base_editoneline_theme;
		base_editoneline_theme.dimensions.border_thickness = 1;
		base_editoneline_theme.brushes.foreground.normal = global::colors.ControlTxt;
		base_editoneline_theme.brushes.foreground.disabled = global::colors.ControlTxt_Disabled;
		base_editoneline_theme.brushes.bk.normal = global::colors.ControlBk;
		base_editoneline_theme.brushes.bk.disabled = global::colors.ControlBk_Disabled;
		base_editoneline_theme.brushes.border.normal = global::colors.Img;
		base_editoneline_theme.brushes.border.disabled = global::colors.Img_Disabled;
		base_editoneline_theme.brushes.selection.normal = global::colors.Selection;
		base_editoneline_theme.brushes.selection.disabled = global::colors.Selection_Disabled;
		return base_editoneline_theme;
		}();

	hiragana_editoneline_theme = [&]()->auto {
		auto hiragana_editoneline_theme = base_editoneline_theme;
		hiragana_editoneline_theme.brushes.foreground.normal = brush_for(learnt_word_elem::hiragana);
		return hiragana_editoneline_theme;
		}();

	kanji_editoneline_theme = [&]()->auto {
		auto kanji_editoneline_theme = base_editoneline_theme;
		kanji_editoneline_theme.brushes.foreground.normal = brush_for(learnt_word_elem::kanji);
		return kanji_editoneline_theme;
		}();

	meaning_editoneline_theme = [&]()->auto {
		auto meaning_editoneline_theme = base_editoneline_theme;
		meaning_editoneline_theme.brushes.foreground.normal = brush_for(learnt_word_elem::meaning);
		return meaning_editoneline_theme;
		}();

	base_page_theme = [&]()->auto {
		page::Theme base_page_theme;
		base_page_theme.brushes.bk.normal = global::colors.ControlBk;
		base_page_theme.brushes.border = base_page_theme.brushes.bk;
		base_page_theme.dimensions.border_thickness = 0;
		return base_page_theme;
		}();
}