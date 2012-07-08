// profiler.cpp

#include "profiler.h"

#ifdef ENABLE_PROFILER

#ifdef PROFILER_CHEATING
extern double full_frame;
extern double update;
extern double wait_updates;
#endif

#include "hp_timer.h"
#include "drawer2D.h"
#include "thread.h"
#include <limits.h>
// BEGIN BOUM
#include <stdio.h>
// END BOUM

Profiler profiler;

// Unit: percentage of the screen dimensions
#define MARGIN_X	0.02f	// left and right margin
#define MARGIN_Y	0.02f	// bottom margin
#define LINE_HEIGHT 0.01f   // height of a line representing a thread

//#define TIME_DRAWN_MS 60.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
#define TIME_DRAWN_MS 120.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
//#define TIME_DRAWN_MS 15.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds

#define GPU_COUNT	1	// TODO: multiple GPUs are not supported

// -----
#define	PROFILER_WIDTH		(1.0f - 2.0f*MARGIN_X)
#define	X_OFFSET			MARGIN_X
#define	Y_OFFSET			(MARGIN_Y + LINE_HEIGHT)
#define	X_FACTOR			( (float)(PROFILER_WIDTH / (TIME_DRAWN_MS * 1000000.0)) )

//#define TEXT_POS	0.02f, (1.0f-0.02f)	// Where we write text on-screen	// TODO
//#define Y_TEXT_POS			( LINE_HEIGHT * (GPU_COUNT + m_cpu_thread_infos.getMaxSize()) )	// TODO

//-----------------------------------------------------------------------------
void Profiler::init(int win_w, int win_h, int mouse_x, int mouse_y)
{
	m_cur_frame = 0;
	m_freeze_state = UNFROZEN;
	m_enabled = true;

	updateBackgroundRect();

	m_win_w = win_w;
	m_win_h = win_h;
	m_mouse_x = mouse_x;
	m_mouse_y = mouse_y;

	mutexCreate(&m_cpu_mutex);

	for(size_t i=0 ; i < NB_RECORDED_FRAMES ; i++)
	{
		m_frame_info[i].frame = -1;
		m_frame_info[i].time_sync_start = INVALID_TIME;
		m_frame_info[i].time_sync_end = INVALID_TIME;
	}
}

//-----------------------------------------------------------------------------
void Profiler::shut()
{
	// Release GPU timer queries
	for(size_t i=0 ; i < NB_GPU_MARKERS ; i++)
	{
		GpuMarker&	marker = m_gpu_thread_info.markers[i];
		if(marker.id_query_start != INVALID_QUERY)
		{
			glDeleteQueries(1, &marker.id_query_start);
			marker.id_query_start = INVALID_QUERY;
		}
		if(marker.id_query_end != INVALID_QUERY)
		{
			glDeleteQueries(1, &marker.id_query_end);
			marker.id_query_end = INVALID_QUERY;
		}
	}

	mutexDestroy(&m_cpu_mutex);
}

//-----------------------------------------------------------------------------
/// Push a new marker that starts now
void Profiler::pushCpuMarker(const char* name, const Color& color)
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	CpuThreadInfo& ti = getOrAddCpuThreadInfo();

	Marker& marker = ti.markers[ti.cur_write_id];
	assert(marker.frame != m_cur_frame && "looping: too many markers, no free slots available");

	marker.start = getTimeNs();
	marker.end = INVALID_TIME;
	marker.layer = ti.nb_pushed_markers;
	strncpy(marker.name, name, MARKER_NAME_MAX_LENGTH);
	marker.color = color;
	marker.frame = m_cur_frame;

	incrementCycle(&ti.cur_write_id, NB_MARKERS_PER_CPU_THREAD);
	ti.nb_pushed_markers++;
}

