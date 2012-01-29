src_list = Split("""
drawer2D.cpp
hp_timer.cpp
main.cpp
scene.cpp
profiler.cpp
thread.cpp
utils.cpp
math_utils.cpp
grid.cpp
tgaloader.cpp
profiler2.cpp
""")

env = Environment()
env.ParseConfig('pkg-config glew libglfw --cflags --libs')
env.Append(LIBS=['GLU'])
env.Append(CCFLAGS=['-g', '-Wall'])
env.Program('profiler', src_list)
