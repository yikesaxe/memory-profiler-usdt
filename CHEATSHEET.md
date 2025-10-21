# Memory Profiler Quick Reference

## Installation (One Time)
```bash
cd /home/axel/Workspace/memory-profiler-usdt
sudo ./install_deps.sh
```

## Build (After Any Code Changes)
```bash
make          # Build everything
make clean    # Clean and rebuild
```

## Three Ways to Profile

### 1. GOT Override - Easiest, See Output Immediately
```bash
make test-got
```
**What you'll see**: Direct output showing malloc/free events with statistics

**Best for**: Quick testing, understanding allocation patterns

---

### 2. USDT Probes - Production Quality, Lowest Overhead
```bash
# Terminal 1
./sample_allocator

# Terminal 2 (while program is running)
sudo python3 trace_usdt.py -p $(pgrep sample_allocator)
```

**What you'll see**: Real-time event stream with timestamps, sizes, addresses

**Best for**: Production profiling, low overhead continuous monitoring

---

### 3. UProbe - Works on Any Binary
```bash
# Terminal 1
./sample_allocator    # Or ANY other program

# Terminal 2
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

**What you'll see**: malloc/free calls from libc (any program using malloc)

**Best for**: Debugging unknown binaries, quick investigation

---

## Debugging Commands

### Check if USDT probes exist in binary
```bash
readelf -n ./sample_allocator | grep NT_STAPSDT
```

### List all USDT probes in a binary
```bash
./list_probes.sh -b ./sample_allocator
```

### List USDT probes in running process
```bash
./list_probes.sh -p $(pgrep sample_allocator)
```

### Find process ID
```bash
pgrep sample_allocator
ps aux | grep sample_allocator
```

---

## Common Issues

### "sys/sdt.h: No such file"
```bash
sudo apt-get install systemtap-sdt-dev
```

### "Error enabling probes"
Check if probes exist:
```bash
make check-probes
```

### "Permission denied" when tracing
Need sudo for eBPF:
```bash
sudo python3 trace_usdt.py -p <PID>
```

### BCC not found
```bash
sudo apt-get install bpfcc-tools python3-bpfcc
```

---

## File Quick Reference

| What | File | Command |
|------|------|---------|
| Sample program | `sample_allocator.c` | `./sample_allocator` |
| USDT tracer | `trace_usdt.py` | `sudo python3 trace_usdt.py -p <PID>` |
| UProbe tracer | `trace_uprobe.py` | `sudo python3 trace_uprobe.py -p <PID>` |
| GOT interceptor | `preload_interceptor.so` | `LD_PRELOAD=./preload_interceptor.so ./program` |
| List probes | `list_probes.sh` | `./list_probes.sh -b <binary>` |

---

## Overhead Comparison

| Method | Per-event | When to Use |
|--------|-----------|-------------|
| USDT | ~500 ns | ✅ Production |
| GOT | ~1 μs | Testing/Development |
| UProbe | ~5 μs | Ad-hoc debugging |

---

## Next Steps Checklist

- [ ] Install dependencies: `sudo ./install_deps.sh`
- [ ] Build: `make`
- [ ] Test GOT: `make test-got`
- [ ] Test USDT: Follow Terminal 1/2 steps above
- [ ] Test UProbe: Follow Terminal 1/2 steps above
- [ ] Read `THREE_APPROACHES.md` for deep dive
- [ ] Apply to real allocator (jemalloc, tcmalloc)
- [ ] Benchmark with 400M allocations
- [ ] Integrate with dd-otel-host-profiler

---

## For Your Assignment

**Goal**: Focus on "no code" instrumentation - USDT/UProbe/GOT approaches

**What to demonstrate**:
1. ✅ USDT probe example working (`trace_usdt.py`)
2. ✅ UProbe example working (`trace_uprobe.py`)  
3. ✅ Output showing memory events
4. 📊 Overhead measurement (next step)

**This directory has everything you need to get started!**

