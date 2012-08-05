Introduction
------------

This code is an implementation of the graphical real-time profiler using OpenGL timer queries and precise OS-specific CPU time queries that is described in the chapter "A real-time profiling tool" from the book "OpenGL Insights".
This code is provided in the hope that it will be useful.
Patches and improvements are welcome. The code is also available on the following Git repository:
	https://github.com/Funto/OpenGL-Timestamp-Profiler

Licensing
---------
This code is covered by the WTFPL (Do What The Fuck You Want To Public License).

Hardware and driver requirements
--------------------------------
You need at least an OpenGL 3.2 compatible GPU and driver with the ARB_timer_query extension to run the demo.

Dependencies
------------
This code depends on the libraries GLEW and GLFW, which are part of the source tree.

Compiling
---------

* Windows:
To compile with Visual C++ 2010, use the provided project files.
To compile with the MinGW compiler, use the command:
	make -f Makefile.mingw
	
* MacOS X:
To compile on MacOS X, use the command:
	make -f Makefile.osx

* Linux:
To compile on Linux, you need to install SCons, GLEW and GLFW, and type:
	scons

Authors
-------

Lionel Fuentes: main contributor
Robert Menzel: patches for MacOS X support
