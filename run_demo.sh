#!/bin/bash
# Helper script to run the USDT memory profiler demo

echo "=== USDT Memory Profiler Demo ==="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "This script needs to be run with sudo for eBPF tracing"
    echo ""
    echo "Usage: sudo ./run_demo.sh"
    exit 1
fi

# Check if binary exists
if [ ! -f "./sample_allocator" ]; then
    echo "Error: sample_allocator not found"
    echo "Run 'make' first to build it"
    exit 1
fi

# Check if tracer exists
if [ ! -f "./trace_usdt.py" ]; then
    echo "Error: trace_usdt.py not found"
    exit 1
fi

echo "Starting USDT profiler..."
echo "This will run sample_allocator and trace its memory allocations"
echo ""
echo "Press Ctrl-C to stop"
echo ""
sleep 2

# Run the tracer with the sample program
python3 trace_usdt.py -c './sample_allocator'

