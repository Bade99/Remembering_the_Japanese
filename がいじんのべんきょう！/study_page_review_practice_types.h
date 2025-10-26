#pragma once
namespace study::practice::review {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;

			control_type static_review;
			control_type gridview_practices;
			control_type button_continue;
		};
		control_type all[4];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};

	struct page_state {
		std::vector<practice_header*> practices;
	};
}