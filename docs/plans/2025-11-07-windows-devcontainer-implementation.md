# Windows Cross-Compilation Devcontainer Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Create a VSCode devcontainer for cross-compiling piHPSDR to Windows x86_64 using mingw-w64 and GTK3 from MSYS2.

**Architecture:** Debian-based container with mingw-w64 toolchain and MSYS2 precompiled libraries (GTK3, FFTW, libusb, libcurl, PortAudio). Build scripts automate compilation and DLL packaging.

**Tech Stack:** Docker, mingw-w64, MSYS2 packages, GTK3, pkg-config, bash

---

## Task 1: Create Devcontainer Directory Structure

**Files:**
- Create: `.devcontainer/` directory
- Create: `.devcontainer/.gitkeep` (temporary placeholder)

**Step 1: Create directory**

```bash
mkdir -p .devcontainer
```

**Step 2: Verify directory exists**

Run: `ls -la .devcontainer`
Expected: Directory exists

**Step 3: Commit**

```bash
git add .devcontainer/
git commit -m "chore: create .devcontainer directory structure"
```

---

## Task 2: Create Dockerfile

**Files:**
- Create: `.devcontainer/Dockerfile`

**Step 1: Create base Dockerfile with toolchain**

Create `.devcontainer/Dockerfile`:

```dockerfile
FROM debian:bookworm

# Install base build tools and mingw-w64 toolchain
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    ca-certificates \
    pkg-config \
    make \
    mingw-w64 \
    mingw-w64-tools \
    mingw-w64-x86-64-dev \
    zstd \
    && rm -rf /var/lib/apt/lists/*

# Set up environment for cross-compilation
ENV MINGW_PREFIX=/usr/x86_64-w64-mingw32
ENV PKG_CONFIG_PATH=${MINGW_PREFIX}/lib/pkgconfig
ENV PATH=${MINGW_PREFIX}/bin:$PATH

# Create workspace directory
WORKDIR /workspace

# Copy and run MSYS2 dependencies installer
COPY install-msys2-deps.sh /tmp/
RUN chmod +x /tmp/install-msys2-deps.sh && \
    /tmp/install-msys2-deps.sh && \
    rm /tmp/install-msys2-deps.sh

# Set default shell
CMD ["/bin/bash"]
```

**Step 2: Verify Dockerfile syntax**

Run: `cat .devcontainer/Dockerfile`
Expected: File contains complete Dockerfile

**Step 3: Commit**

```bash
git add .devcontainer/Dockerfile
git commit -m "feat: add Dockerfile for mingw-w64 cross-compilation"
```

---

## Task 3: Create MSYS2 Dependencies Installer Script

**Files:**
- Create: `.devcontainer/install-msys2-deps.sh`

**Step 1: Create script to download and install MSYS2 packages**

Create `.devcontainer/install-msys2-deps.sh`:

```bash
#!/bin/bash
set -e

# MSYS2 mingw64 repository
REPO_URL="https://repo.msys2.org/mingw/mingw64"
INSTALL_PREFIX="/usr/x86_64-w64-mingw32"

echo "Installing MSYS2 mingw64 packages to ${INSTALL_PREFIX}"

# Function to download and extract a package
install_package() {
    local package_name=$1
    echo "Processing package: ${package_name}"

    # Download package database to find latest version
    local pkg_pattern="${package_name}-[0-9]*.pkg.tar.zst"
    local pkg_url=$(wget -qO- "${REPO_URL}/" | grep -oP "${package_name}-[0-9][^\"]*\.pkg\.tar\.zst" | sort -V | tail -1)

    if [ -z "$pkg_url" ]; then
        echo "ERROR: Could not find package ${package_name}"
        return 1
    fi

    echo "  Found: ${pkg_url}"

    # Download package
    wget -q "${REPO_URL}/${pkg_url}" -O "/tmp/${pkg_url}"

    # Extract to install prefix
    tar -xf "/tmp/${pkg_url}" -C "${INSTALL_PREFIX}" --strip-components=1

    # Cleanup
    rm "/tmp/${pkg_url}"

    echo "  Installed: ${package_name}"
}

# Core packages to install (order matters - dependencies first)
PACKAGES=(
    "mingw-w64-x86_64-gcc-libs"
    "mingw-w64-x86_64-gmp"
    "mingw-w64-x86_64-expat"
    "mingw-w64-x86_64-gettext-runtime"
    "mingw-w64-x86_64-libiconv"
    "mingw-w64-x86_64-zlib"
    "mingw-w64-x86_64-libffi"
    "mingw-w64-x86_64-pcre2"
    "mingw-w64-x86_64-glib2"
    "mingw-w64-x86_64-bzip2"
    "mingw-w64-x86_64-cairo"
    "mingw-w64-x86_64-fontconfig"
    "mingw-w64-x86_64-freetype"
    "mingw-w64-x86_64-fribidi"
    "mingw-w64-x86_64-harfbuzz"
    "mingw-w64-x86_64-libpng"
    "mingw-w64-x86_64-pango"
    "mingw-w64-x86_64-pixman"
    "mingw-w64-x86_64-gdk-pixbuf2"
    "mingw-w64-x86_64-atk"
    "mingw-w64-x86_64-gtk3"
    "mingw-w64-x86_64-fftw"
    "mingw-w64-x86_64-libusb"
    "mingw-w64-x86_64-curl"
    "mingw-w64-x86_64-portaudio"
)

# Install each package
for package in "${PACKAGES[@]}"; do
    install_package "$package"
done

# Create pkg-config wrapper for cross-compilation
cat > /usr/local/bin/x86_64-w64-mingw32-pkg-config << 'EOF'
#!/bin/bash
export PKG_CONFIG_PATH=/usr/x86_64-w64-mingw32/lib/pkgconfig
exec pkg-config "$@"
EOF

chmod +x /usr/local/bin/x86_64-w64-mingw32-pkg-config

echo "MSYS2 packages installed successfully"
echo "Installed to: ${INSTALL_PREFIX}"
```

