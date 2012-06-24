// Camera.h

#ifndef CAMERA_H
#define CAMERA_H

#include "math_utils.h"

struct Camera
{
	float view_matrix[16];
	float proj_matrix[16];

	void setPerspective(float fovy_degrees, float aspect_ratio,
						float znear, float zfar)
	{
		matrixPerspective(proj_matrix, fovy_degrees, aspect_ratio, 1.0f, 100.0f);
	}

	void lookAt(const float eye[3],
	            const float target[3],
				const float up[3])
	{
		matrixLookAt(view_matrix, eye, target, up);
	}
};

#endif // CAMERA_H
