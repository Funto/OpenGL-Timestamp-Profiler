// main.cpp

#include <stdio.h>
#include "scene.h"

#include <GL/glew.h>

#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

#include <GL/glfw.h>
#include <stdlib.h>
#include <math.h>
#include "hp_timer.h"
#include "profiler2.h"
#include "drawer2D.h"
#include "thread.h"
#include "math_utils.h"

static volatile bool	done = false;
static volatile int		frame_count = 0;
Scene					scene;

void GLFWCALL onMouseClick(int x, int y);
void GLFWCALL onKey(int key, int action);

/*
void DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
				   GLsizei length, const GLchar* message, GLvoid* userParam)
{
	// TODO
	printf("*** DebugCallback\n");
}
*/

/*
void*	threadProc(void* data)
{
	while(!done)
	{
		int i = frame_count % 3;
		const Color* p_color = &COLOR_RED;
		if(i == 1)
			p_color = &COLOR_GREEN;
		else if(i == 2)
			p_color = &COLOR_BLUE;

		PROFILER2_PUSH_CPU_MARKER("threadProc", *p_color);
		msleep(10);
		PROFILER2_POP_CPU_MARKER();
		//usleep(100);
		msleep(3);
	}
	return NULL;
}
*/

// BEGIN BOUM
double full_frame = 0.0;
double update = 0.0;
double wait_updates = 0.0;
// END BOUM

int main()
{
	int win_w, win_h;
	int prev_win_w, prev_win_h;
	int mouse_x, mouse_y;
	int prev_mouse_x, prev_mouse_y;
	double t;

	initTimer();

	// Initialize GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return EXIT_FAILURE;
	}

	// Open a window and create its OpenGL context
//	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR,  3);

	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR,  3);
//	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR,  1);

	//glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//	glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);

