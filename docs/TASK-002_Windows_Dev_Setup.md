# TASK-002: Windows Development Environment Setup

**Date:** 2025-11-06
**Task ID:** TASK-002
**Dependencies:** TASK-001 (completed)
**Estimated Time:** 8 hours (mostly downloads and installations)
**Status:** Ready to execute

---

## Overview

This document provides **step-by-step instructions** for setting up a complete Windows development environment for building piHPSDR using MSYS2 and MinGW-w64.

**Target audience:** Windows developers who want to build piHPSDR from source.

---

## Prerequisites

### System Requirements

- **OS:** Windows 10 (64-bit) version 1809 or later, or Windows 11
- **Disk Space:** ~5 GB free (MSYS2 + all dependencies)
- **RAM:** 4 GB minimum, 8 GB recommended
- **Internet:** Broadband connection (will download ~2 GB)
- **Administrator Access:** Required for initial MSYS2 installation

### Before You Begin

1. **Disable antivirus temporarily** (optional but recommended during installation)
   - Some antivirus software interferes with MSYS2 package installation
   - Re-enable after setup is complete

2. **Ensure Windows Defender allows downloads**
   - MSYS2 installer and packages might be flagged

3. **Close all command prompts and terminals**

---

## Step 1: Install MSYS2

### 1.1 Download MSYS2 Installer

1. Go to https://www.msys2.org/
2. Click the large **"Download MSYS2"** button
3. Download the latest installer: `msys2-x86_64-xxxxxxxx.exe` (~90 MB)
4. Save to your Downloads folder

**Direct link (as of 2025-11-06):**
https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe

### 1.2 Run Installer

1. **Double-click** the downloaded installer
2. If Windows SmartScreen appears, click **"More info"** → **"Run anyway"**
3. Follow the installation wizard:
   - **Installation Folder:** Leave default `C:\msys64` (recommended)
   - Click **"Next"** through all prompts
   - **DO NOT** uncheck "Run MSYS2 now"

**Installation time:** ~2 minutes

### 1.3 Initial MSYS2 Setup

After installation, an **MSYS2 terminal** window will open automatically.

Run the following command to update the package database:

```bash
pacman -Syu
```

**Expected output:**
```
:: Synchronizing package databases...
 mingw32 is up to date
 mingw64 is up to date
 ucrt64 is up to date
 clang64 is up to date
 msys is up to date
:: Starting core system upgrade...
 there is nothing to do
```

**If prompted to close the terminal:**
- Type `Y` and press Enter
- The terminal will close
- **Reopen MSYS2** from the Start Menu: "MSYS2 MSYS"
- Run `pacman -Syu` again

**Repeat until no more updates are available.**

### 1.4 Verify MSYS2 Installation

In the MSYS2 terminal, check versions:

```bash
pacman --version
```

**Expected output:**
```
 .--.                  Pacman v6.x.x - libalpm v14.x.x
...
```

**✅ MSYS2 is now installed!**

---

## Step 2: Install MinGW-w64 Toolchain

MSYS2 has multiple environments. We will use **MinGW64** for 64-bit Windows applications.

### 2.1 Understanding MSYS2 Environments

MSYS2 provides different shortcuts in the Start Menu:

| Shortcut | Purpose | Use for piHPSDR? |
|----------|---------|------------------|
| **MSYS2 MSYS** | POSIX-compatible environment | ❌ No (for system tools only) |
| **MSYS2 MINGW64** | 64-bit Windows applications | ✅ **YES - Use this!** |
| **MSYS2 MINGW32** | 32-bit Windows applications | ❌ No (we target 64-bit) |
| **MSYS2 UCRT64** | Universal C Runtime (newer) | ⚠️ Alternative (experimental) |
| **MSYS2 CLANG64** | Clang/LLVM compiler | ❌ No (we use GCC) |

**Important:** Always use **"MSYS2 MinGW64"** for building piHPSDR!

### 2.2 Launch MinGW64 Shell

