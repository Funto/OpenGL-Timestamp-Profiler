// profiler.cpp

#include "profiler.h"

#ifdef ENABLE_PROFILER

#include "hp_timer.h"
#include "drawer2D.h"
#include "thread.h"
#include <limits.h>
#include <stdio.h>

Profiler profiler;

// Unit: percentage of the screen dimensions
#define MARGIN_X	0.02f	// left and right margin
#define MARGIN_Y	0.02f	// bottom margin
#define LINE_HEIGHT 0.01f   // height of a line representing a thread

//#define TIME_DRAWN_MS 30.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
//#define TIME_DRAWN_MS 60.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
#define TIME_DRAWN_MS 120.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds

#define GPU_COUNT	1	// TODO: multiple GPUs are not supported

// -----
#define	PROFILER_WIDTH		(1.0f - 2.0f*MARGIN_X)
#define	X_OFFSET			MARGIN_X
#define	Y_OFFSET			(MARGIN_Y + LINE_HEIGHT)
#define	X_FACTOR			( (float)(PROFILER_WIDTH / (TIME_DRAWN_MS * 1000000.0)) )

#define Y_SCALE_OFFSET		0.002f	// By how much do we reduce the height when displaying
									// a marker that is lower in the hierarchy

#define NB_MAX_TEXT_LINES	20
#define Y_TEXT_MARGIN		0.05f	// size between 2 lines of text

#define COLOR_FROZEN		Color(0xD0, 0xD0, 0xD0)

//-----------------------------------------------------------------------------
void Profiler::init(int win_w, int win_h, int mouse_x, int mouse_y)
{
	m_cur_frame = 0;
	m_freeze_state = UNFROZEN;
	m_visible = true;

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
	// Don't do anything when frozen
	if(isFrozen())
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
	// Don't do anything when frozen
	if(isFrozen())
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
}

//-----------------------------------------------------------------------------
/// Push a new GPU marker that starts when the previously issued commands are processed
void Profiler::pushGpuMarker(const char* name, const Color& color)
{
	// Don't do anything when frozen
	if(isFrozen())
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
	// Don't do anything when frozen
	if(isFrozen())
		return;

	GpuThreadInfo& ti = m_gpu_thread_info;

	// Get the most recent marker that has not been closed yet
	int index = ti.cur_write_id-1;
	while(ti.markers[index].end != INVALID_TIME)	// skip closed markers
		decrementCycle(&index, NB_GPU_MARKERS);

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
	// Freeze/unfreeze as needed
	if(m_freeze_state == WAITING_FOR_FREEZE)
		m_freeze_state = FROZEN;
	else if(m_freeze_state == WAITING_FOR_UNFREEZE)
		m_freeze_state = UNFROZEN;

	// Don't do anything when frozen
	if(isFrozen())
		return;

	// Next frame
	m_cur_frame++;

	// Copy: cur_read_id <- next_read_id
	// -> GPU:
	m_gpu_thread_info.cur_read_id = m_gpu_thread_info.next_read_id;
	// -> CPUs:
	for(size_t i=m_cpu_thread_infos.begin() ;
		i != m_cpu_thread_infos.getMaxSize() ;
		i = m_cpu_thread_infos.next(i))
	{
		CpuThreadInfo	&ti = m_cpu_thread_infos.get(i);
		ti.cur_read_id = ti.next_read_id;
	}

	// Frame time information
	uint64_t	now = getTimeNs();

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
}

//-----------------------------------------------------------------------------
/// Draw the markers
void Profiler::draw()
{
	if(m_visible)
		drawBackground();

	int displayed_frame = m_cur_frame - int(NB_RECORDED_FRAMES-1);
	if(displayed_frame < 0)	// don't draw anything during the first frames
		return;

	// Used when drawing information on the markers hovered by the mouse
	int		read_indices[GPU_COUNT + NB_MAX_CPU_THREADS];	// indices of the first drawn markers, per thread
	int		thread_index = 0;								// read_indices[thread_index]

	// --- Find the FrameInfo (start and end times) for the frame we want to display ---
	FrameInfo* frame_info = NULL;
	for(int index_frame_info = 0 ; index_frame_info < int(NB_RECORDED_FRAMES) ; index_frame_info++)
	{
		if(m_frame_info[index_frame_info].frame == displayed_frame)
		{
			frame_info = &m_frame_info[index_frame_info];
			break;
		}
	}
	if(!frame_info || frame_info->time_sync_end == INVALID_TIME)
		return;

	const uint64_t	frame_delta_time = frame_info->time_sync_end - frame_info->time_sync_start;

	// --- Draw the end of the frame ---
	{
		Rect	rect_end;
		rect_end.x = X_OFFSET + X_FACTOR*frame_delta_time;
		rect_end.y = m_back_rect.y;
		rect_end.w = 0.003f;
		rect_end.h = m_back_rect.h;

		if(m_visible)
			drawer2D.drawRect(rect_end, COLOR_BLACK);
	}

	// ---- Draw the GPU markers ----
	{
		GpuThreadInfo&	ti = m_gpu_thread_info;
		int read_id = ti.cur_read_id;

		read_indices[thread_index++] = read_id;

		// Get the times and draw the markers
		uint64_t	first_start = INVALID_TIME;

		// Select only the markers that belong to this frame.
		// As GPU times are not synchronized with CPU times, we can't cleanly handle markers that started
		// in the previous frame and finish in this one, so we just display the GPU markers that belong
		// to the displayed frame.
		while(ti.markers[read_id].frame == displayed_frame)
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
				rect.y += Y_SCALE_OFFSET		*marker.layer;
				rect.h -= (2.0f*Y_SCALE_OFFSET)	*marker.layer;

				if(m_visible)
					drawer2D.drawRect(rect, marker.color);
			}

			incrementCycle(&read_id, NB_GPU_MARKERS);
		}

		ti.next_read_id = read_id;
	}

	// ---- Draw the CPU markers ----
	// For each thread:
	for(size_t i=m_cpu_thread_infos.begin() ;
		i != m_cpu_thread_infos.getMaxSize() ;
		i = m_cpu_thread_infos.next(i))
	{
		CpuThreadInfo	&ti = m_cpu_thread_infos.get(i);

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

		// In the worst case, we try to draw a marker that is out of this frame:
		// it just gets clamped and nothing is visible

		read_indices[thread_index++] = read_id;

		// Draw the markers
		while(ti.markers[read_id].frame >= displayed_frame-1 &&	// - for markers that started in the previous frame and finished
																// in this frame
			  ti.markers[read_id].frame <= displayed_frame)		// - for "regular" markers, that started in this frame
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
			rect.y += Y_SCALE_OFFSET*marker.layer;
			rect.h -= (2.0f*Y_SCALE_OFFSET)*marker.layer;

			if(m_visible)
				drawer2D.drawRect(rect, marker.color);
			incrementCycle(&read_id, NB_MARKERS_PER_CPU_THREAD);
		}

		ti.next_read_id = read_id;
	}

	if(m_visible)
		drawHoveredMarkersText(read_indices, frame_info);
}

