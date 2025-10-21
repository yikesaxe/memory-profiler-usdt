# Quick Start Guide

## Prerequisites Installation

Run the installation script:

```bash
cd /home/axel/Workspace/memory-profiler-usdt
sudo ./install_deps.sh
```

Or install manually:

```bash
sudo apt-get update
sudo apt-get install -y systemtap-sdt-dev bpfcc-tools python3-bpfcc build-essential
```

## Build and Run

### Step 1: Build the sample program

```bash
make
```

You should see:
```
Built successfully!
Check USDT probes with: readelf -n sample_allocator | grep NT_STAPSDT
```

### Step 2: Verify USDT probes are present

```bash
make check-probes
```

You should see output showing the USDT probes embedded in the binary:
- `memory_profiler:malloc_entry`
- `memory_profiler:free_entry`
- `memory_profiler:startup`
- `memory_profiler:shutdown`

### Step 3: Run the sample program

Open a terminal and run:

```bash
./sample_allocator
```

The program will:
1. Print its PID
2. Wait 3 seconds for you to attach a tracer
3. Run various allocation patterns
4. Exit

### Step 4: Trace with USDT (in a second terminal)

While `sample_allocator` is running, in another terminal:

```bash
sudo python3 trace_usdt.py -p $(pgrep sample_allocator)
```

You should see output like:

```
=== USDT Memory Profiler ===
Tracing PID 12345...
Hit Ctrl-C to end.

TIME(s)      PID      EVENT              SIZE         ADDRESS
--------------------------------------------------------------------------------
3.002145     12345    MALLOC             32           0x55a1c2e4d2a0
3.012456     12345    FREE               32           0x55a1c2e4d2a0
...
```

## Alternative: One-Command Test

You can also trace a command directly:

```bash
sudo python3 trace_usdt.py -c './sample_allocator'
```

## Testing UProbe (for comparison)

To test the UProbe approach (works on ANY program):

```bash
# In terminal 1
./sample_allocator

# In terminal 2
sudo python3 trace_uprobe.py -p $(pgrep sample_allocator)
```

Or trace malloc/free system-wide (very noisy!):

```bash
sudo python3 trace_uprobe.py
```

## What You Should See

### USDT Output

The USDT tracer will show:
- ✅ Allocation size and address
- ✅ Free address and size
- ✅ Timing information
- ✅ Statistics at the end (total allocations, frees, potential leaks)

### Example Output

```
TIME(s)      PID      EVENT              SIZE         ADDRESS
--------------------------------------------------------------------------------
3.002145     12345    MALLOC             32           0x55a1c2e4d2a0
3.012456     12345    FREE               32           0x55a1c2e4d2a0
3.022789     12345    MALLOC             48           0x55a1c2e4d2d0
3.033123     12345    FREE               48           0x55a1c2e4d2d0
...

================================================================================
STATISTICS
================================================================================
Total allocations:     80
Total frees:           64
Bytes allocated:       15,872
Bytes freed:           12,096
Active allocations:    16
Potential leak:        3,776 bytes

Active allocations:
  0x55a1c2e4d300: 256 bytes
  0x55a1c2e4d400: 512 bytes
  ...
```

## Troubleshooting

### "No such file or directory: sys/sdt.h"

Install systemtap-sdt-dev:
```bash
sudo apt-get install systemtap-sdt-dev
```

### "Error enabling probes"

Make sure the binary has USDT probes:
```bash
readelf -n ./sample_allocator | grep NT_STAPSDT
```

### "Permission denied" when running tracer

The tracer needs root privileges:
```bash
sudo python3 trace_usdt.py -p <PID>
```

### Can't find BCC

Install BCC:
```bash
sudo apt-get install bpfcc-tools python3-bpfcc
```

## Next Steps

1. ✅ Get basic USDT example working
2. 📊 Measure overhead of USDT vs UProbe
3. 🔬 Apply to real allocators (jemalloc, tcmalloc)
4. 📈 Test with high allocation rates (400M allocs/min)
5. 🚀 Integrate with dd-otel-host-profiler

See `COMPARISON.md` for detailed comparison of USDT vs UProbe approaches.

