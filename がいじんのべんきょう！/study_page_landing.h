#pragma once
namespace べんきょう {
	namespace landing {
		void preload_page(ProcState* state, page_controls& controls, user_stats* stats) {
			constexpr bool test_stats = false;
			if constexpr (!test_stats) {
				SendMessage(controls.score_accuracy, SC_SETSCORE, f32_to_WPARAM(stats->accuracy()), 0);

				state->pageanim.word_count.origin = 0;
				state->pageanim.word_count.dest = stats->word_cnt;
				state->pageanim.word_count.wnd = controls.static_word_cnt;
				animate_number_range(&state->pageanim.word_count, state->pageanim.word_count.dest > 100 ? 1000 : 500);
				//SendMessage(controls.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->word_cnt).c_str());
				state->pageanim.practice_count.origin = 0;
				state->pageanim.practice_count.dest = stats->times_practiced;
				state->pageanim.practice_count.wnd = controls.static_practice_cnt;
				animate_number_range(&state->pageanim.practice_count, state->pageanim.practice_count.dest > 100 ? 1000 : 500);
				//SendMessage(controls.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(stats->times_practiced).c_str());
				//TODO(fran): timeline, we'll probably need to store that as blob or text in the db, this is were mongodb would be nice, just throw a js obj for each timepoint
				//TODO(fran): if the timeline is empty we should simply put the current accuracy, or leave it empty idk
				graph::set_points(controls.graph_accuracy_timeline, stats->accuracy_timeline.mem, stats->accuracy_timeline.cnt); //TODO(fran): change the accuracy graph into a per practice percentage of correct answers graph (I dont think the accuracy graph is too useful, we could keep it or throw it away)
				graph::graph_dimensions grid_dims;
				grid_dims.set_top_point(100);
				grid_dims.set_bottom_point(0);
				grid_dims.set_viewable_points_range(0, stats->accuracy_timeline.cnt);
				graph::set_dimensions(controls.graph_accuracy_timeline, grid_dims);
			}
			else {
				SendMessage(controls.score_accuracy, SC_SETSCORE, f32_to_WPARAM(.6f), 0);
				SendMessage(controls.static_word_cnt, WM_SETTEXT, 0, (LPARAM)to_str(1452).c_str());
				SendMessage(controls.static_practice_cnt, WM_SETTEXT, 0, (LPARAM)to_str(559).c_str());
				i32 accu[]{ 77,56,32,12,48,95,65,32,54,67,79,88,100 };
				graph::set_points(controls.graph_accuracy_timeline, accu, ARRAYSIZE(accu));
				graph::graph_dimensions grid_dims;
				grid_dims.set_top_point(100);
				grid_dims.set_bottom_point(0);
				grid_dims.set_viewable_points_range(0, ARRAYSIZE(accu));
				graph::set_dimensions(controls.graph_accuracy_timeline, grid_dims);
			}
		}

		void set_current_page(ProcState* state) {
			//The 'Recently Added' listbox needs to have elements added to it, TODO(fran): this could be handled on smth like func_on_wm_show
			//TODO(fran): this should only be done if there was a change on the db since the last time it was called

			auto& controls = state->pages.landing;

			//TODO(fran): date information requested from the db or sent to it should all be in local time, the db can bother with storing gmt and doing the appropiate conversions, but the application shouldnt have to even bother with that
			auto [start, end] = day_range(get_latest_word_creation_date(state->settings->db));

			ptr<learnt_word16> recents = get_learnt_word_by_date(state->settings->db, start, end);

			//TODO(fran): this idea of returning an array and then having to add an extra array in order to be able to use it is pretty annoying, yeah it's faster and scales really well, but it's beyond confusing
			ptr<void*> elems{ 0 }; elems.alloc(recents.cnt); defer{ elems.free(); };
			for (size_t i = 0; i < recents.cnt; i++) elems[i] = &recents[i];

			{//Free previous elements
				ptr<void*> elements = listbox::get_all_elements(controls.listbox_recents);//HACK
				for (auto e : elements) ((decltype(recents.mem))e)->free();
				if (elements.cnt)free(elements[0]);
			}

			listbox::set_elements(controls.listbox_recents, elems.mem, elems.cnt);

			EnableWindow(controls.button_recents, (BOOL)recents.cnt);


			user_stats stats = get_user_stats(state->settings->db);
			get_user_stats_accuracy_timeline(state->settings->db, &stats, 30); defer{ stats.accuracy_timeline.free(); };
			preload_page(state, ProcState::page::landing, &stats);//TODO(fran): this seems dumb, I either initialize everything here or on preload_page, but not in both
		}

