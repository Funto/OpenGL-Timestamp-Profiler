// profiler.h

#ifndef PROFILER_H
#define PROFILER_H

#include <GL/glew.h>
#include <list>
#include <vector>
#include <stack>
#include <string>
#include <map>
#include "thread.h"
#include "utils.h"

class Profiler;
extern Profiler profiler;

#define ENABLE_PROFILER	// comment this to remove the profiler

#ifdef ENABLE_PROFILER
	#define PROFILER_PUSH_CPU_MARKER(name, color) \
		profiler.pushCpuMarker(name, color)

	#define PROFILER_POP_CPU_MARKER()  \
		profiler.popCpuMarker()

	#define PROFILER_PUSH_GPU_MARKER(name, color) \
		profiler.pushGpuMarker(name, color)

	#define PROFILER_POP_GPU_MARKER()  \
		profiler.popGpuMarker()

	#define PROFILER_SYNC_FRAME()   \
		profiler.synchronizeFrame()

	#define PROFILER_DRAW() \
		profiler.draw()
#else
	#define PROFILER_PUSH_CPU_MARKER(name, color)
	#define PROFILER_POP_CPU_MARKER()
	#define PROFILER_PUSH_GPU_MARKER(name, color)
	#define PROFILER_POP_GPU_MARKER()
	#define PROFILER_SYNC_FRAME()
	#define PROFILER_DRAW()
#endif

class Profiler
{
private:
	static const int NB_RECORDED_FRAMES = 2;	// TODO: increase

	struct Marker
	{
		double  start;  // Times of start and end, in milliseconds,
		double  end;	// relatively to the time of last synchronization
		size_t  layer;

		std::string	 name;
		Color		color;

		GLuint		id_query_start;
		GLuint		id_query_end;

		Marker(	double start, double end, const char* name="N/A",
				Color color=COLOR_BLACK,
				size_t layer=0,
				GLuint id_query_start=0,
				GLuint id_query_end=0)
			: start(start), end(end), layer(layer), name(name), color(color),
			  id_query_start(id_query_start), id_query_end(id_query_end)
		{
		}
		// TODO: remove
/*
		Marker(const Marker& ref)
			: start(ref.start), end(ref.end), layer(ref.layer), name(ref.name), color(ref.color),
			  type(ref.type), gpu_query_id(ref.gpu_query_id)
		{
		}*/
	};

	typedef	std::list<Marker>		MarkerList;
	typedef	std::stack<Marker>		MarkerStack;

	enum ThreadType
	{
		THREAD_CPU,
		THREAD_GPU
	};

	struct ThreadInfo
	{
		ThreadType		type;
		MarkerList		markers_done[NB_RECORDED_FRAMES];
		MarkerStack		markers_stack[NB_RECORDED_FRAMES];
		Mutex			mutex;
	};

	typedef	std::map<ThreadId, ThreadInfo>	CpuThreadInfoMap;
	typedef	std::vector<ThreadInfo>			GpuThreadInfoList;

	CpuThreadInfoMap	m_cpu_thread_infos;
	GpuThreadInfoList	m_gpu_thread_infos;

	int					m_write_id;
	double				m_time_last_sync;
	double				m_time_between_sync;

	// Handling freeze/unfreeze by clicking on the display
	enum FreezeState
	{
		UNFROZEN,
		WAITING_FOR_FREEZE,
		FROZEN,
		WAITING_FOR_UNFREEZE,
	};

	FreezeState	 m_freeze_state;

	// Handling interaction with the mouse
	int m_mouse_x, m_mouse_y;
	int m_win_w, m_win_h;

	Rect m_back_rect;

public:
	Profiler();
	virtual ~Profiler();

	void	init(int win_w, int win_h, int mouse_x, int mouse_y);
	void	shut();

	void	pushCpuMarker(const char* name="N/A", const Color& color=COLOR_WHITE);
	void	popCpuMarker();

	void	pushGpuMarker(const char* name="N/A", const Color& color=COLOR_WHITE);
	void	popGpuMarker();

	void	synchronizeFrame();

	void	draw();

	void	onMousePos(int x, int y)	{m_mouse_x=x;	m_mouse_y=y;}
	void	onLeftClick();
	void	onResize(int w, int h)		{m_win_w=w; m_win_h=h;}

protected:
	// TODO: detect on which thread this is called to support multithreading
	ThreadInfo& getCpuThreadInfo();
	ThreadInfo& getGpuThreadInfo();
	void		drawBackground();
};

#endif // PROFILER_H
