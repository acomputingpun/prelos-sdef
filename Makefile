CC = gcc
CFLAGS = -Wall -c
ALLO = wedges.o tiles.o poses.o

prelos: ${ALLO} prelos.o localgrids.o
	${CC} -o prelos prelos.o ${ALLO} localgrids.o

shared: ${ALLO} linkgrids.o
	${CC} -o prelos.so -shared ${ALLO} linkgrids.o

prelos.o: prelos.c poses.h wedges.h rays.h tiles.h
	${CC} ${CFLAGS} prelos.c

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
	rm ${ALLO} prelos prelos.so prelos.o
