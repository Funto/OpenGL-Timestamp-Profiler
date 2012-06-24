CC=g++
CFLAGS=-Wall -g
CPPFLAGS=-Iglew-1.7.0/include -Iglfw-2.7.5.bin.WIN32\include
LDFLAGS=-Lglew-1.7.0/lib -lglew32 -Lglfw-2.7.5.bin.WIN32\lib-mingw -lglfw -lopengl32 -lgdi32
EXEC=glprofiler
SRC= $(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)

all: $(EXEC)

glprofiler: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.h

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f *.o $(EXEC)
