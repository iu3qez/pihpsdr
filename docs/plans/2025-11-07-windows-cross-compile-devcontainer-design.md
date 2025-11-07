# Windows Cross-Compilation Devcontainer Design

**Date:** 2025-11-07
**Objective:** Create a devcontainer for cross-compiling piHPSDR to Windows (x86_64) using mingw-w64 and GTK3

## Overview

This design provides a VSCode devcontainer that enables cross-compilation of piHPSDR from Linux to Windows, producing a standalone `.exe` with all necessary DLLs. The approach uses mingw-w64 toolchain with precompiled libraries from MSYS2 repository.

## Requirements

- Target: Windows 64-bit (x86_64-w64-mingw32)
- GUI Framework: GTK3 for Windows
- Dependencies: FFTW3, libusb, libcurl, PortAudio
- WDSP: Compiled from source (included in project root)
- Base OS: Debian/Ubuntu
- Scope: Compilation only (no Wine/GUI testing)

## Architecture

### File Structure

```
.devcontainer/
├── devcontainer.json           # VSCode devcontainer configuration
├── Dockerfile                  # Container build instructions
├── install-msys2-deps.sh       # Script to download MSYS2 packages
├── make.config.pihpsdr         # Makefile config for Windows build
└── build-windows.sh            # Helper script to build and package
```

### Base Image

**Choice:** `debian:bookworm` (or `ubuntu:22.04`)

**Rationale:** Stable, well-maintained mingw-w64 packages, good pkg-config support.

## Components

### 1. Toolchain (from Debian packages)

- `mingw-w64` - Complete mingw-w64 toolchain
- `mingw-w64-tools` - Additional utilities
- `mingw-w64-x86-64-dev` - Windows headers and base libraries
- `build-essential`, `git`, `pkg-config`, `make`

### 2. Libraries (from MSYS2 repository)

**Source:** https://repo.msys2.org/mingw/mingw64/

**Packages:**
- `mingw-w64-x86_64-gtk3` (includes glib2, cairo, pango, etc.)
- `mingw-w64-x86_64-fftw`
- `mingw-w64-x86_64-libusb`
- `mingw-w64-x86_64-curl`
- `mingw-w64-x86_64-portaudio`

**Installation approach:**
- Download `.tar.zst` packages from MSYS2 repo
- Extract to `/usr/x86_64-w64-mingw32/`
- Script handles dependency resolution automatically

### 3. Installation Script: `install-msys2-deps.sh`

**Purpose:** Download and install MSYS2 precompiled mingw64 packages with automatic dependency resolution.

**Process:**
1. Download MSYS2 package database
2. For each required package, find latest version
3. Download `.tar.zst` archive
4. Extract to `/usr/x86_64-w64-mingw32/`
5. Update pkg-config paths

**Install prefix:** `/usr/x86_64-w64-mingw32/`

### 4. Build Configuration: `make.config.pihpsdr`

Cross-compilation settings for piHPSDR Makefile:

```makefile
CC=x86_64-w64-mingw32-gcc
LINK=x86_64-w64-mingw32-gcc
PKG_CONFIG=x86_64-w64-mingw32-pkg-config

# Disable Linux-specific features
GPIO=OFF
SATURN=OFF
USBOZY=OFF
SOAPYSDR=OFF
STEMLAB=OFF

# Windows-compatible options
AUDIO=PORTAUDIO
MIDI=OFF
TTS=OFF
```

### 5. Build Helper: `build-windows.sh`

**Purpose:** Simplify the build and packaging process.

**Steps:**
1. Copy Windows build config to project root
2. Clean previous builds
3. Compile with make
4. Copy resulting `pihpsdr.exe` to `dist/`
5. Copy all required DLLs from `/usr/x86_64-w64-mingw32/bin/` to `dist/`
6. Create ready-to-deploy package

**Output:** `dist/` folder with executable and all dependencies.

### 6. VSCode Configuration: `devcontainer.json`

```json
{
  "name": "piHPSDR Windows Cross-Compile",
  "build": {
    "dockerfile": "Dockerfile"
  },
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.makefile-tools"
      ]
    }
  },
  "workspaceFolder": "/workspace",
  "workspaceMount": "source=${localWorkspaceFolder},target=/workspace,type=bind",
  "postCreateCommand": "echo 'Devcontainer ready for Windows cross-compilation'"
}
```

## Workflow

### Setup
1. Open project in VSCode
2. Select "Reopen in Container"
3. Container builds with all mingw dependencies

### Build
```bash
# Option 1: Use helper script
./devcontainer/build-windows.sh

# Option 2: Manual build
cp .devcontainer/make.config.pihpsdr .
make clean
make
```

### Package
The `build-windows.sh` script creates a `dist/` folder containing:
- `pihpsdr.exe`
- All required GTK3 DLLs
- FFTW, libusb, libcurl, PortAudio DLLs
- Any other runtime dependencies

### Deploy
Copy the entire `dist/` folder to a Windows machine and run `pihpsdr.exe`.

## pkg-config Configuration

**Key requirement:** Cross-compilation toolchain must find Windows libraries, not Linux ones.

**Solution:**
- Set `PKG_CONFIG_PATH=/usr/x86_64-w64-mingw32/lib/pkgconfig/`
- Use `x86_64-w64-mingw32-pkg-config` wrapper
- MSYS2 packages include proper `.pc` files

## Excluded Components

- **WDSP:** Compiled from source in project root (not from external library)
- **GPIO:** Linux-specific, disabled for Windows
- **SATURN/XDMA:** Linux-specific hardware support
- **PulseAudio:** Linux audio system, replaced with PortAudio
- **Wine/Testing:** No GUI testing in container, test on real Windows

## Benefits

1. **Reproducible builds:** Container ensures consistent environment
2. **No Windows needed:** Cross-compile entirely from Linux
3. **Automated dependencies:** MSYS2 packages handle complex GTK3 dependencies
4. **Easy packaging:** Script bundles all DLLs automatically
5. **Version control:** Dockerfile documents exact build environment

## Future Enhancements

- Support for 32-bit builds (i686-w64-mingw32) if needed
- Automated CI/CD integration for Windows releases
- NSIS/WiX installer generation
- Automated testing with Wine (if GUI issues can be resolved)
