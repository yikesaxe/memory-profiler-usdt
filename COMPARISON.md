# USDT vs UProbe Comparison

## Overview

This project demonstrates two approaches to "no code" instrumentation for memory profiling:

### 1. USDT (User Statically-Defined Tracing)
- **File**: `trace_usdt.py`
- **Requires**: Application compiled with USDT probes (`sample_allocator.c`)
- **Pros**:
  - Lower overhead (probes compile to NOPs when not active)
  - Can pass custom arguments (size, address, etc.)
  - Stable ABI - probes won't break with library updates
  - More semantic information available
- **Cons**:
  - Requires recompilation with probe points
  - Only works with instrumented applications
  - Need to plan probe locations ahead of time

### 2. UProbe (Dynamic Tracing)
- **File**: `trace_uprobe.py`
- **Requires**: Nothing - works on any binary
- **Pros**:
  - Works on any existing binary (true "no code" change needed)
  - Can attach to system libraries (libc malloc/free)
  - No recompilation required
  - Can trace applications you don't have source for
- **Cons**:
  - Higher overhead (trap to kernel on each probe hit)
  - Limited access to function arguments
  - May break with library updates (symbol changes)
  - Harder to get full context (e.g., allocation size in free)

## Performance Comparison

### USDT Overhead
- Inactive probe: ~0-1 ns (NOP instruction)
- Active probe: ~500-1000 ns per event
- Scales well with high allocation rates

### UProbe Overhead
- Each uprobe hit: ~1-5 μs (context switch to kernel)
- ~5-10x higher overhead than USDT
- Can significantly impact high-throughput applications

## When to Use Each

### Use USDT when:
- You control the source code
- You want minimal overhead
- You need rich contextual information
- You want production-ready profiling
- Building a new allocator or library

### Use UProbe when:
- You can't modify the source
- Tracing third-party applications
- Quick ad-hoc debugging
- Prototyping before adding USDT probes
- Overhead is acceptable

## Real-World Examples

### USDT Examples
- **TCMalloc**: Has built-in USDT probes for allocation events
- **PostgreSQL**: Extensive USDT probes for query tracing
- **MySQL**: USDT probes for monitoring

### UProbe Examples
- **OpenTelemetry eBPF Profiler**: Uses uprobes for CPU profiling
- **bpftrace**: Uses uprobes for ad-hoc system analysis
- **Production debugging**: Attach to running processes without restart

## Testing Both Approaches

```bash
# Terminal 1: Run sample program
./sample_allocator

# Terminal 2: Trace with USDT (low overhead)
sudo python3 trace_usdt.py -p $(pgrep sample_allocator)

# OR Terminal 2: Trace with UProbe (higher overhead, but works on any program)
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

## For Your Project

Based on your project description:

1. **Research Phase**: Start with UProbes
   - Quickly test different allocators (glibc, jemalloc, tcmalloc)
   - Understand overhead characteristics
   - No need to modify applications

2. **Production Phase**: Move to USDT
   - Work with allocator maintainers to add USDT probes
   - Much lower overhead for continuous profiling
   - TCMalloc already has sampling - could add USDT to export samples

3. **Hybrid Approach**:
   - USDT for applications you control
   - UProbe fallback for third-party apps
   - eBPF program could support both simultaneously

## Next Steps

For your "no code" instrumentation assignment:

1. ✅ Run USDT example: `./sample_allocator` + `trace_usdt.py`
2. ✅ Run UProbe example: `trace_uprobe.py` on any program
3. 📊 Measure overhead: Compare performance impact
4. 🔬 Test on real allocators: glibc malloc, jemalloc
5. 📈 Benchmark: 400M allocations on 20 threads (your use case)

## GOT Override Approach (Third Option)

You also mentioned "GOT override approach" - this is another technique:

- **Global Offset Table (GOT) hooking**: Redirect function calls at runtime
- Used by tools like `LD_PRELOAD` malloc interceptors
- Lower overhead than UProbe, but requires process injection
- Could be combined with eBPF for data collection

Let me know if you want an example of GOT override too!

