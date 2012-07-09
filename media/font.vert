// font.vert

#version 330 core

// ---------------------------------------------------------------------
// Default precision
//precision highp float;
//precision highp int;

// ---------------------------------------------------------------------
//in vec2 vertex_position;
layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 vertex_uv;

out vec2	uv;

// ---------------------------------------------------------------------
void main()
{
	vec2 xy = 2.0*vertex_position - vec2(1.0);

	uv = vertex_uv;	// TODO: bug where we see a part of the letter above...

	gl_Position = vec4(xy, 0.0, 1.0);
}
