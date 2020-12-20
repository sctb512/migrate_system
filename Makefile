.PHONY: clean cleanobj run r up bak tb_run tb tm_run tm tt_run tt

INCLUDE := ./include

CFLAGS  := -Wall -Werror -g -I ${INCLUDE}
LD      := gcc
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

BINDIR	:= ./bin
NAME	:= main

VPATH 	:= ./common:./migrate:./perf:./rdma:.

OBJ 		:= log.o rbtree.o list.o common.o rdma.o migcomm.o perf_comm.o throughput.o migrate.o userfaultfd.o tools.o rpc.o main.o

APP		:= ${BINDIR}/${NAME}

all: clean ${APP} cleanobj

${APP}: ${OBJ}
	${LD} -o ${APP} $^ ${LDLIBS}

clean:
	rm -rf *.o ${APP} ./bin/file/*

cleanobj:
	rm -rf *.o

TARGET := abin@n2
FOLDER := Desktop/migrate_system

up:
	-ssh -fn ${TARGET} "pkill ${NAME}"
	-scp ${APP} ${TARGET}:~/${FOLDER}

run:
	cd bin && ./${NAME}

tb_run:
	cd bin && ./${NAME} tb
tm_run:
	-ssh -fn ${TARGET} "cd ~/${FOLDER} && ./${NAME} tc"
	cd bin && ./${NAME} tm
	-ssh -fn ${TARGET} "pkill ${NAME}"
tt_run:
	cd bin && ./${NAME} tt

bak:
	cd bin && ./bak.sh

r: all up run

tb:all tb_run
tm:all up tm_run
tt:all tt_run