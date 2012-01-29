// profiler.cpp

#include <GL/glew.h>
#include "profiler.h"
#include "hp_timer.h"
#include "drawer2D.h"
#include "thread.h"

#include <stdio.h>	// DEBUG

#include <assert.h>
#include <stack>
#include <sstream>

Profiler profiler;

// Unit is in pencentage of the screen dimensions
#define MARGIN_X	0.02f	// left and right margin
#define MARGIN_Y	0.02f	// top margin
#define LINE_HEIGHT 0.01f   // height of a line representing a thread

#define MARKERS_NAMES_POS	  50,50,150,150

//#define TIME_DRAWN_MS 30.0f // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds
#define TIME_DRAWN_MS 120.0f // the width of the profiler corresponds to TIME_DRAWN_MS milliseconds

//-----------------------------------------------------------------------------
Profiler::Profiler()
{
}

//-----------------------------------------------------------------------------
Profiler::~Profiler()
{
	// Delete all mutexes
	CpuThreadInfoMap::iterator it_end = m_cpu_thread_infos.end();
	for(CpuThreadInfoMap::iterator it = m_cpu_thread_infos.begin() ; it != it_end ; it++)
	{
		ThreadInfo& cpu_ti = it->second;
		mutexDestroy(&cpu_ti.mutex);
	}
}

//-----------------------------------------------------------------------------
void Profiler::init(int win_w, int win_h, int mouse_x, int mouse_y)
{
	m_write_id = 0;
	m_time_last_sync = getTimeMs();
	m_time_between_sync = 0.0;
	m_freeze_state = UNFROZEN;

	m_back_rect.set(MARGIN_X, MARGIN_Y, (1.0f-2.0f*MARGIN_X), (2.0f*LINE_HEIGHT));

	m_win_w = win_w;
	m_win_h = win_h;
	m_mouse_x = mouse_x;
	m_mouse_y = mouse_y;
}

//-----------------------------------------------------------------------------
void Profiler::shut()
{
}

//-----------------------------------------------------------------------------
/// Push a new marker that starts now
void Profiler::pushCpuMarker(const char* name, const Color& color)
{
	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	ThreadInfo& ti = getCpuThreadInfo();
	mutexLock(&ti.mutex);

	MarkerStack& markers_stack = ti.markers_stack[m_write_id];
	double  start = getTimeMs() - m_time_last_sync;
	size_t  layer = markers_stack.size();

	// Add to the stack of current markers
	Marker	marker(start, -1.0, name, color, layer);
	markers_stack.push(marker);

	mutexUnlock(&ti.mutex);

//	printf("DEBUG: push CPU: m_write_id=%d, start=%lf, layer=%d, markers_stack.size() == %d\n",
//		m_write_id, start, (int)layer, (int)markers_stack.size());
}

//-----------------------------------------------------------------------------
/// Stop the last pushed marker
void Profiler::popCpuMarker()
{
	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	ThreadInfo&	ti = getCpuThreadInfo();
	mutexLock(&ti.mutex);

	assert(ti.markers_stack[m_write_id].size() > 0);

	MarkerStack& markers_stack = ti.markers_stack[m_write_id];
	MarkerList&  markers_done  = ti.markers_done[m_write_id];

	// Update the date of end of the marker
	Marker&	 marker = markers_stack.top();
	marker.end = getTimeMs() - m_time_last_sync;

	// Remove the marker from the stack and add it to the list of markers done
	markers_done.push_front(marker);
	markers_stack.pop();

	mutexUnlock(&ti.mutex);

//	printf("DEBUG: pop CPU: m_write_id=%d, start=%lf, end=%lf, layer=%d, markers_stack.size() == %d, markers_done.size() == %d\n",
//		m_write_id, marker.start, marker.end, (int)marker.layer, (int)markers_stack.size(), (int)markers_done.size());
}

//-----------------------------------------------------------------------------
/// Push a new GPU marker that starts when the previously issued commands are processed
void Profiler::pushGpuMarker(const char* name, const Color& color)
{
	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

/*	ThreadInfo& ti = getCpuThreadInfo();
	MarkerStack& markers_stack = ti.markers_stack[m_write_id];
	size_t  layer = markers_stack.size();

	GLuint	id_query_start;
	glGenQueries(1, &id_query_start);
	glQueryCounter(GL_TIMESTAMP, id_query_start);

	// Add to the stack of current markers
	Marker marker(-1.0, -1.0, name, color, layer, id_query_start);

	markers_stack.push(marker);
*/
//	printf("DEBUG: push GPU: m_write_id=%d, start=%lf, layer=%d, markers_stack.size() == %d\n",
//		m_write_id, start, (int)layer, (int)markers_stack.size());
}

