#pragma once
#include "がいじんの_Platform.h"
#include "unCap_Vector.h"
#include "がいじんの_Helpers.h" //Only for testing

//i32
static i32 distance(i32 a, i32 b) {
	return abs(a - b);
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

static i32 clamp(i32 min, i32 n, i32 max) { //clamps between [min,max]
	i32 res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

#include <ctime>//time() //TODO(fran):remove
#include <random>//TODO(fran):remove
static i32 random_between(i32 min, i32 max) { //NOTE: random integer between user specified range [min,max]
	i32 res;
	static std::mt19937 generator((u32)time(NULL));
	std::uniform_int_distribution<> distribution{ min,max };
	res = distribution(generator);
	return res;
}
//static i32 random_between(i32 min, i32 max) {
//	//TODO(fran): better random function
//	srand((u32)time(NULL));
//	i32 res = min + (i32)( ((f64)rand() / (f64)RAND_MAX) * (f64)(max - min) );
//	return res;
//}

//f32
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

f32 lerp(f32 n1, f32 t, f32 n2) {
	//NOTE: interesting that https://en.wikipedia.org/wiki/Linear_interpolation mentions this is the Precise method
	return (1.f - t) * n1 + t * n2;
}

v4 lerp(v4 n1, f32 t, v4 n2) {
	return (1.f - t) * n1 + t * n2;
}

f32 squared(f32 n) {
	f32 res = n * n;
	return res;
}

f32 square_root(f32 n) {
	f32 res = sqrtf(n);
	return res;
}

u32 round_f32_to_u32(f32 n) {
	Assert(n >= 0.f);
	//TODO(fran): intrinsic
	u32 res = (u32)(n + .5f);
	return res;
}

i32 round_f32_to_i32(f32 n) {
	//TODO(fran): intrinsic
	i32 res = (i32)(n + .5f); //TODO(fran): does this work correctly with negative numbers?
	return res;
}

f32 clamp(f32 min, f32 n, f32 max) {
	f32 res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

f32 clamp01(f32 n) {
	return clamp(0.f, n, 1.f);
}

v4 clamp01(v4 v) {
	v4 res;
	res.r = clamp01(v.r);
	res.g = clamp01(v.g);
	res.b = clamp01(v.b);
	res.a = clamp01(v.a);
	return res;
}


//f64

static f64 distance(f64 a, f64 b) {
	return abs(a - b);
}

//Rectangle
union rect_i32 {
	struct { i32 x, y, w, h; };
	struct { i32 left, top, w, h; };
	struct { v2_i32 xy, wh; };

	i32 right() { return left + w; }
	i32 bottom() { return top + h; }
	i32 center_x() { return left + w / 2; }
	i32 center_y() { return top + h / 2; }
};


//size_t
static size_t distance(size_t a, size_t b) {
	size_t res;
	if (a >= b) res = a - b;
	else res = b - a;
	return res;
}