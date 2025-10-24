#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sdt.h>  // For USDT probes
#include <string.h>
#include <time.h>

// Define USDT probes
// DTRACE_PROBE(provider, name)
// DTRACE_PROBE1(provider, name, arg1)
// etc.

#define ALLOC_COUNT 100
#define MAX_ALLOC_SIZE 1024

// Simple allocation wrapper with USDT probes
void* tracked_malloc(size_t size) {
    void* ptr = malloc(size);
    
    // Fire USDT probe for allocation
    // Provider: memory_profiler, Probe: malloc_entry
    DTRACE_PROBE2(memory_profiler, malloc_entry, size, ptr);
    
    return ptr;
}

// Simple free wrapper with USDT probes
void tracked_free(void* ptr, size_t size) {
    // Fire USDT probe for deallocation
    DTRACE_PROBE2(memory_profiler, free_entry, ptr, size);
    
    free(ptr);
}

// Simulate some allocation patterns
void allocation_pattern_1() {
    printf("Pattern 1: Small frequent allocations\n");
    
    for (int i = 0; i < 50; i++) {
        size_t size = 16 + (i % 64);
        void* ptr = tracked_malloc(size);
        
        if (ptr) {
            memset(ptr, 0, size);  // Touch the memory
            usleep(10000);  // 10ms
            tracked_free(ptr, size);
        }
    }
}

void allocation_pattern_2() {
    printf("Pattern 2: Large allocations\n");
    
    void* ptrs[10];
    size_t sizes[10];
    
    for (int i = 0; i < 10; i++) {
        sizes[i] = 1024 * (i + 1);
        ptrs[i] = tracked_malloc(sizes[i]);
        
        if (ptrs[i]) {
            memset(ptrs[i], 0xAB, sizes[i]);
        }
        usleep(50000);  // 50ms
    }
    
    // Free in reverse order
    for (int i = 9; i >= 0; i--) {
        if (ptrs[i]) {
            tracked_free(ptrs[i], sizes[i]);
        }
    }
}

void allocation_pattern_3() {
    printf("Pattern 3: Mixed sizes with leak simulation\n");
    
    for (int i = 0; i < 20; i++) {
        size_t size = (rand() % 512) + 1;
        void* ptr = tracked_malloc(size);
        
        if (ptr) {
            memset(ptr, i, size);
            
            // Intentionally "leak" some allocations (don't free them)
            if (i % 5 != 0) {
                usleep(20000);  // 20ms
                tracked_free(ptr, size);
            } else {
                printf("  [Intentional leak] Not freeing %zu bytes at %p\n", size, ptr);
            }
        }
    }
}

int main(int argc, char** argv) {
    printf("=== USDT Memory Profiler Sample ===\n");
    printf("PID: %d\n", getpid());
    printf("This program demonstrates USDT probes for memory profiling\n\n");
    
    // Give time to attach tracer
    printf("Waiting 5 seconds for tracer to attach...\n");
    sleep(5);
    
    srand(time(NULL));
    
    // Fire a startup probe
    DTRACE_PROBE(memory_profiler, startup);
    
    printf("\n--- Starting allocation patterns ---\n\n");
    
    allocation_pattern_1();
    sleep(1);
    
    allocation_pattern_2();
    sleep(1);
    
    allocation_pattern_3();
    sleep(1);
    
    // Fire a shutdown probe
    DTRACE_PROBE(memory_profiler, shutdown);
    
    printf("\n--- Finished allocation patterns ---\n");
    printf("Program will exit in 2 seconds...\n");
    sleep(2);
    
    return 0;
}

