#!/bin/bash

#####################################################
#
# Cross-compilation build script for piHPSDR
# For use in Linux devcontainer with MinGW toolchain
#
######################################################

echo "piHPSDR Cross-Compilation Build Script (MinGW on Linux)"
echo "========================================================="

# Check if we're in the right directory
if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
    echo "Error: This script must be run from the piHPSDR root directory"
    exit 1
fi

# Check if MinGW cross-compiler is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: MinGW cross-compiler not found"
    echo "Please install: apt-get install mingw-w64 gcc-mingw-w64-x86-64"
    exit 1
fi

echo "Detected MinGW cross-compiler: $(x86_64-w64-mingw32-gcc --version | head -1)"

# Set up cross-compilation environment
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
export AR=x86_64-w64-mingw32-ar
export RANLIB=x86_64-w64-mingw32-ranlib
export STRIP=x86_64-w64-mingw32-strip
export WINDRES=x86_64-w64-mingw32-windres

# Override OS to force Windows detection in Makefile
# The Makefile checks for OS=Windows_NT before running uname
export OS=Windows_NT

# Set PKG_CONFIG to use MinGW paths
export PKG_CONFIG_PATH=/usr/x86_64-w64-mingw32/lib/pkgconfig:/usr/x86_64-w64-mingw32/lib64/pkgconfig
export PKG_CONFIG_LIBDIR=/usr/x86_64-w64-mingw32/lib/pkgconfig:/usr/x86_64-w64-mingw32/lib64/pkgconfig

# Add Windows compatibility headers to include path
export CFLAGS="-I./Windows -O3 -Wall -Wextra -Wimplicit-fallthrough -Wno-unused-parameter -Wno-deprecated-declarations"

echo ""
echo "Cross-compilation environment:"
echo "  CC=$CC"
echo "  CXX=$CXX"
echo "  PKG_CONFIG_PATH=$PKG_CONFIG_PATH"
echo "  CFLAGS=$CFLAGS"
echo ""

# Create configuration file if it doesn't exist
if [ ! -f "make.config.pihpsdr" ]; then
    echo "Creating make.config.pihpsdr from Windows example..."
    cp Windows/make.config.pihpsdr.example make.config.pihpsdr
    echo "Configuration file created. You can edit make.config.pihpsdr to customize build options."
    echo ""
else
    echo "Found existing make.config.pihpsdr file"
    echo "Current configuration:"
    cat make.config.pihpsdr
    echo ""
fi

# Ask user what to build
echo "What would you like to build?"
echo "  1) Clean and build everything (wdsp + pihpsdr)"
echo "  2) Build only wdsp library"
echo "  3) Build only pihpsdr (assumes wdsp is already built)"
echo "  4) Clean only"
read -p "Enter choice [1-4]: " choice

case $choice in
    1)
        echo "Building WDSP library..."
        cd wdsp
        make clean
        make CC="$CC" AR="$AR" RANLIB="$RANLIB"

        if [ $? -ne 0 ]; then
            echo "Error: Failed to build WDSP library"
            exit 1
        fi

        cd ..

        echo "Building piHPSDR..."
        make clean
        make

        if [ $? -ne 0 ]; then
            echo "Error: Failed to build piHPSDR"
            exit 1
        fi
        ;;
    2)
        echo "Building WDSP library..."
        cd wdsp
        make clean
        make CC="$CC" AR="$AR" RANLIB="$RANLIB"

        if [ $? -ne 0 ]; then
            echo "Error: Failed to build WDSP library"
            exit 1
        fi

        cd ..
        ;;
    3)
        echo "Building piHPSDR..."
        make clean
        make

        if [ $? -ne 0 ]; then
            echo "Error: Failed to build piHPSDR"
            exit 1
        fi
        ;;
    4)
        echo "Cleaning..."
        make clean
        cd wdsp && make clean && cd ..
        echo "Clean completed"
        exit 0
        ;;
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "Build completed successfully!"
echo ""
echo "Output: pihpsdr.exe"
echo ""
echo "To test or package for Windows:"
echo "1. Copy pihpsdr.exe to a Windows machine"
echo "2. Make sure all MinGW64 DLLs are available (from /usr/x86_64-w64-mingw32/bin/)"
echo "3. Include GTK3 runtime files"
echo ""
