#pragma once

struct anim_number_range {
	HWND wnd;//window to be updated with new numbers //TODO(fran): should I keep this or use the HWND provided by SetTimer?
	i64 origin, dest;
	f32 t;//[0.0,1.0]
	f32 dt;//increment in t, not actually a delta of time since we are normalized 0 to 1
};

//NOTE: you must previously specify wnd, origin and dest inside the animation struct
void animate_number_range(anim_number_range* anim_state, u32 ms) {
	Assert(anim_state);
	const f32 duration_sec = (f32)ms / 1000.f;
	const u32 total_frames = (u32)ceilf(duration_sec / (1.f / win32_get_refresh_rate_hz(anim_state->wnd)));
	const f32 dt = 1.f / total_frames;
	const UINT_PTR timer_id = (decltype(timer_id))anim_state;

	anim_state->t = 0.f;
	anim_state->dt = dt;

	static void (*number_range_anim)(HWND, UINT, UINT_PTR, DWORD) =
		[](HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
		anim_number_range* anim_state = (decltype(anim_state))anim_id;
		if (anim_state) {
#if 0
			f32 delta = ParametricBlend(anim_state->t);//TODO(fran): we may want linear for this one
#else
			f32 delta = anim_state->t;//linear blend
#endif
			i64 pos = lerp(anim_state->origin, delta, anim_state->dest);

			auto txt = std::to_wstring(pos);
			SendMessage(anim_state->wnd, WM_SETTEXT, 0, (LPARAM)txt.c_str());
			f32 oldt = anim_state->t;
			anim_state->t += anim_state->dt;
			anim_state->t = clamp01(anim_state->t);
			if (oldt >= 1.f) {
				KillTimer(hwnd, anim_id);
			}
			else {
				i32 ms = (i32)((1.f / win32_get_refresh_rate_hz(hwnd)) * 1000.f);
				SetTimer(hwnd, anim_id, ms, number_range_anim);
			}
		}
		else KillTimer(hwnd, anim_id);//this should never happen, if we get here we got a bug
		};

	SetTimer(anim_state->wnd, timer_id, 0, number_range_anim);
}