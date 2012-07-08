// profiler.h

#ifndef PROFILER_H
#define PROFILER_H

#include <GL/glew.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "hole_array.h"
#include "thread.h"
#include "utils.h"

#define ENABLE_PROFILER	// comment this to disable the profiler

#define INVALID_TIME	((uint64_t)(-1))
#define INVALID_QUERY	((GLuint)0)

#ifndef ENABLE_PROFILER
	#define PROFILER_INIT(win_w, win_h, mouse_x, mouse_y)
	#define PROFILER_SHUT()

	#define PROFILER_ON_MOUSE_POS(mouse_x, mouse_y)
	#define PROFILER_ON_RESIZE(win_w, win_h)
	#define PROFILER_ON_LEFT_CLICK()

	#define PROFILER_PUSH_CPU_MARKER(name, color)
	#define PROFILER_POP_CPU_MARKER()
	#define PROFILER_PUSH_GPU_MARKER(name, color)
	#define PROFILER_POP_GPU_MARKER()

	#define PROFILER_DRAW()
	#define PROFILER_SYNC_FRAME()

#else
	class Profiler;
	extern Profiler profiler;

	#define PROFILER_INIT(win_w, win_h, mouse_x, mouse_y)	profiler.init(win_w, win_h, mouse_x, mouse_y)
	#define PROFILER_SHUT()									profiler.shut()

	#define PROFILER_ON_MOUSE_POS(mouse_x, mouse_y)			profiler.onMousePos(mouse_x, mouse_y)
	#define PROFILER_ON_RESIZE(win_w, win_h)				profiler.onResize(win_w, win_h)
	#define PROFILER_ON_LEFT_CLICK()						profiler.onLeftClick()

	#define PROFILER_PUSH_CPU_MARKER(name, color)			profiler.pushCpuMarker(name, color)
	#define PROFILER_POP_CPU_MARKER()						profiler.popCpuMarker()
	#define PROFILER_PUSH_GPU_MARKER(name, color)			profiler.pushGpuMarker(name, color)
	#define PROFILER_POP_GPU_MARKER()						profiler.popGpuMarker()

	#define PROFILER_DRAW()									profiler.draw()
	#define PROFILER_SYNC_FRAME()							profiler.synchronizeFrame()

class Profiler
{
private:
	static const size_t	NB_RECORDED_FRAMES = 3;

	static const size_t	NB_MAX_CPU_MARKERS_PER_FRAME = 100;
	static const size_t	NB_MARKERS_PER_CPU_THREAD = NB_RECORDED_FRAMES * NB_MAX_CPU_MARKERS_PER_FRAME;

	static const size_t	NB_MAX_GPU_MARKERS_PER_FRAME = 10;
	static const size_t	NB_GPU_MARKERS = NB_RECORDED_FRAMES * NB_MAX_GPU_MARKERS_PER_FRAME;

	static const size_t	NB_MAX_CPU_THREADS = 32;
	//static const size_t	NB_FRAMES_BEFORE_KICK_CPU_THREAD = 4;	// TODO: remove threads that are not used anymore

	static const size_t	MARKER_NAME_MAX_LENGTH = 32;

	struct Marker
	{
		uint64_t	start;  // Times of start and end, in nanoseconds,
		uint64_t	end;	// relatively to the time of last synchronization

		size_t		layer;	// Number of markers pushed at the time this one is pushed
		int			frame;	// Frame at which the marker was started

		char		name[MARKER_NAME_MAX_LENGTH];
		Color		color;

		Marker() : start(INVALID_TIME), end(INVALID_TIME), frame(-1) {}	// unused by default
	};

	// --- Device-specific markers ---
	typedef Marker CpuMarker;

	struct GpuMarker : public Marker
	{
		GLuint		id_query_start;
		GLuint		id_query_end;

		GpuMarker() : Marker(), id_query_start(INVALID_QUERY), id_query_end(INVALID_QUERY) {}
	};

	// Markers for a CPU thread
	struct CpuThreadInfo
	{
		ThreadId	thread_id;
		CpuMarker	markers[NB_MARKERS_PER_CPU_THREAD];

		int			cur_read_id;	// Index of the last pushed marker in the previous frame
		int			cur_write_id;	// Index of the next cell we will write to

		int			next_read_id;	// draw() writes next_read_id, synchronizeFrame() copies cur_read_id <- next_read_id
									// This deferring is needed for handling freeze/unfreeze.

		size_t		nb_pushed_markers;

		void	init(ThreadId id)	{cur_read_id=cur_write_id=next_read_id=0; thread_id = id; nb_pushed_markers=0;}
	};

	// Markers for the GPU
	struct GpuThreadInfo
	{
		GpuMarker	markers[NB_GPU_MARKERS];

		int			cur_read_id;	// Index of the last pushed marker in the previous frame
		int			cur_write_id;	// Index of the next cell we will write to

		int			next_read_id;	// draw() writes next_read_id, synchronizeFrame() copies cur_read_id <- next_read_id.
									// This deferring is needed for handling freeze/unfreeze.

		size_t		nb_pushed_markers;

		void	init()	{cur_read_id=cur_write_id=next_read_id=0; nb_pushed_markers=0;}
	};

	typedef	HoleArray<CpuThreadInfo, NB_MAX_CPU_THREADS>	CpuThreadInfoList;

	CpuThreadInfoList	m_cpu_thread_infos;
	Mutex				m_cpu_mutex;

	GpuThreadInfo		m_gpu_thread_info;

	volatile int		m_cur_frame;		// Global frame counter

	// Frame time information
	struct FrameInfo
	{
		int			frame;
		uint64_t	time_sync_start;
		uint64_t	time_sync_end;
	};
	FrameInfo			m_frame_info[NB_RECORDED_FRAMES];

	// Handling freeze/unfreeze by clicking on the displayed profiler
	enum FreezeState
	{
		UNFROZEN,
		WAITING_FOR_FREEZE,
		FROZEN,
		WAITING_FOR_UNFREEZE,
	};

	FreezeState	 m_freeze_state;

	bool	m_enabled;

	// Handling interaction with the mouse
	int		m_mouse_x, m_mouse_y;
	int		m_win_w, m_win_h;

	Rect	m_back_rect;	// Background

public:
	Profiler() {}
	virtual ~Profiler() {}

	void	init(int win_w, int win_h, int mouse_x, int mouse_y);
	void	shut();

	void	pushCpuMarker(const char* name, const Color& color);
	void	popCpuMarker();

	void	pushGpuMarker(const char* name, const Color& color);
	void	popGpuMarker();

	void	synchronizeFrame();

	void	draw();

	void	setEnabled(bool enabled)	{m_enabled=enabled;}
	bool	isEnabled() const			{return m_enabled;}

	bool	isFrozen() const			{return m_freeze_state == FROZEN || m_freeze_state == WAITING_FOR_UNFREEZE;}

	// Input handling
	void	onMousePos(int x, int y)	{m_mouse_x=x;	m_mouse_y=y;}
	void	onLeftClick();
	void	onResize(int w, int h)		{m_win_w=w;	m_win_h=h;}

protected:
	// Get the CpuThreadInfo corresponding to the calling thread
	CpuThreadInfo&	getOrAddCpuThreadInfo();

	void	drawBackground();
	void	drawHoveredMarkersText(const int *read_indices, const FrameInfo* frame_info);
	void	updateBackgroundRect();
};

#endif	// defined(ENABLE_PROFILER)

#endif // PROFILER_H