1. **Close any open MSYS2 windows**
2. Open Start Menu
3. Search for **"MSYS2 MinGW64"**
4. Click to launch (or pin to taskbar for easy access)

**You should see:**
```
username@HOSTNAME MINGW64 ~
$
```

The `MINGW64` in the prompt confirms you're in the correct environment.

### 2.3 Install GCC Toolchain

Install the complete MinGW-w64 GCC toolchain:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain
```

**Prompt:**
```
:: There are 19 members in group mingw-w64-x86_64-toolchain:
:: Repository mingw64
   1) mingw-w64-x86_64-binutils  2) mingw-w64-x86_64-crt-git
   ...
Enter a selection (default=all):
```

**Press Enter** to install all (recommended).

**Confirmation prompt:**
```
Proceed with installation? [Y/n]
```

**Type `Y` and press Enter.**

**Installation time:** ~5 minutes (downloads ~300 MB)

### 2.4 Install Essential Build Tools

```bash
pacman -S --needed base-devel git
```

This installs:
- `make` (GNU Make)
- `automake`, `autoconf`
- `git` (version control)
- Other build essentials

**Installation time:** ~2 minutes

### 2.5 Verify Toolchain Installation

Check installed tools:

```bash
gcc --version
g++ --version
make --version
git --version
pkg-config --version
```

**Expected output (versions may vary):**
```
gcc (Rev1, Built by MSYS2 project) 13.x.x
GNU Make 4.x
git version 2.x.x
pkgconf 2.x.x
```

**✅ MinGW-w64 toolchain is ready!**

---

## Step 3: Install piHPSDR Dependencies

Now install all libraries required to build piHPSDR.

### 3.1 Install GTK3 and Dependencies

GTK3 is the UI framework. This will install ~150 packages (GTK, GLib, Cairo, Pango, etc.).

```bash
pacman -S --needed mingw-w64-x86_64-gtk3
```

**Installation time:** ~10 minutes (downloads ~200 MB)

**Verify:**
```bash
pkg-config --modversion gtk+-3.0
```

**Expected output:**
```
3.24.x
```

### 3.2 Install PortAudio

PortAudio is the cross-platform audio library.

```bash
pacman -S --needed mingw-w64-x86_64-portaudio
```

**Verify:**
```bash
pkg-config --modversion portaudio-2.0
```

**Expected output:**
```
19
```

### 3.3 Install FFTW3

FFTW3 is the Fast Fourier Transform library (used by WDSP).

```bash
pacman -S --needed mingw-w64-x86_64-fftw
```

**Verify:**
```bash
pkg-config --modversion fftw3
```

**Expected output:**
```
3.3.x
```

### 3.4 Install OpenSSL

OpenSSL is used for TCI and client/server password encryption.

```bash
pacman -S --needed mingw-w64-x86_64-openssl
```

**Verify:**
```bash
pkg-config --modversion openssl
```

**Expected output:**
```
3.x.x
```

### 3.5 Install Optional Dependencies

#### SoapySDR (optional - for SoapySDR radio support)

```bash
pacman -S --needed mingw-w64-x86_64-soapysdr
```

#### libusb (optional - for USB OZY support)

```bash
pacman -S --needed mingw-w64-x86_64-libusb
```

#### libcurl (optional - for Stemlab/RedPitaya discovery)

```bash
pacman -S --needed mingw-w64-x86_64-curl
```

### 3.6 Summary of Installed Packages

Run this to see all installed packages:

```bash
pacman -Q | grep mingw-w64-x86_64 | wc -l
```

**Expected:** ~250-300 packages (GTK3 has many dependencies)

---

## Step 4: Configure Git (First Time Only)

If this is your first time using Git on this machine:

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

**Verify:**
```bash
git config --global --list
```

---

## Step 5: Clone piHPSDR Repository

### 5.1 Create a Projects Directory

```bash
cd ~
mkdir -p Projects
cd Projects
```

This creates `C:\msys64\home\<username>\Projects`

### 5.2 Clone the Repository

```bash
git clone https://github.com/g0orx/pihpsdr.git
cd pihpsdr
```

**If you're working on a fork:**
```bash
git clone https://github.com/<your-username>/pihpsdr.git
cd pihpsdr
```

**Verify:**
```bash
ls -la
```

You should see `Makefile`, `src/`, `wdsp/`, etc.

---

## Step 6: Environment Verification

Before attempting to build, let's verify everything is correctly installed.

### 6.1 Create Verification Script

Create a file to test all dependencies:

```bash
cat > verify_setup.sh << 'EOF'
#!/bin/bash
echo "========================================"
echo "piHPSDR Windows Build Environment Check"
echo "========================================"
echo ""

