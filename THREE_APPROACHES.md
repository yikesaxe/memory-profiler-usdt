# Three "No Code" Instrumentation Approaches

This project demonstrates three different approaches to instrumenting memory allocations without modifying application code (or with minimal changes).

## Overview Table

| Approach | Overhead | Flexibility | Setup Complexity | Use Case |
|----------|----------|-------------|------------------|----------|
| **USDT** | Lowest (~500ns) | High (custom args) | Medium (needs recompile) | Production profiling |
| **UProbe** | Highest (~5μs) | Medium (function args only) | Low (works on any binary) | Ad-hoc debugging |
| **GOT Override** | Medium (~1μs) | High (full control) | Low (LD_PRELOAD) | Testing & prototyping |

## 1. USDT (User Statically-Defined Tracing)

### What It Is
Static probe points compiled into the binary. When not traced, they compile to NOP instructions (nearly zero overhead).

### How to Use

```bash
# Build with USDT probes
make sample_allocator

# Trace with eBPF
sudo python3 trace_usdt.py -c './sample_allocator'
```

### Code Example

```c
#include <sys/sdt.h>

void* my_malloc(size_t size) {
    void* ptr = malloc(size);
    
    // Fire USDT probe
    DTRACE_PROBE2(memory_profiler, malloc_entry, size, ptr);
    
    return ptr;
}
```

### eBPF Attachment

```python
usdt = USDT(path="./sample_allocator")
usdt.enable_probe(probe="malloc_entry", fn_name="trace_malloc")
b = BPF(text=bpf_text, usdt_contexts=[usdt])
```

### Pros
- ✅ Lowest overhead (NOP when inactive)
- ✅ Can pass custom arguments (size, address, metadata)
- ✅ Stable across library updates
- ✅ Production-ready

### Cons
- ❌ Requires recompilation
- ❌ Need to plan probe locations
- ❌ Only works on instrumented binaries

### Real-World Examples
- TCMalloc allocator
- PostgreSQL, MySQL
- systemd, Node.js

## 2. UProbe (Dynamic Tracing)

### What It Is
Dynamic instrumentation that attaches to any function at runtime by inserting breakpoints.

### How to Use

```bash
# Trace ANY program (no recompilation needed)
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

### eBPF Attachment

```python
b = BPF(text=bpf_text)
b.attach_uprobe(name="c", sym="malloc", fn_name="uprobe_malloc")
b.attach_uprobe(name="c", sym="free", fn_name="uprobe_free")
```

### Pros
- ✅ Works on ANY binary (true "no code" change)
- ✅ Can attach to system libraries (libc, jemalloc, etc.)
- ✅ No recompilation needed
- ✅ Great for debugging unknown binaries

### Cons
- ❌ Higher overhead (kernel trap on each hit)
- ❌ Limited argument access
- ❌ Can break with library symbol changes
- ❌ Hard to correlate malloc/free (size lost at free)

### Real-World Examples
- bpftrace for ad-hoc analysis
- OpenTelemetry eBPF profiler (CPU profiling)
- Production debugging tools

## 3. GOT Override (LD_PRELOAD)

### What It Is
Intercept function calls by preloading a shared library that overrides malloc/free in the Global Offset Table.

### How to Use

```bash
# Build interceptor
make preload_interceptor.so