//-----------------------------------------------------------------------------
/// Stop the last pushed marker
void Profiler::popCpuMarker()
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	CpuThreadInfo& ti = getOrAddCpuThreadInfo();
	assert(ti.nb_pushed_markers != 0);

	// Get the most recent marker that has not been closed yet
	int index = ti.cur_write_id-1;
	while(ti.markers[index].end != INVALID_TIME)	// skip closed markers
		decrementCycle(&index, NB_MARKERS_PER_CPU_THREAD);

	Marker& marker = ti.markers[index];
	assert(marker.end == INVALID_TIME);
	marker.end = getTimeNs();

	ti.nb_pushed_markers--;

#ifdef PROFILER_CHEATING
	if(&ti == &m_cpu_thread_infos[0])
	{
		if(ti.markers[index].color.r == COLOR_GRAY.r &&
			ti.markers[index].color.g == COLOR_GRAY.g &&
			ti.markers[index].color.b == COLOR_GRAY.b)
		{
			full_frame = (double)((ti.markers[index].end - ti.markers[index].start) / (uint64_t)(1000));
			full_frame /= 1000.0;
		}

		if(ti.markers[index].color.r == COLOR_YELLOW.r &&
			ti.markers[index].color.g == COLOR_YELLOW.g &&
			ti.markers[index].color.b == COLOR_YELLOW.b)
		{
			wait_updates = (double)((ti.markers[index].end - ti.markers[index].start) / (uint64_t)(1000));
			wait_updates  /= 1000.0;
		}

		if(ti.markers[index].color.r == COLOR_GREEN.r &&
			ti.markers[index].color.g == COLOR_GREEN.g &&
			ti.markers[index].color.b == COLOR_GREEN.b)
		{
			update= (double)((ti.markers[index].end - ti.markers[index].start) / (uint64_t)(1000));
			update  /= 1000.0;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
/// Push a new GPU marker that starts when the previously issued commands are processed
void Profiler::pushGpuMarker(const char* name, const Color& color)
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	GpuThreadInfo&	ti = m_gpu_thread_info;
	GpuMarker& marker = ti.markers[ti.cur_write_id];

	assert(marker.frame != m_cur_frame && "looping: too many markers, no free slots available");

	// Issue timer query
	if(marker.id_query_start == INVALID_QUERY)
		glGenQueries(1, &marker.id_query_start);
	glQueryCounter(marker.id_query_start, GL_TIMESTAMP);

	// Fill in marker
	marker.start = INVALID_TIME;
	marker.end = INVALID_TIME;
	marker.layer = ti.nb_pushed_markers;
	strncpy(marker.name, name, MARKER_NAME_MAX_LENGTH);
	marker.color = color;
	marker.frame = m_cur_frame;

	incrementCycle(&ti.cur_write_id, NB_GPU_MARKERS);
	ti.nb_pushed_markers++;
}

//-----------------------------------------------------------------------------
/// Stop the last pushed GPU marker when the previously issued commands are processed
void Profiler::popGpuMarker()
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	GpuThreadInfo& ti = m_gpu_thread_info;

	// BEGIN BOUM
	// TODO: the bug appears when we looped (not present when we increase the number of GPU markers)
	// -> should be present with CPU markers too
	// Get the most recent marker that has not been closed yet
	int index = ti.cur_write_id-1;
	while(ti.markers[index].end != INVALID_TIME)	// skip closed markers
		decrementCycle(&index, NB_GPU_MARKERS);

	// Get the index for the marker to pop
//	int index = ti.cur_write_id - (int)(ti.nb_pushed_markers);
//	if(index < 0)
//		index += NB_GPU_MARKERS;
	// END BOUM

	GpuMarker&	marker = ti.markers[index];

	// Issue timer query
	if(marker.id_query_end == INVALID_QUERY)
		glGenQueries(1, &marker.id_query_end);
	glQueryCounter(marker.id_query_end, GL_TIMESTAMP);

	ti.nb_pushed_markers--;
}