//-----------------------------------------------------------------------------
/// Stop the last pushed GPU marker when the previously issued commands are processed
void Profiler::popGpuMarker()
{
	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

/*	ThreadInfo&	ti = getThreadInfo();
	assert(ti.markers_stack[m_write_id].size() > 0);

	MarkerStack& markers_stack = ti.markers_stack[m_write_id];
	MarkerList&  markers_done  = ti.markers_done[m_write_id];

	// Update the date of end of the marker
	Marker&	 marker = markers_stack.top();
	glGenQueries(1, &marker.id_query_end);
	glQueryCounter(GL_TIMESTAMP, marker.id_query_end);

	// Remove the marker from the stack and add it to the list of markers done
	markers_done.push_front(marker);
	markers_stack.pop();
*/
//	printf("DEBUG: pop GPU: m_write_id=%d, start=%lf, end=%lf, layer=%d, markers_stack.size() == %d, markers_done.size() == %d\n",
//		m_write_id, marker.start, marker.end, (int)marker.layer, (int)markers_stack.size(), (int)markers_done.size());
}

//-----------------------------------------------------------------------------
/// Swap buffering for the markers
void Profiler::synchronizeFrame()
{
//	printf("DEBUG: sync start: m_write_id=%d, markers_stack.size() == %d, markers_done.size() == %d\n",
//		m_write_id, (int)markers_stack.size(), (int)markers_done.size());
//	printf("DEBUG: sync start: m_write_id=%d\n", m_write_id);

	// Don't do anything when frozen
	if(m_freeze_state == FROZEN)
		return;

	// Avoid using several times getTimeMs(), which would yield different results
	double now = getTimeMs();

	// BIG LOCK
	CpuThreadInfoMap::iterator it_end = m_cpu_thread_infos.end();
	for(CpuThreadInfoMap::iterator it = m_cpu_thread_infos.begin() ; it != it_end ; it++)
	{
		ThreadInfo& ti = it->second;
		mutexLock(&ti.mutex);
	}

	// Swap buffers
	int old_write_id = m_write_id;
	m_write_id = !m_write_id;

	// For each thread:
	for(CpuThreadInfoMap::iterator it = m_cpu_thread_infos.begin() ; it != it_end ; it++)
	{
		// Get the thread information
		ThreadInfo& ti = it->second;

		MarkerList&  old_markers_done  = ti.markers_done[old_write_id];
		MarkerStack& old_markers_stack = ti.markers_stack[old_write_id];

		MarkerList&  new_markers_done  = ti.markers_done[m_write_id];
		MarkerStack& new_markers_stack = ti.markers_stack[m_write_id];

		// Clear the containers for the new frame
		new_markers_done.clear();
		while(!new_markers_stack.empty())
			new_markers_stack.pop();

		// Finish the markers in the stack of the previous frame
		// and start them for the next frame.

		// For each marker in the old stack:
		while(!old_markers_stack.empty())
		{
			// - finish the marker for the previous frame and add it to the old "done" list
			Marker& m = old_markers_stack.top();
			m.end = now - m_time_last_sync;
			old_markers_done.push_front(m);

			// - start a new one for the new frame
			Marker new_marker(0.0, -1.0, m.name.c_str(), m.color);
			new_markers_stack.push(new_marker);

			// - next iteration
			old_markers_stack.pop();
		}

		mutexUnlock(&ti.mutex);
	}

	// Remember the date of last synchronization
	m_time_between_sync = now - m_time_last_sync;
	m_time_last_sync = now;

	// Freeze/unfreeze as needed
	if(m_freeze_state == WAITING_FOR_FREEZE)
		m_freeze_state = FROZEN;
	else if(m_freeze_state == WAITING_FOR_UNFREEZE)
		m_freeze_state = UNFROZEN;

	// BIG UNLOCK
	for(CpuThreadInfoMap::iterator it = m_cpu_thread_infos.begin() ; it != it_end ; it++)
	{
		ThreadInfo& ti = it->second;
		mutexUnlock(&ti.mutex);
	}

//	printf("DEBUG: sync end: m_write_id=%d, markers_stack.size() == %d, markers_done.size() == %d\n",
//		m_write_id, (int)markers_stack.size(), (int)markers_done.size());
//	printf("=== DEBUG: sync end: m_write_id=%d\n, m_time_between_sync=%lf, m_time_last_sync=%lf",
//		m_write_id, m_time_between_sync, m_time_last_sync);
}

