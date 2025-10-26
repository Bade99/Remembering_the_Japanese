#pragma once
namespace study::wordbook_all {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;

			//type static_orderby;
			control_type combo_orderby;
			//type static_filterby;
			control_type combo_filterby;

			control_type listbox_words;
		};
		control_type all[4];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};
}