// tgaloader.cpp
// Version 2.1

#include "tgaloader.h"

#ifdef TGA_USE_LOG_H
#include "../log/Log.h"
#endif

using namespace std;

TGALoader::TGALoader()
: data(NULL), loaded(false), width(0), height(0), bpp(0)
{
}

TGALoader::TGALoader(const TGALoader& ref)
: loaded(ref.loaded), width(ref.width), height(ref.height), bpp(ref.bpp)
{
	if(loaded)
	{
		data = new unsigned char[width*height*bpp];
		for(unsigned int i=0; i<width*height*bpp ; i++)
			data[i] = ref.data[i];
	}
	else
		data = NULL;
}

TGALoader::TGALoader(const char* path, TGAErrorCode* error)
: data(NULL), loaded(false), width(0), height(0), bpp(0)
{
	if (error == NULL) loadFile(path);
	else *error = loadFile(path);
}

TGALoader::~TGALoader()
{
	this->free();
}

TGAErrorCode TGALoader::loadFile(const char* path)
{
	unsigned char header[18];
	ifstream file((const char*)path, ios::in | ios::binary);
	unsigned int i=0, j=0;
	bool origin_non_bottoleft=false;	// Indicates if the origin of the image is
										// bottom left (default) or top left.

#ifdef TGA_USE_LOG_H
	logInfo("loading \"", path, "\"...");
#endif

#ifdef TGA_USE_LOG_IOSTREAM
	cout << "loading \"" << path << "\"..." << endl;
#endif

	if (!file)
	{
#ifdef TGA_USE_LOG_H
		logFailed(TGA_FILE_NOT_FOUND);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
		cerr << "TGA_FILE_NOT_FOUND" << endl;
#endif
		return TGA_FILE_NOT_FOUND;
	}

	file.read((char*)header, 18);

	this->free();

	width = ((unsigned int)header[13] << 8) + (unsigned int)header[12];
	height = ((unsigned int)header[15] << 8) + (unsigned int)header[14];
	bpp = ((unsigned int)header[16])>>3;	// (x>>3 is the same as x/8)

	// Bit 5 from byte 17
	origin_non_bottoleft = ((header[17] & 0x20) == 0x20 ? true : false);

	if ((data = (unsigned char*)malloc(height*width*bpp)) == NULL)
	{
		loaded = false;
#ifdef TGA_USE_LOG_H
		logFailed(TGA_NOT_ENOUGH_MEMORY);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
		cerr << "TGA_NOT_ENOUGH_MEMORY" << endl;
#endif
		return TGA_NOT_ENOUGH_MEMORY;
	}

	// Put the file pointer after the header (18 first bytes) and after the
	// identification field, which size is indicated in header[0].
	file.seekg(18+(long)header[0], ios::beg);

	// header[2] determines the type of image:
	// 1  -> uncompressed, uses a palette
	// 2  -> uncompressed, true colors
	// 9  -> RLE compressed, uses a palette
	// 10 -> RLE compressed, doesn't use a palette
	// Other image types are not supported.

	// TYPE 2 - uncompressed, true colors, 24 or 32 bits
	if (header[2] == 2 && (bpp == 3 || bpp == 4))
	{
		for(i=0 ; i<width*height*bpp ; i+=bpp)
		{
			data[i+2] = file.get();					// R
			data[i+1] = file.get();					// G
			data[i] = file.get();						// B
			if(bpp == 4) data[i+3] = file.get();	// A
		}
	}

	// TYPE 10 - RLE, true colors, 24 or 32 bits
	else if (header[2] == 10 && (bpp == 3 || bpp == 4))
	{
		unsigned char packet_header, red, green, blue, alpha=0;
		unsigned int nb_pixels;	// Number of pixels represented by one packet
		for(i=0 ; i<width*height*bpp ; i += nb_pixels*bpp)
		{
			packet_header = file.get();
			nb_pixels = (unsigned int)(packet_header & 0x7F) + 1;

			// RLE packet:
			if ((packet_header & 0x80) == 0x80)
			{
				blue = file.get();
				green = file.get();
				red = file.get();
				if(bpp == 4) alpha = file.get();

				for(j=0 ; j<nb_pixels*bpp ; j += bpp)
				{
					data[i+j] = red;
					data[i+j+1] = green;
					data[i+j+2] = blue;
					if(bpp == 4) data[i+j+3] = alpha;
				}
			}
			// Raw packet:
			else
			{
				for(j=0 ; j<nb_pixels*bpp ; j+=bpp)
				{
					data[i+j+2] = file.get();					// B
					data[i+j+1] = file.get();					// G
					data[i+j] = file.get();					// R
					if(bpp == 4) data[i+j+3] = file.get();	// A
				}
			}
		}
	}

	// For images of an unsupported type, return an error and load nothing
	else
	{
		::free(data);
		data = NULL;
		loaded = false;
#ifdef TGA_USE_LOG_H
		logFailed(TGA_UNSUPPORTED_TYPE);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
		cerr << TGA_UNSUPPORTED_TYPE << endl;
#endif
		return TGA_UNSUPPORTED_TYPE;
	}

	// Flip vertically for images where the origin is in the top left
	if (origin_non_bottoleft)
	{
		unsigned char temp;
		for(i=0 ; i<height/2 ; i++)
		{
			for(j=0 ; j<width*bpp ; j++)
			{
				temp = data[i*width*bpp + j];
				data[i*width*bpp + j] = data[(height-i-1)*width*bpp + j];
				data[(height-i-1)*width*bpp + j] = temp;
			}
		}
	}

	loaded = true;
	file.close();

#ifdef TGA_USE_LOG_H
	logSuccess(TGA_OK);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
	cout << TGA_OK << endl;
#endif
	return TGA_OK;
}


