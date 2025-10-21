/**
 * GOT Override / LD_PRELOAD Approach for Memory Profiling
 * 
 * This demonstrates the third approach: intercepting malloc/free
 * using LD_PRELOAD to override the GOT (Global Offset Table).
 * 
 * Compile: gcc -shared -fPIC -o preload_interceptor.so preload_interceptor.c -ldl
 * Run: LD_PRELOAD=./preload_interceptor.so ./sample_allocator
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

// Function pointers to real malloc/free
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;

// Statistics (in a real profiler, you'd send this to eBPF maps)
static __thread int in_hook = 0;  // Prevent recursion
static uint64_t total_allocations = 0;
static uint64_t total_frees = 0;
static uint64_t total_bytes_allocated = 0;
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

// Get current timestamp in microseconds
static uint64_t get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Initialize real function pointers
static void init_hooks() {
    if (real_malloc == NULL) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        real_free = dlsym(RTLD_NEXT, "free");
        
        if (real_malloc == NULL || real_free == NULL) {
            fprintf(stderr, "Error: failed to load real malloc/free\n");
            return;
        }
        
        fprintf(stderr, "[GOT Override] Hooks initialized\n");
    }
}

// Intercepted malloc
void* malloc(size_t size) {
    init_hooks();
    
    // Prevent recursion (e.g., fprintf might call malloc)
    if (in_hook) {
        return real_malloc(size);
    }
    
    in_hook = 1;
    
    // Call real malloc
    void* ptr = real_malloc(size);
    
    // Track allocation
    if (ptr != NULL) {
        pthread_mutex_lock(&stats_lock);
        total_allocations++;
        total_bytes_allocated += size;
        pthread_mutex_unlock(&stats_lock);
        
        // In a real profiler, you'd write to eBPF map here
        // For now, just log (sampling to avoid spam)
        if (total_allocations % 10 == 0) {
            uint64_t ts = get_timestamp_us();
            fprintf(stderr, "[%lu] MALLOC: size=%zu ptr=%p (total: %lu allocs, %lu bytes)\n",
                    ts, size, ptr, total_allocations, total_bytes_allocated);
        }
    }
    
    in_hook = 0;
    return ptr;
}

// Intercepted free
void free(void* ptr) {
    init_hooks();
    
    if (ptr == NULL) {
        return;
    }
    
    if (in_hook) {
        real_free(ptr);
        return;
    }
    
    in_hook = 1;
    
    // Track deallocation
    pthread_mutex_lock(&stats_lock);
    total_frees++;
    pthread_mutex_unlock(&stats_lock);
    
    // Log (sampling)
    if (total_frees % 10 == 0) {
        uint64_t ts = get_timestamp_us();
        fprintf(stderr, "[%lu] FREE: ptr=%p (total: %lu frees)\n",
                ts, ptr, total_frees);
    }
    
    // Call real free
    real_free(ptr);
    
    in_hook = 0;
}

// Optional: calloc, realloc interceptors
void* calloc(size_t nmemb, size_t size) {
    init_hooks();
    
    if (in_hook) {
        static void* (*real_calloc)(size_t, size_t) = NULL;
        if (real_calloc == NULL) {
            real_calloc = dlsym(RTLD_NEXT, "calloc");
        }
        return real_calloc(nmemb, size);
    }
    
    in_hook = 1;
    void* ptr = malloc(nmemb * size);
    if (ptr) {
        // Zero out memory
        char* p = ptr;
        for (size_t i = 0; i < nmemb * size; i++) {
            p[i] = 0;
        }
    }
    in_hook = 0;
    return ptr;
}

// Print final statistics on program exit
__attribute__((destructor))
void print_stats() {
    fprintf(stderr, "\n=== GOT Override Statistics ===\n");
    fprintf(stderr, "Total allocations: %lu\n", total_allocations);
    fprintf(stderr, "Total frees:       %lu\n", total_frees);
    fprintf(stderr, "Total bytes:       %lu\n", total_bytes_allocated);
    fprintf(stderr, "Leaked:            %lu allocations\n", 
            total_allocations - total_frees);
}

