#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <linux/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

struct alloc_event_t {
    __u32 pid;
    __u64 timestamp;
    __u64 size;
    __u64 addr;
};

static volatile bool exiting = false;

static void sig_handler(int sig) { exiting = true; }

static void handle_event(void *ctx, int cpu, void *data, __u32 data_sz) {
    struct alloc_event_t *e = data;
    printf("[PID %u] malloc %llu bytes at %p (time %llu ns)\n",
           e->pid, e->size, (void*)e->addr, e->timestamp);
}

static void handle_lost_events(void *ctx, int cpu, __u64 lost_cnt) {
    fprintf(stderr, "Lost %llu events on CPU %d\n", lost_cnt, cpu);
}

int main(int argc, char **argv) {
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    struct perf_buffer *pb = NULL;

    signal(SIGINT, sig_handler);

    // Load BPF object
    obj = bpf_object__open_file("sample_allocator.bpf.o", NULL);
    if (libbpf_get_error(obj)) return 1;
    if (bpf_object__load(obj)) return 1;

    // Find program and attach to running process
    prog = bpf_object__find_program_by_name(obj, "trace_malloc");
    if (!prog) return 1;

    // Replace 0 with PID of target process if you want specific process
    link = bpf_program__attach_usdt(prog, 0, "./sample_allocator",
                                    "memory_profiler", "malloc_entry", NULL);
    if (libbpf_get_error(link)) return 1;

    // Open perf buffer
    int map_fd = bpf_map__fd(bpf_object__find_map_by_name(obj, "malloc_events"));
    printf("map_fd=%d\n", map_fd);

    pb = perf_buffer__new(map_fd, 8, handle_event, handle_lost_events, NULL, NULL);
    
    if (libbpf_get_error(pb)) return 1;

    printf("Listening for malloc events... Press Ctrl+C to exit.\n");

    while (!exiting) {
        int err = perf_buffer__poll(pb, 100);
        if (err < 0 && err != -EINTR) {
            fprintf(stderr, "Error polling perf buffer: %d\n", err);
            break;
        }
    }

    perf_buffer__free(pb);
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}