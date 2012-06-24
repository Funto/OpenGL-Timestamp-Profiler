// grid.frag

#version 330 core

// ---------------------------------------------------------------------
// Default precision:
//precision highp float;
//precision mediump int;

// ---------------------------------------------------------------------
in vec3 var_color;

out vec4 frag_color;

// ---------------------------------------------------------------------
void main()
{
	frag_color = vec4(var_color, 1.0);
}
