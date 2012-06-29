// grid.h

#ifndef GRID_H
#define GRID_H

#include <GL/glew.h>

#include "camera.h"
#include "utils.h"

class Grid
{
private:
	static const int	GRID_WIDTH;
	static const int	GRID_HEIGHT;

	static const int	GRID_NB_INDICES;

	static const float	GRID_WORLD_SIZE;

	static const float	GRID_Y_FLOOR;
	static const float	GRID_Y_DISTO;

	struct Vertex
	{
		GLfloat pos[3];
		GLfloat col[3];
	};

	Vertex*	m_grid_vertices;
	GLuint*	m_grid_indices;
	float*	m_grid_temp_buffer;

	float	m_time_offset;

	// Shader IDs
	GLuint	m_id_vert_grid;
	GLuint	m_id_frag_grid;
	GLuint	m_id_prog_grid;

	// Shader uniforms
	GLint	m_id_uniform_mvp;

	// VBO/IBO
	GLuint	m_id_ibo_grid;
	GLuint	m_id_vbo_grid;

	Camera	m_camera;

	Color	m_color;

public:
	Grid();
	bool	init(float time_offset, const Color& color);
	void	shut();

	const	Color&	getColor() const	{return m_color;}

	const	Camera&	getCamera() const	{return m_camera;}
			Camera&	getCamera()			{return m_camera;}

	void	update(double elapsed, double t);
	void	draw(const float mvp_matrix[16]);
};

#endif // GRID_H
