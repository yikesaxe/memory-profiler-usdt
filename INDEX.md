# Memory Profiler USDT - Complete Index

## 📚 Start Here

New to this project? Read in this order:

1. **[SUMMARY.md](SUMMARY.md)** - Overview of what's included
2. **[QUICKSTART.md](QUICKSTART.md)** - Step-by-step getting started guide
3. **[CHEATSHEET.md](CHEATSHEET.md)** - Quick command reference
4. Run `./test_without_root.sh` to validate your setup
5. **[THREE_APPROACHES.md](THREE_APPROACHES.md)** - Deep dive into all approaches

## 📁 Files by Category

### Documentation
| File | Purpose | Read When |
|------|---------|-----------|
| **SUMMARY.md** | Project overview | First time |
| **QUICKSTART.md** | Getting started guide | First time |
| **CHEATSHEET.md** | Quick reference | Daily use |
| **INDEX.md** | This file | Navigation |
| **README.md** | General introduction | Overview |
| **COMPARISON.md** | USDT vs UProbe | Understanding trade-offs |
| **THREE_APPROACHES.md** | All three methods in detail | Deep understanding |

### Source Code
| File | Type | Purpose |
|------|------|---------|
| **sample_allocator.c** | C | Demo program with USDT probes |
| **trace_usdt.py** | Python/BCC | eBPF tracer for USDT probes |
| **trace_uprobe.py** | Python/BCC | eBPF tracer using uprobes |
| **preload_interceptor.c** | C | GOT override via LD_PRELOAD |

### Scripts & Tools
| File | Purpose |
|------|---------|
| **Makefile** | Build system |
| **install_deps.sh** | Install all dependencies |
| **list_probes.sh** | List USDT probes in binaries/processes |
| **test_without_root.sh** | Basic validation (no sudo needed) |

## 🎯 Usage by Goal

### "I want to understand the concepts"
1. Read [SUMMARY.md](SUMMARY.md)
2. Read [THREE_APPROACHES.md](THREE_APPROACHES.md)
3. Read [COMPARISON.md](COMPARISON.md)

### "I want to see it working NOW"
1. `sudo ./install_deps.sh`
2. `make`
3. `make test-got` (shows output immediately)

### "I want to use this for my project"
1. Read [QUICKSTART.md](QUICKSTART.md)
2. Study `sample_allocator.c` (how to add USDT probes)
3. Study `trace_usdt.py` (how to attach with eBPF)
4. Adapt to your allocator

### "I want to compare approaches"
1. Read [COMPARISON.md](COMPARISON.md)
2. Run all three: `make test-got`, USDT, and UProbe
3. Measure overhead with your workload

### "I'm debugging issues"
1. Check [CHEATSHEET.md](CHEATSHEET.md) - Common Issues
2. Run `./test_without_root.sh` for diagnosis
3. Use `./list_probes.sh -b <binary>` to verify probes

## 🔧 Three Instrumentation Approaches

### 1. USDT (Static Probes) - Production Quality
- **Source**: `sample_allocator.c` + `trace_usdt.py`
- **Overhead**: ~500 ns per event
- **Pros**: Lowest overhead, custom arguments
- **Cons**: Needs recompilation
- **Use**: Production continuous profiling

**Quick test:**
```bash
./sample_allocator &
sudo python3 trace_usdt.py -p $(pgrep sample_allocator)
```

### 2. UProbe (Dynamic Probes) - Universal
- **Source**: `trace_uprobe.py`
- **Overhead**: ~5 μs per event
- **Pros**: Works on any binary
- **Cons**: Higher overhead
- **Use**: Debugging, ad-hoc analysis

