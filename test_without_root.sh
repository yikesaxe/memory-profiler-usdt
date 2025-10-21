#!/bin/bash
# Test script that doesn't require root (for CI/basic validation)

set -e

echo "=== Memory Profiler Test Suite (No Root Required) ==="
echo ""

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

success() { echo -e "${GREEN}✓${NC} $1"; }
error() { echo -e "${RED}✗${NC} $1"; }
warning() { echo -e "${YELLOW}⚠${NC} $1"; }

# Test 1: Check source files
echo "Test 1: Checking source files..."
if [ -f "sample_allocator.c" ] && [ -f "trace_usdt.py" ]; then
    success "All source files present"
else
    error "Missing source files"
    exit 1
fi

# Test 2: Check dependencies
echo ""
echo "Test 2: Checking dependencies..."
DEPS_OK=true

if [ -f "/usr/include/sys/sdt.h" ]; then
    success "systemtap-sdt-dev installed"
else
    warning "systemtap-sdt-dev not found (run: sudo ./install_deps.sh)"
    DEPS_OK=false
fi

# Test 3: Try to build
echo ""
echo "Test 3: Building..."

if [ -f "/usr/include/sys/sdt.h" ]; then
    if make clean && make 2>&1 | tail -10; then
        success "Build successful"
    else
        error "Build failed"
        DEPS_OK=false
    fi
else
    warning "Skipping build (missing dependencies)"
fi

echo ""
if [ "$DEPS_OK" = true ]; then
    success "All tests passed!"
    echo "Run: make test-got  (to see it in action)"
else
    warning "Install dependencies: sudo ./install_deps.sh"
fi
