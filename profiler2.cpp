// profiler2.cpp

#include "profiler2.h"

#ifdef ENABLE_PROFILER2

#include "hp_timer.h"
#include "drawer2D.h"
#include "thread.h"
#include "limits.h"
#include <stdio.h>	// BOUM TODO remove

Profiler2 profiler2;

// Unit: percentage of the screen dimensions
#define MARGIN_X	0.02f	// left and right margin
#define MARGIN_Y	0.02f	// top margin
//#define MARGIN_Y	0.1f	// top margin
#define LINE_HEIGHT 0.01f   // height of a line representing a thread
//#define LINE_HEIGHT 0.008f   // height of a line representing a thread
#define TEXT_POS	0.02f, (1.0f-0.02f)	// Where we write text on-screen

//#define TIME_DRAWN_MS 60.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
#define TIME_DRAWN_MS 120.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
//#define TIME_DRAWN_MS 15.0 // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
//#define TIME_DRAWN_NS	(120*1000*1000)

#define GPU_COUNT	1

//-----------------------------------------------------------------------------
void Profiler2::init(int win_w, int win_h, int mouse_x, int mouse_y)
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
void Profiler2::shut()
{
	// TODO: destroy all GPU timer queries

	mutexDestroy(&m_cpu_mutex);
}

//-----------------------------------------------------------------------------
/// Push a new marker that starts now
void Profiler2::pushCpuMarker(const char* name, const Color& color)
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

// BEGIN BOUM
extern double full_frame;
extern double update;
extern double wait_updates;
// END BOUM

//-----------------------------------------------------------------------------
/// Stop the last pushed marker
void Profiler2::popCpuMarker()
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

/*	CpuThreadInfo& ti = getOrAddCpuThreadInfo();
	ti.markers[ti.cur_pop_id].end = getTimeNs();
	incrementCycle(&ti.cur_pop_id, NB_MARKERS_PER_CPU_THREAD);
	ti.nb_pushed_markers--;
*/

	CpuThreadInfo& ti = getOrAddCpuThreadInfo();
	assert(ti.nb_pushed_markers != 0);

	int index = ti.cur_write_id-1;
	while(ti.markers[index].end != INVALID_TIME)
	{
		decrementCycle(&index, NB_MARKERS_PER_CPU_THREAD);
	}
	ti.markers[index].end = getTimeNs();
	ti.nb_pushed_markers--;
	
	// BEGIN BOUM
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
	// END BOUM
}

//-----------------------------------------------------------------------------
/// Push a new GPU marker that starts when the previously issued commands are processed
void Profiler2::pushGpuMarker(const char* name, const Color& color)
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	GpuThreadInfo&	ti = m_gpu_thread_info;
	GpuMarker& marker = ti.markers[ti.cur_push_id];

	assert(marker.frame != m_cur_frame && "looping: too many markers, no free slots available");

	if(marker.id_query_start == INVALID_QUERY)
		glGenQueries(1, &marker.id_query_start);
	glQueryCounter(marker.id_query_start, GL_TIMESTAMP);

	marker.layer = ti.nb_pushed_markers;
	strncpy(marker.name, name, MARKER_NAME_MAX_LENGTH);
	marker.color = color;
	marker.frame = m_cur_frame;

	incrementCycle(&ti.cur_push_id, NB_GPU_MARKERS);
	ti.nb_pushed_markers++;
}

//-----------------------------------------------------------------------------
/// Stop the last pushed GPU marker when the previously issued commands are processed
void Profiler2::popGpuMarker()
{
	if(!m_enabled)
		return;

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	GpuThreadInfo& ti = m_gpu_thread_info;
	GpuMarker&	marker = ti.markers[ti.cur_pop_id];

	if(marker.id_query_end == INVALID_QUERY)
		glGenQueries(1, &marker.id_query_end);
	glQueryCounter(marker.id_query_end, GL_TIMESTAMP);

	incrementCycle(&ti.cur_pop_id, NB_GPU_MARKERS);
	ti.nb_pushed_markers--;
}

