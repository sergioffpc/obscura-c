#ifndef __OBSCURA_TENSOR_H__
#define __OBSCURA_TENSOR_H__ 1

#include <immintrin.h>
#include <math.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEG2RADF(deg)	((float) ((deg) * M_PI / 180))
#define RAD2DEGF(rad)	((float) ((rad) * 180 / M_PI))

/*
 * Declares the storage for a homogenous array of floating-point values.
 */
typedef __m128	vec4 __attribute__((aligned(16)));

__extern_always_inline float vec4_dot(vec4 const u, vec4 const v) {
	vec4 const dot = _mm_dp_ps(u, v, 0xf1);

	float res = 0;
	_mm_store_ss(&res, dot);

	return res;
}

__extern_always_inline float vec4_length(vec4 const u) {
	vec4 const dot  = _mm_dp_ps(u, u, 0xff);
	vec4 const sqrt = _mm_sqrt_ss(dot);

	float res = 0;
	_mm_store_ss(&res, sqrt);

	return res;
}

__extern_always_inline float vec4_distance(vec4 const u, vec4 const v) {
	vec4 const sub = _mm_sub_ps(u, v);

	float res = vec4_length(sub);

	return res;
}

__extern_always_inline vec4 vec4_cross(vec4 const u, vec4 const v) {
	vec4 const swp0 = _mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 0, 2, 1));
	vec4 const swp1 = _mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 1, 0, 2));
	vec4 const swp2 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1));
	vec4 const swp3 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2));
	vec4 const mul0 = _mm_mul_ps(swp0, swp3);
	vec4 const mul1 = _mm_mul_ps(swp1, swp2);
	vec4 const res  = _mm_sub_ps(mul0, mul1);

	return res;
}

__extern_always_inline vec4 vec4_normalize(vec4 const u) {
	vec4 const dot = _mm_dp_ps(u, u, 0xff);
	vec4 const isr = _mm_rsqrt_ps(dot);
	vec4 const res = _mm_mul_ps(u, isr);

	return res;
}

/*
 * Describes transformations that embody mathematical changes to points
 * within a coordinate system or the coordinate system itself.
 */
typedef	vec4	mat4[4] __attribute__((aligned(64)));

__extern_always_inline void mat4_add(mat4 const a, mat4 const b, mat4 out) {
	out[0] = _mm_add_ps(a[0], b[0]);
	out[1] = _mm_add_ps(a[1], b[1]);
	out[2] = _mm_add_ps(a[2], b[2]);
	out[3] = _mm_add_ps(a[3], b[3]);
}

__extern_always_inline void mat4_sub(mat4 const a, mat4 const b, mat4 out) {
	out[0] = _mm_sub_ps(a[0], b[0]);
	out[1] = _mm_sub_ps(a[1], b[1]);
	out[2] = _mm_sub_ps(a[2], b[2]);
	out[3] = _mm_sub_ps(a[3], b[3]);
}

