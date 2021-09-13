INSTALLDIR ?= /bin

ifeq ($(cat /proc/cpuinfo | grep "uarch" | head -n 1 | cut -d" " -f 2),"sifive,u54-mc")
	PERF_INVOK_PLATFORM_SIFIVE_FU540=1
endif

.PHONY: clean

perf-invok: src/main.c src/sample.c src/breakpoint.c
	gcc -g -o perf-invok src/main.c src/sample.c src/breakpoint.c

install: perf-invok
	cp perf-invok $(INSTALLDIR)/perf-invok

clean:
	rm -rf perf-invok
