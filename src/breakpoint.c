#include "debug.h"
#include "breakpoint.h"
#include <stdio.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#if defined(__s390x__)
#include <sys/uio.h>
#endif

void setBreakpoint(unsigned long pid, unsigned long long address,
                   Breakpoint *breakpoint) {
    breakpoint->address = address;
    errno = 0;
    debug_print("setBreakpoint 0x%016llX to 0x%08X (orig 0x%08llX)\n", address, 0, breakpoint->originalData);
    breakpoint->originalData = ptrace(PTRACE_PEEKDATA, pid, address, 0);
    if (errno != 0) { perror("ERROR while setting breakpoint (read)"); exit(EXIT_FAILURE);};
    long ret = ptrace(PTRACE_POKEDATA, pid, address, 0);
    if (ret != 0) { perror("ERROR while setting breakpoint (write)"); exit(EXIT_FAILURE);};
}

void resetBreakpoint(unsigned long pid, Breakpoint *breakpoint) {
    debug_print("resetBreakpoint 0x%016llX to 0x%08llX\n", breakpoint->address, breakpoint->originalData);
    long ret = ptrace(PTRACE_POKEDATA, pid, breakpoint->address, breakpoint->originalData);
    if (ret != 0) { perror("ERROR while restoring breakpoint"); exit(EXIT_FAILURE);};
}

#if defined(__s390x__)
void displace_pc(long pid, long displ) {
    long buf[2];
    struct iovec iov;
    iov.iov_len = sizeof(buf);
    iov.iov_base = buf;

    long ret = ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
    if (ret != 0) { perror("ERROR while reading PC"); exit(EXIT_FAILURE);};
    long pc = buf[1];

	debug_print("displace_pc from 0x%016lX to 0x%016lX\n", pc, pc + displ);

    buf[1] = pc + displ;
    ret = ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
    if (ret != 0) { perror("ERROR while setting PC"); exit(EXIT_FAILURE);};
}
#endif
