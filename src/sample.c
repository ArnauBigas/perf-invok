#include "sample.h"
#include "debug.h"

#ifdef PERF_INVOK_PLATFORM_SIFIVE_FU540

#include "platform/sifive_fu540.h"

// API events
#define EVENT_MEM_INSTR_RETIRED (EVENT_BIT_INT_LOAD_RETIRED | EVENT_BIT_INT_STORE_RETIRED | EVENT_BIT_FLT_LOAD_RETIRED | EVENT_BIT_FLT_STORE_RETIRED)
#define EVENT_DATA_CACHE_BUSY EVENT_BIT_DATA_CACHE_BUSY
#define EVENT_DATA_CACHE_MISS EVENT_BIT_DATA_CACHE_MISS

#define READ_CSR(reg) ({unsigned long long __tmp; asm volatile ("csrr %0, " #reg : "=r"(__tmp)); __tmp;})
#define WRITE_CSR(reg, val) asm volatile ("csrw " #reg ", %0" :: "r" (val))

#else

#include <linux/perf_event.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>

struct read_format {
  unsigned long long nr;
  struct {
    unsigned long long value;
    unsigned long long id;
  } values[];
};

int fdCycles, fdInstructions, fdMemoryLoads, fdMemoryWrites, fdMemoryReadMisses,
    fdMemoryWriteMisses;
unsigned long long idCycles, idInstructions, idMemoryLoads, idMemoryWrites,
                   idMemoryReadMisses, idMemoryWriteMisses;
struct timeval start;

#endif