//-----------------------------------------------------------------------------
/// Update frame information and frame counter
void Profiler::synchronizeFrame()
{
	if(!m_enabled)
		return;

//	printf("DEBUG: sync start: m_write_id=%d, markers_stack.size() == %d, markers_done.size() == %d\n",
//		m_write_id, (int)markers_stack.size(), (int)markers_done.size());
//	printf("DEBUG: sync start: m_write_id=%d\n", m_write_id);

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	// Freeze/unfreeze as needed
	if(m_freeze_state == WAITING_FOR_FREEZE)
	{
		m_freeze_state = FROZEN;
		return; // TODO
	}
	else if(m_freeze_state == WAITING_FOR_UNFREEZE)
		m_freeze_state = UNFROZEN;

	// Next frame
	m_cur_frame++;

	uint64_t	now = getTimeNs();

	// Frame time information
	size_t	index_oldest = 0;
	for(size_t i=0 ; i < NB_RECORDED_FRAMES ; i++)
	{
		if(m_frame_info[i].frame < m_frame_info[index_oldest].frame)
			index_oldest = i;
		if(m_frame_info[i].frame == m_cur_frame-1)
			m_frame_info[i].time_sync_end = now;
	}

	FrameInfo	&new_frame = m_frame_info[index_oldest];
	new_frame.time_sync_start = now;
	new_frame.time_sync_end = INVALID_TIME;
	new_frame.frame = m_cur_frame;

	//pushGpuMarker("synchronize", COLOR_WHITE);
	//popGpuMarker();

//	printf("DEBUG: sync end: m_write_id=%d, markers_stack.size() == %d, markers_done.size() == %d\n",
//		m_write_id, (int)markers_stack.size(), (int)markers_done.size());
//	printf("=== DEBUG: sync end: m_write_id=%d\n, m_time_between_sync=%lf, m_time_last_sync=%lf",
//		m_write_id, m_time_between_sync, m_time_last_sync);
}

