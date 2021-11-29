ARCH := $(shell arch)
ifeq ($(ARCH),arm64)
#$(error Unsupported architecture arm64!)
else ifeq ($(ARCH),x86_64)
CFLAGS=-O3 -march=native -Wno-unused-function
else
CFLAGS=-O3 -Wno-unused-function
endif

ifneq ($(arm_neon),) # if arm_neon is defined
ifeq ($(aarch64),)   #if aarch64 is not defined
	CFLAGS+=-D_FILE_OFFSET_BITS=64 -mfpu=neon -fsigned-char
else                 #if aarch64 is defined
	CFLAGS+=-D_FILE_OFFSET_BITS=64 -fsigned-char
endif
endif

CC=gcc

all:trimadap

trimadap: trimadap.c ksw.c kthread.c
	$(CC) $(CFLAGS) -pthread $^ -o $@ -lisal -lm
	@rm -rf trimadap.dSYM

clean:
	@rm -fr trimadap.dSYM
	rm -fr trimadap