void configureEvents(pid_t pid) {

    debug_print("Configuring events for process %d\n", pid);

#ifdef PERF_INVOK_PLATFORM_SIFIVE_FU540

    debug_print("Configuring EVENT_MEM_INSTR_RETIRED");
    WRITE_CSR(mhpmevent3, EVENT_MEM_INSTR_RETIRED);
    debug_print("Configuring EVENT_DATA_CACHE_MISS");
    WRITE_CSR(mhpmevent4, EVENT_DATA_CACHE_MISS);

#else

    struct perf_event_attr attributes;
    memset(&attributes, 0, sizeof(struct perf_event_attr));

    attributes.size = sizeof(struct perf_event_attr);
    attributes.disabled = 1;
    attributes.exclude_kernel = 1;
    attributes.exclude_hv = 1;
    attributes.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

    debug_print("Configuring PERF_COUNT_HW_CPU_CYCLES\n", 0);
    attributes.config = PERF_COUNT_HW_CPU_CYCLES;
    attributes.type = PERF_TYPE_HARDWARE;
    fdCycles = syscall(__NR_perf_event_open, &attributes, pid, -1, -1, 0);
    ioctl(fdCycles, PERF_EVENT_IOC_ID, &idCycles);

    debug_print("Configuring PERF_COUNT_HW_INSTRUCTIONS\n", 0);
    attributes.config = PERF_COUNT_HW_INSTRUCTIONS;
    attributes.type = PERF_TYPE_HARDWARE;
    fdInstructions =
        syscall(__NR_perf_event_open, &attributes, pid, -1, fdCycles, 0);
    ioctl(fdInstructions, PERF_EVENT_IOC_ID, &idInstructions);

    debug_print("Configuring PERF_COUNT_HW_CACHE_L1D:READ:ACCESS\n", 0);
    attributes.config = PERF_COUNT_HW_CACHE_L1D
                        | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                        | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    attributes.type = PERF_TYPE_HW_CACHE;
    fdMemoryLoads =
        syscall(__NR_perf_event_open, &attributes, pid, -1, fdCycles, 0);
    ioctl(fdMemoryLoads, PERF_EVENT_IOC_ID, &idMemoryLoads);

    debug_print("Configuring PERF_COUNT_HW_CACHE_L1D:WRITE:ACCESS\n", 0);
    attributes.config = PERF_COUNT_HW_CACHE_L1D
                        | (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
                        | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    attributes.type = PERF_TYPE_HW_CACHE;
    fdMemoryWrites =
        syscall(__NR_perf_event_open, &attributes, pid, -1, fdCycles, 0);
    ioctl(fdMemoryWrites, PERF_EVENT_IOC_ID, &idMemoryWrites);

    debug_print("Configuring PERF_COUNT_HW_CACHE_L1D:READ:MISS\n", 0);
    attributes.config = PERF_COUNT_HW_CACHE_L1D
                        | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                        | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    attributes.type = PERF_TYPE_HW_CACHE;
    fdMemoryReadMisses =
        syscall(__NR_perf_event_open, &attributes, pid, -1, fdCycles, 0);
    ioctl(fdMemoryReadMisses, PERF_EVENT_IOC_ID, &idMemoryReadMisses);

    debug_print("Configuring PERF_COUNT_HW_CACHE_L1D:WRITE:MISS\n", 0);
    attributes.config = PERF_COUNT_HW_CACHE_L1D
                        | (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
                        | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    attributes.type = PERF_TYPE_HW_CACHE;
    fdMemoryWriteMisses =
        syscall(__NR_perf_event_open, &attributes, pid, -1, fdCycles, 0);
    ioctl(fdMemoryWriteMisses, PERF_EVENT_IOC_ID, &idMemoryWriteMisses);

#endif

    debug_print("Configuring events for process %d done\n", pid);

}

void beginSample(Sample *sample) {

    debug_print("Begin sample, reset counters\n", 0);
#ifdef PERF_INVOK_PLATFORM_SIFIVE_FU540

    WRITE_CSR(mhpmcounter3, 0);
    WRITE_CSR(mhpmcounter4, 0);
    WRITE_CSR(instret, 0);
    WRITE_CSR(cycle, 0);
    sample->time = READ_CSR(time);

#else

    memset(sample, 0, sizeof(Sample));

    gettimeofday(&start, NULL);

    ioctl(fdCycles, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fdCycles, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);

#endif
    debug_print("Begin sample, reset counters done\n", 0);
}

void endSample(Sample *sample) {

    debug_print("End sample, read counters\n", 0);

#ifdef PERF_INVOK_PLATFORM_SIFIVE_FU540

    sample->cycles = READ_CSR(cycle);
    sample->retiredInstructions = READ_CSR(instret);
    sample->retiredMemoryInstructions = READ_CSR(mhpmcounter3);
    sample->dataCacheMisses = READ_CSR(mhpmcounter4);
    sample->time = (READ_CSR(time) - sample->time) * TIME_TO_US;

#else

    ioctl(fdCycles, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    struct timeval end;
    gettimeofday(&end, NULL);
    timersub(&end, &start, &end);
    sample->time = end.tv_sec * 1000000 + end.tv_usec;

    char buf[4096];
    struct read_format *rf = (struct read_format *) buf;
    read(fdCycles, buf, sizeof(buf));
    for (unsigned int i = 0; i < rf->nr; i++) {
        unsigned long long id = rf->values[i].id;
        unsigned long long value = rf->values[i].value;
        if (id == idCycles) {
            sample->cycles = value;
        } else if (id == idInstructions) {
            sample->retiredInstructions = value;
        } else if (id == idMemoryLoads || id == idMemoryWrites) {
            sample->retiredMemoryInstructions += value;
        } else if (id == idMemoryReadMisses || id == idMemoryWriteMisses) {
            sample->dataCacheMisses += value;
        }
    }

#endif

    debug_print("End sample, read counters done\n", 0);

}

void printSamples(FILE *fd, unsigned int sampleCount, Sample *samples,
                  int printHeaders) {

    if (printHeaders) {
        fprintf(fd, "Cycles, Time Elapsed (us), Retired Instructions, "
                    "Retired Memory Instructions, Data Cache Misses, "
                    "Instructions Per Cycle, Miss Percentage\n");
    }

    for (unsigned int i = 0; i < sampleCount; i++) {
        Sample *sample = &samples[i];

        float ipc = sample->retiredInstructions / (float) sample->cycles;
        float missrate =
            sample->dataCacheMisses / (float) sample->retiredMemoryInstructions;

        fprintf(fd, "%llu, %llu, %llu, %llu, %llu, %f, %f\n",
                sample->cycles, sample->time,
                sample->retiredInstructions, sample->retiredMemoryInstructions,
                sample->dataCacheMisses,
                ipc, 100.0f * missrate);
    }
}
