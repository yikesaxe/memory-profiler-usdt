#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sdt.h> 
#include <string.h>
#include <time.h>


#define ALLOC_COUNT 100
#define MAX_ALLOC_SIZE 1024

int rate = 10;
int counter = 0; // prob not thread safe to do it like this

// sampling path
void sample_malloc(size_t size, void* ptr) {
    DTRACE_PROBE2(memory_profiler, malloc_entry, size, ptr);
}

// wrapper for malloc
void* tracked_malloc(size_t size) {
    void* ptr = malloc(size);
    if (++counter == rate) {
        sample_malloc(size, ptr);
        counter = 0;
    }
    return ptr;
}

// Simulate some allocation patterns
void allocation_pattern_1() {
    printf("Pattern 1: Small frequent allocations\n");
    
    for (int i = 0; i < 5000000; i++) {
        size_t size = 16;
        void* ptr = tracked_malloc(size);
        
        if (ptr) {
            memset(ptr, 0, size);  // Touch the memory
            free(ptr);
        }
    }
}

void allocation_pattern_2() {
    printf("Pattern 2: Large allocations\n");

    size_t size = 1024;
    for (int i = 0; i < 5000000; i++) {
        void *ptr = tracked_malloc(size);
        
        if (ptr) {
            memset(ptr, 0xAB, size);
            free(ptr);
        }
    }
    

}
int main(int argc, char **argv)
{
    struct timespec ts_start, ts_end;
    sleep(10);
    printf("\n--- Starting allocation patterns ---\n\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    
    allocation_pattern_1();
    allocation_pattern_2();

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double elapsed_ms = (ts_end.tv_sec - ts_start.tv_sec) * 1000.0 + 
                        (ts_end.tv_nsec - ts_start.tv_nsec) / 1e6;

    printf("Elapsed time: %f ms\n", elapsed_ms);
    printf("\n--- Finished allocation patterns ---\n");
}