		void create_page(ProcState* state) {
			auto& controls = state->pages.landing;

			controls.page = create_empty_page(state, base_page_theme);

			auto page = controls.page;

#if 0
			//NOTE: for this to be usable as a page background we need to work with alpha in order to do proper transparency on everything that goes on top of this
			controls.candy = CreateWindowW(eyecandy::wndclass, 0, WS_CHILD | WS_CLIPCHILDREN
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			eyecandy::Theme candy_theme;
			candy_theme.brushes.bk.normal = global::colors.ControlBk;//global::colors.Bk_wrong_answer
			candy_theme.brushes.foreground.normal = global::colors.Img;
			candy_theme.dimensions.border_thickness = 0;
			eyecandy::set_theme(controls.candy, &candy_theme);
			eyecandy::set_db(controls.candy, state->settings->db);
			page = controls.candy;
#endif

			controls.listbox_recents = CreateWindowW(listbox::wndclass, 0, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			listbox::set_function_render(controls.listbox_recents, listbox_recents_func_render);
			listbox::set_user_extra(controls.listbox_recents, state);
			listbox::set_function_on_click(controls.listbox_recents, [](void* element, void* user_extra) {
				ProcState* state = (decltype(state))user_extra;
				learnt_word16* txt = (decltype(txt))element;

				stored_word16_res res = get_stored_word(state->settings->db, *txt/*TODO(fran): make sure this isnt a copy*/);  defer{ if (res.found) free_stored_word(res.word); };
				if (res.found) {
					preload_page(state, ProcState::page::show_word, &res.word);
					store_previous_page(state, state->current_page);
					set_current_page(state, ProcState::page::show_word);
				}
				//TODO(fran): else {notify user of error finding the word}, we need to get good error info from the db functions
				}
			);

			controls.button_recents = CreateWindowW(button::wndclass, NULL, style_button_txt
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.button_recents, 103);
			button::set_theme(controls.button_recents, &dark_btn_theme);
			button::set_user_extra(controls.button_recents, state);
			button::set_function_render(controls.button_recents, button_recents_func_render);
			button::set_function_on_click(controls.button_recents,
				[](void* element, void* user_extra) {
					ProcState* state = (decltype(state))user_extra;
					HWND listbox = state->pages.landing.listbox_recents;
					state->pagestate.landing.hide_recents = !state->pagestate.landing.hide_recents;
					//flip_visibility(listbox);
					ask_for_resize(state);
					ask_for_repaint(state);
					force_repaint(listbox);
				}
			);

			controls.static_word_cnt_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_word_cnt_title, 351);
			static_oneline::set_theme(controls.static_word_cnt_title, &base_static_theme);

			controls.static_word_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_word_cnt, &base_static_theme);

			controls.static_practice_cnt_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_practice_cnt_title, 352);
			static_oneline::set_theme(controls.static_practice_cnt_title, &base_static_theme);

			controls.static_practice_cnt = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER | SO_AUTOFONTSIZE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			static_oneline::set_theme(controls.static_practice_cnt, &base_static_theme);

			controls.static_accuracy_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_accuracy_title, 353);
			static_oneline::set_theme(controls.static_accuracy_title, &base_static_theme);

