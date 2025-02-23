﻿#pragma once
#include "win32_Platform.h"
#include "unCap_Vector.h"
#include "win32_Helpers.h" //Only for testing

//i32
static i32 distance(i32 a, i32 b) {
	return abs(a - b);
}
static i32 distance(long a, long b) { return abs(a - b); } //NOTE(fran): added due to new dumb compiler complaints whereby it doesnt know whether con convert long to int or float

v2_i32 lerp(v2_i32 n1, f32 t, v2_i32 n2) {
	//NOTE: interesting that https://en.wikipedia.org/wiki/Linear_interpolation mentions this is the Precise method
	return (1.f - t) * n1 + t * n2;
}

static i32 roundNup(i32 N, i32 num) {
	//Thanks https://stackoverflow.com/questions/3407012/c-rounding-up-to-the-nearest-multiple-of-a-number
	//TODO(fran): check to which side it rounds for negatives
	Assert(N);
	i32 isPositive = (i32)(num >= 0);
	i32 res = ((num + isPositive * (N - 1)) / N) * N;
	return res;
}

static i32 round2up(i32 n) {
	return roundNup(2, n);
}

static i32 roundNdown(i32 N, i32 num) {
	//Thanks https://stackoverflow.com/questions/3407012/c-rounding-up-to-the-nearest-multiple-of-a-number
	//TODO(fran): doesnt work for negatives
	Assert(N);
	i32 res = num - (num % N);
	return res;
}

static i32 safe_ratioN(i32 dividend, i32 divisor, i32 n) {
	i32 res;
	if (divisor != 0) {
		res = dividend / divisor;
	}
	else {
		res = n;
	}
	return res;
}

static i32 safe_ratio1(i32 dividend, i32 divisor) {
	return safe_ratioN(dividend, divisor, 1);
}

static i32 safe_ratio0(i32 dividend, i32 divisor) {
	return safe_ratioN(dividend, divisor, 0);
}

