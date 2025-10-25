#pragma once
namespace べんきょう {
	namespace practice { //TODO(fran): it's not yet clear if this should go inside a practice::common namespace or smth else
		void clear_practices_vector(decltype(review::page_state::practices)& practices) {
			for (auto p : practices) {
				switch (p->type) {
				case decltype(p->type)::writing:
				{
					practice_writing* data = (decltype(data))p;
					if (data->user_answer.str) { free_any_str(data->user_answer.str); data->user_answer.sz = 0; }
					data->practice->word.free();
					free(data->practice);
				} break;
				case decltype(p->type)::multiplechoice:
				{
					practice_multiplechoice* data = (decltype(data))p;
					data->practice->choices.free();
					data->practice->question.free();
					free(data->practice);
				} break;
				case decltype(p->type)::drawing:
				{
					practice_drawing* data = (decltype(data))p;
					data->practice->question.free();
					DeleteBitmap(data->user_answer);
					free(data->practice);
				} break;
				default: Assert(0);
				}
				free(p);//the object, no matter what type, can always be freed the same way since p is the ptr returned by the allocator
			}
			practices.clear();
		}

		auto prepare_practice(ProcState* state) {
			struct practice_data { ProcState::page page; void* data; } res;
			auto& practices = state->settings->practices;
			Assert(practices);
			available_practices practice = (available_practices)random_bit_set(practices);

			auto get_hiragana_kanji_meaning = [](learnt_word16* w)->multiflag<learnt_word_elem> {
				//Indicates which parts of the word are filled/valid
				multiflag<learnt_word_elem> res = 0;
				if (w) {
					bool is_radical = get_lexical_category(w->attributes.lexical_category) == lexical_category::radical;

					if (!is_radical && w->attributes.hiragana.str && *w->attributes.hiragana.str) res |= (u32)learnt_word_elem::hiragana;
					if (w->attributes.kanji.str && *w->attributes.kanji.str) res |= (u32)learnt_word_elem::kanji;
					if (w->attributes.meaning.str && *w->attributes.meaning.str) res |= (u32)learnt_word_elem::meaning;
				}
				return res;
				};
			i32 max_retries = 5; //TODO(fran): improvement: improve the sql logic to select the proper words straight from the db to avoid needing to retry

			switch (practice) {
			case available_practices::writing:
			{
				//NOTE: writing itself has many different practices
				writing::word* data = (decltype(data))malloc(sizeof(*data));//TODO(fran): MEMLEAK practice_writing page will take care of freeing this once the page completes its practice
				for (i32 tries = 1; tries <= max_retries; tries++) {
					learnt_word16 practice_word = get_practice_word(state->settings->db);//get a target word

					multiflag<writing::variant> practice_types{};
					auto word_types = get_hiragana_kanji_meaning(&practice_word); Assert(word_types);
					if (word_types & (u32)learnt_word_elem::hiragana) {
						if (word_types & (u32)learnt_word_elem::meaning) {
							practice_types |= (i32)writing::variant::hiragana_to_meaning;
							practice_types |= (i32)writing::variant::meaning_to_hiragana;
						}
						if (word_types & (u32)learnt_word_elem::kanji)
							practice_types |= (i32)writing::variant::kanji_to_hiragana;
					}
					if (word_types & (u32)learnt_word_elem::kanji && word_types & (u32)learnt_word_elem::meaning)
						practice_types |= (i32)writing::variant::kanji_to_meaning;

					auto original_practice_types = practice_types;
					practice_types &= state->settings->practice_writing_variants;

					if (!practice_types && tries != max_retries) {
						practice_word.free();
						continue;
					}
					else if (!practice_types) {
						//Give up and take whatever practice that we can
						practice_types = original_practice_types;
					}

					data->word = std::move(practice_word);
					data->practice_type = (writing::variant)random_bit_set(practice_types);
					res.page = ProcState::page::practice_writing;
					res.data = data;
					break;
				}
			} break;
			case available_practices::multiplechoice:
			{
				multiplechoice::word* data = (decltype(data))malloc(sizeof(*data));

				for (i32 tries = 1; tries <= max_retries; tries++) {
					learnt_word16 practice_word = get_practice_word(state->settings->db);//get a target word

					multiflag<learnt_word_elem> q_and_a = get_hiragana_kanji_meaning(&practice_word);//NOTE: the type of the question and choices is limited by our target word

					auto variants = state->settings->practice_multiplechoice_variants;
					if (!(q_and_a & (u32)learnt_word_elem::hiragana)) variants ^= (i32)multiplechoice::variant::hiragana_to_kanji | (i32)multiplechoice::variant::hiragana_to_meaning | (i32)multiplechoice::variant::kanji_to_hiragana | (i32)multiplechoice::variant::meaning_to_hiragana;
					if (!(q_and_a & (u32)learnt_word_elem::kanji)) variants ^= (i32)multiplechoice::variant::kanji_to_hiragana | (i32)multiplechoice::variant::kanji_to_meaning | (i32)multiplechoice::variant::meaning_to_kanji | (i32)multiplechoice::variant::hiragana_to_kanji;
					if (!(q_and_a & (u32)learnt_word_elem::meaning)) variants ^= (i32)multiplechoice::variant::meaning_to_hiragana | (i32)multiplechoice::variant::meaning_to_kanji | (i32)multiplechoice::variant::kanji_to_meaning | (i32)multiplechoice::variant::hiragana_to_meaning;

					if (variants) {
						auto variant = random_bit_set(variants);

						switch (variant) {
						case (u64)multiplechoice::variant::hiragana_to_meaning:
							data->question_type = learnt_word_elem::hiragana;
							data->choices_type = learnt_word_elem::meaning;
							break;
						case (u64)multiplechoice::variant::hiragana_to_kanji:
							data->question_type = learnt_word_elem::hiragana;
							data->choices_type = learnt_word_elem::kanji;
							break;
						case (u64)multiplechoice::variant::meaning_to_hiragana:
							data->question_type = learnt_word_elem::meaning;
							data->choices_type = learnt_word_elem::hiragana;
							break;
						case (u64)multiplechoice::variant::meaning_to_kanji:
							data->question_type = learnt_word_elem::meaning;
							data->choices_type = learnt_word_elem::kanji;
							break;
						case (u64)multiplechoice::variant::kanji_to_hiragana:
							data->question_type = learnt_word_elem::kanji;
							data->choices_type = learnt_word_elem::hiragana;
							break;
						case (u64)multiplechoice::variant::kanji_to_meaning:
							data->question_type = learnt_word_elem::kanji;
							data->choices_type = learnt_word_elem::meaning;
							break;
						default: Assert(0); break;
						}
					}
					else if (tries != max_retries) {
						practice_word.free();
						continue;
					}
					else {
						//Give up and take whatever practice that we can
						data->question_type = (decltype(data->question_type))random_bit_set(q_and_a);
						data->choices_type = (decltype(data->question_type))random_bit_set(q_and_a & (~(u32)data->question_type));
					}

					data->question = std::move(practice_word);
					break;
				}

				ptr<utf16*> _choices = get_word_choices(state->settings->db, data->choices_type, 5, &data->question); defer{ _choices.free(); };
				data->idx_answer = random_between(0u, (u32)_choices.cnt);

				data->choices = { 0 };//clear it to avoid free() problems, TODO(fran): I dont like the free() inside alloc() idea, maaybe we could create two functions alloc() and alloc_free_prev()
				data->choices.alloc(_choices.cnt + 1);

				//Move all the choices plus the correct choice into the new array
				for (int i = 0, j = 0; i < data->choices.cnt; i++) {
					if (i == data->idx_answer) data->choices[i] = _wcsdup(str_for((learnt_word16*)&data->question, data->choices_type).str);//TODO(fran): replace for my own duplication method
					else data->choices[i] = _choices[j++];
				}

				data->question_str = str_for(&data->question, data->question_type).str;

				res.page = ProcState::page::practice_multiplechoice;
				res.data = data;
			} break;
			case available_practices::drawing:
			{
				drawing::word* data = (decltype(data))malloc(sizeof(*data));
				for (i32 tries = 1; tries <= max_retries; tries++) {
					learnt_word16 practice_word = get_practice_word(state->settings->db, false, true, false);//get a target word

					//TODO(fran): practice_word could be empty if there are no words with kanji, handle this case

					u32 q_elems = get_hiragana_kanji_meaning(&practice_word) & (~(u32)learnt_word_elem::kanji);
					auto original_q_elems = q_elems;
					if (!(state->settings->practice_drawing_variants & (u32)drawing::variant::hiragana_to_kanji))
						q_elems &= ~(u32)learnt_word_elem::hiragana;
					if (!(state->settings->practice_drawing_variants & (u32)drawing::variant::meaning_to_kanji))
						q_elems &= ~(u32)learnt_word_elem::meaning;

					if (!q_elems && tries != max_retries) {
						practice_word.free();
						continue;
					}
					else if (!q_elems) {
						//Give up and take whatever practice that we can
						q_elems = original_q_elems;
					}

					data->question = std::move(practice_word);
					data->question_type = (decltype(data->question_type))random_bit_set(q_elems);
					data->question_str = str_for(&data->question, data->question_type).str;
					res.page = ProcState::page::practice_drawing;
					res.data = data;
					break;
				}
			} break;

			default: Assert(0);
			}
			return res;
		}

		void _next_practice_level(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
			ProcState* state = get_state(hwnd);
			KillTimer(state->wnd, anim_id);

			if (state->practice_cnt-- > 0) {
				auto practice = prepare_practice(state);//get a random practice
				reset_page(state, practice.page);
				preload_page(state, practice.page, practice.data);//load the practice
				set_current_page(state, practice.page);//go practice!
			}
			else {
				user_stats_increment_times_practiced(state->settings->db);
				preload_page(state, ProcState::page::review_practice, &state->multipagestate.temp_practices);
				set_current_page(state, ProcState::page::review_practice);
			}
		}

		//decreases the practice counter and loads/sets a new practice level or goes to the review page if the practice is over
		void next_practice_level(ProcState* state, bool add_delay = true) {
			u32 delay = add_delay ? 500 : USER_TIMER_MINIMUM;
			SetTimer(state->wnd, timerIDs.next_practice_level, delay, _next_practice_level);
		}
	}
}