__extern_always_inline void mat4_mul(mat4 const a, mat4 const b, mat4 out) {
	{
		vec4 const e0 = _mm_shuffle_ps(b[0], b[0], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const e1 = _mm_shuffle_ps(b[0], b[0], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const e2 = _mm_shuffle_ps(b[0], b[0], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const e3 = _mm_shuffle_ps(b[0], b[0], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const m0 = _mm_mul_ps(a[0], e0);
		vec4 const m1 = _mm_mul_ps(a[1], e1);
		vec4 const m2 = _mm_mul_ps(a[2], e2);
		vec4 const m3 = _mm_mul_ps(a[3], e3);

		vec4 const a0 = _mm_add_ps(m0, m1);
		vec4 const a1 = _mm_add_ps(m2, m3);
		vec4 const a2 = _mm_add_ps(a0, a1);

		out[0] = a2;
	}

	{
		vec4 const e0 = _mm_shuffle_ps(b[1], b[1], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const e1 = _mm_shuffle_ps(b[1], b[1], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const e2 = _mm_shuffle_ps(b[1], b[1], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const e3 = _mm_shuffle_ps(b[1], b[1], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const m0 = _mm_mul_ps(a[0], e0);
		vec4 const m1 = _mm_mul_ps(a[1], e1);
		vec4 const m2 = _mm_mul_ps(a[2], e2);
		vec4 const m3 = _mm_mul_ps(a[3], e3);

		vec4 const a0 = _mm_add_ps(m0, m1);
		vec4 const a1 = _mm_add_ps(m2, m3);
		vec4 const a2 = _mm_add_ps(a0, a1);

		out[1] = a2;
	}

	{
		vec4 const e0 = _mm_shuffle_ps(b[2], b[2], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const e1 = _mm_shuffle_ps(b[2], b[2], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const e2 = _mm_shuffle_ps(b[2], b[2], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const e3 = _mm_shuffle_ps(b[2], b[2], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const m0 = _mm_mul_ps(a[0], e0);
		vec4 const m1 = _mm_mul_ps(a[1], e1);
		vec4 const m2 = _mm_mul_ps(a[2], e2);
		vec4 const m3 = _mm_mul_ps(a[3], e3);

		vec4 const a0 = _mm_add_ps(m0, m1);
		vec4 const a1 = _mm_add_ps(m2, m3);
		vec4 const a2 = _mm_add_ps(a0, a1);

		out[2] = a2;
	}

	{
		vec4 const e0 = _mm_shuffle_ps(b[3], b[3], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const e1 = _mm_shuffle_ps(b[3], b[3], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const e2 = _mm_shuffle_ps(b[3], b[3], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const e3 = _mm_shuffle_ps(b[3], b[3], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const m0 = _mm_mul_ps(a[0], e0);
		vec4 const m1 = _mm_mul_ps(a[1], e1);
		vec4 const m2 = _mm_mul_ps(a[2], e2);
		vec4 const m3 = _mm_mul_ps(a[3], e3);

		vec4 const a0 = _mm_add_ps(m0, m1);
		vec4 const a1 = _mm_add_ps(m2, m3);
		vec4 const a2 = _mm_add_ps(a0, a1);

		out[3] = a2;
	}
}

__extern_always_inline vec4 mat4_transform(mat4 const a, vec4 const u) {
	vec4 const v0 = _mm_shuffle_ps(u, u, _MM_SHUFFLE(0, 0, 0, 0));
	vec4 const v1 = _mm_shuffle_ps(u, u, _MM_SHUFFLE(1, 1, 1, 1));
	vec4 const v2 = _mm_shuffle_ps(u, u, _MM_SHUFFLE(2, 2, 2, 2));
	vec4 const v3 = _mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 3, 3, 3));

	vec4 const m0 = _mm_mul_ps(a[0], v0);
	vec4 const m1 = _mm_mul_ps(a[1], v1);
	vec4 const m2 = _mm_mul_ps(a[2], v2);
	vec4 const m3 = _mm_mul_ps(a[3], v3);

	vec4 const a0 = _mm_add_ps(m0, m1);
	vec4 const a1 = _mm_add_ps(m2, m3);
	vec4 const a2 = _mm_add_ps(a0, a1);

	return a2;
}

__extern_always_inline void mat4_transpose(mat4 const a, mat4 out) {
	vec4 const tmp0 = _mm_shuffle_ps(a[0], a[1], 0x44);
	vec4 const tmp2 = _mm_shuffle_ps(a[0], a[1], 0xee);
	vec4 const tmp1 = _mm_shuffle_ps(a[2], a[3], 0x44);
	vec4 const tmp3 = _mm_shuffle_ps(a[2], a[3], 0xee);

	out[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
	out[1] = _mm_shuffle_ps(tmp0, tmp1, 0xdd);
	out[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
	out[3] = _mm_shuffle_ps(tmp2, tmp3, 0xdd);
}

__extern_always_inline void mat4_inverse(mat4 const a, mat4 out) {
	vec4 fac0;
	{
		vec4 const swp0a = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(3, 3, 3, 3));
		vec4 const swp0b = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(2, 2, 2, 2));

		vec4 const swp00 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const swp01 = _mm_shuffle_ps(swp0a, swp0a, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp02 = _mm_shuffle_ps(swp0b, swp0b, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp03 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const mul00 = _mm_mul_ps(swp00, swp01);
		vec4 const mul01 = _mm_mul_ps(swp02, swp03);
		fac0 = _mm_sub_ps(mul00, mul01);
	}

	vec4 fac1;
	{
		vec4 const swp0a = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(3, 3, 3, 3));
		vec4 const swp0b = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(1, 1, 1, 1));

		vec4 const swp00 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const swp01 = _mm_shuffle_ps(swp0a, swp0a, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp02 = _mm_shuffle_ps(swp0b, swp0b, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp03 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const mul00 = _mm_mul_ps(swp00, swp01);
		vec4 const mul01 = _mm_mul_ps(swp02, swp03);
		fac1 = _mm_sub_ps(mul00, mul01);
	}


	vec4 fac2;
	{
		vec4 const swp0a = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const swp0b = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(1, 1, 1, 1));

		vec4 const swp00 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const swp01 = _mm_shuffle_ps(swp0a, swp0a, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp02 = _mm_shuffle_ps(swp0b, swp0b, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp03 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(2, 2, 2, 2));

		vec4 const mul00 = _mm_mul_ps(swp00, swp01);
		vec4 const mul01 = _mm_mul_ps(swp02, swp03);
		fac2 = _mm_sub_ps(mul00, mul01);
	}

	vec4 fac3;
	{
		vec4 const swp0a = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(3, 3, 3, 3));
		vec4 const swp0b = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(0, 0, 0, 0));

		vec4 const swp00 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const swp01 = _mm_shuffle_ps(swp0a, swp0a, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp02 = _mm_shuffle_ps(swp0b, swp0b, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp03 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(3, 3, 3, 3));

		vec4 const mul00 = _mm_mul_ps(swp00, swp01);
		vec4 const mul01 = _mm_mul_ps(swp02, swp03);
		fac3 = _mm_sub_ps(mul00, mul01);
	}

	vec4 fac4;
	{
		vec4 const swp0a = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(2, 2, 2, 2));
		vec4 const swp0b = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(0, 0, 0, 0));

		vec4 const swp00 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const swp01 = _mm_shuffle_ps(swp0a, swp0a, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp02 = _mm_shuffle_ps(swp0b, swp0b, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp03 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(2, 2, 2, 2));

		vec4 const mul00 = _mm_mul_ps(swp00, swp01);
		vec4 const mul01 = _mm_mul_ps(swp02, swp03);
		fac4 = _mm_sub_ps(mul00, mul01);
	}

	vec4 fac5;
	{
		vec4 const swp0a = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(1, 1, 1, 1));
		vec4 const swp0b = _mm_shuffle_ps(a[3], a[2], _MM_SHUFFLE(0, 0, 0, 0));

		vec4 const swp00 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(0, 0, 0, 0));
		vec4 const swp01 = _mm_shuffle_ps(swp0a, swp0a, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp02 = _mm_shuffle_ps(swp0b, swp0b, _MM_SHUFFLE(2, 0, 0, 0));
		vec4 const swp03 = _mm_shuffle_ps(a[2], a[1], _MM_SHUFFLE(1, 1, 1, 1));

		vec4 const mul00 = _mm_mul_ps(swp00, swp01);
		vec4 const mul01 = _mm_mul_ps(swp02, swp03);
		fac5 = _mm_sub_ps(mul00, mul01);
	}

	vec4 const signa = _mm_set_ps( 1.0f,-1.0f, 1.0f,-1.0f);
	vec4 const signb = _mm_set_ps(-1.0f, 1.0f,-1.0f, 1.0f);

	vec4 const temp0 = _mm_shuffle_ps(a[1], a[0], _MM_SHUFFLE(0, 0, 0, 0));
	vec4 const vec0 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 2, 2, 0));

	vec4 const temp1 = _mm_shuffle_ps(a[1], a[0], _MM_SHUFFLE(1, 1, 1, 1));
	vec4 const vec1 = _mm_shuffle_ps(temp1, temp1, _MM_SHUFFLE(2, 2, 2, 0));

	vec4 const temp2 = _mm_shuffle_ps(a[1], a[0], _MM_SHUFFLE(2, 2, 2, 2));
	vec4 const vec2 = _mm_shuffle_ps(temp2, temp2, _MM_SHUFFLE(2, 2, 2, 0));

	vec4 const temp3 = _mm_shuffle_ps(a[1], a[0], _MM_SHUFFLE(3, 3, 3, 3));
	vec4 const vec3 = _mm_shuffle_ps(temp3, temp3, _MM_SHUFFLE(2, 2, 2, 0));

	vec4 const mul00 = _mm_mul_ps(vec1, fac0);
	vec4 const mul01 = _mm_mul_ps(vec2, fac1);
	vec4 const mul02 = _mm_mul_ps(vec3, fac2);
	vec4 const sub00 = _mm_sub_ps(mul00, mul01);
	vec4 const add00 = _mm_add_ps(sub00, mul02);
	vec4 const inv0 = _mm_mul_ps(signb, add00);

	vec4 const mul03 = _mm_mul_ps(vec0, fac0);
	vec4 const mul04 = _mm_mul_ps(vec2, fac3);
	vec4 const mul05 = _mm_mul_ps(vec3, fac4);
	vec4 const sub01 = _mm_sub_ps(mul03, mul04);
	vec4 const add01 = _mm_add_ps(sub01, mul05);
	vec4 const inv1 = _mm_mul_ps(signa, add01);

	vec4 const mul06 = _mm_mul_ps(vec0, fac1);
	vec4 const mul07 = _mm_mul_ps(vec1, fac3);
	vec4 const mul08 = _mm_mul_ps(vec3, fac5);
	vec4 const sub02 = _mm_sub_ps(mul06, mul07);
	vec4 const add02 = _mm_add_ps(sub02, mul08);
	vec4 const inv2 = _mm_mul_ps(signb, add02);

	vec4 const mul09 = _mm_mul_ps(vec0, fac2);
	vec4 const mul10 = _mm_mul_ps(vec1, fac4);
	vec4 const mul11 = _mm_mul_ps(vec2, fac5);
	vec4 const sub03 = _mm_sub_ps(mul09, mul10);
	vec4 const add03 = _mm_add_ps(sub03, mul11);
	vec4 const inv3 = _mm_mul_ps(signa, add03);

	vec4 const row0 = _mm_shuffle_ps(inv0, inv1, _MM_SHUFFLE(0, 0, 0, 0));
	vec4 const row1 = _mm_shuffle_ps(inv2, inv3, _MM_SHUFFLE(0, 0, 0, 0));
	vec4 const row2 = _mm_shuffle_ps(row0, row1, _MM_SHUFFLE(2, 0, 2, 0));

	vec4 const det0 = _mm_dp_ps(a[0], row2, 0xff);
	vec4 const rcp0 = _mm_div_ps(_mm_set1_ps(1.0f), det0);

	out[0] = _mm_mul_ps(inv0, rcp0);
	out[1] = _mm_mul_ps(inv1, rcp0);
	out[2] = _mm_mul_ps(inv2, rcp0);
	out[3] = _mm_mul_ps(inv3, rcp0);
}

__extern_always_inline void mat4_lookat(const vec4 eye, const vec4 interest, const vec4 up, mat4 out) {
	vec4 const f = vec4_normalize(interest - eye);
	vec4 const s = vec4_normalize(vec4_cross(f, up));
	vec4 const u = vec4_cross(s, f);

	out[0][0] = s[0];
	out[1][0] = s[1];
	out[2][0] = s[2];
	out[3][0] = 0;

	out[0][1] = u[0];
	out[1][1] = u[1];
	out[2][1] = u[2];
	out[3][1] = 0;

	out[0][2] = -f[0];
	out[1][2] = -f[1];
	out[2][2] = -f[2];
	out[3][2] =  0;

	out[3][0] = -vec4_dot(s, eye);
	out[3][1] = -vec4_dot(u, eye);
	out[3][2] =  vec4_dot(f, eye);
	out[3][3] =  1;
}

__extern_always_inline bool quad_solver(float a, float b, float c, float *x0, float *x1) {
	float d = b * b - 4 * a * c;
	if (d < 0) {
		return false;
	} else if (d == 0) {
		*x0 = *x1 = -0.5 * b / a;
	} else {
		float q = (b > 0) ? -0.5 * (b + sqrt(d)) : -0.5 * (b - sqrt(d));
		*x0 = q / a;
		*x1 = c / q;
	}

	if (x0 > x1) {
		float tmp = *x0;
		*x0 = *x1;
		*x1 = tmp;
	}

	return true;
}

#ifdef __cplusplus
}
#endif

#endif
