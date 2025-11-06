#!/bin/bash
#
# piHPSDR Windows Build Environment Verification Script
#
# This script checks that all required tools and libraries are installed
# for building piHPSDR on Windows using MSYS2/MinGW64.
#
# Usage: ./verify_build_env.sh
#
# Author: piHPSDR Windows Porting Team
# Date: 2025-11-06
#

echo "========================================"
echo "piHPSDR Windows Build Environment Check"
echo "========================================"
echo ""

ERRORS=0
WARNINGS=0

# Check shell environment
echo "Environment Check:"
echo "------------------"
echo -n "Current shell: $MSYSTEM"
if [ "$MSYSTEM" != "MINGW64" ]; then
    echo " ❌ ERROR"
    echo "   Not running in MINGW64 environment!"
    echo "   Please launch 'MSYS2 MinGW64' from Start Menu"
    ERRORS=$((ERRORS + 1))
else
    echo " ✅"
fi
echo ""

echo "Build Tools:"
echo "------------"

# Check GCC
echo -n "GCC (C compiler): "
if gcc --version &> /dev/null; then
    VERSION=$(gcc -dumpversion)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-gcc"
    ERRORS=$((ERRORS + 1))
fi

# Check G++
echo -n "G++ (C++ compiler): "
if g++ --version &> /dev/null; then
    VERSION=$(g++ -dumpversion)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-gcc"
    ERRORS=$((ERRORS + 1))
fi

# Check Make
echo -n "GNU Make: "
if make --version &> /dev/null; then
    VERSION=$(make --version | head -1 | awk '{print $3}')
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S make"
    ERRORS=$((ERRORS + 1))
fi

# Check pkg-config
echo -n "pkg-config: "
if pkg-config --version &> /dev/null; then
    VERSION=$(pkg-config --version)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-pkgconf"
    ERRORS=$((ERRORS + 1))
fi

# Check Git
echo -n "Git: "
if git --version &> /dev/null; then
    VERSION=$(git --version | awk '{print $3}')
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S git"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "Required Libraries:"
echo "-------------------"

# Check GTK3
echo -n "GTK+ 3.0: "
if pkg-config --exists gtk+-3.0; then
    VERSION=$(pkg-config --modversion gtk+-3.0)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-gtk3"
    ERRORS=$((ERRORS + 1))
fi

# Check GLib
echo -n "GLib 2.0: "
if pkg-config --exists glib-2.0; then
    VERSION=$(pkg-config --modversion glib-2.0)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND (should be installed with GTK3)"
    ERRORS=$((ERRORS + 1))
fi

# Check PortAudio
echo -n "PortAudio: "
if pkg-config --exists portaudio-2.0; then
    VERSION=$(pkg-config --modversion portaudio-2.0)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-portaudio"
    ERRORS=$((ERRORS + 1))
fi

# Check FFTW3
echo -n "FFTW3: "
if pkg-config --exists fftw3; then
    VERSION=$(pkg-config --modversion fftw3)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-fftw"
    ERRORS=$((ERRORS + 1))
fi

# Check OpenSSL
echo -n "OpenSSL: "
if pkg-config --exists openssl; then
    VERSION=$(pkg-config --modversion openssl)
    echo "✅ $VERSION"
else
    echo "❌ NOT FOUND"
    echo "   Install: pacman -S mingw-w64-x86_64-openssl"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "Optional Libraries:"
echo "-------------------"

# Check SoapySDR
echo -n "SoapySDR: "
if pkg-config --exists SoapySDR; then
    VERSION=$(pkg-config --modversion SoapySDR)
    echo "✅ $VERSION"
else
    echo "⚠️  Not installed (optional for SoapySDR radio support)"
    echo "   Install: pacman -S mingw-w64-x86_64-soapysdr"
    WARNINGS=$((WARNINGS + 1))
fi

# Check libusb
echo -n "libusb-1.0: "
if pkg-config --exists libusb-1.0; then
    VERSION=$(pkg-config --modversion libusb-1.0)
    echo "✅ $VERSION"
else
    echo "⚠️  Not installed (optional for USB OZY radio support)"
    echo "   Install: pacman -S mingw-w64-x86_64-libusb"
    WARNINGS=$((WARNINGS + 1))
fi

# Check libcurl
echo -n "libcurl: "
if pkg-config --exists libcurl; then
    VERSION=$(pkg-config --modversion libcurl)
    echo "✅ $VERSION"
else
    echo "⚠️  Not installed (optional for Stemlab/RedPitaya support)"
    echo "   Install: pacman -S mingw-w64-x86_64-curl"
    WARNINGS=$((WARNINGS + 1))
fi

echo ""
echo "========================================"
echo "Summary:"
echo "========================================"
echo "Errors: $ERRORS"
echo "Warnings: $WARNINGS"
echo ""

if [ $ERRORS -eq 0 ]; then
    echo "✅ Build environment is ready!"
    echo ""
    echo "You can now proceed to build piHPSDR:"
    echo "  cd /path/to/pihpsdr"
    echo "  make"
    echo ""
    exit 0
else
    echo "❌ Build environment has $ERRORS error(s)"
    echo ""
    echo "Please install missing components listed above."
    echo "See docs/TASK-002_Windows_Dev_Setup.md for detailed instructions."
    echo ""
    exit 1
fi
