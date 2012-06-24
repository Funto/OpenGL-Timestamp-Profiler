// font.frag

//#version 330 core
#version 330

// ---------------------------------------------------------------------
// Default precision:
//precision highp float;
//precision mediump int;

// ---------------------------------------------------------------------
uniform vec3		font_color;
uniform sampler2D	tex_font;

// ---------------------------------------------------------------------
in vec2	uv;

// ---------------------------------------------------------------------
out vec4 frag_color;

// ---------------------------------------------------------------------
void main()
{
	float a = texture(tex_font, uv).a;
	vec3 color = font_color;
	frag_color = vec4(color,a);
}
