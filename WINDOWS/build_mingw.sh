#!/bin/sh

################################################################
#
# build_mingw.sh - Prepare MSYS2/MinGW-w64 for building piHPSDR
# and create a Windows executable.
#
################################################################

# Update the MSYS2 environment and install toolchain/libraries
pacman --noconfirm -Syuu
pacman --noconfirm -S \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-gtk3 \
    mingw-w64-x86_64-fftw \
    mingw-w64-x86_64-portaudio \
    mingw-w64-x86_64-libusb

# Build the WDSP library first
(cd ../wdsp && make)

# Cross compile piHPSDR using the MinGW makefile
make -f Makefile.win

# Copy required DLLs to the local directory
cp /mingw64/bin/*.dll ./

