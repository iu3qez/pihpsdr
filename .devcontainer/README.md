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
