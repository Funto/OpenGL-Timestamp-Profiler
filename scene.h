// scene.h

#ifndef SCENE_H
#define SCENE_H

#include <GL/glew.h>

#include "camera.h"
#include "grid.h"
#include "thread.h"
#include "utils.h"

class Scene
{
private:
	static const int	NB_GRIDS_X = 2;
	static const int	NB_GRIDS_Y = 2;
	static const int	NB_THREADS = NB_GRIDS_X * NB_GRIDS_Y;
	static const Color	COLORS[NB_THREADS];

	Camera	m_camera;

	// Multithread update
	struct	ThreadData
	{
		Scene*			p_scene;
		Grid			grid;
		ThreadId		thread_id;
		Event			update_event;
		Event			main_event;
	};

	bool			m_multithread;
	ThreadData		m_thread_data[NB_THREADS];
	volatile bool	m_shut;
	volatile float	m_elapsed_time;
	volatile float	m_update_time;

public:
	bool	init();
	void	shut();
	void	update(double elapsed, double t);
	void	draw();

	void	setMultithreaded(bool multithread) {m_multithread = multithread;}
	bool	isMultithreaded() const	{return m_multithread;}

private:
	void	threadUpdate(ThreadData* data);

	static void* threadUpdateWrapper(void* user_data);
};

#endif // SCENE_H
