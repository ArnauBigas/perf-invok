INSTALLDIR ?= /bin

ifeq ($(shell cat /proc/cpuinfo | grep "uarch" | head -n 1 | cut -d" " -f 2),sifive,u54-mc)
CFLAGS := ${CFLAGS} -DPERF_INVOK_PLATFORM_SIFIVE_FU540
endif

.PHONY: clean

perf-invok: src/main.c src/sample.c src/breakpoint.c
	gcc ${CFLAGS} -o perf-invok src/main.c src/sample.c src/breakpoint.c

debug: src/main.c src/sample.c src/breakpoint.c src/debug.h
	gcc ${CFLAGS} -DDEBUG_MODE -o perf-invok src/main.c src/sample.c src/breakpoint.c

install: perf-invok
	cp perf-invok $(INSTALLDIR)/perf-invok

clean:
	rm -rf perf-invok