TGAErrorCode TGALoader::loadFromData(unsigned char *data)
{
	unsigned int i=0, j=0, pos_data=0;
	bool origin_non_bottoleft=false;	// Indicates if the origin of the image is
										// at bottom-left (default) or top-left

#ifdef TGA_USE_LOG_H
	logInfo("loading from data...");
#endif
#ifdef TGA_USE_LOG_IOSTREAM
	cout << "loading from data..." << endl;
#endif

	this->free();

	width = ((unsigned int)data[13] << 8) + (unsigned int)data[12];
	height = ((unsigned int)data[15] << 8) + (unsigned int)data[14];
	bpp = ((unsigned int)data[16])>>3;	// (x>>3 is the same as x/8)

	// Bit 5 from byte 17
	origin_non_bottoleft = ((data[17] & 0x20) == 0x20 ? true : false);

	if ((this->data = (unsigned char*)malloc(height*width*bpp)) == NULL)
	{
		loaded = false;
#ifdef TGA_USE_LOG_H
		logFailed(TGA_NOT_ENOUGH_MEMORY);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
		cerr << TGA_NOT_ENOUGH_MEMORY << endl;
#endif
		return TGA_NOT_ENOUGH_MEMORY;
	}

	// Put the file pointer after the header (18 first bytes) and after the
	// identification field, which size is indicated in header[0].
	pos_data = 18+(unsigned int)data[0];

	// data[2] determines the type of image:
	// 1  -> uncompressed, uses a palette
	// 2  -> uncompressed, true colors
	// 9  -> RLE compressed, uses a palette
	// 10 -> RLE compressed, doesn't use a palette
	// Other image types are not supported.

	// TYPE 2 - uncompressed, true colors, 24 or 32 bits
	if (data[2] == 2 && (bpp == 3 || bpp == 4))
	{
		for(i=0 ; i<width*height*bpp ; i+=bpp)
		{
			this->data[i+2] = data[pos_data]; pos_data++;					// R
			this->data[i+1] =data[pos_data]; pos_data++;					// G
			this->data[i] = data[pos_data]; pos_data++;						// B
			if(bpp == 4) { this->data[i+3] = data[pos_data]; pos_data++;	} // A
		}
	}

	// TYPE 10 - RLE, true colors, 24 or 32 bits
	else if (data[2] == 10 && (bpp == 3 || bpp == 4))
	{
		unsigned char packet_header, red, green, blue, alpha=0;
		unsigned int nb_pixels;	// Number of pixels represented by one packet
		for(i=0 ; i<width*height*bpp ; i += nb_pixels*bpp)
		{
			packet_header = data[pos_data]; pos_data++;
			nb_pixels = (unsigned int)(packet_header & 0x7F) + 1;

			// RLE packet:
			if ((packet_header & 0x80) == 0x80)
			{
				blue = data[pos_data]; pos_data++;
				green = data[pos_data]; pos_data++;
				red = data[pos_data]; pos_data++;
				if(bpp == 4) { alpha = data[pos_data]; pos_data++; }

				for(j=0 ; j<nb_pixels*bpp ; j += bpp)
				{
					this->data[i+j] = red;
					this->data[i+j+1] = green;
					this->data[i+j+2] = blue;
					if(bpp == 4) this->data[i+j+3] = alpha;
				}
			}
			// Raw packet:
			else
			{
				for(j=0 ; j<nb_pixels*bpp ; j+=bpp)
				{
					this->data[i+j+2] = data[pos_data]; pos_data++;					// B
					this->data[i+j+1] = data[pos_data]; pos_data++;					// G
					this->data[i+j] = data[pos_data]; pos_data++;					// R
					if(bpp == 4) { this->data[i+j+3] = data[pos_data]; pos_data++; }	// A
				}
			}
		}
	}

	// For images of an unsupported type, return an error and load nothing
	else
	{
		::free(this->data);
		this->data = NULL;
		loaded = false;
#ifdef TGA_USE_LOG_H
		logFailed(TGA_UNSUPPORTED_TYPE);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
		cerr << TGA_UNSUPPORTED_TYPE << endl;
#endif
		return TGA_UNSUPPORTED_TYPE;
	}

	// Flip vertically for images where the origin is in the top left
	if (origin_non_bottoleft)
	{
		unsigned char temp;
		for(i=0 ; i<height/2 ; i++)
		{
			for(j=0 ; j<width*bpp ; j++)
			{
				temp = data[i*width*bpp + j];
				this->data[i*width*bpp + j] = data[(height-i-1)*width*bpp + j];
				this->data[(height-i-1)*width*bpp + j] = temp;
			}
		}
	}

	loaded = true;

#ifdef TGA_USE_LOG_H
	logSuccess(TGA_OK);
#endif
#ifdef TGA_USE_LOG_IOSTREAM
	cout << TGA_OK << endl;
#endif
	return TGA_OK;
}