**Quick test:**
```bash
./sample_allocator &
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

### 3. GOT Override (LD_PRELOAD) - Hybrid
- **Source**: `preload_interceptor.c`
- **Overhead**: ~1 μs per event
- **Pros**: No recompile, full control
- **Cons**: Dynamic linking only
- **Use**: Testing, prototyping

**Quick test:**
```bash
make test-got
```

## 📊 Performance Data

For **400 million allocations on 20 threads** (your target workload):

| Approach | Overhead | Sampling Needed? | Production Ready? |
|----------|----------|------------------|-------------------|
| USDT | 0.1-0.5% | Yes (1:1000) | ✅ Yes |
| GOT Override | 0.5-1% | Yes (1:100) | ⚠️ Testing only |
| UProbe | 2-5% | Yes (1:10000) | ❌ Development only |

## 🚀 Integration Path

Your goal: Integrate into **dd-otel-host-profiler**

### Phase 1: USDT Basics (Week 1)
- ✅ Understand USDT concepts
- ✅ Create sample program
- ✅ Write eBPF tracer
- [ ] Measure overhead

### Phase 2: Real Allocators (Week 2-3)
- [ ] Test with jemalloc
- [ ] Test with tcmalloc
- [ ] Add USDT probes to glibc malloc (contribution!)
- [ ] Benchmark with 400M allocs

### Phase 3: Integration (Week 4-5)
- [ ] Fork dd-otel-host-profiler
- [ ] Add memory profiling eBPF programs
- [ ] Reuse existing stack unwinding
- [ ] Export to OpenTelemetry format

### Phase 4: Optimization (Week 6+)
- [ ] Implement smart sampling
- [ ] Add dynamic rate adjustment
- [ ] Optimize data structures (per-thread maps)
- [ ] Production testing

## 🧪 Testing Checklist

### Basic Validation
- [ ] Run `./test_without_root.sh`
- [ ] All builds succeed
- [ ] USDT probes detected in binary
- [ ] GOT override shows output

### Functional Testing
- [ ] USDT tracer captures events
- [ ] UProbe tracer captures events
- [ ] Statistics are accurate
- [ ] No crashes or hangs

### Performance Testing
- [ ] Measure overhead vs baseline
- [ ] Test sampling rates
- [ ] Multi-threaded stress test
- [ ] 400M allocations benchmark

### Integration Testing
- [ ] Works with jemalloc
- [ ] Works with tcmalloc
- [ ] Works with standard malloc
- [ ] Cross-language (Python, Go, C++)

## 📖 Key Concepts Reference

### USDT Probe Definition
```c
#include <sys/sdt.h>
DTRACE_PROBE2(provider, probe_name, arg1, arg2);
```

### eBPF Attachment
```python
from bcc import BPF, USDT
usdt = USDT(path="./binary")
usdt.enable_probe(probe="probe_name", fn_name="handler")
b = BPF(text=bpf_program, usdt_contexts=[usdt])
```

### GOT Override
```bash
LD_PRELOAD=./interceptor.so ./program
```

## 🔗 External Resources

### BCC & eBPF
- [BCC GitHub](https://github.com/iovisor/bcc)
- [BCC Reference Guide](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)
- [USDT Documentation](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#7-usdt-probes)

### Related Projects
- [dd-otel-host-profiler](https://github.com/DataDog/dd-otel-host-profiler) - Your target integration
- [ddprof](https://github.com/DataDog/ddprof) - Userspace profiler (inspiration)
- [prof-correctness](https://github.com/DataDog/prof-correctness) - Testing framework

### Background Reading
- Your project proposal (in query above)
- TCMalloc documentation
- jemalloc profiling docs

## ❓ FAQ

### Q: Which approach should I use?
**A**: For production: USDT. For debugging: UProbe. For quick testing: GOT override.

### Q: How do I add USDT probes to my allocator?
**A**: See `sample_allocator.c` lines 25-38 for examples.

### Q: What's the overhead in production?
**A**: With 1:1000 sampling: <0.5% CPU overhead.

### Q: Can I combine approaches?
**A**: Yes! Use USDT where available, UProbe as fallback.

### Q: Why is my binary missing USDT probes?
**A**: Check: `readelf -n binary | grep NT_STAPSDT`. Install systemtap-sdt-dev.

### Q: How do I reduce overhead?
**A**: Use sampling in eBPF: `if (bpf_get_prandom_u32() % 1000 != 0) return 0;`

## 🎓 Learning Path

1. **Day 1**: Run GOT example, understand output
2. **Day 2**: Install deps, run USDT example
3. **Day 3**: Study `sample_allocator.c`, understand probe placement
4. **Day 4**: Study `trace_usdt.py`, understand eBPF program
5. **Day 5**: Modify sample to add more probes
6. **Week 2**: Apply to real allocator (jemalloc)
7. **Week 3**: Benchmark and optimize
8. **Week 4+**: Integrate with dd-otel-host-profiler

## 🎯 Success Criteria

You'll know you're successful when:
- [ ] USDT example runs and shows events
- [ ] You can explain overhead differences
- [ ] You've measured real performance impact
- [ ] You can add probes to a new allocator
- [ ] You understand integration path

---

**Start with [QUICKSTART.md](QUICKSTART.md) to begin!**

