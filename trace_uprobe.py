#!/usr/bin/env python3
"""
UProbe-based memory profiler using BCC (for comparison with USDT)
Traces malloc/free from libc using dynamic uprobes
"""

from bcc import BPF
import argparse
import time

# eBPF program
bpf_text = """
#include <uapi/linux/ptrace.h>

struct alloc_event_t {
    u32 pid;
    u64 timestamp;
    u64 size;
    u64 addr;
    char comm[16];
};

struct free_event_t {
    u32 pid;
    u64 timestamp;
    u64 addr;
    char comm[16];
};

BPF_PERF_OUTPUT(malloc_events);
BPF_PERF_OUTPUT(free_events);

// UProbe on malloc entry
int uprobe_malloc(struct pt_regs *ctx, size_t size) {
    struct alloc_event_t event = {};
    
    u64 pid_tgid = bpf_get_current_pid_tgid();
    event.pid = pid_tgid >> 32;
    event.timestamp = bpf_ktime_get_ns();
    event.size = size;
    bpf_get_current_comm(&event.comm, sizeof(event.comm));
    
    malloc_events.perf_submit(ctx, &event, sizeof(event));
    return 0;
}

// URetprobe on malloc return
int uretprobe_malloc(struct pt_regs *ctx) {
    struct alloc_event_t event = {};
    
    u64 pid_tgid = bpf_get_current_pid_tgid();
    event.pid = pid_tgid >> 32;
    event.timestamp = bpf_ktime_get_ns();
    event.addr = PT_REGS_RC(ctx);  // Return value (pointer)
    bpf_get_current_comm(&event.comm, sizeof(event.comm));
    
    // We don't have the size here in uretprobe, would need to store it
    // This is a limitation of uprobe vs USDT
    
    return 0;
}

// UProbe on free
int uprobe_free(struct pt_regs *ctx, void *ptr) {
    if (ptr == NULL)
        return 0;
        
    struct free_event_t event = {};
    
    u64 pid_tgid = bpf_get_current_pid_tgid();
    event.pid = pid_tgid >> 32;
    event.timestamp = bpf_ktime_get_ns();
    event.addr = (u64)ptr;
    bpf_get_current_comm(&event.comm, sizeof(event.comm));
    
    free_events.perf_submit(ctx, &event, sizeof(event));
    return 0;
}
"""

# Statistics tracking
stats = {
    'total_allocs': 0,
    'total_frees': 0,
}

start_time = time.time()

def print_header():
    print("%-12s %-8s %-16s %-18s %-12s %-18s" % 
          ("TIME(s)", "PID", "COMM", "EVENT", "SIZE", "ADDRESS"))
    print("-" * 90)

def handle_malloc_event(cpu, data, size):
    event = b["malloc_events"].event(data)
    elapsed = (event.timestamp / 1e9) - start_time
    
    stats['total_allocs'] += 1
    
    print("%-12.6f %-8d %-16s %-18s %-12d 0x%-16x" % 
          (elapsed, event.pid, event.comm.decode('utf-8', 'replace'), 
           "MALLOC", event.size, event.addr))

def handle_free_event(cpu, data, size):
    event = b["free_events"].event(data)
    elapsed = (event.timestamp / 1e9) - start_time
    
    stats['total_frees'] += 1
    
    print("%-12.6f %-8d %-16s %-18s %-12s 0x%-16x" % 
          (elapsed, event.pid, event.comm.decode('utf-8', 'replace'),
           "FREE", "-", event.addr))

def print_statistics():
    print("\n" + "=" * 90)
    print("STATISTICS")
    print("=" * 90)
    print(f"Total malloc calls:    {stats['total_allocs']}")
    print(f"Total free calls:      {stats['total_frees']}")

def main():
    parser = argparse.ArgumentParser(
        description="Trace malloc/free using UProbes (dynamic tracing)")
    parser.add_argument("-p", "--pid", type=int, 
                       help="PID of the process to trace")
    args = parser.parse_args()
    
    # Load BPF program
    global b
    b = BPF(text=bpf_text)
    
    # Attach uprobes to libc malloc/free
    # These are DYNAMIC probes - they work on any program using libc
    b.attach_uprobe(name="c", sym="malloc", fn_name="uprobe_malloc", pid=args.pid or -1)
    b.attach_uprobe(name="c", sym="free", fn_name="uprobe_free", pid=args.pid or -1)
    
    print("=== UProbe Memory Profiler ===")
    print("Tracing malloc/free from libc (dynamic tracing)")
    if args.pid:
        print(f"Filtering to PID {args.pid}")
    else:
        print("Tracing ALL processes (this can be noisy!)")
    print("Hit Ctrl-C to end.\n")
    
    print_header()
    
    # Attach callbacks
    b["malloc_events"].open_perf_buffer(handle_malloc_event)
    b["free_events"].open_perf_buffer(handle_free_event)
    
    # Poll for events
    try:
        while True:
            b.perf_buffer_poll()
    except KeyboardInterrupt:
        print("\n\nDetaching...")
    
    print_statistics()

if __name__ == "__main__":
    main()

