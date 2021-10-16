ARCH := $(shell arch)
ifeq ($(ARCH),arm64)
$(error Unsupported architecture arm64!)
else ifeq ($(ARCH),x86_64)
CFLAGS=-O3 -march=native -Wno-unused-function
else
CFLAGS=-O3 -Wno-unused-function
endif

CC=gcc

all:trimadap

trimadap: trimadap.c ksw.c kthread.c
	$(CC) $(CFLAGS) -pthread $^ -o $@ -lisal -lm
	@rm -rf trimadap.dSYM

clean:
	@rm -fr trimadap.dSYM
	rm -fr trimadap