# Check shell
echo "Current shell: $MSYSTEM"
if [ "$MSYSTEM" != "MINGW64" ]; then
    echo "❌ ERROR: Not running in MINGW64 environment!"
    echo "   Please launch 'MSYS2 MinGW64' from Start Menu"
    exit 1
fi
echo "✅ MINGW64 environment"
echo ""

# Check GCC
echo -n "GCC: "
if gcc --version &> /dev/null; then
    echo "✅ $(gcc -dumpversion)"
else
    echo "❌ NOT FOUND"
fi

# Check Make
echo -n "Make: "
if make --version &> /dev/null; then
    echo "✅ $(make --version | head -1 | cut -d' ' -f3)"
else
    echo "❌ NOT FOUND"
fi

# Check pkg-config
echo -n "pkg-config: "
if pkg-config --version &> /dev/null; then
    echo "✅ $(pkg-config --version)"
else
    echo "❌ NOT FOUND"
fi

# Check Git
echo -n "Git: "
if git --version &> /dev/null; then
    echo "✅ $(git --version | cut -d' ' -f3)"
else
    echo "❌ NOT FOUND"
fi

echo ""
echo "Checking libraries via pkg-config:"
echo "-----------------------------------"

# Check GTK3
echo -n "GTK+ 3.0: "
if pkg-config --modversion gtk+-3.0 &> /dev/null; then
    echo "✅ $(pkg-config --modversion gtk+-3.0)"
else
    echo "❌ NOT FOUND"
fi

# Check PortAudio
echo -n "PortAudio: "
if pkg-config --modversion portaudio-2.0 &> /dev/null; then
    echo "✅ $(pkg-config --modversion portaudio-2.0)"
else
    echo "❌ NOT FOUND"
fi

# Check FFTW3
echo -n "FFTW3: "
if pkg-config --modversion fftw3 &> /dev/null; then
    echo "✅ $(pkg-config --modversion fftw3)"
else
    echo "❌ NOT FOUND"
fi

# Check OpenSSL
echo -n "OpenSSL: "
if pkg-config --modversion openssl &> /dev/null; then
    echo "✅ $(pkg-config --modversion openssl)"
else
    echo "❌ NOT FOUND"
fi

echo ""
echo "Optional libraries:"
echo "-------------------"

# Check SoapySDR
echo -n "SoapySDR: "
if pkg-config --modversion SoapySDR &> /dev/null; then
    echo "✅ $(pkg-config --modversion SoapySDR)"
else
    echo "⚠️  Not installed (optional)"
fi

# Check libusb
echo -n "libusb-1.0: "
if pkg-config --modversion libusb-1.0 &> /dev/null; then
    echo "✅ $(pkg-config --modversion libusb-1.0)"
else
    echo "⚠️  Not installed (optional)"
fi

# Check libcurl
echo -n "libcurl: "
if pkg-config --modversion libcurl &> /dev/null; then
    echo "✅ $(pkg-config --modversion libcurl)"
else
    echo "⚠️  Not installed (optional)"
fi

echo ""
echo "========================================"
echo "Environment Check Complete!"
echo "========================================"
EOF

chmod +x verify_setup.sh
```

### 6.2 Run Verification

```bash
./verify_setup.sh
```

**Expected output:**

```
========================================
piHPSDR Windows Build Environment Check
========================================

