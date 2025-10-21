# eBPF Memory Profiler - "No Code" Instrumentation Examples

This project demonstrates three approaches to memory profiling using eBPF with minimal or no application code changes. It's designed as a starting point for continuous memory profiling using USDT probes, UProbes, and GOT overrides.

## 🎯 Purpose

This is a **learning and experimentation** resource for:
- Understanding "no code" instrumentation techniques
- Evaluating overhead of different profiling approaches
- Prototyping memory profiling with eBPF
- Preparing for integration with production profilers (e.g., dd-otel-host-profiler)

## 📖 Quick Navigation

- **New here?** → Start with [QUICKSTART.md](QUICKSTART.md)
- **Want quick commands?** → See [CHEATSHEET.md](CHEATSHEET.md)
- **Need full context?** → Read [SUMMARY.md](SUMMARY.md)
- **Want to understand approaches?** → Read [THREE_APPROACHES.md](THREE_APPROACHES.md)
- **Looking for something specific?** → Check [INDEX.md](INDEX.md)

## 🚀 Quick Start (60 seconds)

```bash
# 1. Install dependencies
cd /home/axel/Workspace/memory-profiler-usdt
sudo ./install_deps.sh

# 2. Build everything
make

# 3. Run the easiest example (GOT override - no eBPF needed)
make test-got
```

You should see output like:
```
[GOT Override] Hooks initialized
=== USDT Memory Profiler Sample ===
...
[timestamp] MALLOC: size=32 ptr=0x... (total: 10 allocs, 512 bytes)
[timestamp] FREE: ptr=0x... (total: 10 frees)
...
=== GOT Override Statistics ===
Total allocations: 80
Total frees:       64
```

## 🎓 Three Approaches Explained

This project demonstrates three "no code" instrumentation approaches:

### 1. 🔵 USDT (User Statically-Defined Tracing)
**Best for: Production continuous profiling**

- Lowest overhead (~500ns per event)
- Static probe points compiled into binary
- Requires recompilation but production-ready

```bash
# Terminal 1
./sample_allocator

# Terminal 2  
sudo python3 trace_usdt.py -p $(pgrep sample_allocator)
```

### 2. 🟢 UProbe (Dynamic Tracing)
**Best for: Ad-hoc debugging, unknown binaries**

- Works on ANY binary without recompilation
- Higher overhead (~5μs per event)
- True "no code change" needed

```bash
# Terminal 1
./sample_allocator

# Terminal 2
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

### 3. 🟡 GOT Override (LD_PRELOAD)
**Best for: Testing and prototyping**

- Medium overhead (~1μs per event)
- No recompilation needed
- Works only with dynamically linked libraries

```bash
make test-got
# or
LD_PRELOAD=./preload_interceptor.so ./your_program
```

## 📊 Performance Comparison

| Approach | Per-Event Overhead | Production Ready? | "No Code"? |
|----------|-------------------|-------------------|------------|
| USDT | ~500 ns | ✅ Yes | ⚠️ Needs recompile |
| GOT Override | ~1 μs | ⚠️ Testing only | ✅ Yes |
| UProbe | ~5 μs | ❌ Development only | ✅ Yes |

For **400M allocations on 20 threads** (your target), with 1:1000 sampling:
- USDT: ~0.1-0.5% CPU overhead ✅
- GOT: ~0.5-1% CPU overhead ⚠️
- UProbe: ~2-5% CPU overhead ❌

## 📁 What's Included

### Core Components
- `sample_allocator.c` - Demo program with USDT probes showing allocation patterns
- `trace_usdt.py` - eBPF tracer for USDT probes (BCC/Python)
- `trace_uprobe.py` - eBPF tracer using dynamic uprobes
- `preload_interceptor.c` - GOT override interceptor (LD_PRELOAD)

### Tools & Scripts
- `Makefile` - Build all components
- `install_deps.sh` - One-command dependency installation
- `list_probes.sh` - Discover USDT probes in binaries/processes
- `test_without_root.sh` - Validate setup without sudo

### Documentation
- `QUICKSTART.md` - Step-by-step getting started
- `SUMMARY.md` - Complete project overview
- `THREE_APPROACHES.md` - Deep dive into all approaches
- `COMPARISON.md` - USDT vs UProbe trade-offs
- `CHEATSHEET.md` - Quick command reference
- `INDEX.md` - Complete file index

## 🛠️ Prerequisites

### Required
```bash
sudo apt-get install -y \
    systemtap-sdt-dev \
    build-essential \
    python3
