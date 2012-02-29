CFLAGS=-ggdb -g3 -Wall
LIB_FLAGS=-L. -lrobot_if
CPP_LIB_FLAGS=${LIB_FLAGS} -lrobot_if++
LIB_LINK=-lhighgui -lcv -lcxcore -lm

PROGS=robot_camera_example

all: ${PROGS}

robot_camera_example: robot_camera_example.c
	gcc ${CFLAGS} -c robot_camera_example.c
	gcc ${CFLAGS} -o robot_camera_example robot_camera_example.o ${LIB_FLAGS} ${LIB_LINK}

clean:
	rm -rf *.o *~
	rm -rf ${PROGS}