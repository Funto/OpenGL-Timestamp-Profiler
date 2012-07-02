// scene.cpp

#include <stdio.h>
#include "utils.h"
#include "profiler.h"

#include "scene.h"
#include "math_utils.h"

//#define _DEBUG_PRINTF

#ifdef _DEBUG_PRINTF
	#define DbgPrintf printf
#else
	#define DbgPrintf(...)
#endif

bool Scene::init()
{
	m_multithread = false;
	m_shut = false;
	m_elapsed_time = 0.0;
	m_update_time = 0.0;

	m_colors[0] = COLOR_DARK_RED;
	m_colors[1] = COLOR_DARK_GREEN;
	m_colors[2] = COLOR_DARK_BLUE;
	m_colors[3] = COLOR_GRAY;

	bool ok = true;

	// Launch the threads
	for(int i=0 ; i < NB_THREADS ; i++)
	{
		m_thread_data[i].p_scene = this;
		ok = ok && m_thread_data[i].grid.init((float)i, m_colors[i]);
		if(!ok)
			return false;

		eventCreate(&m_thread_data[i].update_event);
		eventCreate(&m_thread_data[i].main_event);
		m_thread_data[i].index = i;
		m_thread_data[i].thread_handle = threadCreate(&threadUpdateWrapper, &m_thread_data[i]);
	}

	return ok;
}

void Scene::shut()
{
	DbgPrintf("[main] shut\n");
	m_shut = true;
	for(int i=0 ; i < NB_THREADS ; i++)
	{
		DbgPrintf("[main] trigger update %d\n", i);
		eventTrigger(&m_thread_data[i].update_event);

		DbgPrintf("[main] join %d\n", i);

		threadJoin(m_thread_data[i].thread_handle);

		DbgPrintf("[main] destroy update event %d\n", i);
		eventDestroy(&m_thread_data[i].update_event);

		DbgPrintf("[main] destroy main event %d\n", i);
		eventDestroy(&m_thread_data[i].main_event);

		m_thread_data[i].grid.shut();
	}
}

void Scene::update(double elapsed, double t)
{
	if(m_multithread)
	{
		// Trigger update on multiple threads and wait for completion
		PROFILER_PUSH_CPU_MARKER("Multithread update", COLOR_CYAN);

		DbgPrintf("--- [main] begin update ---\n");
		m_elapsed_time = elapsed;
		m_update_time = t;
		for(int i=0 ; i < NB_THREADS ; i++)
		{
			DbgPrintf("[main] trigger update event %d\n", i);
			eventTrigger(&m_thread_data[i].update_event);
		}

		PROFILER_POP_CPU_MARKER();

		PROFILER_PUSH_CPU_MARKER("Wait for update", COLOR_YELLOW);
		DbgPrintf("[main] wait and reset all main events...\n");
		for(int i=0 ; i < NB_THREADS ; i++)
		{
			DbgPrintf("[main] wait for main event %d\n", i);
			eventWait(&m_thread_data[i].main_event);

			DbgPrintf("[main] reset main event %d\n", i);
			eventReset(&m_thread_data[i].main_event);
		}

		DbgPrintf("--- [main] end update ---\n");
		PROFILER_POP_CPU_MARKER();
	}
	else
	{
		// Sequential update
		for(int i=0 ; i < NB_THREADS ; i++)
		{
			char str_marker[32];
			sprintf(str_marker, "Multithread update %d", i);
			PROFILER_PUSH_CPU_MARKER(str_marker, m_colors[i]);
			m_thread_data[i].grid.update(elapsed, t);
			PROFILER_POP_CPU_MARKER();
		}
	}
}

void Scene::draw(int win_w, int win_h)
{
	// Select and setup the projection matrix
	m_camera.setPerspective(65.0f, (float)win_w/(float)win_h, 1.0f, 100.0f);

	// Select and setup the modelview matrix
	float eye[3]	= { 0.0f, 5.0f, 4.0f };
	float view[3]	= { 0.0f, 0.0f, 0.0f };
	float up[3]		= { 0.0f, 1.0f, 0.0f };
	matrixLookAt(m_camera.view_matrix,	eye, view, up);

	float proj_view_matrix[16];
	matrixMult(proj_view_matrix, m_camera.proj_matrix, m_camera.view_matrix);

	const float start_x = -6.0f;
	const float start_y = -6.0f;
	const float step_x = 12.0f;
	const float step_y = -12.0f;

	float mvp_matrix[16];
	float trans_matrix[16];
	float trans_vec[3];

	for(int x=0 ; x < NB_GRIDS_X ; x++)
	{
		for(int y=0 ; y < NB_GRIDS_Y ; y++)
		{
			Color debug_color = COLOR_DARK_RED;
			if(x == 0 && y == 1)
				debug_color = COLOR_DARK_GREEN;
			else if(x == 1 && y == 0)
				debug_color = COLOR_DARK_BLUE;
			else if(x == 1 && y == 1)
				debug_color = COLOR_GRAY;

			trans_vec[0] = start_x + float(x)*step_x;
			trans_vec[1] = 0;
			trans_vec[2] = start_y + float(y)*step_y;
			matrixTranslate(trans_matrix, trans_vec);
			matrixMult(mvp_matrix, proj_view_matrix, trans_matrix);

			Grid&	grid = m_thread_data[x + y*NB_GRIDS_X].grid;
			char	str_marker[32];
			sprintf(str_marker, "[GPU] draw grid %d,%d", x, y);

			Color	color = grid.getColor();

			PROFILER_PUSH_GPU_MARKER(str_marker, color);
			grid.draw(mvp_matrix);
			PROFILER_POP_GPU_MARKER();
		}

	}
}

void Scene::threadUpdate(ThreadData *data)
{
	while(!m_shut)
	{
		DbgPrintf("(%d) wait update event %d\n", data->index, data->index);
		eventWait(&data->update_event);

		DbgPrintf("(%d) reset update event %d\n", data->index, data->index);
		eventReset(&data->update_event);

		PROFILER_PUSH_CPU_MARKER("Thread update", data->grid.getColor());

		DbgPrintf("(%d) update\n", data->index);
		data->grid.update(m_elapsed_time, m_update_time);

		PROFILER_POP_CPU_MARKER();

		DbgPrintf("(%d) trigger main event %d\n", data->index, data->index);
		eventTrigger(&data->main_event);
	}
	DbgPrintf("(%d): done\n", data->index);
}

void* Scene::threadUpdateWrapper(void* user_data)
{
	ThreadData* data = (ThreadData*)user_data;
	data->p_scene->threadUpdate(data);
	return NULL;
}