//-----------------------------------------------------------------------------
/// Draw the markers
void Profiler::draw()
{
	if(!m_enabled)
		return;

	drawBackground();

	int displayed_frame = m_cur_frame - int(NB_RECORDED_FRAMES-1);
	if(displayed_frame < 0)
		return;

	// Used when drawing information on the markers hovered by the mouse
	int		read_indices[GPU_COUNT + NB_MAX_CPU_THREADS];
	int		thread_index = 0;

	// --- Find the FrameInfo (start and end times) for the frame we want to display ---
	FrameInfo* frame_info = NULL;
	for(size_t index_frame_info = 0 ; index_frame_info < int(NB_RECORDED_FRAMES) ; index_frame_info++)
	{
		if(m_frame_info[index_frame_info].frame == displayed_frame)
		{
			frame_info = &m_frame_info[index_frame_info];
			break;
		}
	}
	if(!frame_info || frame_info->time_sync_end == INVALID_TIME)
		return;

	uint64_t	frame_delta_time = frame_info->time_sync_end - frame_info->time_sync_start;

	// --- Draw the end of the frame ---
	{
		Rect	rect_end;
		rect_end.x = X_OFFSET + X_FACTOR*frame_delta_time;
		rect_end.y = m_back_rect.y;
		rect_end.w = 0.003f;
		rect_end.h = m_back_rect.h;

		drawer2D.drawRect(rect_end, COLOR_BLACK);
	}

	// ---- Draw the GPU markers ----
	{
		GpuThreadInfo&	ti = m_gpu_thread_info;
		int read_id = ti.cur_read_id;

		// --- BEGIN TODO ---
		// Jump back to the last marker that ends after the start of this frame
/*		while(ti.markers[read_id].id_query_end != INVALID_QUERY &&
			  ti.markers[read_id].end != INVALID_TIME &&
			  ti.markers[read_id].end > frame_info->time_sync_start)
		{
			decrementCycle(&read_id, NB_GPU_MARKERS);
		}
*/
		// In the worst case, we try to draw a marker that is out of this frame:
		// it just gets clamped and nothing is visible
		// --- END TODO ---

		read_indices[thread_index++] = read_id;

		// Get the times and draw the markers
		uint64_t	first_start = INVALID_TIME;

		while(ti.markers[read_id].frame == displayed_frame)
		//while(ti.markers[read_id].frame <= displayed_frame)
		{
			GpuMarker&	marker = ti.markers[read_id];

			bool ok = true;
			if(marker.id_query_start == INVALID_QUERY ||
			   marker.id_query_end == INVALID_QUERY)
				ok = false;

			if(ok)
			{
				GLint	start_ok = 0;
				GLint	end_ok = 0;
				glGetQueryObjectiv(marker.id_query_start, GL_QUERY_RESULT_AVAILABLE, &start_ok);
				glGetQueryObjectiv(marker.id_query_end, GL_QUERY_RESULT_AVAILABLE, &end_ok);
				ok = (bool)(start_ok && end_ok);
			}

			if(ok)
			{
				uint64_t	start, end;

				glGetQueryObjectui64v(marker.id_query_start, GL_QUERY_RESULT, &marker.start);
				glGetQueryObjectui64v(marker.id_query_end, GL_QUERY_RESULT, &marker.end);

				start = marker.start;
				end = marker.end;

				if(first_start == INVALID_TIME)
					first_start = start;

				start -= first_start;
				end -= first_start;

				Rect	rect;
				rect.x = X_OFFSET + X_FACTOR * (float)(start);
				rect.y = Y_OFFSET;
				rect.w = X_FACTOR * (float)(end - start);
				rect.h = LINE_HEIGHT;

				// Reduce vertically the size of the markers according to their layer
				rect.y += 0.002f*marker.layer;
				rect.h -= 0.004f*marker.layer;

				drawer2D.drawRect(rect, marker.color);
			}

			incrementCycle(&read_id, NB_GPU_MARKERS);
		}

		ti.cur_read_id = read_id;
	}

	// ---- Draw the CPU markers ----
	// For each thread:
	for(size_t i=m_cpu_thread_infos.begin() ;
		i != m_cpu_thread_infos.getMaxSize() ;
		i = m_cpu_thread_infos.next(i))
	{
		CpuThreadInfo	&ti = m_cpu_thread_infos.get(i);

		// Jump back to the last marker that ends after the start of this frame
/*		int read_id = ti.cur_read_id;
		while(ti.markers[read_id].frame >= 0 &&
			  ti.markers[read_id].end > frame_info->time_sync_start)
		{
			decrementCycle(&read_id, NB_MARKERS_PER_CPU_THREAD);
		}
*/
		// Jump back to the last marker that ends after the start of this frame.
		// Avoid going to a frame older than displayed_frame-1.
		// -> handle markers that overlap the previous and the displayed frame
		int read_id = ti.cur_read_id;
		while(true)
		{
			int candidate_id = read_id;
			decrementCycle(&candidate_id, NB_MARKERS_PER_CPU_THREAD);

			if(ti.markers[candidate_id].frame >= displayed_frame-1 &&
			   ti.markers[candidate_id].end > frame_info->time_sync_start)
			{
				read_id = candidate_id;
				continue;
			}

			break;
		}

		read_indices[thread_index++] = read_id;

		// In the worst case, we try to draw a marker that is out of this frame:
		// it just gets clamped and nothing is visible

		// Draw the markers
		while(ti.markers[read_id].frame >= displayed_frame-1 &&
			  ti.markers[read_id].frame <= displayed_frame)
		{
			CpuMarker	&marker = ti.markers[read_id];

			uint64_t	start, end;
			start	= clamp(marker.start,	frame_info->time_sync_start, frame_info->time_sync_end);
			end		= clamp(marker.end,		frame_info->time_sync_start, frame_info->time_sync_end);

			start	-= frame_info->time_sync_start;
			end		-= frame_info->time_sync_start;

			Rect	rect;
			rect.x = X_OFFSET + X_FACTOR * (float)(start);
			rect.y = Y_OFFSET + (i+GPU_COUNT)*LINE_HEIGHT;
			rect.w = X_FACTOR * (float)(end - start);
			rect.h = LINE_HEIGHT;

			// Reduce vertically the size of the markers according to their layer
			rect.y += 0.002f*marker.layer;
			rect.h -= 0.004f*marker.layer;

			drawer2D.drawRect(rect, marker.color);
			incrementCycle(&read_id, NB_MARKERS_PER_CPU_THREAD);
		}

		ti.cur_read_id = read_id;
	}

	drawHoveredMarkers(read_indices, frame_info);
}

