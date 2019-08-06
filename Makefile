# MPI
MPICC=/usr/bin/mpic++
#MPICC=/usr/lib64/openmpi/bin/mpic++

FILE=invaders.c

NUMPROC=55
MPIRUN=/usr/bin/mpirun
#MPIRUN=/usr/lib64/openmpi/bin/mpirun


mpitest: ${FILE} invaderstructs.h
	${MPICC} ${FILE} -o invaders -lm -lncurses -I/tmp/NCURSES/include/ncurses -I/tmp/NCURSES/include -L/tmp/NCURSES/lib


run:
	${MPIRUN} -np ${NUMPROC} ./invaders
