# ----------------------
# Makefile for ARM64 Memory Profiler with USDT
# ----------------------

TARGET = sample_allocator
READER = perf_reader
BPFOBJ = sample_allocator.bpf.o

CFLAGS = -g -O2
LIBBPF_CFLAGS = $(shell pkg-config --cflags libbpf 2>/dev/null)
LIBBPF_LIBS   = $(shell pkg-config --libs libbpf 2>/dev/null)

# Default target: build everything
all: $(TARGET) $(BPFOBJ) $(READER)

# Build sample_allocator (USDT probe)
$(TARGET): sample_allocator.c
	$(CC) $(CFLAGS) $< -o $@ -ldl

# Build eBPF object
$(BPFOBJ): sample_allocator.bpf.c
	clang -target bpf -D__TARGET_ARCH_arm64 -Wall -Wno-unused-value -Wno-pointer-sign \
	      -Wno-compare-distinct-pointer-types -O2 -g \
	      -c $< -o $@

# Build perf_reader (userland loader)
$(READER): perf_reader.c
	$(CC) $(CFLAGS) $(LIBBPF_CFLAGS) $< -o $@ $(LIBBPF_LIBS)

# Clean build files
clean:
	rm -f $(TARGET) $(READER) $(BPFOBJ)