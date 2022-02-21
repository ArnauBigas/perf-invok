INSTALLDIR ?= /bin
CFLAGS ?= -O3 -Wall -Werror -pedantic -flto
ifeq ($(shell cat /proc/cpuinfo | grep "uarch" | head -n 1 | cut -d" " -f 2),sifive,u54-mc)
CFLAGS := ${CFLAGS} -DPERF_INVOK_PLATFORM_SIFIVE_FU540
endif
TEST_PROGRAMS = vector_add sandwich_return

.PHONY: clean test

all: perf-invok

perf-invok: src/main.c src/sample.c src/breakpoint.c
	gcc ${CFLAGS} -o perf-invok src/main.c src/sample.c src/breakpoint.c

debug: src/main.c src/sample.c src/breakpoint.c src/debug.h
	gcc ${CFLAGS} -DDEBUG_MODE -g -o perf-invok-debug src/main.c src/sample.c src/breakpoint.c

vector_add: test_programs/vector_add.c
	gcc -O0 -no-pie -fno-pic -o $@ $^

ifeq ($(shell uname -m),riscv64)
sandwich_return: test_programs/sandwiched_return.c  test_programs/sandwiched_return_riscv.s
	gcc -O0 -no-pie -fno-pic -o $@ $^
else ifeq ($(shell uname -m),ppc64le)
sandwich_return: test_programs/sandwiched_return.c  test_programs/sandwiched_return_ppc64.s
	gcc -O0 -no-pie -fno-pic -o $@ $^
else ifeq ($(shell uname -m),s390x)
sandwich_return: test_programs/sandwiched_return.c  test_programs/sandwiched_return_s390x.s
	gcc -O0 -no-pie -fno-pic -o $@ $^
else
	$(error Architecture not supported)
endif

test: all $(TEST_PROGRAMS)
	@echo Testing generic code...
	@tests/test_vector_add.sh
	@echo Testing sandwich return...
	@tests/test_sandwich_return.sh

install: perf-invok
	cp perf-invok $(INSTALLDIR)/perf-invok

clean:
	rm -rf perf-invok perf-invok-debug vector_add sandwich_return