//-----------------------------------------------------------------------------
/// Handle freeze/unfreeze
void Profiler::onLeftClick()
{
	if(!m_enabled)
		return;

	float fx = float(m_mouse_x) / float(m_win_w);
	float fy = float(m_win_h-1 - m_mouse_y) / float(m_win_h);

	if(m_back_rect.isPointInside(fx, fy))
	{
		switch(m_freeze_state)
		{
		case UNFROZEN:
			m_freeze_state = WAITING_FOR_FREEZE;
			break;

		case FROZEN:
			m_freeze_state = WAITING_FOR_UNFREEZE;
			break;

		case WAITING_FOR_FREEZE:
		case WAITING_FOR_UNFREEZE:
			// the user should not be that quick, and we prefer avoiding to introduce
			// bugs by unfrozing it while it has not frozen yet.
			// Same the other way around.
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Get the CpuThreadInfo corresponding to the calling thread
Profiler::CpuThreadInfo& Profiler::getOrAddCpuThreadInfo()
{
	ThreadId	thread_id = threadGetCurrentId();

	for(size_t i = m_cpu_thread_infos.begin();
		i != m_cpu_thread_infos.getMaxSize() ;
		i = m_cpu_thread_infos.next(i))
	{
		if(m_cpu_thread_infos.isUsed(i) &&
		   m_cpu_thread_infos.get(i).thread_id == thread_id)
		{
			return m_cpu_thread_infos.get(i);
		}
	}

	mutexLock(&m_cpu_mutex);
	size_t	i = m_cpu_thread_infos.add();
	mutexUnlock(&m_cpu_mutex);

	CpuThreadInfo	&ti = m_cpu_thread_infos.get(i);
	ti.init(thread_id);

	updateBackgroundRect();

	return ti;
}

//-----------------------------------------------------------------------------
void Profiler::drawBackground()
{
	Color	back_color = COLOR_WHITE;
	if(m_freeze_state == FROZEN)
		back_color.set(COLOR_LIGHT_GRAY);
	drawer2D.drawRect(m_back_rect, back_color);
}

void Profiler::drawHoveredMarkers(const int *read_indices, const FrameInfo* frame_info)
{
	// Compute some values for drawing

	float fx = float(m_mouse_x) / float(m_win_w);
	float fy = float(m_win_h-1 - m_mouse_y) / float(m_win_h);

	size_t			nb_max_markers = 0;
	const Marker	*markers = NULL;
	int				read_id = -1;
	uint64_t		start_time = 0;
	size_t			sizeof_marker = 0;
	bool			is_gpu = false;	// TODO: remove
#define GET_MARKER(__i)	(const Marker*)(((const uint8_t*)markers) + (__i)*sizeof_marker)

	float			y_text = 0.25f;	// TODO: depend on existing lines...

	Rect	rect;
	rect.x = X_OFFSET;
	rect.y = Y_OFFSET;
	rect.w = PROFILER_WIDTH;
	rect.h = LINE_HEIGHT;

	// Get information on the markers hovered by the mouse
	if(rect.isPointInside(fx, fy))
	{
		// Hovering the GPU line
		nb_max_markers	= NB_GPU_MARKERS;
		markers			= &m_gpu_thread_info.markers[0];
		read_id			= read_indices[0];
		start_time		= m_gpu_thread_info.markers[read_id].start;
		sizeof_marker	= sizeof(GpuMarker);
		is_gpu			= true;
	}
	else
	{
		for(size_t i=m_cpu_thread_infos.begin() ;
			i != m_cpu_thread_infos.getMaxSize() ;
			i = m_cpu_thread_infos.next(i))
		{
			rect.y += LINE_HEIGHT;

			if(rect.isPointInside(fx, fy))
			{
				// Hovering a CPU line
				nb_max_markers	= NB_MARKERS_PER_CPU_THREAD;
				markers			= &m_cpu_thread_infos[i].markers[0];
				read_id			= read_indices[i+GPU_COUNT];
				start_time		= frame_info->time_sync_start;
				sizeof_marker	= sizeof(CpuMarker);
				break;
			}
		}
	}

	if(!markers)
		return;	// we don't hover any line

	// Draw information on the markers that are hovered
	int				counter = 0;
	const Marker*	m = GET_MARKER(read_id);
	while(	m->frame >= frame_info->frame-1 &&
			m->frame <= frame_info->frame)
	{
		// Get the relative start and end of the marker
		uint64_t	start, end;

		// TODO: unify
		if(!is_gpu)
		{
			start	= clamp(m->start,	frame_info->time_sync_start, frame_info->time_sync_end);
			end		= clamp(m->end,		frame_info->time_sync_start, frame_info->time_sync_end);
		}
		else
		{
			start	= m->start;
			end		= m->end;
		}

		start	-= start_time;
		end		-= start_time;

		rect.x = X_OFFSET + X_FACTOR * (float)(start);
		rect.w = X_FACTOR * (float)(end - start);
		if(rect.isPointInside(fx, fy))
		{
			double	marker_time_ms = (double)((m->end - m->start) / (uint64_t)(1000000));

			char str[256];
			sprintf(str, "[%2.1lfms] %s", marker_time_ms, m->name);
			size_t len=strlen(str);
			for(size_t i=0 ; i < m->layer ; i++)
				str[len++] = ' ';
			str[len] = '\0';

			// TODO
			drawer2D.drawString(str, 0.01f, 0.25f - 0.05f*counter, m->color);

			/*//#define Y_TEXT_POS			( LINE_HEIGHT * (GPU_COUNT + m_cpu_thread_infos.getMaxSize()) )	// TODO*/

			counter++;
		}

		// Next
		incrementCycle(&read_id, nb_max_markers);
		m = GET_MARKER(read_id);
	}

/*
#ifdef PROFILER_CHEATING
		char str[256];
		sprintf(str, "[%2.1lfms] Full frame", full_frame);
		drawer2D.drawString(str, 0.01f, 0.25f, COLOR_GRAY);

		sprintf(str, "[%2.1lfms]  Update scene", update);
		drawer2D.drawString(str, 0.01f, 0.2f, COLOR_GREEN);

		sprintf(str, "[%2.1lfms]   Wait for thread updates", wait_updates);
		drawer2D.drawString(str, 0.01f, 0.15f, COLOR_YELLOW);
#endif
*/
#undef GET_MARKER
}

void Profiler::updateBackgroundRect()
{
	size_t nb_threads = m_cpu_thread_infos.getSize();
	nb_threads += GPU_COUNT;

	m_back_rect.x = MARGIN_X;
	m_back_rect.y = MARGIN_Y;
	m_back_rect.w = 1.0f-2.0f*MARGIN_X;
	m_back_rect.h = ((float)(nb_threads) + 2.0f)*LINE_HEIGHT;
}

#endif // defined(ENABLE_PROFILER)
