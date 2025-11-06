#ifndef __TARGET_ARCH_arm64
#define __TARGET_ARCH_arm64 1
#endif
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/usdt.bpf.h>

struct alloc_event_t {
    u32 pid;
    u64 timestamp;
    u64 size;
    u64 addr;
};

struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(u32));
    __uint(value_size, sizeof(u32));
    __uint(max_entries, 1024);
} malloc_events SEC(".maps");


// Track malloc events

SEC("usdt/./sample_allocator:memory_profiler:malloc_entry")
int BPF_USDT(trace_malloc, u64 size, u64 ptr) {
    struct alloc_event_t event = {};
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.timestamp = bpf_ktime_get_ns();
    event.size = size;
    event.addr = ptr;

    bpf_perf_event_output(ctx, &malloc_events, BPF_F_CURRENT_CPU, &event, sizeof(event));
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