//-----------------------------------------------------------------------------
/// Swap buffering for the markers
void Profiler2::synchronizeFrame()
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
		m_freeze_state = FROZEN;
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
void Profiler2::draw()
{
	if(!m_enabled)
		return;

	drawBackground();

	// Compute some values for drawing
	const float profiler_width = (1.0f - 2.0f*MARGIN_X);
	const float x_offset	= MARGIN_X;
	const float y_offset	= (MARGIN_Y + LINE_HEIGHT);

	const double factor = profiler_width / (TIME_DRAWN_MS * 1000000.0);

	int displayed_frame = m_cur_frame - int(NB_RECORDED_FRAMES-1);
	if(displayed_frame < 0)
		return;

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
		rect_end.x = x_offset + factor*frame_delta_time;
		rect_end.y = m_back_rect.y;
		rect_end.w = 0.003f;
		rect_end.h = m_back_rect.h;

		drawer2D.drawRect(rect_end, COLOR_BLACK);
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
			rect.x = x_offset + factor * (double)(start);
			rect.y = y_offset + (i+GPU_COUNT)*LINE_HEIGHT;
			rect.w = factor * (double)(end - start);
			rect.h = LINE_HEIGHT;

			// Reduce vertically the size of the markers according to their layer
			rect.y += 0.002f*marker.layer;
			rect.h -= 0.004f*marker.layer;

			drawer2D.drawRect(rect, marker.color);
			incrementCycle(&read_id, NB_MARKERS_PER_CPU_THREAD);
		}

		ti.cur_read_id = read_id;
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


		// Get the times and draw the markers
		uint64_t	first_start = INVALID_TIME;

		int debug_nb_markers = 0;
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
				rect.x = x_offset + factor * (double)(start);
				rect.y = y_offset;
				rect.w = factor * (double)(end - start);
				rect.h = LINE_HEIGHT;

				// Reduce vertically the size of the markers according to their layer
				rect.y += 0.002f*marker.layer;
				rect.h -= 0.004f*marker.layer;

				drawer2D.drawRect(rect, marker.color);

				debug_nb_markers++;
			}

			incrementCycle(&read_id, NB_GPU_MARKERS);
		}

		//printf("debug_nb_markers == %d\n", debug_nb_markers);

		ti.cur_read_id = read_id;
	}

	// Draw the hovered markers' names
/*	gui::ScalableFont* font = GUIEngine::getFont();
	if(font)
	{
		core::stringw text;
		while(!hovered_markers.empty())
		{
			Marker& m = hovered_markers.top();
			std::ostringstream oss;
			oss << m.name << " [" << (m.end-m.start) << " ms]" << std::endl;
			text += oss.str().c_str();
			hovered_markers.pop();
		}
		font->draw(text, MARKERS_NAMES_POS, video::SColor(0xFF, 0xFF, 0x00, 0x00));
	}
*/
}

//-----------------------------------------------------------------------------
/// Handle freeze/unfreeze
void Profiler2::onLeftClick()
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
Profiler2::CpuThreadInfo& Profiler2::getOrAddCpuThreadInfo()
{
	ThreadId	thread_id = threadGetId();

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
/// Helper to draw a white background
void Profiler2::drawBackground()
{
	Color	back_color = COLOR_WHITE;
	if(m_freeze_state == FROZEN)
		back_color.set(COLOR_LIGHT_GRAY);
	drawer2D.drawRect(m_back_rect, back_color);
}

void Profiler2::updateBackgroundRect()
{
	size_t nb_threads = m_cpu_thread_infos.getSize();
	nb_threads += GPU_COUNT;

	m_back_rect.x = MARGIN_X;
	m_back_rect.y = MARGIN_Y;
	m_back_rect.w = 1.0f-2.0f*MARGIN_X;
	m_back_rect.h = ((float)(nb_threads) + 2.0f)*LINE_HEIGHT;
}

#endif // defined(ENABLE_PROFILER2)
