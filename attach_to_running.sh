#!/bin/bash
# Helper script to attach to an already-running sample_allocator

echo "=== Attach USDT Tracer to Running Process ==="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "This script needs to be run with sudo for eBPF tracing"
    echo ""
    echo "Usage: sudo ./attach_to_running.sh"
    exit 1
fi

# Find the process
PID=$(pgrep -f sample_allocator | head -1)

if [ -z "$PID" ]; then
    echo "Error: sample_allocator is not running"
    echo ""
    echo "In another terminal, run: ./sample_allocator"
    echo "Then run this script again"
    exit 1
fi

echo "Found sample_allocator with PID: $PID"
echo ""
echo "Attaching tracer... Press Ctrl-C to stop"
echo ""
sleep 1

# Attach to the running process
python3 trace_usdt.py -p $PID

