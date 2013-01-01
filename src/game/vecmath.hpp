#pragma once
#ifndef VECMATH_HPP_INCLUDED
#define VECMATH_HPP_INCLUDED

#define _USE_MATH_DEFINES
#include <math.h>

struct vec4
{
	float x, y, z, w;

	inline vec4(float cx, float cy, float cz, float cw) : x(cx), y(cy), z(cz), w(cw)
	{}

	inline void operator*=(float s)
	{
		x *= s;
		y *= s;
		z *= s;
	}
};

struct mat4
{
	vec4 x;
	vec4 y;
	vec4 z;
	vec4 w;

	inline static mat4 identity()
	{
		return mat4(
			1.0f, 0.0f, 0.0f, 0.0f, 
			0.0f, 1.0f, 0.0f, 0.0f, 
			0.0f, 0.0f, 1.0f, 0.0f, 
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline mat4(
		float cxx, float cxy, float cxz, float cxw,
		float cyx, float cyy, float cyz, float cyw,
		float czx, float czy, float czz, float czw,
		float cwx, float cwy, float cwz, float cww) :
		x(cxx, cxy, cxz, cxw),
		y(cyx, cyy, cyz, cyw),
		z(czx, czy, czz, czw),
		w(cwx, cwy, cwz, cww)
	{}

	inline void translate(float tx, float ty, float tz)
	{
		w.x += x.x * tx + y.x * ty + z.x * tz;
		w.y += x.y * tx + y.y * ty + z.y * tz;
		w.z += x.z * tx + y.z * ty + z.z * tz;
		w.w += x.w * tx + y.w * ty + z.w * tz;
	}

	inline void rotate(float angle, float rx, float ry, float rz)
	{
		float radians = (angle * float(M_PI)) / 180.0f;
		float s = sinf(radians);
		float c = cosf(radians);
		float mag = sqrtf(rx * rx + ry * ry + rz * rz);
		if(mag > 0.0f) {
			rx /= mag;
			ry /= mag;
			rz /= mag;

			mat4 rot_mat(
				rx*rx*(1.0f-c)+c   , ry*rx*(1.0f-c)-rz*s, rz*rx*(1.0f-c)+ry*s, 0.0f,
				rx*ry*(1.0f-c)+rz*s, ry*ry*(1.0f-c)+c   , rz*ry*(1.0f-c)-rx*s, 0.0f,
				rx*rz*(1.0f-c)-ry*s, ry*rz*(1.0f-c)+rx*s, rz*rz*(1.0f-c)+c   , 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);
			*this = rot_mat * *this;
		}
	}

	inline void translate(vec4 t)
	{
		w.x += x.x * t.x + y.x * t.y + z.x * t.z;
		w.y += x.y * t.x + y.y * t.y + z.y * t.z;
		w.z += x.z * t.x + y.z * t.y + z.z * t.z;
		w.w += x.w * t.x + y.w * t.y + z.w * t.z;
	}

	inline void scale(float sx, float sy, float sz)
	{
		x *= sx;
		y *= sy;
		z *= sz;
	}

	friend mat4 operator *(const mat4& a, const mat4& b);
};

inline mat4 operator*(const mat4& a, const mat4& b)
{
    return mat4(
        a.x.x*b.x.x + a.y.x*b.x.y + a.z.x*b.x.z + a.w.x*b.x.w,
        a.x.y*b.x.x + a.y.y*b.x.y + a.z.y*b.x.z + a.w.y*b.x.w,
        a.x.z*b.x.x + a.y.z*b.x.y + a.z.z*b.x.z + a.w.z*b.x.w,
        a.x.w*b.x.x + a.y.w*b.x.y + a.z.w*b.x.z + a.w.w*b.x.w,
        a.x.x*b.y.x + a.y.x*b.y.y + a.z.x*b.y.z + a.w.x*b.y.w,
        a.x.y*b.y.x + a.y.y*b.y.y + a.z.y*b.y.z + a.w.y*b.y.w,
        a.x.z*b.y.x + a.y.z*b.y.y + a.z.z*b.y.z + a.w.z*b.y.w,
        a.x.w*b.y.x + a.y.w*b.y.y + a.z.w*b.y.z + a.w.w*b.y.w,
        a.x.x*b.z.x + a.y.x*b.z.y + a.z.x*b.z.z + a.w.x*b.z.w,
        a.x.y*b.z.x + a.y.y*b.z.y + a.z.y*b.z.z + a.w.y*b.z.w,
        a.x.z*b.z.x + a.y.z*b.z.y + a.z.z*b.z.z + a.w.z*b.z.w,
        a.x.w*b.z.x + a.y.w*b.z.y + a.z.w*b.z.z + a.w.w*b.z.w,
        a.x.x*b.w.x + a.y.x*b.w.y + a.z.x*b.w.z + a.w.x*b.w.w,
        a.x.y*b.w.x + a.y.y*b.w.y + a.z.y*b.w.z + a.w.y*b.w.w,
        a.x.z*b.w.x + a.y.z*b.w.y + a.z.z*b.w.z + a.w.z*b.w.w,
        a.x.w*b.w.x + a.y.w*b.w.y + a.z.w*b.w.z + a.w.w*b.w.w);
}

#endif // VECMATH_HPP_INCLUDED
