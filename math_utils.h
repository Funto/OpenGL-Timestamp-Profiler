// math_utils.h

#ifndef MAT_UTILS_H
#define MAT_UTILS_H

#include <math.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

// -----------------------------------------
// Vectors
inline float vecLength(const float v[3])
{
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

inline float vecNormalize(float v[3])
{
	float len = vecLength(v);
	v[0] /= len;
	v[1] /= len;
	v[2] /= len;
	return len;
}

inline void vecCross(float result[3], const float a[3], const float b[3])
{
	result[0] = a[1] * b[2] - a[2] * b[1];
	result[1] = - (a[0] * b[2] - a[2] * b[0]);
	result[2] = a[0] * b[1] - a[1] * b[0];
}

inline float vecDot(const float a[3], const float b[3])
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// -----------------------------------------
// Matrices are stored following the OpenGL convention:
// float mat[16];	-> "column-major order": mat[0..3] is the first column, etc.
//
// 0  4  8  12
// 1  5  9  13
// 2  6  10 14
// 3  7  11 15
//
// mat[i] -> ith column

#define MATRIX_IDENTITY		{	1.0f, 0.0f, 0.0f, 0.0f,		\
								0.0f, 1.0f, 0.0f, 0.0f,		\
								0.0f, 0.0f, 1.0f, 0.0f,		\
								0.0f, 0.0f, 0.0f, 1.0f	}

// result = a*b
void matrixMult(float result[16], const float* a, const float* b);

// Translation
void matrixTranslate(float result[16], const float v[3]);

// Rotation
void matrixRotateX(float result[16], float theta_degrees);
void matrixRotateY(float result[16], float theta_degrees);
void matrixRotateZ(float result[16], float theta_degrees);

// Projection matrix handling - see http://www.opengl.org/wiki/GluPerspective_code
void matrixPerspective(float result[16], float fovy_degrees, float aspect_ratio,
					  float znear, float zfar);

// glFrustum() equivalent
void matrixFrustum(	float result[16],
					float left, float right,
					float bottom, float top,
					float znear, float zfar);

// gluLookAt() equivalent
void matrixLookAt(	float result[16],
					const float eye[3],
					const float target[3],
					const float up[3]);

#endif // MAT_UTILS_H
