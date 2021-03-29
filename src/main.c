#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <limits.h>
#include <assert.h>

#include "breakpoint.h"
#include "sample.h"

#define MAX_SAMPLES 8192

int pid;
Sample samples[MAX_SAMPLES];
unsigned int sampleCount = 0;

void handler(int signum) {
    kill(pid, signum);
    printSamples(stderr, sampleCount, samples);
    exit(-1);
}

int main(int argc, char **argv) {
    assert(argc >= 4);

    unsigned long long addrStart = 0;
    unsigned long long addrEnd = 0;
    unsigned int maxSamples = UINT_MAX;
    unsigned int programStart = 1;
    char *output = NULL;

    enum {
        EXPECTING_OPT, EXPECTING_ADDR_START, EXPECTING_ADDR_END,
        EXPECTING_MAX_SAMPLES, EXPECTING_PROGRAM, EXPECTING_OUTPUT
    } state = EXPECTING_OPT;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        switch (state) {
            case EXPECTING_OPT:
                if (strcmp(arg, "-begin") == 0) state = EXPECTING_ADDR_START;
                else if (strcmp(arg, "-end") == 0) state = EXPECTING_ADDR_END;
                else if (strcmp(arg, "-max") == 0) state = EXPECTING_MAX_SAMPLES;
                else if (strcmp(arg, "-o") == 0) state = EXPECTING_OUTPUT;
                else {
                    state = EXPECTING_PROGRAM;
                    programStart = i;
                }
                break;
            case EXPECTING_ADDR_START:
                addrStart = strtoull(argv[i], NULL, 16);
                state = EXPECTING_OPT;
                break;
            case EXPECTING_ADDR_END:
                addrEnd = strtoull(argv[i], NULL, 16);
                state = EXPECTING_OPT;
                break;
            case EXPECTING_MAX_SAMPLES:
                maxSamples = atoi(argv[i]);
                state = EXPECTING_OPT;
                break;
            case EXPECTING_OUTPUT:
                output = argv[i];
                state = EXPECTING_OPT;
                break;
            case EXPECTING_PROGRAM:
                break;
        }
    }

    printf("Measuring performance counters from 0x%llx to 0x%llx (max. samples: %u).\n", addrStart, addrEnd, maxSamples);
    printf("Executing ");
    for (int i = programStart; i < argc; i++) printf("%s ", argv[i]);
    printf("\n");

    int pid = fork();

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);

    if (pid == 0) {
        unsigned int numParams = argc - 3;
        char **newargs = malloc(sizeof(char*) * (numParams + 2));
        memcpy(newargs, &argv[programStart], sizeof(char*) * (numParams + 1));
        newargs[numParams + 1] = NULL;
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execvp(argv[programStart], newargs);
    } else {
        struct sigaction sa;
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGKILL, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);

        int status;
        waitpid(pid, &status, 0); // Wait for child to start

        Breakpoint bp;
        setBreakpoint(pid, addrStart, &bp);
        ptrace(PTRACE_CONT, pid, 0, 0);

        configureEvents();

        while (waitpid(pid, &status, 0) != -1 && sampleCount < maxSamples) {
            resetBreakpoint(pid, &bp);
            setBreakpoint(pid, addrEnd, &bp);

            beginSample(&samples[sampleCount]);
            ptrace(PTRACE_CONT, pid, 0, 0);
            waitpid(pid, &status, 0);
            endSample(&samples[sampleCount++]);

            resetBreakpoint(pid, &bp);
            setBreakpoint(pid, addrStart, &bp);
            ptrace(PTRACE_CONT, pid, 0, 0);
        }

        if (sampleCount == maxSamples) kill(pid, SIGTERM);

        FILE *outputFile = (output != NULL ? fopen(output, "w") : stderr);
        assert(outputFile != NULL);

        printSamples(outputFile, sampleCount, samples);

        if (outputFile != stderr) fclose(outputFile);

        return status;
    }
}
