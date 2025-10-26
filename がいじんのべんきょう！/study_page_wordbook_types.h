#pragma once
namespace study::wordbook {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;
			//TODO(fran): sorting options
			control_type listbox_last_days_words[4];
			control_type button_all_words;
		};
		control_type all[6];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};
}