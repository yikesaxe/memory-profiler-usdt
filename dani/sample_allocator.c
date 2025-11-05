#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sdt.h>  // For USDT probes
#include <string.h>
#include <time.h>


#define ALLOC_COUNT 100
#define MAX_ALLOC_SIZE 1024

int rate = 1;
int counter = 0; // prob not thread safe to do it like this

// sampling path
void sample_malloc(size_t size, void* ptr) {
    printf("got to sampling path\n");
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
    
    for (int i = 0; i < 50; i++) {
        size_t size = 16 + (i % 64);
        void* ptr = tracked_malloc(size);
        
        if (ptr) {
            memset(ptr, 0, size);  // Touch the memory
            usleep(10000);  // 10ms
            free(vptr);
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
            free(ptrs[i]);
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
            usleep(20000);  // 20ms
            free(ptr);
        }
    }
}
int main(int argc, char **argv)
{
    printf("\n--- Starting allocation patterns ---\n\n");
        
        allocation_pattern_1();
        sleep(1);
        
        allocation_pattern_2();
        sleep(1);
        
        allocation_pattern_3();
        sleep(1);
        
    printf("\n--- Finished allocation patterns ---\n");
    /*
    // now set up bpf
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;

    // Load and verify BPF application
    fprintf(stderr, "Loading BPF code in memory\n");
    obj = bpf_object__open_file("sample_allocator.bpf.o", NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "ERROR: opening BPF object file failed\n");
        return 1;
    }

    fprintf(stderr, "Loading and verifying the code in the kernel\n");
    if (bpf_object__load(obj)) {
        fprintf(stderr, "ERROR: loading BPF object file failed\n");
        return 1;
    }

    // Attach BPF program
    fprintf(stderr, "Attaching BPF program to probe\n");
    prog = bpf_object__find_program_by_name(obj, "trace_malloc");
    link = bpf_program__attach_usdt(
        prog,
        0,
        "./sample_allocator",
        "memory_profiler",
        "malloc_entry"
    );
    if (libbpf_get_error(link)) {
           fprintf(stderr, "ERROR: Attaching BPF program to USDT probe failed\n");
            return 1;
    }

    

cleanup:
    bpf_link__destroy(link);
    bpf_object__close(obj);

    return 0;
    */
}