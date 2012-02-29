CFLAGS=-ggdb -g3 -Wall
LIB_FLAGS=-L. -lrobot_if
CPP_LIB_FLAGS=${LIB_FLAGS} -lrobot_if++
LIB_LINK=-lhighgui -lcv -lcxcore -lm

PROGS=robot_vision

all: ${PROGS}

robot_vision: robot_vision.c
	gcc ${CFLAGS} -c robot_vision.c ${LIB_FLAGS}
	gcc ${CFLAGS} -o robot_vision robot_vision.o ${LIB_FLAGS} ${LIB_LINK}

clean:
	rm -rf *.o *~
	rm -rf ${PROGS}
