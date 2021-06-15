#include "breakpoint.h"
#include <sys/ptrace.h>

#define RV_INSTR_EBREAK 0x00100073 // EBREAK Instruction
#define RV_INSTR_CEBREAK 0x9002    // C.BREAK Instruction

void setBreakpoint(unsigned long pid, unsigned long long address,
                   Breakpoint *breakpoint) {
    breakpoint->address = address;
    breakpoint->originalData = ptrace(PTRACE_PEEKDATA, pid, address, 0);
    unsigned long long modifiedData = ((breakpoint->originalData & ~0xffff) | RV_INSTR_CEBREAK);
    ptrace(PTRACE_POKEDATA, pid, address, modifiedData);
}

void resetBreakpoint(unsigned long pid, Breakpoint *breakpoint) {
    ptrace(PTRACE_POKEDATA, pid, breakpoint->address, breakpoint->originalData);
}
