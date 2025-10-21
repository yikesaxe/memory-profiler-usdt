CC = gcc
CFLAGS = -Wall -O2 -g
LDFLAGS = -lelf -lz

# For USDT support
USDT_CFLAGS = -I/usr/include

.PHONY: all clean

all: sample_allocator

sample_allocator: sample_allocator.c
	$(CC) $(CFLAGS) $(USDT_CFLAGS) -o $@ $<
	@echo ""
	@echo "Built successfully!"
	@echo "Check USDT probes with: readelf -n $@ | grep NT_STAPSDT"
	@echo ""

clean:
	rm -f sample_allocator *.o
	@echo "Cleaned build artifacts"

# Helper targets
check-probes: sample_allocator
	@echo "=== USDT Probes in binary ==="
	readelf -n sample_allocator | grep -A 4 NT_STAPSDT || echo "No USDT probes found"

install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y \
		bpfcc-tools \
		python3-bpfcc \
		libbpf-dev \
		systemtap-sdt-dev \
		clang \
		llvm \
		build-essential \
		linux-headers-$(shell uname -r)

help:
	@echo "Available targets:"
	@echo "  make              - Build sample_allocator"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make check-probes - Show USDT probes in binary"
	@echo "  make install-deps - Install required dependencies"
	@echo ""
	@echo "Usage:"
	@echo "  1. Build: make"
	@echo "  2. Run tracer: sudo python3 trace_usdt.py -c './sample_allocator'"
	@echo "     or in two terminals:"
	@echo "       Terminal 1: ./sample_allocator"
	@echo "       Terminal 2: sudo python3 trace_usdt.py -p \$$(pgrep sample_allocator)"