**Step 2: Make script executable**

Run: `chmod +x .devcontainer/install-msys2-deps.sh`

**Step 3: Verify script exists and is executable**

Run: `ls -l .devcontainer/install-msys2-deps.sh`
Expected: File exists with execute permissions

**Step 4: Commit**

```bash
git add .devcontainer/install-msys2-deps.sh
git commit -m "feat: add MSYS2 package installer script"
```

---

## Task 4: Create Windows Build Configuration

**Files:**
- Create: `.devcontainer/make.config.pihpsdr`

**Step 1: Create Makefile configuration for Windows**

Create `.devcontainer/make.config.pihpsdr`:

```makefile
# Windows cross-compilation configuration for piHPSDR
# This file configures the Makefile for building Windows executables

# Cross-compilation toolchain
CC=x86_64-w64-mingw32-gcc
LINK=x86_64-w64-mingw32-gcc
PKG_CONFIG=x86_64-w64-mingw32-pkg-config

# Disable Linux-specific features
GPIO=OFF
SATURN=OFF
USBOZY=OFF
SOAPYSDR=OFF
STEMLAB=OFF

# Windows-compatible features
AUDIO=PORTAUDIO
MIDI=OFF
TTS=OFF

# Additional Windows-specific flags
CFLAGS+=-DWINDOWS
```

**Step 2: Verify file contents**

Run: `cat .devcontainer/make.config.pihpsdr`
Expected: File contains Windows build configuration

**Step 3: Commit**

```bash
git add .devcontainer/make.config.pihpsdr
git commit -m "feat: add Windows build configuration for Makefile"
```

---

## Task 5: Create Build and Package Script

**Files:**
- Create: `.devcontainer/build-windows.sh`

**Step 1: Create build automation script**

Create `.devcontainer/build-windows.sh`:

```bash
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
```

**Step 2: Make script executable**

Run: `chmod +x .devcontainer/build-windows.sh`

**Step 3: Verify script exists**

Run: `ls -l .devcontainer/build-windows.sh`
Expected: File exists with execute permissions

**Step 4: Commit**

```bash
git add .devcontainer/build-windows.sh
git commit -m "feat: add Windows build and packaging script"
```

---

## Task 6: Create VSCode Devcontainer Configuration

**Files:**
- Create: `.devcontainer/devcontainer.json`

**Step 1: Create devcontainer.json**

Create `.devcontainer/devcontainer.json`:

```json
{
  "name": "piHPSDR Windows Cross-Compile",
  "build": {
    "dockerfile": "Dockerfile",
    "context": "."
  },
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.makefile-tools"
      ],
      "settings": {
        "terminal.integrated.defaultProfile.linux": "bash"
      }
    }
  },
  "workspaceFolder": "/workspace",
  "workspaceMount": "source=${localWorkspaceFolder},target=/workspace,type=bind,consistency=cached",
  "postCreateCommand": "echo 'Devcontainer ready! Run ./devcontainer/build-windows.sh to build for Windows'",
  "remoteUser": "root"
}
```

**Step 2: Verify JSON syntax**

Run: `cat .devcontainer/devcontainer.json | python3 -m json.tool > /dev/null && echo "Valid JSON"`
Expected: "Valid JSON"

**Step 3: Commit**

```bash
git add .devcontainer/devcontainer.json
git commit -m "feat: add VSCode devcontainer configuration"
```

---

## Task 7: Add Documentation

**Files:**
- Create: `.devcontainer/README.md`

**Step 1: Create README for devcontainer**

Create `.devcontainer/README.md`:

