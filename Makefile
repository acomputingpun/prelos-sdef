CC = gcc
CFLAGS = -Wall -c -lm
ALLO = wedges.o tiles.o poses.o nodes.o

prelos: ${ALLO} prelos.o localgrids.o
	${CC} -o prelos prelos.o ${ALLO} localgrids.o -lm

shared: ${ALLO} linkgrids.o
	${CC} -o prelos.so -shared ${ALLO} linkgrids.o -lm

prelos.o: prelos.c poses.h wedges.h rays.h tiles.h nodes.h
	${CC} ${CFLAGS} prelos.c

nodes.o: nodes.c nodes.h wedges.h grids.h tiles.h poses.h rays.h
	${CC} ${CFLAGS} nodes.c

wedges.o: wedges.c wedges.h rays.h poses.h
	${CC} ${CFLAGS} wedges.c

tiles.o: tiles.c tiles.h poses.h
	${CC} ${CFLAGS} tiles.c

localgrids.o: localgrids.c grids.h poses.h
	${CC} ${CFLAGS} localgrids.c
linkgrids.o: linkgrids.c grids.h poses.h
	${CC} ${CFLAGS} linkgrids.c

poses.o: poses.c poses.h
	${CC} ${CFLAGS} poses.c

clean:
	rm -f ${ALLO} prelos prelos.so prelos.o