//clamps between [min,max]
static i32 clamp(i32 min, i32 n, i32 max) {
	i32 res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

static u32 clamp(u32 min, u32 n, u32 max) {
	u32 res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

static u64 clamp(u64 min, u64 n, u64 max) {
	u64 res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

#include <ctime>//time() //TODO(fran):remove
#include <random>//TODO(fran):remove
static i32 random_between(i32 min, i32 max) { //NOTE: random integer between user specified range [min,max]
	i32 res;
	static std::mt19937 generator((u32)time(0));
	std::uniform_int_distribution<i32> distribution{ min,max };
	res = distribution(generator);
	return res;
}
static u32 random_between(u32 min, u32 max) { //NOTE: random integer between user specified range [min,max]
	u32 res;
	static std::mt19937 generator((u32)time(0));
	std::uniform_int_distribution<u32> distribution{ min,max };
	res = distribution(generator);
	return res;
}
static u64 random_between(u64 min, u64 max) { //NOTE: random integer between user specified range [min,max]
	u64 res;
	static std::mt19937 generator((u32)time(0));
	std::uniform_int_distribution<u64> distribution{ min,max };
	res = distribution(generator);
	return res;
}
//static i32 random_between(i32 min, i32 max) {
//	//TODO(fran): better random function
//	srand((u32)time(NULL));
//	i32 res = min + (i32)( ((f64)rand() / (f64)RAND_MAX) * (f64)(max - min) );
//	return res;
//}
static f32 random_between(f32 min, f32 max) { //NOTE: random float between user specified range [min,max]
	f32 res;
	static std::mt19937 generator((u32)time(0));
	std::uniform_real_distribution<f32> distribution{ min,max };
	res = distribution(generator);
	return res;
}

#include <immintrin.h>
static u64 nthset(u64 val, unsigned n) {
	//Thanks https://stackoverflow.com/questions/7669057/find-nth-set-bit-in-an-int
	return _pdep_u64(1ULL << n, val);
}

//Usage example: val = 1101 1000 0100 1100 1000 1010, we find the 4th bit set and return 0000 0000 0000 0100 0000 0000
static u64 random_bit_set(u64 val) {
	//TODO(fran): #optimize
	u64 res=0;
	u32 cnt = (u32)__popcnt64(val);
	if (cnt) {
		auto idx = random_between(0u, cnt-1);
		res = nthset(val, idx);
	}
	return res;
}

static constexpr u64 popcnt64(u64 x)
{
	//Thanks https://github.com/kimwalisch/libpopcnt/blob/master/libpopcnt.h
	u64 m1 = 0x5555555555555555ull;
	u64 m2 = 0x3333333333333333ull;
	u64 m4 = 0x0F0F0F0F0F0F0F0Full;
	u64 h01 = 0x0101010101010101ull;

	x -= (x >> 1) & m1;
	x = (x & m2) + ((x >> 2) & m2);
	x = (x + (x >> 4)) & m4;

	return (x * h01) >> 56;
}

static u32 get_bit_set_position(u32 val) {
	u32 res = _tzcnt_u32(val);
	return res;
}

static u64 get_bit_set_position(u64 val) {
	u64 res = _tzcnt_u64(val);
	return res;
}

static constexpr u32 get_last_bit_set_position_slow(u32 val) {
	u32 res = 0;
	for (u32 i = 0; i < 32; i++) {
		if (val & 0x8000'0000) {
			res = 31 - i;
			break;
		}
		val = val << 1;
	}
	return res;
}

//f32

//Ease in & out
static f32 ParametricBlend(f32 t /*[0.0,1.0]*/) {
	// thanks https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula
	f32 sqt = t * t;
	return sqt / (2.0f * (sqt - t) + 1.0f);

	//more blend functions:
	//https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula
	//https://math.stackexchange.com/questions/121720/ease-in-out-function/121755#121755
	//https://github.com/jesusgollonet/ofpennereasing/tree/master/PennerEasing
}

static f32 safe_ratioN(f32 dividend, f32 divisor, f32 n) {
	f32 res;
	if (divisor != 0.f) {
		res = dividend / divisor;
	}
	else {
		res = n;
	}
	return res;
}

static f32 safe_ratio1(f32 dividend, f32 divisor) {
	return safe_ratioN(dividend, divisor, 1.f);
}

static f32 safe_ratio0(f32 dividend, f32 divisor) {
	return safe_ratioN(dividend, divisor, 0.f);
}

static f32 dot(v2 a, v2 b) {
	f32 res = a.x * b.x + a.y * b.y;
	return res;
}

static f32 length_sq(v2 v) {
	f32 res;
	res = dot(v, v);
	return res;
}

static f32 length(v2 v) {
	f32 res = sqrtf(length_sq(v));
	return res;
}

static v2 normalize(v2 v) {
	v2 res = v / length(v); //TODO(fran): beware of division by zero
	return res;
}

static f32 lerp(f32 n1, f32 t, f32 n2) {
	//NOTE: interesting that https://en.wikipedia.org/wiki/Linear_interpolation mentions this is the Precise method
	return (1.f - t) * n1 + t * n2;
}

static v4 lerp(v4 n1, f32 t, v4 n2) {
	return (1.f - t) * n1 + t * n2;
}

static f32 squared(f32 n) {
	f32 res = n * n;
	return res;
}

static f32 square_root(f32 n) {
	f32 res = sqrtf(n);
	return res;
}

static u32 round_f32_to_u32(f32 n) {
	Assert(n >= 0.f);
	//TODO(fran): intrinsic
	u32 res = (u32)(n + .5f);
	return res;
}

static i32 round_f32_to_i32(f32 n) {
	//TODO(fran): intrinsic
	i32 res = (i32)(n + .5f); //TODO(fran): does this work correctly with negative numbers?
	return res;
}

static f32 clamp(f32 min, f32 n, f32 max) {
	f32 res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

static f32 clamp01(f32 n) {
	return clamp(0.f, n, 1.f);
}

static bool is_between_inclusive(f32 min, f32 n, f32 max) {
	bool res = n <= max && n >= min;
	return res;
}

static v4 clamp01(v4 v) {
	v4 res;
	res.r = clamp01(v.r);
	res.g = clamp01(v.g);
	res.b = clamp01(v.b);
	res.a = clamp01(v.a);
	return res;
}

static v2 perp(v2 v) {//generate orthogonal vector (counter clockwise)
	v2 res;
	res.x = -v.y;
	res.y = v.x;
	return res;
}

//f64

static f64 distance(f64 a, f64 b) {
	return abs(a - b);
}

//size_t
static size_t distance(size_t a, size_t b) {
	size_t res;
	if (a >= b) res = a - b;
	else res = b - a;
	return res;
}

//returns a - b if a >= b otherwise returns n
static size_t safe_subtractN(size_t a, size_t b, size_t n) {
	Assert(b >= 0);
	size_t res;
	if (a >= b) res = a - b;
	else res = n;
	return res;
}

static size_t safe_subtract0(size_t a, size_t b) {
	return safe_subtractN(a, b, (decltype(a))0);
}

//i64
static i64 lerp(i64 n1, f32 t, i64 n2) {
	//NOTE: interesting that https://en.wikipedia.org/wiki/Linear_interpolation mentions this is the Precise method
	return (i64)((1.f - t) * (f64)n1 + t * (f64)n2);
}

//templates
template<typename T>
static T minimum(T a, T b) {
	return (a < b) ? a : b;
}

template<typename T>
static T maximum(T a, T b) {
	return (a > b) ? a : b;
}

template <typename T>
static T signum(T val) {
	return (T)((T(0) < val) - (val < T(0)));
}

template <typename T>
T signOf(T num) {
	return signum(num) + !num;
}

template <typename T>
static T safe_ratioN(T dividend, T divisor, T n) {
	T res;
	if (divisor != (T)0) {
		res = dividend / divisor;
	}
	else {
		res = n;
	}
	return res;
}

template <typename T>
static T safe_ratio1(T dividend, T divisor) {
	return safe_ratioN(dividend, divisor, (T)1);
}

template <typename T>
static T safe_ratio0(T dividend, T divisor) {
	return safe_ratioN(dividend, divisor, (T)0);
}