// profiler2.h

#ifndef PROFILER2_H
#define PROFILER2_H

#include <GL/glew.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "hole_array.h"
#include "thread.h"
#include "utils.h"

#define INVALID_TIME	((uint64_t)(-1))
#define INVALID_QUERY	((GLuint)0)

#define ENABLE_PROFILER2	// comment this to remove the profiler

#ifndef ENABLE_PROFILER2
	#define PROFILER2_PUSH_CPU_MARKER(name, color)
	#define PROFILER2_POP_CPU_MARKER()
	#define PROFILER2_PUSH_GPU_MARKER(name, color)
	#define PROFILER2_POP_GPU_MARKER()
	#define PROFILER2_SYNC_FRAME()
	#define PROFILER2_DRAW()
#else
	class Profiler2;
	extern Profiler2 profiler2;

	#define PROFILER2_PUSH_CPU_MARKER(name, color) \
		profiler2.pushCpuMarker(name, color)

	#define PROFILER2_POP_CPU_MARKER()  \
		profiler2.popCpuMarker()

	#define PROFILER2_PUSH_GPU_MARKER(name, color) \
		profiler2.pushGpuMarker(name, color)

	#define PROFILER2_POP_GPU_MARKER()  \
		profiler2.popGpuMarker()

	#define PROFILER2_SYNC_FRAME()   \
		profiler2.synchronizeFrame()

	#define PROFILER2_DRAW() \
		profiler2.draw()

class Profiler2
{
// BEGIN BOUM
public:
//private:
// END BOUM
	static const size_t	NB_RECORDED_FRAMES = 3;

	static const size_t	NB_MAX_CPU_MARKERS_PER_FRAME = 100;
	static const size_t	NB_MARKERS_PER_CPU_THREAD = NB_RECORDED_FRAMES * NB_MAX_CPU_MARKERS_PER_FRAME;

	static const size_t	NB_MAX_GPU_MARKERS_PER_FRAME = 10;
	static const size_t	NB_GPU_MARKERS = NB_RECORDED_FRAMES * NB_MAX_GPU_MARKERS_PER_FRAME;

	static const size_t	NB_MAX_CPU_THREADS = 32;
	static const size_t	NB_FRAMES_BEFORE_KICK_CPU_THREAD = 4;

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

		int			cur_read_id;	// Index to the last pushed marker in the previous frame
		int			cur_write_id;	// Index to the next cell we will write to

		size_t		nb_pushed_markers;

		void	init(ThreadId id)	{cur_read_id=cur_write_id=0; thread_id = id; nb_pushed_markers=0;}
	};

	// Markers for the GPU
	struct GpuThreadInfo
	{
		GpuMarker	markers[NB_GPU_MARKERS];

		int			cur_read_id;
		int			cur_push_id;
		int			cur_pop_id;

		size_t		nb_pushed_markers;

		void	init()	{cur_read_id=cur_push_id=cur_pop_id=0; nb_pushed_markers=0;}
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
	Profiler2() {}
	virtual ~Profiler2() {}

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

	// Input handling
	void	onMousePos(int x, int y)	{m_mouse_x=x;	m_mouse_y=y;}
	void	onLeftClick();
	void	onResize(int w, int h)		{m_win_w=w;	m_win_h=h;}

protected:
	// Get the CpuThreadInfo corresponding to the calling thread
	CpuThreadInfo&	getOrAddCpuThreadInfo();

	void	drawBackground();
	void	updateBackgroundRect();
};

#endif	// defined(ENABLE_PROFILER2)

#endif // PROFILER2_H
