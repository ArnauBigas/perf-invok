#include "sample.h"

#include "platform/sifive_fu540.h"

// API events
#define EVENT_MEM_INSTR_RETIRED (EVENT_BIT_INT_LOAD_RETIRED | EVENT_BIT_INT_STORE_RETIRED | EVENT_BIT_FLT_LOAD_RETIRED | EVENT_BIT_FLT_STORE_RETIRED)
#define EVENT_DATA_CACHE_BUSY EVENT_BIT_DATA_CACHE_BUSY
#define EVENT_DATA_CACHE_MISS EVENT_BIT_DATA_CACHE_MISS

#define read_csr_safe(reg) ({unsigned long long __tmp; asm volatile ("csrr %0, " #reg : "=r"(__tmp)); __tmp;})
#define write_csr_safe(reg, val) asm volatile ("csrw " #reg ", %0" :: "r" (val))

#define SAMPLE_COUNT_EVENTS(counter, hwcounter) \
    counter = read_csr_safe(hwcounter)

void configureEvents() {
    write_csr_safe(mhpmevent3, EVENT_MEM_INSTR_RETIRED);
    write_csr_safe(mhpmevent4, EVENT_DATA_CACHE_MISS);
}

void beginSample(Sample *sample) {
    write_csr_safe(mhpmcounter3, 0);
    write_csr_safe(mhpmcounter4, 0);
    write_csr_safe(instret, 0);
    write_csr_safe(cycle, 0);
}

void endSample(Sample *sample) {
    SAMPLE_COUNT_EVENTS(sample->cycles, cycle);
    SAMPLE_COUNT_EVENTS(sample->retiredInstructions, instret);
    SAMPLE_COUNT_EVENTS(sample->retiredMemoryInstructions, mhpmcounter3);
    SAMPLE_COUNT_EVENTS(sample->dataCacheMisses, mhpmcounter4);
}

void printSamples(FILE *fd, unsigned int sampleCount, Sample *samples) {
    fprintf(fd, "Cycles, Retired Instructions, Retired Memory Instructions, "
                "Data Cache Misses, Instructions Per Cycle, Miss Percentage\n");

    for (unsigned int i = 0; i < sampleCount; i++) {
        Sample *sample = &samples[i];

        float ipc = sample->retiredInstructions / (float) sample->cycles;
        float missrate =
            sample->dataCacheMisses / (float) sample->retiredMemoryInstructions;

        fprintf(fd, "%llu, %llu, %llu, %llu, %f, %f\n",
                sample->cycles,
                sample->retiredInstructions,
                sample->retiredMemoryInstructions,
                sample->dataCacheMisses,
                ipc,
                100.0f * missrate);
    }
}
