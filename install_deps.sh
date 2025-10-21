#!/bin/bash
# Installation script for USDT memory profiler dependencies

echo "=== Installing dependencies for USDT Memory Profiler ==="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "This script needs sudo privileges. Run as:"
    echo "  sudo ./install_deps.sh"
    exit 1
fi

echo "Updating package list..."
apt-get update

echo ""
echo "Installing BCC tools and Python bindings..."
apt-get install -y \
    bpfcc-tools \
    python3-bpfcc \
    libbpf-dev

echo ""
echo "Installing USDT development headers..."
apt-get install -y systemtap-sdt-dev

echo ""
echo "Installing build tools..."
apt-get install -y \
    clang \
    llvm \
    build-essential \
    linux-headers-$(uname -r)

echo ""
echo "=== Installation complete! ==="
echo ""
echo "Next steps:"
echo "  1. Build: make"
echo "  2. Check probes: make check-probes"
echo "  3. Run: ./sample_allocator (in one terminal)"
echo "  4. Trace: sudo python3 trace_usdt.py -p \$(pgrep sample_allocator)"

