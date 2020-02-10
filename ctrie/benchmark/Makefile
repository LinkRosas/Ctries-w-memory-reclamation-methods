LIB_PATH?=../Ctrie

CC=gcc
CFLAGS=-std=gnu11 -Wall -I$(LIB_PATH)
OPT=-O3 -flto
LFLAGS=-lpthread
DEBUG=-g -ggdb -Og -DDEBUG=1

default:bench
debug: bench_debug
all: bench bench_debug

$(LIB_PATH)/Ctrie.a: $(LIB_PATH)/Ctrie.c $(LIB_PATH)/Ctrie.h 
	cd $(LIB_PATH) && make Ctrie.a

bench: bench.c $(LIB_PATH)/Ctrie.c $(LIB_PATH)/Ctrie.h $(LIB_PATH)/Nodes.h
	$(CC) bench.c $(LIB_PATH)/Ctrie.c $(LIB_PATH)/Nodes.c $(CFLAGS) $(OPT) $(LFLAGS) -o bench

bench_debug: bench.c $(LIB_PATH)/Ctrie.c $(LIB_PATH)/Ctrie.h $(LIB_PATH)/Nodes.c  $(LIB_PATH)/Nodes.h
	$(CC) bench.c $(LIB_PATH)/Ctrie.c $(LIB_PATH)/Nodes.c $(CFLAGS) $(DEBUG) $(LFLAGS) -o bench_debug

clean:
	rm -f *.o bench bench_debug
	cd $(LIB_PATH) && rm -f *.o