Current shell: MINGW64
✅ MINGW64 environment

GCC: ✅ 13.2.0
Make: ✅ 4.4.1
pkg-config: ✅ 2.1.0
Git: ✅ 2.43.0

Checking libraries via pkg-config:
-----------------------------------
GTK+ 3.0: ✅ 3.24.38
PortAudio: ✅ 19
FFTW3: ✅ 3.3.10
OpenSSL: ✅ 3.2.0

Optional libraries:
-------------------
SoapySDR: ⚠️  Not installed (optional)
libusb-1.0: ⚠️  Not installed (optional)
libcurl: ⚠️  Not installed (optional)

========================================
Environment Check Complete!
========================================
```

**All ✅ = Ready to build!**

---

## Step 7: Test Compilation (Sanity Check)

Before modifying the Makefile for Windows, let's see what happens if we try to build as-is.

### 7.1 Attempt Initial Build

```bash
cd ~/Projects/pihpsdr
make clean
make 2>&1 | tee build_log.txt
```

**Expected result:** Will fail with errors (normal - Makefile doesn't support Windows yet)

**Common errors you'll see:**
1. ❌ GPIO/I2C/SATURN errors (expected - not supported on Windows)
2. ❌ Socket/network errors (needs Winsock2 compatibility)
3. ❌ Path separator errors (needs `/` vs `\` handling)
4. ❌ Audio library errors (PulseAudio not available on Windows)

**This is NORMAL and EXPECTED!**

### 7.2 Save Error Log

The file `build_log.txt` now contains all compilation errors. This will be useful for TASK-007 (Makefile modifications).

```bash
wc -l build_log.txt
```

**Expected:** 50-200 lines of errors

---

## Troubleshooting

### Problem: "bash: pacman: command not found"

**Cause:** You're in the wrong MSYS2 environment or pacman is not in PATH.

**Solution:**
1. Close terminal
2. Open **"MSYS2 MSYS"** from Start Menu
3. Run: `pacman -Syu`
4. Then switch to **"MSYS2 MinGW64"** for building

### Problem: "error: failed to commit transaction (conflicting files)"

**Cause:** Package file conflicts during installation.

**Solution:**
```bash
pacman -S --overwrite '*' <package-name>
```

### Problem: Package download is very slow

**Cause:** MSYS2 mirrors might be slow.

**Solution:** Change to a faster mirror:
1. Edit `/etc/pacman.d/mirrorlist.mingw`
2. Move a geographically closer mirror to the top
3. Run `pacman -Syu` again

### Problem: "pkg-config: command not found"

**Cause:** `pkg-config` not installed.

**Solution:**
```bash
pacman -S --needed mingw-w64-x86_64-pkgconf
```

### Problem: GTK3 installation fails

**Cause:** Interrupted download or corrupted packages.

**Solution:**
```bash
pacman -Scc  # Clear package cache
pacman -Syu  # Update system
pacman -S --needed mingw-w64-x86_64-gtk3
```

### Problem: Windows Defender blocks MSYS2

**Cause:** False positive from antivirus.

**Solution:**
1. Open Windows Security
2. Virus & threat protection → Manage settings
3. Add exclusion: `C:\msys64`
4. Retry installation

### Problem: "Access denied" during installation

**Cause:** Insufficient permissions.

**Solution:**
1. Close MSYS2
2. Right-click **"MSYS2 MinGW64"** → Run as Administrator
3. Retry installation

---

## PATH and Environment Variables

### Understanding MSYS2 Paths

MSYS2 uses Unix-style paths internally, but Windows paths work too:

| Windows Path | MSYS2 Path |
|--------------|------------|
| `C:\msys64\home\user` | `/home/user` or `~` |
| `C:\msys64\mingw64` | `/mingw64` |
| `C:\Users\User\Documents` | `/c/Users/User/Documents` |
| `D:\Projects` | `/d/Projects` |

**Tip:** Use `cygpath` to convert paths:
```bash
cygpath -w /home/user     # Convert to Windows path
cygpath -u 'C:\msys64'    # Convert to Unix path
```

### Setting Persistent Environment Variables

To set environment variables for MinGW64 shell only:

```bash
echo 'export MY_VAR="value"' >> ~/.bashrc
source ~/.bashrc
```

---

## Quick Reference

### Essential Commands

| Task | Command |
|------|---------|
| **Update MSYS2** | `pacman -Syu` |
| **Install package** | `pacman -S <package>` |
| **Search package** | `pacman -Ss <keyword>` |
| **List installed** | `pacman -Q` |
| **Remove package** | `pacman -R <package>` |
| **Clean cache** | `pacman -Scc` |
| **Check GCC version** | `gcc --version` |
| **Find library** | `pkg-config --list-all \| grep <name>` |

### Package Name Format

All MinGW64 packages start with `mingw-w64-x86_64-`:

```bash
# Install GTK3
pacman -S mingw-w64-x86_64-gtk3