```markdown
# Windows Cross-Compilation Devcontainer

This devcontainer provides a complete environment for cross-compiling piHPSDR to Windows (x86_64) using mingw-w64 and GTK3.

## Quick Start

1. Open this project in VSCode
2. Click "Reopen in Container" when prompted (or use Command Palette: "Dev Containers: Reopen in Container")
3. Wait for container to build (first time only, ~5-10 minutes)
4. Run the build script:
   ```bash
   ./devcontainer/build-windows.sh
   ```
5. Find the Windows executable in `dist/pihpsdr.exe`

## What's Included

- mingw-w64 cross-compilation toolchain (x86_64)
- GTK3 for Windows (from MSYS2)
- Dependencies: FFTW, libusb, libcurl, PortAudio
- Automated build and DLL packaging scripts

## Files

- `Dockerfile` - Container build instructions
- `devcontainer.json` - VSCode devcontainer configuration
- `install-msys2-deps.sh` - Downloads and installs MSYS2 packages
- `make.config.pihpsdr` - Windows build configuration for Makefile
- `build-windows.sh` - Build and package script

## Manual Build

If you prefer to build manually:

```bash
# Copy Windows configuration
cp .devcontainer/make.config.pihpsdr .

# Build
make clean
make

# The executable will be pihpsdr.exe
```

## Deploying to Windows

Copy the entire `dist/` folder to a Windows machine. The folder contains:
- `pihpsdr.exe` - Main executable
- `*.dll` - Required runtime libraries
- `share/` - GTK schemas and resources

Run `pihpsdr.exe` from the `dist/` folder.

## Troubleshooting

**Container fails to build:**
- Check Docker is running
- Check internet connection (downloads MSYS2 packages)
- Try rebuilding: Command Palette → "Dev Containers: Rebuild Container"

**Build fails:**
- Check that WDSP compiled successfully
- Check for missing dependencies in console output

**Missing DLLs on Windows:**
- The `build-windows.sh` script should copy all required DLLs
- If something is missing, manually copy from `/usr/x86_64-w64-mingw32/bin/`

## Architecture Details

See `docs/plans/2025-11-07-windows-cross-compile-devcontainer-design.md` for full design documentation.
```

**Step 2: Verify file**

Run: `cat .devcontainer/README.md | head -20`
Expected: Shows README content

**Step 3: Commit**

```bash
git add .devcontainer/README.md
git commit -m "docs: add devcontainer usage documentation"
```

---

## Task 8: Test Devcontainer Build

**Files:**
- None (verification only)

**Step 1: Verify all files are present**

Run: `ls -la .devcontainer/`
Expected output:
```
Dockerfile
devcontainer.json
install-msys2-deps.sh (executable)
make.config.pihpsdr
build-windows.sh (executable)
README.md
```

**Step 2: Verify file permissions**

Run: `ls -l .devcontainer/*.sh`
Expected: Both .sh files should have execute permission (x)

**Step 3: Check git status**

Run: `git status`
Expected: Working tree clean, all files committed

**Step 4: Create final commit if needed**

If there are uncommitted changes:
```bash
git add -A
git commit -m "chore: finalize devcontainer setup"
```

---

## Task 9: Add .gitignore Entries

**Files:**
- Modify: `.gitignore` (if exists) or Create: `.gitignore`

**Step 1: Check if .gitignore exists**

Run: `ls -la .gitignore`

**Step 2: Add devcontainer-related ignore patterns**

Add to `.gitignore`:

```
# Windows build artifacts
*.exe
dist/
make.config.pihpsdr

# Devcontainer
.devcontainer/.tmp/
```

**Step 3: Verify .gitignore**

Run: `cat .gitignore | grep -A 5 "Windows build"`
Expected: Shows the added entries

**Step 4: Commit**

```bash
git add .gitignore
git commit -m "chore: add .gitignore entries for Windows builds"
```

---

## Verification Steps

After completing all tasks, verify the complete setup:

1. **Check all devcontainer files exist:**
   ```bash
   ls -la .devcontainer/
   ```
   Should show: Dockerfile, devcontainer.json, install-msys2-deps.sh, make.config.pihpsdr, build-windows.sh, README.md

2. **Check all files are committed:**
   ```bash
   git status
   ```
   Should show: "working tree clean"

3. **Test container build (optional, requires VSCode):**
   - Open VSCode
   - Command Palette → "Dev Containers: Reopen in Container"
   - Wait for build to complete
   - Open terminal in container
   - Run: `./devcontainer/build-windows.sh`
   - Verify: `dist/pihpsdr.exe` exists

## Notes

- The MSYS2 installer script downloads packages from repo.msys2.org - internet connection required during container build
- First container build takes ~5-10 minutes due to downloading GTK3 and dependencies
- Subsequent builds use Docker layer caching and are much faster
- The build script uses `objdump` to automatically detect required DLLs
- WDSP is compiled from the project's included source, not from external library

## Next Steps

After implementation:
1. Test the devcontainer builds successfully
2. Test that compilation produces a working .exe
3. Test the .exe on a Windows machine
4. Document any additional DLLs or runtime files needed
5. Consider creating a GitHub Actions workflow for automated Windows builds
