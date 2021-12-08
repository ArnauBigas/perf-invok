#include "debug.h"
#include "breakpoint.h"
#include <sys/ptrace.h>

#define RV_INSTR_EBREAK 0x00100073 // EBREAK Instruction
#define RV_INSTR_CEBREAK 0x9002    // C.BREAK Instruction

void setBreakpoint(unsigned long pid, unsigned long long address,
                   Breakpoint *breakpoint) {
    breakpoint->address = address;
    breakpoint->originalData = ptrace(PTRACE_PEEKDATA, pid, address, 0);

    debug_print("setBreakpoint 0x%016X to 0x%08X (orig 0x%08X)\n", address, 0, breakpoint->originalData);
    ptrace(PTRACE_POKEDATA, pid, address, 0);
}

void resetBreakpoint(unsigned long pid, Breakpoint *breakpoint) {
    debug_print("resetBreakpoint 0x%016X to 0x%08X\n", breakpoint->address, breakpoint->originalData);
    ptrace(PTRACE_POKEDATA, pid, breakpoint->address, breakpoint->originalData);
}
