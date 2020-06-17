CC = gcc
CFLAGS = -Wall -c -lm -fPIC -O2
READONLYO = poses.o nodes.o files.o
ALLO = ${READONLYO} create_nodes.o wdicts.o wedges.o wrapper.o tiles.o

prelos: ${ALLO} prelos.o localgrids.o
	${CC} -o prelos prelos.o ${ALLO} localgrids.o -lm

ro_prelos: ${READONLYO} prelos.o localgrids.o ro_wrapper.o
	"not implemented!"
#	${CC} -o ro_prelos prelos.o ${ALLO} localgrids.o -lm

shared: ${ALLO} linkgrids.o
	${CC} -o prelos.so -shared ${ALLO} linkgrids.o -lm

ro_shared: ${READONLYO} linkgrids.o ro_wrapper.o
	${CC} -o ro_prelos.so -shared ${READONLYO} linkgrids.o -lm

prelos.o: prelos.c poses.h wedges.h rays.h tiles.h nodes.h
	${CC} ${CFLAGS} prelos.c

ro_wrapper.o: wrapper.c poses.h rays.h nodes.h files.h
	${CC} ${CFLAGS} ro_wrapper.c

wrapper.o: wrapper.c poses.h wedges.h rays.h tiles.h nodes.h files.h
	${CC} ${CFLAGS} wrapper.c

files.o: files.c files.h nodes.h
	${CC} ${CFLAGS} files.c

nodes.o: nodes.c nodes.h grids.h poses.h rays.h
	${CC} ${CFLAGS} nodes.c

create_nodes.o: create_nodes.c create_nodes.h wedges.h grids.h tiles.h poses.h rays.h
	${CC} ${CFLAGS} create_nodes.c

wdicts.o: wdicts.c wedges.h wdicts.h rays.h poses.h
	${CC} ${CFLAGS} wdicts.c

wedges.o: wedges.c wedges.h wdicts.h rays.h poses.h
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
	rm -f ${ALLO} prelos prelos.so prelos.o ro_prelos ro_prelos.so ro_wrapper.o
