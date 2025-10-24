#!/usr/bin/env python3
"""
USDT-based memory profiler using BCC
Traces memory allocations and deallocations via USDT probes
"""

from bcc import BPF, USDT
import argparse
import sys
import time

# eBPF program
bpf_text = """
#include <uapi/linux/ptrace.h>

struct alloc_event_t {
    u32 pid;
    u64 timestamp;
    u64 size;
    u64 addr;
};

struct free_event_t {
    u32 pid;
    u64 timestamp;
    u64 addr;
    u64 size;
};

BPF_PERF_OUTPUT(malloc_events);
BPF_PERF_OUTPUT(free_events);

// Track malloc events
int trace_malloc(struct pt_regs *ctx) {
    struct alloc_event_t event = {};
    
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.timestamp = bpf_ktime_get_ns();
    
    // Read USDT probe arguments
    bpf_usdt_readarg(1, ctx, &event.size);  // size argument
    bpf_usdt_readarg(2, ctx, &event.addr);  // pointer argument
    
    malloc_events.perf_submit(ctx, &event, sizeof(event));
    return 0;
}

// Track free events
int trace_free(struct pt_regs *ctx) {
    struct free_event_t event = {};
    
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.timestamp = bpf_ktime_get_ns();
    
    bpf_usdt_readarg(1, ctx, &event.addr);  // pointer argument
    bpf_usdt_readarg(2, ctx, &event.size);  // size argument
    
    free_events.perf_submit(ctx, &event, sizeof(event));
    return 0;
}

// Track startup/shutdown
int trace_startup(struct pt_regs *ctx) {
    bpf_trace_printk("Application startup detected\\n");
    return 0;
}

int trace_shutdown(struct pt_regs *ctx) {
    bpf_trace_printk("Application shutdown detected\\n");
    return 0;
}
"""

# Statistics tracking
stats = {
    'total_allocs': 0,
    'total_frees': 0,
    'total_bytes_allocated': 0,
    'total_bytes_freed': 0,
    'active_allocations': {},  # addr -> size
}

# first_event_ts = None
all_events = []

def get_elapsed_ns(event_ns):
    global first_event_ts
    if first_event_ts is None:
        first_event_ts = event_ns

    return (event_ns - first_event_ts) / 1e9

def print_header():
    print("%-12s %-8s %-18s %-12s %-18s" % 
          ("TIME(s)", "PID", "EVENT", "SIZE", "ADDRESS"))
    print("-" * 80)

def handle_malloc_event(cpu, data, size):
    event = b["malloc_events"].event(data)
    all_events.append(("M", event))
    
def handle_free_event(cpu, data, size):
    event = b["free_events"].event(data)
    all_events.append(("F", event))
    
def print_statistics():
    print("\n" + "=" * 80)
    print("STATISTICS")
    print("=" * 80)
    print(f"Total allocations:     {stats['total_allocs']}")
    print(f"Total frees:           {stats['total_frees']}")
    print(f"Bytes allocated:       {stats['total_bytes_allocated']:,}")
    print(f"Bytes freed:           {stats['total_bytes_freed']:,}")
    print(f"Active allocations:    {len(stats['active_allocations'])}")
    
    if stats['active_allocations']:
        leaked_bytes = sum(stats['active_allocations'].values())
        print(f"Potential leak:        {leaked_bytes:,} bytes")
        print("\nActive allocations:")
        for addr, size in list(stats['active_allocations'].items())[:10]:
            print(f"  0x{addr:x}: {size} bytes")
        if len(stats['active_allocations']) > 10:
            print(f"  ... and {len(stats['active_allocations']) - 10} more")

def print_all_events():
    if not all_events:
        print("No malloc or free events tracked.")
        return

    # sort events by timestamp
    all_events.sort(key = lambda e: e[1].timestamp)
    first_event_ts = all_events[0][1].timestamp

    print_header()

    # print events
    for name, ev in all_events:
        elapsed = (ev.timestamp - first_event_ts) / 1e9
        if name == "M":
            stats['total_allocs'] += 1
            stats['total_bytes_allocated'] += ev.size
            stats['active_allocations'][ev.addr] = ev.size
            print("%-12.6f %-8d %-18s %-12d 0x%-16x" %
                      (elapsed, ev.pid, "MALLOC", ev.size, ev.addr))
        else:
            stats['total_frees'] += 1
            stats['total_bytes_freed'] += ev.size
            
            if ev.addr in stats['active_allocations']:
                del stats['active_allocations'][ev.addr]

            print("%-12.6f %-8d %-18s %-12d 0x%-16x" %
                      (elapsed, ev.pid, "FREE", ev.size, ev.addr))

    print_statistics()

import os

def is_process_alive(pid):
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Trace memory allocations using USDT probes")
    parser.add_argument("-p", "--pid", type=int, 
                       help="PID of the process to trace")
    parser.add_argument("-c", "--command", 
                       help="Command to run and trace")
    args = parser.parse_args()
    
    if not args.pid and not args.command:
        print("Error: Must specify either --pid or --command")
        print("\nExample: sudo python3 trace_usdt.py -p <PID>")
        print("     or: sudo python3 trace_usdt.py -c './sample_allocator'")
        sys.exit(1)
    
    # Setup USDT
    usdt = None
    if args.pid:
        usdt = USDT(pid=args.pid)
    else:
        usdt = USDT(path="./sample_allocator")
    
    # Enable USDT probes
    try:
        usdt.enable_probe(probe="malloc_entry", fn_name="trace_malloc")
        usdt.enable_probe(probe="free_entry", fn_name="trace_free")
        usdt.enable_probe(probe="startup", fn_name="trace_startup")
        usdt.enable_probe(probe="shutdown", fn_name="trace_shutdown")
    except Exception as e:
        print(f"Error enabling probes: {e}")
        print("\nMake sure the binary is compiled with USDT support")
        print("Run: readelf -n ./sample_allocator | grep NT_STAPSDT")
        sys.exit(1)
    
    # Load BPF program
    global b
    b = BPF(text=bpf_text, usdt_contexts=[usdt])
    
    # Attach callbacks
    b["malloc_events"].open_perf_buffer(handle_malloc_event)
    b["free_events"].open_perf_buffer(handle_free_event)
    
    print("=== USDT Memory Profiler ===")
    if args.pid:
        print(f"Tracing PID {args.pid}...")
    else:
        print(f"Tracing command: {args.command}")
    print("Hit Ctrl-C to end.\n")
    
    
    # Poll for events
    try:
        while True:
            b.perf_buffer_poll(timeout=100)
            if not is_process_alive(args.pid):
                print("\nTraced process has exited.");
                break;
    except KeyboardInterrupt:
        print("\n\nDetaching...")
    
    print_all_events()
    
    #print_statistics()

if __name__ == "__main__":
    main()

