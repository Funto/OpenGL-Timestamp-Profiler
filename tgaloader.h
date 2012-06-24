// tgaloader.h
// Version 2.1

#ifndef TGA_LOADER_H
#define TGA_LOADER_H

// Configuration:
#define TGA_LOADER_INCLUDE_GLEW	// Define this if the project uses GLEW,
								// for including GL/glew.h before GL/gl.h
//#define TGA_OPENGL_SUPPORT
//#define TGA_USE_LOG_H	// Define this to use log/Log.h for logging (logError(), etc)
#define TGA_USE_LOG_IOSTREAM	// Define this to use std::cout/std::cerr for logging

// ---------------------------------------------------------------------
#ifdef TGA_LOADER_INCLUDE_GLEW
#include <GL/glew.h>
#endif

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#ifdef TGA_OPENGL_SUPPORT
	#ifdef WIN32
	#include <windows.h>
	#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif // defined TGA_OPENGL_SUPPORT

enum TGAErrorCode
{
	TGA_OK,
	TGA_FILE_NOT_FOUND,
	TGA_UNSUPPORTED_TYPE,
	TGA_NOT_ENOUGH_MEMORY
};

#ifdef TGA_OPENGL_SUPPORT
enum TGAFiltering
{
	TGA_NO_FILTER,
	TGA_LINEAR,
	TGA_BILINEAR,
	TGA_TRILINEAR
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
	,TGA_ANISOTROPIC
#endif
};
#endif

class TGALoader
{
private:
	unsigned char* data;
	bool loaded;
	unsigned int width, height;
	unsigned int bpp;	// Bytes Per Pixel : 0, 3 or 4

public:
	TGALoader();
	TGALoader(const TGALoader& ref);
	TGALoader(const char* path, TGAErrorCode* error=NULL);
	virtual ~TGALoader();
	TGAErrorCode loadFile(const char* path);
	TGAErrorCode loadFromData(unsigned char *data);

	TGALoader& operator=(const TGALoader& ref);

	// Convert an error to an explicit string
	static std::string errorToString(TGAErrorCode error);

#ifdef TGA_OPENGL_SUPPORT
	GLuint sendToOpenGL(TGAFiltering filtering=TGA_NO_FILTER);

	void sendToOpenGLWithID(GLuint ID, TGAFiltering filtering=TGA_NO_FILTER);

	TGAErrorCode loadOpenGLTexture(const char* path, GLuint* pID=NULL,
		TGAFiltering filtering=TGA_NO_FILTER);

	TGAErrorCode loadOpenGLTextureWithID(const char* path, GLuint ID,
		TGAFiltering filtering=TGA_NO_FILTER);

	TGAErrorCode loadOpenGLTextureFromData(unsigned char *data, GLuint* pID=NULL,
		TGAFiltering filtering=TGA_NO_FILTER);

	TGAErrorCode loadOpenGLTextureFromDataWithID(unsigned char *data, GLuint ID,
		TGAFiltering filtering=TGA_NO_FILTER);

#endif

	void free();

	inline unsigned char* getData() {return data;}
	inline bool isLoaded() {return loaded;}
	inline unsigned int getHeight() {return height;}
	inline unsigned int getWidth() {return width;}
	inline unsigned int getBpp() {return bpp;}
};

// Overloading << for printing error codes using iostream
inline std::ostream& operator<<(std::ostream& os, const TGAErrorCode& error)
{
	return os << TGALoader::errorToString(error);
}

#endif // !defined TGA_LOADER_H