//-----------------------------------------------------------------------------
/// Draw the markers
void Profiler::draw()
{
	std::stack<Marker>	  hovered_markers;

	drawBackground();

	int i=0;
	int read_id = !m_write_id;

	// Compute some values for drawing
	const float profiler_width = (1.0f - 2.0f*MARGIN_X);
	const float x_offset	= MARGIN_X;
	const float y_offset	= (MARGIN_Y + LINE_HEIGHT);
	const float line_height = LINE_HEIGHT;

	const double factor = profiler_width / TIME_DRAWN_MS;

	// For each thread:
	CpuThreadInfoMap::iterator it_ti_end = m_cpu_thread_infos.end();
	for(CpuThreadInfoMap::iterator it_ti = m_cpu_thread_infos.begin() ;
		it_ti != it_ti_end ;
		it_ti++, i++)
	{
		ThreadInfo& ti = it_ti->second;

		// Draw all markers
		//MarkerList& markers = m_thread_infos[i].markers_done[read_id];
		MarkerList& markers = ti.markers_done[read_id];

		//printf("DEBUG: markers.size() == %u\n", (unsigned)markers.size());

		if(markers.empty())
			continue;

		MarkerList::const_iterator it_end = markers.end();
		for(MarkerList::const_iterator it = markers.begin() ; it != it_end ; it++)
		{
			const Marker&	m = *it;
			assert(m.end >= 0.0);

//			printf("DEBUG DRAW: [%f to %f]\n", x1, x2);

			// Draw
			Rect	rect(x_offset + factor*m.start,
						 y_offset + i*line_height,
						 factor*(m.end - m.start),
						 line_height);

			// Reduce vertically the size of the markers according to their layer
			rect.h -= 0.002f*m.layer;
			rect.y += 0.001f*m.layer;

//			printf("DEBUG: drawRect({%f, %f, %f, %f}, {%d, %d, %d}\n", rect.x, rect.y, rect.w, rect.h,
//				   int(m.color.r), int(m.color.g), int(m.color.b));
			drawer2D.drawRect(rect, m.color);

			// If the mouse cursor is over the marker, get its information
			// TODO
			//if(pos.isPointInside(mouse_pos))
			//	hovered_markers.push(m);
		}
	}

	// Draw the end of the frame
	{
		Rect	rect_end = Rect(x_offset + factor*m_time_between_sync,
								m_back_rect.y,
								0.005f,
								m_back_rect.h);
		drawer2D.drawRect(rect_end, COLOR_BLACK);
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
void Profiler::onLeftClick()
{
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
Profiler::ThreadInfo& Profiler::getCpuThreadInfo()
{
	// Try to find the ThreadInfo structure corresponding to the calling thread
	ThreadId	thread_id = threadGetId();
	CpuThreadInfoMap::iterator	it = m_cpu_thread_infos.find(thread_id);
	if(it == m_cpu_thread_infos.end())
	{
		// Add it if we haven't found it
		ThreadInfo cpu_ti;
		cpu_ti.type = THREAD_CPU;
		mutexCreate(&cpu_ti.mutex);
		m_cpu_thread_infos.insert(std::pair<ThreadId, ThreadInfo>(thread_id, cpu_ti));	// TODO: THREAD-SAFE!
		it = m_cpu_thread_infos.find(thread_id);

		// Update the background rectangle
		m_back_rect.h += LINE_HEIGHT;
	}
	return it->second;
}

//-----------------------------------------------------------------------------
/// Helper to draw a white background
void Profiler::drawBackground()
{
	Color	back_color = COLOR_WHITE;
	if(m_freeze_state == FROZEN)
		back_color.set(COLOR_LIGHT_GRAY);
	drawer2D.drawRect(m_back_rect, back_color);
}