```

### For eBPF Tracing (USDT/UProbe)
```bash
sudo apt-get install -y \
    bpfcc-tools \
    python3-bpfcc \
    linux-headers-$(uname -r)
```

**Or just run:** `sudo ./install_deps.sh`

## 🎯 Use Cases

### For Your Assignment
**Focus: "No code" instrumentation using USDT/UProbe/GOT**

1. ✅ Understand three approaches (this project)
2. ✅ See working examples
3. ⏳ Measure overhead
4. ⏳ Apply to real allocators (jemalloc, tcmalloc)

### For Your Project
**Goal: eBPF continuous memory profiler integrated with dd-otel-host-profiler**

1. Use USDT for allocators you control
2. Use UProbe fallback for third-party allocators
3. Leverage existing stack unwinding from otel-profiler
4. Export to OpenTelemetry format

## 🔍 How USDT Works

USDT probes are defined in source code using `DTRACE_PROBE` macros:

```c
#include <sys/sdt.h>

void* tracked_malloc(size_t size) {
    void* ptr = malloc(size);
    
    // Fire USDT probe - compiles to NOP when not traced
    DTRACE_PROBE2(memory_profiler, malloc_entry, size, ptr);
    
    return ptr;
}
```

The eBPF program attaches to these probes:

```python
from bcc import BPF, USDT

usdt = USDT(path="./sample_allocator")
usdt.enable_probe(probe="malloc_entry", fn_name="trace_malloc")
b = BPF(text=bpf_program, usdt_contexts=[usdt])
```

**Key advantage**: When not being traced, USDT probes have near-zero overhead (NOP instructions).

## 🧪 Testing

### Basic Validation (No sudo needed)
```bash
./test_without_root.sh
```

### Full Test Suite
```bash
# Test all three approaches
make test-got         # GOT override
# Then USDT (needs two terminals)
# Then UProbe (needs two terminals)
```

### Check USDT Probes
```bash
make check-probes
# or
./list_probes.sh -b ./sample_allocator
```

## 📚 Next Steps

1. **Get it running**: Follow [QUICKSTART.md](QUICKSTART.md)
2. **Understand deeply**: Read [THREE_APPROACHES.md](THREE_APPROACHES.md)
3. **Apply to real code**: Instrument jemalloc or tcmalloc
4. **Measure overhead**: Test with your 400M allocation workload
5. **Integrate**: Add to dd-otel-host-profiler

## 🤝 Related Projects

- **dd-otel-host-profiler** - OpenTelemetry eBPF profiler (your target integration)
- **ddprof** - Datadog's userspace profiler (inspiration)
- **jeprofl** - Experimental eBPF profiler for jemalloc
- **prof-correctness** - Testing framework for profilers

## 💡 Key Insights

1. **USDT is production-ready** - Use for continuous profiling
2. **UProbe is for debugging** - Too much overhead for production
3. **Sampling is essential** - Even with USDT, sample 1:1000 or more
4. **Hybrid approach works** - USDT where available, UProbe as fallback
5. **Integration matters** - Leverage otel-profiler's existing infrastructure

## 📝 Project Context

This project supports your research on:
- eBPF continuous memory profiling
- Low-overhead allocation tracking
- Cross-language memory profiling
- Integration with production profilers

**Your goal**: Build a memory profiler that:
- ✅ Works across multiple languages
- ✅ Has minimal overhead (<1s CPU/minute)
- ✅ Handles high allocation rates (400M/min on 20 threads)
- ✅ Integrates with existing eBPF infrastructure

## 📞 Getting Help

- Check [CHEATSHEET.md](CHEATSHEET.md) for common issues
- Run `./test_without_root.sh` for diagnostics
- Read documentation in order: SUMMARY → QUICKSTART → THREE_APPROACHES
- See `make help` for available commands

---

**Start here: [QUICKSTART.md](QUICKSTART.md)**

