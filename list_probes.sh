#!/bin/bash
# Helper script to list USDT probes in binaries or running processes

usage() {
    echo "Usage: $0 [-p PID | -b BINARY]"
    echo ""
    echo "List USDT probes in a binary or running process"
    echo ""
    echo "Options:"
    echo "  -p PID      List probes in running process"
    echo "  -b BINARY   List probes in binary file"
    echo ""
    echo "Examples:"
    echo "  $0 -b ./sample_allocator"
    echo "  $0 -p \$(pgrep sample_allocator)"
    exit 1
}

if [ $# -eq 0 ]; then
    usage
fi

while getopts "p:b:h" opt; do
    case $opt in
        p)
            PID=$OPTARG
            echo "=== USDT Probes in Process $PID ==="
            echo ""
            
            # Get binary path
            BINARY=$(readlink -f /proc/$PID/exe)
            echo "Binary: $BINARY"
            echo ""
            
            # Use readelf to show probes
            readelf -n "$BINARY" | grep -A 8 NT_STAPSDT | grep -E "Name:|Provider:|Location:|Arguments:"
            
            echo ""
            echo "=== Using tplist (if available) ==="
            if command -v tplist-bpfcc &> /dev/null; then
                tplist-bpfcc -p $PID
            else
                echo "tplist-bpfcc not found. Install with: sudo apt-get install bpfcc-tools"
            fi
            ;;
        b)
            BINARY=$OPTARG
            echo "=== USDT Probes in Binary $BINARY ==="
            echo ""
            
            if [ ! -f "$BINARY" ]; then
                echo "Error: Binary $BINARY not found"
                exit 1
            fi
            
            # Use readelf to show probes
            echo "--- readelf output ---"
            readelf -n "$BINARY" | grep -A 8 NT_STAPSDT
            
            echo ""
            echo "--- Summary ---"
            readelf -n "$BINARY" | grep "NT_STAPSDT" | wc -l | xargs echo "Total probes:"
            
            echo ""
            echo "Probe names:"
            readelf -n "$BINARY" | grep -A 2 NT_STAPSDT | grep "Name:" | awk '{print "  " $2}'
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done

