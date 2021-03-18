#include "sample.h"

#include "platform/sifive_fu540.h"

// API events
#define EVENT_MEM_INSTR_RETIRED (EVENT_BIT_INT_LOAD_RETIRED | EVENT_BIT_INT_STORE_RETIRED | EVENT_BIT_FLT_LOAD_RETIRED | EVENT_BIT_FLT_STORE_RETIRED)
#define EVENT_DATA_CACHE_BUSY EVENT_BIT_DATA_CACHE_BUSY
#define EVENT_DATA_CACHE_MISS EVENT_BIT_DATA_CACHE_MISS

#define READ_CSR(reg) ({unsigned long long __tmp; asm volatile ("csrr %0, " #reg : "=r"(__tmp)); __tmp;})
#define WRITE_CSR(reg, val) asm volatile ("csrw " #reg ", %0" :: "r" (val))

void configureEvents() {
    WRITE_CSR(mhpmevent3, EVENT_MEM_INSTR_RETIRED);
    WRITE_CSR(mhpmevent4, EVENT_DATA_CACHE_MISS);
}

void beginSample(Sample *sample) {
    WRITE_CSR(mhpmcounter3, 0);
    WRITE_CSR(mhpmcounter4, 0);
    WRITE_CSR(instret, 0);
    WRITE_CSR(cycle, 0);
    sample->time = READ_CSR(time);
}

void endSample(Sample *sample) {
    sample->cycles = READ_CSR(cycle);
    sample->retiredInstructions = READ_CSR(instret);
    sample->retiredMemoryInstructions = READ_CSR(mhpmcounter3);
    sample->dataCacheMisses = READ_CSR(mhpmcounter4);
    sample->time = READ_CSR(time) - sample->time;
}

void printSamples(FILE *fd, unsigned int sampleCount, Sample *samples) {
    fprintf(fd, "Cycles, Time Elapsed (us), Retired Instructions, "
                "Retired Memory Instructions, Data Cache Misses, "
                "Instructions Per Cycle, Miss Percentage\n");

    for (unsigned int i = 0; i < sampleCount; i++) {
        Sample *sample = &samples[i];

        float ipc = sample->retiredInstructions / (float) sample->cycles;
        float missrate =
            sample->dataCacheMisses / (float) sample->retiredMemoryInstructions;

        fprintf(fd, "%llu, %llu, %llu, %llu, %llu, %f, %f\n",
                sample->cycles, sample->time * TIME_TO_US,
                sample->retiredInstructions, sample->retiredMemoryInstructions,
                sample->dataCacheMisses,
                ipc, 100.0f * missrate);
    }
}
