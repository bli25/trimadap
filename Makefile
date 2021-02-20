CC=gcc
CFLAGS=-g -Wall -O2 -Wno-unused-function

all:trimadap

trimadap: trimadap.c ksw.c kthread.c
	$(CC) $(CFLAGS) -pthread $^ -o $@ -lz -lm
	rm -rf trimadap.dSYM

clean:
	rm -fr trimadap.dSYM trimadap