# Install library "foo"
pacman -S mingw-w64-x86_64-foo
```

### Finding Package Names

```bash
# Search for a library
pacman -Ss portaudio

# Show package details
pacman -Si mingw-w64-x86_64-portaudio

# List files in package
pacman -Ql mingw-w64-x86_64-portaudio
```

---

## What's Next?

After completing TASK-002, you should have:

✅ MSYS2 installed and updated
✅ MinGW-w64 GCC toolchain working
✅ All piHPSDR dependencies installed
✅ piHPSDR repository cloned
✅ Environment verified with `verify_setup.sh`
✅ Initial build attempt logged for analysis

**Next tasks:**
- **TASK-003:** Install GTK3 Development Libraries (✅ Already done in Step 3.1!)
- **TASK-004:** Build/Obtain PortAudio (✅ Already done in Step 3.2!)
- **TASK-005:** Build/Obtain FFTW3 (✅ Already done in Step 3.3!)
- **TASK-006:** Set Up pthreads (✅ Already done - MinGW includes pthreads!)
- **TASK-007:** Create Windows-Specific Makefile Configuration (Next major task)

---

## Appendix A: Complete Package List

All packages installed in this guide:

### Core Toolchain
- `mingw-w64-x86_64-toolchain` (19 packages including gcc, g++, gdb, binutils)
- `base-devel` (make, automake, autoconf, etc.)
- `git`

### Required Libraries
- `mingw-w64-x86_64-gtk3` (~150 packages including GLib, Cairo, Pango)
- `mingw-w64-x86_64-portaudio`
- `mingw-w64-x86_64-fftw`
- `mingw-w64-x86_64-openssl`

### Optional Libraries
- `mingw-w64-x86_64-soapysdr` (for SoapySDR radios)
- `mingw-w64-x86_64-libusb` (for USB OZY radios)
- `mingw-w64-x86_64-curl` (for Stemlab/RedPitaya)

**Total disk usage:** ~4-5 GB

---

## Appendix B: Uninstalling MSYS2

If you need to completely remove MSYS2:

1. Close all MSYS2 windows
2. Delete `C:\msys64` folder
3. Remove MSYS2 shortcuts from Start Menu
4. (Optional) Remove user data: `C:\Users\<username>\.msys2`

---

## Appendix C: Alternative Installation Method

### Using Chocolatey (Advanced)

If you use Chocolatey package manager:

```powershell
# In PowerShell (Admin)
choco install msys2
```

Then follow steps 1.3 onwards.

---

## References

- **MSYS2 Official Website:** https://www.msys2.org/
- **MSYS2 Package Repository:** https://packages.msys2.org/
- **MinGW-w64 Project:** https://www.mingw-w64.org/
- **GTK for Windows:** https://www.gtk.org/docs/installations/windows/
- **PortAudio Documentation:** http://www.portaudio.com/

---

**TASK-002 Complete!** ✅

**Estimated total time:** 1-2 hours (including downloads)

**Next:** TASK-007 - Modify Makefile for Windows support

---

**END OF DOCUMENT**
