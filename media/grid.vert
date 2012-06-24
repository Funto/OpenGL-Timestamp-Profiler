// grid.vert

#version 330 core

// ---------------------------------------------------------------------
// Default precision
//precision highp float;
//precision highp int;

uniform mat4 MVP;

// ---------------------------------------------------------------------
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;

out vec3 var_color;

// ---------------------------------------------------------------------
void main()
{
	var_color = vertex_color;

	gl_Position = MVP * vec4(vertex_position, 1.0);
}
