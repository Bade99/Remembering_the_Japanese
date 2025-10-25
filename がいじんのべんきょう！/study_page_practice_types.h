#pragma once
namespace べんきょう::practice {
	union page_controls {
		using control_type = HWND;
		struct {
			control_type page;
			control_type settings_button_practices;
			control_type settings_listbox_practices;
			control_type button_start;

			control_type button_words_practiced;//TODO(fran): should simply be a static control
			control_type listbox_words_practiced;
		};
		control_type all[6];
	private: void _() { static_assert(sizeof(all) == sizeof(*this), "Update the array's element count!"); }
	};

	struct page_state {
		bool settings_visibility;
	};


	template <typename DataType, typename ChildType, u64 childCount = 0>
	struct treeview_element {
		u8 level;
		bool open;
		DataType data;
		fixed_array<ChildType, childCount> children;

		void add_child(ChildType child, bool open) {
			child.level = this->level + 1;
			this->children.add(child);
		}

		void build_treeview(std::vector<void*>& treeview, u8 level = -1) {
			this->level = level;
			if (this->open) {
				for (auto& e : this->children) {
					treeview.push_back(&e);
					e.build_treeview(treeview, this->level + 1);
				}
			}
		}
	};
	struct treeview_practice_data {
		u8 type_header;
		u8 practice_type;
	};

	struct dummy_treeview_element {
		void build_treeview(std::vector<void*>& treeview, u8 level) {}
	};

	typedef treeview_element<treeview_practice_data, dummy_treeview_element, 0> treeview_practice_variation;
	typedef treeview_element<treeview_practice_data, treeview_practice_variation, 8> treeview_practice;

	enum treeview_practice_type : u8 {
		treeview_practice_type_root, treeview_practice_type_writing_variant, treeview_practice_type_multiplechoice_variant, treeview_practice_type_drawing_variant
	};


	enum class available_practices : i32 {
		writing = 1 << 0,
		multiplechoice = 1 << 1,
		drawing = 1 << 2,

		_last_bit
	};

	struct practice_header {
		available_practices type;
	};
	struct practice_writing {
		practice_header header;
		practice::writing::word* practice;//#free
		const utf16_str* question;//points to some element inside practice.word
		utf16_str user_answer;//#free
		const utf16_str* correct_answer;//points to some element inside practice.word
		bool answered_correctly;//precalculated value so strcmp is used only once
	};
	struct practice_multiplechoice {
		practice_header header;
		practice::multiplechoice::word* practice;//#free
		size_t user_answer_idx;
		bool answered_correctly;//precalculated value
	};
	struct practice_drawing {
		practice_header header;
		practice::drawing::word* practice;//#free
		HBITMAP user_answer;//#free (DeleteBitmap)
		bool answered_correctly;//precalculated value
	};
}