			controls.score_accuracy = CreateWindowW(score::wndclass, NULL, WS_CHILD
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			score::set_brushes(controls.score_accuracy, FALSE, global::colors.ControlBk, global::colors.Score_RingBk, global::colors.Score_RingFull, global::colors.Score_RingEmpty, global::colors.Score_InnerCircle);


			controls.static_accuracy_timeline_title = CreateWindowW(static_oneline::wndclass, NULL, WS_CHILD | SS_CENTERIMAGE | SS_CENTER
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			AWT(controls.static_accuracy_timeline_title, 354);
			static_oneline::set_theme(controls.static_accuracy_timeline_title, &base_static_theme);

			controls.graph_accuracy_timeline = CreateWindowW(graph::wndclass, NULL, WS_CHILD | GP_CURVE
				, 0, 0, 0, 0, page, 0, NULL, NULL);
			graph::set_brushes(controls.graph_accuracy_timeline, FALSE, global::colors.Graph_Line, global::colors.Graph_BkUnderLine, global::colors.Graph_Bk, global::colors.Graph_Border);

			//TODO(fran): we may want to add smth else like a total number of words practiced

			for (auto ctl : controls.all) SendMessage(ctl, WM_SETFONT, (WPARAM)global::fonts.General, TRUE);
		}

		void layout_page(ProcState* state, i32 w, i32 half_w, i32 w_pad, i32 max_w, i32 h, i32 wnd_h, i32 half_wnd_h, i32 h_pad, i32 page_space_h) {
			auto& controls = state->pages.landing;

			int start_y = 0/*(i32)state->scroll*/;

			rect_i32 button_recents;
			button_recents.y = start_y;
			button_recents.h = wnd_h;
			button_recents.w = min(max_w, avg_str_dim(GetWindowFont(controls.button_recents), 40).cx);
			button_recents.x = (w - button_recents.w) / 2;

			rect_i32 listbox_recents;
			listbox_recents.y = button_recents.bottom();
			listbox_recents.h = state->pagestate.landing.hide_recents ? 0 : wnd_h * (int)listbox::get_element_cnt(controls.listbox_recents);
			listbox_recents.w = button_recents.w;
			listbox_recents.x = (w - listbox_recents.w) / 2;

			int grid_h = wnd_h * 4;
			int grid_w = grid_h * 16 / 9;
			auto grid = create_grid_2x2(grid_h, grid_w, listbox_recents.bottom() + h_pad, w_pad / 2, h_pad / 2, max_w, w);

			rect_i32 cell;

			//First cell
			cell = grid[0][0];
			rect_i32 static_word_cnt_title;
			static_word_cnt_title.w = cell.w;
			static_word_cnt_title.h = min(wnd_h, cell.h);
			static_word_cnt_title.x = cell.center_x() - static_word_cnt_title.w / 2;
			static_word_cnt_title.y = cell.top;

			//NOTE: the values should use a much bigger font
			rect_i32 static_word_cnt;
			static_word_cnt.w = cell.w;
			static_word_cnt.x = cell.center_x() - static_word_cnt.w / 2;
			static_word_cnt.h = distance(cell.bottom(), static_word_cnt_title.bottom());
			static_word_cnt.y = cell.bottom() - static_word_cnt.h;

			//Second cell
			cell = grid[0][1];
			rect_i32 static_practice_cnt_title;
			static_practice_cnt_title.w = cell.w;
			static_practice_cnt_title.h = min(wnd_h, cell.h);
			static_practice_cnt_title.x = cell.center_x() - static_practice_cnt_title.w / 2;
			static_practice_cnt_title.y = cell.top;

			rect_i32 static_practice_cnt;
			static_practice_cnt.w = cell.w;
			static_practice_cnt.x = cell.center_x() - static_practice_cnt.w / 2;
			static_practice_cnt.h = distance(cell.bottom(), static_practice_cnt_title.bottom());
			static_practice_cnt.y = cell.bottom() - static_practice_cnt.h;

			//3rd cell
			cell = grid[1][0];
			rect_i32 static_accuracy_title;
			static_accuracy_title.w = cell.w;
			static_accuracy_title.h = min(wnd_h, cell.h);
			static_accuracy_title.x = cell.center_x() - static_accuracy_title.w / 2;
			static_accuracy_title.y = cell.top;

			rect_i32 score_accuracy;
			score_accuracy.h = min(cell.w, distance(cell.bottom(), static_accuracy_title.bottom()));
			score_accuracy.w = score_accuracy.h;
			score_accuracy.x = cell.center_x() - score_accuracy.w / 2;
			score_accuracy.y = cell.bottom() - score_accuracy.h;

			//4th cell
			cell = grid[1][1];
			rect_i32 static_accuracy_timeline_title;
			static_accuracy_timeline_title.w = cell.w;
			static_accuracy_timeline_title.h = min(wnd_h, cell.h);
			static_accuracy_timeline_title.x = cell.center_x() - static_accuracy_timeline_title.w / 2;
			static_accuracy_timeline_title.y = cell.top;

			rect_i32 graph_accuracy_timeline;
			graph_accuracy_timeline.h = min(cell.w, distance(cell.bottom(), static_accuracy_timeline_title.bottom()));
			graph_accuracy_timeline.w = min(graph_accuracy_timeline.h * 16 / 9, cell.w);
			graph_accuracy_timeline.x = cell.center_x() - graph_accuracy_timeline.w / 2;
			graph_accuracy_timeline.y = cell.bottom() - graph_accuracy_timeline.h;

			rect_i32 bottom_most_control = graph_accuracy_timeline;

			int used_h = distance(start_y, bottom_most_control.bottom());// minus start_y which is always 0
			int y_offset = (h - used_h) / 2;//Vertically center the whole of the controls
			//TODO(fran): correct y_offset: if (used_h > h) dont try centering instead (maybe) apply one h_pad

			page_scroll(controls.page, w, page_space_h, used_h);

			MyMoveWindow_offset(controls.button_recents, button_recents, FALSE);
			MyMoveWindow_offset(controls.listbox_recents, listbox_recents, FALSE);
			listbox::set_dimensions(controls.listbox_recents, listbox::dimensions().set_border_thickness(0).set_element_h(wnd_h));

			MyMoveWindow_offset(controls.static_word_cnt_title, static_word_cnt_title, FALSE);
			MyMoveWindow_offset(controls.static_word_cnt, static_word_cnt, FALSE);
			MyMoveWindow_offset(controls.static_practice_cnt_title, static_practice_cnt_title, FALSE);
			MyMoveWindow_offset(controls.static_practice_cnt, static_practice_cnt, FALSE);
			MyMoveWindow_offset(controls.static_accuracy_title, static_accuracy_title, FALSE);
			MyMoveWindow_offset(controls.score_accuracy, score_accuracy, FALSE);
			MyMoveWindow_offset(controls.static_accuracy_timeline_title, static_accuracy_timeline_title, FALSE);
			MyMoveWindow_offset(controls.graph_accuracy_timeline, graph_accuracy_timeline, FALSE);

#if 0
			//MoveWindow(controls.candy, 0, 0, min(max_w, avg_str_dim(GetWindowFont(controls.button_recents), 10).cx), h, FALSE);//place one on the left (and one on the right of the page)
			MoveWindow(controls.candy, 0, 0, w, h, FALSE);//cover the entire page
#endif
		}
	}
}