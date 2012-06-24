// color.frag

//#version 330 core
#version 330

// ---------------------------------------------------------------------
// Default precision:
//precision highp float;
//precision mediump int;

// ---------------------------------------------------------------------
uniform vec4 color;

// ---------------------------------------------------------------------
out vec4 frag_color;

// ---------------------------------------------------------------------
void main()
{
	frag_color = color;
//	gl_FragColor = color;
}