TGALoader& TGALoader::operator=(const TGALoader& ref)
{
	loaded = ref.loaded;
	width = ref.width;
	height = ref.height;
	bpp = ref.bpp;

	if(loaded)
	{
		data = new unsigned char[width*height*bpp];
		for(unsigned int i=0; i<width*height*bpp ; i++)
			data[i] = ref.data[i];
	}
	else
		data = NULL;
	return *this;
}

// Convert an error to an explicit string
string TGALoader::errorToString(TGAErrorCode error)
{
	switch(error)
	{
	case TGA_OK:
		return "TGA image loaded";
		break;
	case TGA_FILE_NOT_FOUND:
		return "file not found";
		break;
	case TGA_UNSUPPORTED_TYPE:
		return "unsupported type";
		break;
	case TGA_NOT_ENOUGH_MEMORY:
		return "not enough memory";
		break;
	default:
		return "unknown TGA error...";
		break;
	}
}

#ifdef TGA_OPENGL_SUPPORT

GLuint TGALoader::sendToOpenGL(TGAFiltering filtering //=TGA_NO_FILTER
							   )
{
	GLuint result = 0;
	if(loaded)
	{
		glGenTextures(1, &result);
		sendToOpenGLWithID(result, filtering);
	}
	return result;
}

void TGALoader::sendToOpenGLWithID(	GLuint ID,
									TGAFiltering filtering	//=TGA_NO_FILTER
									)
{
	if (loaded)
	{
		glBindTexture(GL_TEXTURE_2D, ID);
		gluBuild2DMipmaps(
			GL_TEXTURE_2D,
			bpp,
			width,
			height,
			(bpp == 3 ? GL_RGB : GL_RGBA),
			GL_UNSIGNED_BYTE,
			data);

		if (filtering == TGA_NO_FILTER)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		else if (filtering == TGA_LINEAR)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		else if (filtering == TGA_BILINEAR)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_NEAREST);
		}
		else if (filtering == TGA_TRILINEAR)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
		}
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
		else if (filtering == TGA_ANISOTROPIC)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f) ;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
		}
#endif

		// NB: the application should do this, not the TGA loader...
		//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	}
}

TGAErrorCode TGALoader::loadOpenGLTexture(
		const char* path,
		GLuint* pID, //=NULL
		TGAFiltering filtering //=TGA_NO_FILTER
		)
{
	TGAErrorCode ret = loadFile(path);
	if(ret == TGA_OK)
	{
		if(pID != NULL)
			(*pID) = sendToOpenGL(filtering);
		else
			sendToOpenGL(filtering);
		this->free();
	}
	return ret;
}

TGAErrorCode TGALoader::loadOpenGLTextureWithID(
		const char* path,
		GLuint ID,
		TGAFiltering filtering //=TGA_NO_FILTER
		)
{
	TGAErrorCode ret = loadFile(path);
	if(ret == TGA_OK)
	{
		sendToOpenGLWithID(ID, filtering);
		this->free();
	}
	return ret;
}

TGAErrorCode TGALoader::loadOpenGLTextureFromData(
		unsigned char* data,
		GLuint* pID, //=NULL
		TGAFiltering filtering //=TGA_NO_FILTER
		)
{
	TGAErrorCode ret = loadFromData(data);
	if(ret == TGA_OK)
	{
		if(pID != NULL)
			(*pID) = sendToOpenGL(filtering);
		else
			sendToOpenGL(filtering);
		this->free();
	}
	return ret;
}

TGAErrorCode TGALoader::loadOpenGLTextureFromDataWithID(
		unsigned char* data,
		GLuint ID,
		TGAFiltering filtering //=TGA_NO_FILTER
		)
{
	TGAErrorCode ret = loadFromData(data);
	if(ret == TGA_OK)
	{
		sendToOpenGLWithID(ID, filtering);
		this->free();
	}
	return ret;
}

#endif	// defined TGA_OPENGL_SUPPORT

void TGALoader::free()
{
	if (loaded)
	{
		::free(data);
		data = NULL;
		loaded = false;
		height = 0;
		width = 0;
		bpp = 0;
	}
}