//	glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	if( !glfwOpenWindow( 640, 480, 0,0,0,0, 0,0, GLFW_WINDOW ) )
	{
		fprintf(stderr, "Failed to open GLFW window\n");

		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwSetWindowTitle( "Profiler - OpenGL Insights" );

//	if(GLEW_ARB_debug_output)
//		glDebugMessageCallbackARB(&DebugCallback, NULL);

	// Load the OpenGL implementation + available extensions
	GLenum error = glewInit();
	if(error != GL_NO_ERROR)
	{
		fprintf(stderr, "Failed to initialize GLEW: error=%d\n", (int)error);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	// Setup a VAO for the whole time the program is executed
	GLuint id_vao;
	glGenVertexArrays(1, &id_vao);
	glBindVertexArray(id_vao);

	// Set the default OpenGL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Mouse and window size
	glfwGetMousePos(&mouse_x, &mouse_y);
	glfwGetWindowSize(&win_w, &win_h);

	glfwSetMouseButtonCallback(&onMouseClick);
	glfwSetKeyCallback(&onKey);

	// Initialize the 2D drawer
	if(!drawer2D.init(win_w, win_h))
	{
		fprintf(stderr, "*** FAILED initializing the Drawer2D\n");
		return EXIT_FAILURE;
	}

	// Initialize the profiler
	profiler2.init(win_w, win_h, mouse_x, mouse_y);

	// Initialize the example scene
	if(!scene.init())
	{
		fprintf(stderr, "*** FAILED initializing the scene\n");
		return EXIT_FAILURE;
	}

	// Enable vertical sync (on cards that support it)
	glfwSwapInterval( 1 );

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("OpenGL version: %d.%d\n", major, minor);

	// Create a thread
//	ThreadId	thread_id = threadCreate(&threadProc, NULL);

	// Main loop
	double elapsed = 0.0;
	double last_t = glfwGetTime();
	do
	{
		t = glfwGetTime();
		elapsed = t - last_t;

		PROFILER2_SYNC_FRAME();
		PROFILER2_PUSH_CPU_MARKER("Full frame", COLOR_GRAY);

		// Update mouse and window size
		prev_mouse_x = mouse_x;		prev_mouse_y = mouse_y;
		glfwGetMousePos( &mouse_x, &mouse_y);

		prev_win_w = win_w;			prev_win_h = win_h;
		win_h = win_h > 0 ? win_h : 1;

		glfwGetWindowSize(&win_w, &win_h);

		// Detect and notice of changes in the mouse position or the window size
		if(prev_mouse_x != mouse_x || prev_mouse_y != mouse_y)
		{
			profiler2.onMousePos(mouse_x, mouse_y);
		}
		if(prev_win_w != win_w || prev_win_h != win_h)
		{
			profiler2.onResize(win_w, win_h);
			drawer2D.onResize(win_w, win_h);
		}

		glViewport( 0, 0, win_w, win_h );

		// Clear color buffer to black
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glEnable(GL_DEPTH_TEST);

//		PROFILER2_PUSH_CPU_MARKER("Setup matrices", COLOR_LIGHT_GREEN);

		//msleep(20);	// TODO: DEBUG

//		PROFILER2_POP_CPU_MARKER();

		PROFILER2_PUSH_CPU_MARKER("Update scene", COLOR_GREEN);
		scene.update(elapsed, t);
		PROFILER2_POP_CPU_MARKER();

		PROFILER2_PUSH_CPU_MARKER("Draw scene", COLOR_RED);
		scene.draw();
		PROFILER2_POP_CPU_MARKER();

		glDisable(GL_DEPTH_TEST);

		PROFILER2_PUSH_GPU_MARKER("Draw profiler", COLOR_DARK_BLUE);
		profiler2.draw();
		PROFILER2_POP_GPU_MARKER();


		// BEGIN BOUM
		/*
		double full_frame = 0.0;
		double update = 0.0;
		double wait_updates = 0.0;
		Profiler2::CpuThreadInfo& info = profiler2.m_cpu_thread_infos.get(0);
		for(int i=0 ; i < info.nb_pushed_markers ; i++)
		{
			if(info.markers[i].color.r == COLOR_GRAY.r &&
				info.markers[i].color.g == COLOR_GRAY.g &&
				info.markers[i].color.b == COLOR_GRAY.b)
			{
				full_frame = (double)((info.markers[i].end - info.markers[i].start) / (uint64_t)(1000));
				full_frame /= 1000.0;
			}
			
			if(info.markers[i].color.r == COLOR_GREEN.r &&
				info.markers[i].color.g == COLOR_GREEN.g &&
				info.markers[i].color.b == COLOR_GREEN.b)
			{
				update = (double)((info.markers[i].end - info.markers[i].start) / (uint64_t)(1000));
				update /= 1000.0;
			}
			
			if(info.markers[i].color.r == COLOR_YELLOW.r &&
				info.markers[i].color.g == COLOR_YELLOW.g &&
				info.markers[i].color.b == COLOR_YELLOW.b)
			{
				wait_updates = (double)((info.markers[i].end - info.markers[i].start) / (uint64_t)(1000));
				wait_updates /= 1000.0;
			}
		}
		*/
		char str[256];
		sprintf(str, "[%2.1lfms] Full frame", full_frame);
		drawer2D.drawString(str, 0.01f, 0.25f, COLOR_GRAY);
		
		sprintf(str, "[%2.1lfms]  Update scene", update);
		drawer2D.drawString(str, 0.01f, 0.2f, COLOR_GREEN);
		
		sprintf(str, "[%2.1lfms]   Wait for thread updates", wait_updates);
		drawer2D.drawString(str, 0.01f, 0.15f, COLOR_YELLOW);
		
		//profiler2.m_m
		/*
		static double last_time = 0.0f;
		double now = glfwGetTime();
		double delta = (now-last_time) * 1000.0;
		last_time = now;
		char str[256];
		
		static double last_delta = 0.0f;
		sprintf(str, "[%2.1lfms] Full frame", last_delta);
		drawer2D.drawString(str, 0.01f, 0.25f, COLOR_GRAY);
		
		sprintf(str, "[%2.1lfms]  Update scene", last_delta * 0.65);
		drawer2D.drawString(str, 0.01f, 0.2f, COLOR_GREEN);
		
		sprintf(str, "[%2.1lfms]   Wait for thread updates", last_delta * 0.65);
		drawer2D.drawString(str, 0.01f, 0.15f, COLOR_YELLOW);
		
		last_delta = delta;
		*/
		
		// END BOUM
		//drawer2D.drawString("Hello World!", 0.05f, 1.0f-0.05f, COLOR_LIGHT_BLUE);	// TODO: DEBUG
		//drawer2D.drawString("Hello World!", 0.01f, 0.12f, COLOR_LIGHT_BLUE);	// TODO: ASOBO BUG!!
		
		//drawer2D.drawString("[52.3ms] Full frame", 0.01f, 0.25f, COLOR_GRAY);
		//drawer2D.drawString("[52.3ms]  Update scene", 0.01f, 0.2f, COLOR_GREEN);
		//drawer2D.drawString("[52.3ms]   Wait for thread updates", 0.01f, 0.15f, COLOR_YELLOW);

		checkGLError();

		frame_count++;

		// Swap buffers
		glfwSwapBuffers();

		PROFILER2_POP_CPU_MARKER();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
		   glfwGetWindowParam( GLFW_OPENED ) );

	done = true;

//	threadJoin(thread_id);

	scene.shut();

	profiler2.shut();

	drawer2D.shut();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	shutTimer();

	return EXIT_SUCCESS;
}

void GLFWCALL onMouseClick(int button, int action)
{
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		profiler2.onLeftClick();
	}
}

void GLFWCALL onKey(int key, int action)
{
	if(action == GLFW_RELEASE && key == 'M')
	{
		if(scene.isMultithreaded())
		{
			printf("Activate multithread\n");
			scene.setMultithreaded(false);
		}
		else
		{
			printf("Deactivate multithread\n");
			scene.setMultithreaded(true);
		}
	}
}
