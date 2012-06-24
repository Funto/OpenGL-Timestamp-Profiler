// utils.h

#ifndef UTILS_H
#define UTILS_H

#include <GL/glew.h>

void		msleep(int ms);
const char* loadText(const char* filename);
bool		loadShaders(const char* vert_filename, const char* frag_filename,
						GLuint&	id_vert, GLuint& id_frag, GLuint& id_prog);
bool		checkGLError();

template <class T>
T clamp(T val, T min_val, T max_val)
{
	if(val < min_val)
		val = min_val;
	if(val > max_val)
		val = max_val;
	return val;
}

inline void	incrementCycle(int* pval, size_t array_size)
{
	int val = *pval;
	val++;
	if(val >= (int)(array_size))
		val = 0;
	*pval = val;
}

inline void	decrementCycle(int* pval, size_t array_size)
{
	int val = *pval;
	val--;
	if(val < 0)
		val = (int)(array_size-1);
	*pval = val;
}

struct Rect
{
	float x, y;
	float w, h;

	Rect() : x(0.0f), y(0.0f), w(0.0f), h(0.0f) {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

	void set(float _x, float _y, float _w, float _h)
	{
		x = _x;	y = _y;	w = _w;	h = _h;
	}

	bool isPointInside(float _x, float _y) const
	{
		return (_x >= x && _x < x+w		&&
				_y >= y && _y < y+h		);
	}
};

struct Color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;

	Color() : r(0xFF), g(0xFF), b(0xFF) {}
	Color(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {}

	void set(unsigned char _r, unsigned char _g, unsigned char _b)
	{
		r = _r;	g = _g;	b = _b;
	}

	void set(const Color&	color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
	}
};

extern const Color	COLOR_WHITE;
extern const Color	COLOR_BLACK;
extern const Color	COLOR_GRAY;
extern const Color	COLOR_LIGHT_GRAY;

extern const Color	COLOR_RED;
extern const Color	COLOR_GREEN;
extern const Color	COLOR_BLUE;

extern const Color	COLOR_DARK_RED;
extern const Color	COLOR_DARK_GREEN;
extern const Color	COLOR_DARK_BLUE;

extern const Color	COLOR_LIGHT_RED;
extern const Color	COLOR_LIGHT_GREEN;
extern const Color	COLOR_LIGHT_BLUE;

extern const Color	COLOR_CYAN;
extern const Color	COLOR_MAGENTA;
extern const Color	COLOR_YELLOW;


#endif // UTILS_H
