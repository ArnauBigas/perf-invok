INSTALLDIR ?= /bin

.PHONY: clean

rvperf: src/main.c src/sample.c src/breakpoint.c
	gcc -g -o rvperf src/main.c src/sample.c src/breakpoint.c

install: rvperf
	cp rvperf $(INSTALLDIR)/rvperf

clean:
	rm -rf rvperf
