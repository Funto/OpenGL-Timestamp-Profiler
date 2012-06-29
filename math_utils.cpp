// math_utils.cpp

#include "math_utils.h"

// result = a*b
void matrixMult(float result[16], const float* a, const float* b)
{
	for(int i=0 ; i<4 ; i++)
	{
		for(int j=0 ; j<4 ; j++)
		{
			float val = 0.0f;

			for(int k=0 ; k<4 ; k++)
				val += a[i + 4*k] * b[k + 4*j];

			result[i + 4*j] = val;
		}
	}
}

// Translation
void matrixTranslate(float result[16], const float v[3])
{
	result[0] = 1.0f;	result[4] = 0.0f;	result[8]	= 0.0f;	result[12] = v[0];
	result[1] = 0.0f;	result[5] = 1.0f;	result[9]	= 0.0f;	result[13] = v[1];
	result[2] = 0.0f;	result[6] = 0.0f;	result[10]	= 1.0f;	result[14] = v[2];
	result[3] = 0.0f;	result[7] = 0.0f;	result[11]	= 0.0f;	result[15] = 1.0f;
}

// Rotation

void matrixRotateX(float result[16], float theta_degrees)
{
	theta_degrees *= (PI_FLOAT / 180.0f);
	float s = sinf(theta_degrees);
	float c = cosf(theta_degrees);

	result[0] = 1.0f;	result[4] = 0.0f;	result[8]	= 0.0f;	result[12] = 0.0f;
	result[1] = 0.0f;	result[5] = c;		result[9]	= -s;	result[13] = 0.0f;
	result[2] = 0.0f;	result[6] = s;		result[10]	= c;	result[14] = 0.0f;
	result[3] = 0.0f;	result[7] = 0.0f;	result[11]	= 0.0f;	result[15] = 1.0f;
}

void matrixRotateY(float result[16], float theta_degrees)
{
	theta_degrees *= (PI_FLOAT / 180.0f);
	float s = sinf(theta_degrees);
	float c = cosf(theta_degrees);

	result[0] = c;		result[4] = 0.0f;	result[8]	= s;	result[12] = 0.0f;
	result[1] = 0.0f;	result[5] = 1.0f;	result[9]	= 0.0f;	result[13] = 0.0f;
	result[2] = -s;		result[6] = 0.0f;	result[10]	= c;	result[14] = 0.0f;
	result[3] = 0.0f;	result[7] = 0.0f;	result[11]	= 0.0f;	result[15] = 1.0f;
}

void matrixRotateZ(float result[16], float theta_degrees)
{
	theta_degrees *= (PI_FLOAT / 180.0f);
	float s = sinf(theta_degrees);
	float c = cosf(theta_degrees);

	result[0] = c;		result[4] = -s;		result[8]	= 0.0f;	result[12] = 0.0f;
	result[1] = s;		result[5] = c;		result[9]	= 0.0f;	result[13] = 0.0f;
	result[2] = 0.0f;	result[6] = 0.0f;	result[10]	= 1.0f;	result[14] = 0.0f;
	result[3] = 0.0f;	result[7] = 0.0f;	result[11]	= 0.0f;	result[15] = 1.0f;
}

// Projection matrix handling - see http://www.opengl.org/wiki/GluPerspective_code
void matrixPerspective(float result[16], float fovy_degrees, float aspect_ratio,
					  float znear, float zfar)
{
	float ymax, xmax;
	ymax = znear * tanf(fovy_degrees * PI_FLOAT / 360.0f);
	xmax = ymax * aspect_ratio;
	matrixFrustum(result, -xmax, xmax, -ymax, ymax, znear, zfar);
}

// glFrustum() equivalent
void matrixFrustum(	float result[16],
					float left, float right,
					float bottom, float top,
					float znear, float zfar)
{
	float temp, temp2, temp3, temp4;
	temp = 2.0f * znear;
	temp2 = right - left;
	temp3 = top - bottom;
	temp4 = zfar - znear;
	result[0] = temp / temp2;
	result[1] = 0.0f;
	result[2] = 0.0f;
	result[3] = 0.0f;
	result[4] = 0.0f;
	result[5] = temp / temp3;
	result[6] = 0.0f;
	result[7] = 0.0f;
	result[8] = (right + left) / temp2;
	result[9] = (top + bottom) / temp3;
	result[10] = (-zfar - znear) / temp4;
	result[11] = -1.0f;
	result[12] = 0.0f;
	result[13] = 0.0f;
	result[14] = (-temp * zfar) / temp4;
	result[15] = 0.0f;
}

// gluLookAt() equivalent
void matrixLookAt(	float result[16],
					const float eye[3],
					const float target[3],
					const float up[3])
{
	float forward[3] = {	target[0] - eye[0],
							target[1] - eye[1],
							target[2] - eye[2]	};
	vecNormalize(forward);

	float side[3];
	vecCross(side, forward, up);

	float up_after[3];
	vecCross(up_after, side, forward);

	float view_matrix[16] = MATRIX_IDENTITY;

	view_matrix[0] = side[0];
	view_matrix[4] = side[1];
	view_matrix[8] = side[2];

	view_matrix[1] = up_after[0];
	view_matrix[5] = up_after[1];
	view_matrix[9] = up_after[2];

	view_matrix[2]  = -forward[0];
	view_matrix[6]  = -forward[1];
	view_matrix[10] = -forward[2];

	// Final translation
	float trans_matrix[16];
	float minus_eye[3] = {-eye[0], -eye[1], -eye[2]};
	matrixTranslate(trans_matrix, minus_eye);

	matrixMult(result, view_matrix, trans_matrix);
}
