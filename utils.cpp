// utils.cpp

#include "utils.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

const Color	COLOR_WHITE			= Color(0xFF, 0xFF, 0xFF);
const Color	COLOR_BLACK			= Color(0x00, 0x00, 0x00);
const Color	COLOR_GRAY			= Color(0x7F, 0x7F, 0x7F);
const Color	COLOR_LIGHT_GRAY	= Color(0xC0, 0xC0, 0xC0);

const Color	COLOR_RED			= Color(0xFF, 0x00, 0x00);
const Color	COLOR_GREEN			= Color(0x00, 0xFF, 0x00);
const Color	COLOR_BLUE			= Color(0x00, 0x00, 0xFF);

const Color	COLOR_DARK_RED		= Color(0x7F, 0x00, 0x00);
const Color	COLOR_DARK_GREEN	= Color(0x00, 0x7F, 0x00);
const Color	COLOR_DARK_BLUE		= Color(0x00, 0x00, 0x7F);

const Color	COLOR_LIGHT_RED		= Color(0xFF, 0x7F, 0x7F);
const Color	COLOR_LIGHT_GREEN	= Color(0x7F, 0xFF, 0x7F);
const Color	COLOR_LIGHT_BLUE	= Color(0x7F, 0x7F, 0xFF);

const Color	COLOR_CYAN			= Color(0x00, 0xFF, 0xFF);
const Color	COLOR_MAGENTA		= Color(0xFF, 0x00, 0xFF);
const Color	COLOR_YELLOW		= Color(0xFF, 0xFF, 0x00);

#ifdef WIN32
	#include <windows.h>
	void msleep(int ms)
	{
		Sleep(ms);
	}
#else
	#include <unistd.h>
	void msleep(int ms)
	{
		usleep(ms*1000);
	}
#endif

// ---------------------------------------------------------------------
// Load a text file:
const char* loadText(const char* filename)
{
	std::ifstream f(filename);
	if(!f)
	{
		fprintf(stderr, "impossible to read the file \"%s\"\n", filename);
		return NULL;
	}

	int filesize = 0;
	char* buf = NULL;

	// Get the file size
	f.seekg(0, std::ios::end);
	filesize = f.tellg();

	// Allocate memory for the text and get the file's contents
	buf = new char[filesize+1];
	memset(buf, 0, filesize);
	f.seekg(0, std::ios::beg);
	f.read(buf, filesize);
	buf[filesize] = '\0';

	f.close();

	return buf;
}

bool	loadShaders(const char *vert_filename, const char *frag_filename,
					GLuint &id_vert, GLuint &id_frag, GLuint &id_prog)
{
	const char*	src;
	GLchar*		buf;
	GLint		len;
	GLint		status;

	// --- Vertex shader ---
	src = loadText(vert_filename);
	if(!src)
	{
		id_vert = id_frag = id_prog = 0;
		fprintf(stderr, "*** FAILED: file %s not found\n", vert_filename);
		return false;
	}

	id_vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(id_vert, 1, &src, NULL);
	glCompileShader(id_vert);
	delete [] src;

	// Print the log:
	glGetShaderiv(id_vert, GL_INFO_LOG_LENGTH, &len);

	buf = new GLchar[len];
	glGetShaderInfoLog(id_vert, len, &len, buf);
	printf("[%s]: vertex shader log:\n%s\n", vert_filename, buf);
	delete [] buf;

	// Check if it compiled
	glGetShaderiv(id_vert, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE)
	{
		glDeleteShader(id_vert);
		id_vert = id_frag = id_prog = 0;
		fprintf(stderr, "*** FAILED compiling vertex shader %s\n", vert_filename);
		return false;
	}

	// --- Fragment shader ---
	src = loadText(frag_filename);
	if(!src)
	{
		glDeleteShader(id_vert);
		id_vert = id_frag = id_prog = 0;
		fprintf(stderr, "*** FAILED: file %s not found\n", frag_filename);
		return false;
	}

	id_frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(id_frag, 1, &src, NULL);
	glCompileShader(id_frag);
	delete [] src;

	// Print the log:
	glGetShaderiv(id_frag, GL_INFO_LOG_LENGTH, &len);

	buf = new GLchar[len];
	glGetShaderInfoLog(id_frag, len, &len, buf);
	printf("[%s]: fragment shader log:\n%s\n", frag_filename, buf);
	delete [] buf;

	// Check if it compiled
	glGetShaderiv(id_vert, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE)
	{
		glDeleteShader(id_frag);
		glDeleteShader(id_vert);
		id_vert = id_frag = id_prog = 0;
		fprintf(stderr, "*** FAILED compiling fragment shader %s\n", frag_filename);
		return false;
	}

	// --- Program ---
	id_prog = glCreateProgram();
	glAttachShader(id_prog, id_vert);
	glAttachShader(id_prog, id_frag);
	glLinkProgram(id_prog);

	// Print the log:
	glGetProgramiv(id_prog, GL_INFO_LOG_LENGTH, &len);

	buf = new GLchar[len];
	glGetProgramInfoLog(id_prog, len, &len, buf);
	printf("[%s][%s]: program log:\n%s\n", vert_filename, frag_filename, buf);
	delete [] buf;

	// Check if the linkage was successful
	glGetProgramiv(id_prog, GL_LINK_STATUS, &status);
	if(status != GL_TRUE)
	{
		glDeleteProgram(id_prog);
		glDeleteShader(id_frag);
		glDeleteShader(id_vert);
	}

	return true;
}

bool checkGLError()
{
	GLenum error = glGetError();
	if(error != GL_NO_ERROR)
	{
		const char* error_msg = NULL;
		switch(error)
		{
		case GL_INVALID_ENUM:					error_msg = "GL_INVALID_ENUM";	break;
		case GL_INVALID_VALUE:					error_msg = "GL_INVALID_VALUE";	break;
		case GL_INVALID_OPERATION:				error_msg = "GL_INVALID_OPERATION";	break;
		case GL_OUT_OF_MEMORY:					error_msg = "GL_OUT_OF_MEMORY";	break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:	error_msg = "GL_INVALID_FRAMEBUFFER_OPERATION";	break;
		default:
			fprintf(stderr, "*** OpenGL ERROR: unknown error: 0x%x\n", error);
			return false;
		}

		fprintf(stderr, "*** OpenGL ERROR: %s\n", error_msg);
	}
	return true;
}
