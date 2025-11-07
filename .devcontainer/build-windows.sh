#!/bin/bash
set -e

echo "Building piHPSDR for Windows (x86_64)..."

# Copy Windows build configuration
cp .devcontainer/make.config.pihpsdr make.config.pihpsdr

# Clean previous builds
echo "Cleaning previous builds..."
make clean || true

# Build
echo "Compiling..."
make -j$(nproc)

# Create distribution directory
echo "Creating distribution package..."
rm -rf dist
mkdir -p dist

# Copy executable
if [ -f pihpsdr.exe ]; then
    cp pihpsdr.exe dist/
    echo "Copied pihpsdr.exe"
else
    echo "ERROR: pihpsdr.exe not found!"
    exit 1
fi

# Copy required DLLs from mingw installation
MINGW_BIN="/usr/x86_64-w64-mingw32/bin"

echo "Copying required DLLs..."

# Find all DLL dependencies using objdump
DLLS=$(x86_64-w64-mingw32-objdump -p pihpsdr.exe | grep 'DLL Name:' | awk '{print $3}' | sort -u)

for dll in $DLLS; do
    # Skip Windows system DLLs
    case $dll in
        KERNEL32.dll|USER32.dll|GDI32.dll|ADVAPI32.dll|SHELL32.dll|ole32.dll|OLEAUT32.dll|msvcrt.dll|WS2_32.dll)
            echo "  Skipping system DLL: $dll"
            ;;
        *)
            if [ -f "${MINGW_BIN}/${dll}" ]; then
                cp "${MINGW_BIN}/${dll}" dist/
                echo "  Copied: $dll"
            else
                echo "  WARNING: DLL not found: $dll"
            fi
            ;;
    esac
done

# Copy GTK runtime files (schemas, icons, etc.)
if [ -d "${MINGW_BIN}/../share/glib-2.0/schemas" ]; then
    mkdir -p dist/share/glib-2.0/schemas
    cp -r ${MINGW_BIN}/../share/glib-2.0/schemas/* dist/share/glib-2.0/schemas/
    echo "Copied GLib schemas"
fi

# Copy GTK themes and icons (optional, can be large)
# Uncomment if needed:
# if [ -d "${MINGW_BIN}/../share/icons" ]; then
#     mkdir -p dist/share/icons
#     cp -r ${MINGW_BIN}/../share/icons/* dist/share/icons/
#     echo "Copied GTK icons"
# fi

echo ""
echo "Build complete!"
echo "Windows executable and DLLs are in: ./dist/"
echo "Copy the entire dist/ folder to Windows to run piHPSDR"