# Run with interception
LD_PRELOAD=./preload_interceptor.so ./sample_allocator
```

### Code Example

```c
// In preload_interceptor.c
void* malloc(size_t size) {
    // Get real malloc
    static void* (*real_malloc)(size_t) = NULL;
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    }
    
    // Track allocation
    void* ptr = real_malloc(size);
    log_allocation(size, ptr);
    
    return ptr;
}
```

### Pros
- ✅ No recompilation needed
- ✅ Full access to arguments and return values
- ✅ Lower overhead than UProbe
- ✅ Can integrate with eBPF maps for data export

### Cons
- ❌ Only works with dynamically linked libraries
- ❌ Can't intercept statically linked malloc
- ❌ Doesn't work with direct syscalls
- ❌ LD_PRELOAD can be bypassed

### Real-World Examples
- jemalloc profiling (jeprof)
- Google's tcmalloc heap profiler
- Memory sanitizers (ASan)

## Performance Comparison

### Test Setup
400 million allocations on 20 threads (your target workload)

### Expected Overhead

| Approach | Per-Event Overhead | Total Overhead (400M events) | CPU Usage |
|----------|-------------------|------------------------------|-----------|
| USDT | 500 ns | ~200 seconds | ~3.3 seconds/min |
| UProbe | 5 μs | ~2000 seconds | ~33 seconds/min |
| GOT Override | 1 μs | ~400 seconds | ~6.6 seconds/min |

*Note: These are estimates. Actual overhead depends on:*
- Sampling rate
- Data collected (stack traces add overhead)
- eBPF program complexity

## Sampling Strategies

To keep overhead low, use sampling:

### 1. USDT with Sampling
```c
static int sample_counter = 0;
if (++sample_counter % 1024 == 0) {  // Sample 1 in 1024
    DTRACE_PROBE2(memory_profiler, malloc_entry, size, ptr);
}
```

### 2. eBPF-Side Sampling
```c
// In eBPF program
u64 rand = bpf_get_prandom_u32();
if (rand % 1024 != 0) {
    return 0;  // Skip
}
```

### 3. GOT with Sampling
```c
void* malloc(size_t size) {
    void* ptr = real_malloc(size);
    
    if (should_sample()) {  // Smart sampling
        log_allocation(size, ptr);
    }
    
    return ptr;
}
```

## Hybrid Approach (Recommended)

For your project, consider a hybrid strategy:

```
┌─────────────────┐
│  Application    │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
┌───▼───┐ ┌──▼──────┐
│ USDT  │ │ UProbe  │  ← Instrumentation layer
│(custom│ │(fallback│
│ code) │ │ generic)│
└───┬───┘ └──┬──────┘
    │         │
    └────┬────┘
         │
┌────────▼─────────┐
│  eBPF Program    │  ← Data collection
│  (kernel space)  │
└────────┬─────────┘
         │
┌────────▼─────────┐
│ Userspace Agent  │  ← Data processing
│ (dd-otel-profiler)│
└──────────────────┘
```

### Strategy:
1. **For allocators you control** (or can modify):
   - Add USDT probes (lowest overhead)
   - Examples: Custom allocators, TCMalloc with patches

2. **For standard allocators**:
   - Use UProbe on libc malloc/free
   - Accept higher overhead, use aggressive sampling

3. **For testing/prototyping**:
   - Use GOT override for quick iteration
   - Easy to modify without recompiling everything

## Integration with dd-otel-host-profiler

Your goal is to integrate memory profiling into the OpenTelemetry eBPF profiler.

### Architecture

```c
// In eBPF program (memory.bpf.c)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, u64);    // allocation address
    __type(value, struct alloc_info);
} allocations SEC(".maps");

// USDT probe handler
SEC("usdt/memory_profiler:malloc_entry")
int usdt_malloc(struct pt_regs *ctx) {
    u64 size, addr;
    bpf_usdt_readarg(1, ctx, &size);
    bpf_usdt_readarg(2, ctx, &addr);
    
    // Capture stack trace (for attribution)
    struct alloc_info info = {};
    info.size = size;
    info.timestamp = bpf_ktime_get_ns();
    bpf_get_stack(ctx, &info.stack, sizeof(info.stack), 
                  BPF_F_USER_STACK);
    
    bpf_map_update_elem(&allocations, &addr, &info, BPF_ANY);
    return 0;
}

// UProbe fallback for uninstrumented allocators
SEC("uprobe/malloc")
int uprobe_malloc(struct pt_regs *ctx) {
    // Similar logic...
}
```

## Testing Each Approach

### 1. Test USDT
```bash
make sample_allocator
sudo python3 trace_usdt.py -c './sample_allocator'
```

### 2. Test UProbe
```bash
./sample_allocator &
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

### 3. Test GOT Override
```bash
make test-got
# Output appears directly in stdout
```

## Next Steps for Your Project

Based on your assignment to focus on "no code" instrumentation:

### Week 1: USDT Basics
- [x] Build sample program with USDT probes
- [ ] Write eBPF program to capture events
- [ ] Measure overhead vs uninstrumented

### Week 2: Real Allocators
- [ ] Test with jemalloc (has some USDT support)
- [ ] Test with tcmalloc (has sampling built-in)
- [ ] Measure overhead with 400M allocs/20 threads

### Week 3: Integration
- [ ] Fork dd-otel-host-profiler
- [ ] Add memory profiling eBPF programs
- [ ] Integrate with existing unwinding

### Week 4: Comparison & Benchmarking
- [ ] Compare USDT vs UProbe overhead
- [ ] Test sampling strategies
- [ ] Write correctness tests

See `QUICKSTART.md` for immediate next steps!

