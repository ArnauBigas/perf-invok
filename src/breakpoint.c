#include "breakpoint.h"
#include <sys/ptrace.h>

#define BREAKPOINT_CONTENTS 0x00100073 // EBREAK Instruction

void setBreakpoint(unsigned long pid, unsigned long long address,
                   Breakpoint *breakpoint) {
    breakpoint->address = address;
    breakpoint->originalData = ptrace(PTRACE_PEEKDATA, pid, address, 0);
    unsigned long long modifiedData = ((breakpoint->originalData & ~0xffffffff) | BREAKPOINT_CONTENTS);
    ptrace(PTRACE_POKEDATA, pid, address, modifiedData);
}

void resetBreakpoint(unsigned long pid, Breakpoint *breakpoint) {
    ptrace(PTRACE_POKEDATA, pid, breakpoint->address, breakpoint->originalData);
}