//-----------------------------------------------------------------------------
/// Handle freeze/unfreeze
void Profiler::onLeftClick()
{
	if(!m_visible)
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
			assert(false && "should not happen - synchronizeFrame() should be called between 2 calls to onLeftClick()");
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
	drawer2D.drawRect(m_back_rect, isFrozen() ? COLOR_FROZEN : COLOR_WHITE);
}

/// Draw text information for the markers that are hovered by the mouse pointer
void Profiler::drawHoveredMarkersText(const int *read_indices, const FrameInfo* frame_info)
{
	// Compute some values for drawing
	float fx = float(m_mouse_x) / float(m_win_w);
	float fy = float(m_win_h-1 - m_mouse_y) / float(m_win_h);

	size_t			nb_max_markers = 0;
	const Marker	*markers = NULL;
	int				read_id = -1;
	uint64_t		start_time = 0;
	size_t			sizeof_marker = 0;

#define GET_MARKER(__i)	(const Marker*)(((const uint8_t*)markers) + (__i)*sizeof_marker)

	Rect	rect;
	rect.x = X_OFFSET;
	rect.y = Y_OFFSET;
	rect.w = PROFILER_WIDTH;
	rect.h = LINE_HEIGHT;

	// --- Which list of markers is hovered by the mouse pointer? ---
	if(rect.isPointInside(fx, fy))
	{
		// Hovering the GPU line
		nb_max_markers	= NB_GPU_MARKERS;
		markers			= &m_gpu_thread_info.markers[0];
		read_id			= read_indices[0];
		start_time		= m_gpu_thread_info.markers[read_id].start;
		sizeof_marker	= sizeof(GpuMarker);
	}
	else
	{
		// CPUs
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
		return;	// mouse pointer doesn't hover any line

	// --- Choose the markers that are to be displayed ---
	const Marker	*chosen_markers[NB_MAX_TEXT_LINES];
	int				nb_chosen_markers = 0;
	{
		int				counter = 0;
		const Marker*	m = GET_MARKER(read_id);
		while(	m->frame >= frame_info->frame-1 &&
				m->frame <= frame_info->frame &&
				nb_chosen_markers < NB_MAX_TEXT_LINES)
		{
			// Get the relative start and end of the marker
			uint64_t	start	= m->start - start_time;
			uint64_t	end		= m->end - start_time;

			rect.x = X_OFFSET + X_FACTOR * (float)(start);
			rect.w = X_FACTOR * (float)(end - start);
			if(rect.isPointInside(fx, fy))
			{
				chosen_markers[nb_chosen_markers++] = m;
				counter++;
			}

			// Next
			incrementCycle(&read_id, nb_max_markers);
			m = GET_MARKER(read_id);
		}
	}

	// --- Draw information on the chosen markers ---
	{
		char	str[256];
		float	y_text = m_back_rect.y + m_back_rect.h + Y_TEXT_MARGIN;
		for(int i=nb_chosen_markers-1 ; i >= 0 ; i--)
		{
			const Marker*	m = chosen_markers[i];

			uint64_t	marker_time_us = (m->end - m->start) / (uint64_t)(1000);
			double	marker_time_ms = double(marker_time_us) / 1000.0;

			sprintf(str, "[%2.1lfms] ", marker_time_ms);
			size_t len=strlen(str);
			for(size_t layer=0 ; layer < m->layer ; layer++)
				str[len++] = '+';
			str[len] = '\0';
			strcat(str, m->name);

			drawer2D.drawString(str, 0.01f, y_text, m->color);
			y_text += Y_TEXT_MARGIN;
		}
	}
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
