#pragma once
namespace べんきょう::landing {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;//parent of all other controls

			//type candy;

			control_type button_recents;
			control_type listbox_recents;

			control_type static_word_cnt_title;
			control_type static_word_cnt;

			control_type static_practice_cnt_title;
			control_type static_practice_cnt;

			control_type static_accuracy_title;
			control_type score_accuracy;

			control_type static_accuracy_timeline_title;
			control_type graph_accuracy_timeline;
		};
		control_type all[11]; //NOTE: make sure you understand structure padding before implementing this, also this should be re-tested if trying with different compilers or alignment
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};

	struct page_state {
		bool hide_recents; // Hide or show the 'recently added words' listbox
	};
}