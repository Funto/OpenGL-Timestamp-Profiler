// grid.cpp

#include "grid.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

const int	Grid::GRID_WIDTH = 300;
const int	Grid::GRID_HEIGHT = 300;

const int	Grid::GRID_NB_INDICES = 6 * (GRID_WIDTH-1) * (GRID_HEIGHT-1);

const float	Grid::GRID_WORLD_SIZE = 10.0f;

const float	Grid::GRID_Y_FLOOR = -3.0f;
const float	Grid::GRID_Y_DISTO = 0.5f;

Grid::Grid()
{
	m_time_offset = 0.0f;
}

bool Grid::init(float time_offset, const Color& color)
{
	m_time_offset = time_offset;
	m_color = color;

	// Load the shaders
	if(!loadShaders("media/grid.vert", "media/grid.frag", m_id_vert_grid, m_id_frag_grid, m_id_prog_grid))
	{
		fprintf(stderr, "*** ERROR: failed loading shaders for the grid\n");
		return false;
	}

	// Get the uniforms
	m_id_uniform_mvp = glGetUniformLocation(m_id_prog_grid, "MVP");
	if(m_id_uniform_mvp < 0)
	{
		fprintf(stderr, "*** Scene::init: FAILED retrieving uniform location\n");
		return false;
	}

	// Initialize the positions and colors for the grid
	m_grid_vertices = new Vertex[GRID_WIDTH * GRID_HEIGHT];

	float r = float(color.r) / 255.0f;
	float g = float(color.g) / 255.0f;
	float b = float(color.b) / 255.0f;

	for(int x=0 ; x < GRID_WIDTH ; x++)
	{
		for(int y=0 ; y < GRID_HEIGHT ; y++)
		{
			int i = x + y*GRID_WIDTH;

			m_grid_vertices[i].pos[0] = GRID_WORLD_SIZE * (float(x-GRID_WIDTH/2) / float(GRID_WIDTH));
			m_grid_vertices[i].pos[1] = GRID_Y_FLOOR;
			m_grid_vertices[i].pos[2] = GRID_WORLD_SIZE * (float(y-GRID_HEIGHT/2) / float(GRID_HEIGHT));

			//float fx = float(x) / float(GRID_WIDTH);
			float fy = float(y) / float(GRID_HEIGHT);

			float fr = fy*r;
			float fg = fy*g;
			float fb = fy*b;

			m_grid_vertices[i].col[0] = fr;
			m_grid_vertices[i].col[1] = fg;
			m_grid_vertices[i].col[2] = fb;
		}
	}

	// Initialize the indices
	m_grid_indices = new GLuint[GRID_NB_INDICES];

	int i = 0;
	for(GLuint x=0 ; x < (GLuint)(GRID_WIDTH-1) ; x++)
	{
		for(GLuint y=0 ; y < (GLuint)(GRID_HEIGHT-1) ; y++)
		{
			GLuint bottom_left	= x     + y    *(GLuint)(GRID_WIDTH);
			GLuint bottom_right	= (x+1) + y    *(GLuint)(GRID_WIDTH);
			GLuint top_right	= (x+1) + (y+1)*(GLuint)(GRID_WIDTH);
			GLuint top_left		= x     + (y+1)*(GLuint)(GRID_WIDTH);

			m_grid_indices[i++] = bottom_left;
			m_grid_indices[i++] = bottom_right;
			m_grid_indices[i++] = top_right;

			m_grid_indices[i++] = bottom_left;
			m_grid_indices[i++] = top_right;
			m_grid_indices[i++] = top_left;
		}
	}

	// Temporary buffer used for the simulation
	m_grid_temp_buffer = new float[GRID_WIDTH * GRID_HEIGHT];
	for(int i=0 ; i < GRID_WIDTH * GRID_HEIGHT ; i++)
		m_grid_temp_buffer[i] = GRID_Y_FLOOR;

	// IBO/VBO
	glGenBuffers(1, &m_id_ibo_grid);
	glGenBuffers(1, &m_id_vbo_grid);

	return true;
}

void Grid::shut()
{
	delete [] m_grid_indices;
	delete [] m_grid_vertices;
	delete [] m_grid_temp_buffer;
}

void Grid::update(double elapsed, double t)
{
	float shifted_t = (float)t + m_time_offset;

	for(int x=0 ; x < GRID_WIDTH ; x++)
	{
		float fx = float(x) / float(GRID_WIDTH);

		for(int y=0 ; y < GRID_HEIGHT ; y++)
		{
			float fy = float(y) / float(GRID_HEIGHT);

			int index = x + y*GRID_WIDTH;

			float f = 0.0f;
			f += cosf(10.0f*fx + 10.0f*shifted_t);
			f += sinf(13.0f*fy + 8.0f*shifted_t);
			m_grid_vertices[index].pos[1] = GRID_Y_FLOOR + f*GRID_Y_DISTO;
		}
	}
}

void Grid::draw(const float mvp_matrix[16])
{
	// Setup shader
	glUseProgram(m_id_prog_grid);
	glUniformMatrix4fv(m_id_uniform_mvp, 1, GL_FALSE, mvp_matrix);

	// Setup IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id_ibo_grid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*GRID_NB_INDICES, (const GLvoid*)m_grid_indices, GL_DYNAMIC_DRAW);

	// Setup VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_id_vbo_grid);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * GRID_WIDTH * GRID_HEIGHT, (const GLvoid*)m_grid_vertices, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);	// vertices
	glEnableVertexAttribArray(1);	// color

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0 );	// vertices
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(GLfloat)));	// colors

	glDrawElements(GL_TRIANGLES, GRID_NB_INDICES, GL_UNSIGNED_INT, (const GLvoid*)0);

	glDisableVertexAttribArray(0);	// vertices
	glDisableVertexAttribArray(1);	// color
}
