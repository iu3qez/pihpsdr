# Windows Porting - Detailed Task List

**Project:** piHPSDR Windows Port
**Version:** 1.0
**Date:** 2025-11-06
**Status:** Planning Phase

---

## Task Organization

Tasks are organized by **Phase** and **Milestone** as defined in the PRD.
Each task includes:
- **ID:** Unique identifier
- **Description:** What needs to be done
- **Dependencies:** Prerequisites (task IDs)
- **Estimate:** Time estimate in hours/days
- **Priority:** P0 (Critical), P1 (High), P2 (Medium), P3 (Low)
- **Assignee:** TBD
- **Status:** Not Started / In Progress / Blocked / Completed

---

## PHASE 1: FOUNDATION (Weeks 1-3)

### Milestone 1.1: Build System Setup

#### TASK-001: Evaluate Build System Options
- **Description:** Research and decide between extending Makefile with MSYS2 vs migrating to CMake
- **Deliverable:** Decision document with pros/cons
- **Dependencies:** None
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Notes:**
  - Evaluate MSYS2 + GNU Make (minimal changes)
  - Evaluate CMake migration (better long-term, more work)
  - Consider vcpkg integration

#### TASK-002: Set Up Windows Development Environment
- **Description:** Install and configure complete Windows build environment
- **Deliverable:** Working dev environment with all tools
- **Dependencies:** TASK-001
- **Estimate:** 8 hours
- **Priority:** P0
- **Status:** Not Started
- **Subtasks:**
  1. Install MSYS2 (or chosen build system)
  2. Install MinGW-w64 toolchain
  3. Install Git for Windows
  4. Configure PATH and environment variables
  5. Test basic compilation (hello world)

#### TASK-003: Install GTK3 Development Libraries
- **Description:** Set up GTK3 and all dependencies for Windows development
- **Dependencies:** TASK-002
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Subtasks:**
  1. Install GTK3 via MSYS2 pacman: `pacman -S mingw-w64-x86_64-gtk3`
  2. Install GLib, Cairo, Pango, GDK-Pixbuf
  3. Verify pkg-config finds GTK3
  4. Test simple GTK3 "hello world" program

#### TASK-004: Build/Obtain PortAudio for Windows
- **Description:** Get PortAudio library working on Windows
- **Dependencies:** TASK-002
- **Estimate:** 6 hours
- **Priority:** P0
- **Status:** Not Started
- **Options:**
  1. Use MSYS2 package: `pacman -S mingw-w64-x86_64-portaudio`
  2. Build from source with WASAPI support
- **Deliverable:** portaudio.dll and headers available

#### TASK-005: Build/Obtain FFTW3 for Windows
- **Description:** Get FFTW3 library for Windows
- **Dependencies:** TASK-002
- **Estimate:** 3 hours
- **Priority:** P0
- **Status:** Not Started
- **Options:**
  1. Use MSYS2: `pacman -S mingw-w64-x86_64-fftw`
  2. Download prebuilt Windows binaries from fftw.org
- **Deliverable:** libfftw3.dll and headers

#### TASK-006: Set Up pthreads-win32
- **Description:** Install pthread library for Windows
- **Dependencies:** TASK-002
- **Estimate:** 2 hours
- **Priority:** P0
- **Status:** Not Started
- **Options:**
  1. Use MinGW's built-in pthread implementation
  2. Use pthreads-win32 library
- **Deliverable:** pthread.h and library available

#### TASK-007: Create Windows-Specific Makefile Configuration
- **Description:** Create Makefile or CMakeLists.txt that works on Windows
- **Dependencies:** TASK-001, TASK-002, TASK-003, TASK-004, TASK-005, TASK-006
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Subtasks:**
  1. Add Windows platform detection (`ifeq ($(OS),Windows_NT)`)
  2. Set Windows-specific compiler flags
  3. Set Windows-specific library paths
  4. Add conditional compilation: `-DPORTAUDIO -DGPIO=OFF`
  5. Exclude Linux-specific source files

#### TASK-008: Add Platform Detection Macros
- **Description:** Ensure code can detect Windows at compile time
- **Dependencies:** None
- **Estimate:** 2 hours
- **Priority:** P0
- **Status:** Not Started
- **Changes:**
  - Use `_WIN32`, `_WIN64`, `__MINGW32__`, `__MINGW64__` macros
  - Add checks in all platform-dependent code

---

### Milestone 1.2: Core Porting - Compatibility Layer

#### TASK-009: Create windows_compat.h Header File
- **Description:** Create central compatibility header for Windows-specific includes/defines
- **Dependencies:** TASK-007
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **File:** `src/windows_compat.h`
- **Contents:**
  ```c
  #ifndef WINDOWS_COMPAT_H
  #define WINDOWS_COMPAT_H

  #ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    // Socket compatibility
    #define close(s) closesocket(s)
    typedef int socklen_t;

    // Path separator
    #define PATH_SEPARATOR '\\'

    // Sleep function
    #define sleep(x) Sleep((x)*1000)
    #define usleep(x) Sleep((x)/1000)
  #else
    // POSIX includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define PATH_SEPARATOR '/'
  #endif

  #endif
  ```

#### TASK-010: Port Network Socket Code to Winsock2
- **Description:** Modify all network code to work with Winsock2
- **Dependencies:** TASK-009
- **Estimate:** 3 days
- **Priority:** P0
- **Status:** Not Started
- **Files to modify:**
  - `src/discovery.c`
  - `src/old_protocol.c`
  - `src/new_protocol.c`
  - `src/client_server.c`
  - `src/rigctl.c`
  - Any other files using sockets
- **Changes:**
  1. Add `#include "windows_compat.h"` to all network files
  2. Add `WSAStartup()` call at program initialization
  3. Add `WSACleanup()` call at program exit
  4. Replace `close()` with `closesocket()` for sockets
  5. Handle `SOCKET` type vs `int` file descriptor
  6. Test error handling (WSAGetLastError vs errno)

#### TASK-011: Disable GPIO Code for Windows
- **Description:** Ensure all GPIO code is conditionally compiled out on Windows
- **Dependencies:** TASK-007
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Files to check:**
  - `src/gpio.c` - wrap entire file in `#ifdef GPIO`
  - `src/encoder.c` - check GPIO dependencies
  - Any files including `<gpiod.h>`
- **Verification:** Grep for "gpiod", "GPIO", ensure none compiled on Windows

#### TASK-012: Disable I2C Code for Windows
- **Description:** Ensure all I2C code is excluded from Windows builds
- **Dependencies:** TASK-007
- **Estimate:** 2 hours
- **Priority:** P0
- **Status:** Not Started
- **Files to check:**
  - `src/i2c.c` - wrap in `#ifdef I2C`
  - Any files including `<i2c-dev.h>`, `<i2c/smbus.h>`
- **Verification:** Grep for "i2c", ensure none compiled on Windows

#### TASK-013: Disable SATURN Code for Windows
- **Description:** Ensure SATURN/G2 XDMA driver code is excluded
- **Dependencies:** TASK-007
- **Estimate:** 2 hours
- **Priority:** P0
- **Status:** Not Started
- **Files to check:**
  - `src/saturn.c` - wrap in `#ifdef SATURN`
- **Makefile:** Add `-DSATURN=OFF` for Windows

#### TASK-014: Fix File Path Handling
- **Description:** Make file paths work correctly on Windows (backslashes, %APPDATA%)
- **Dependencies:** TASK-009
- **Estimate:** 1 day
- **Priority:** P1
- **Status:** Not Started
- **Changes:**
  1. Replace hardcoded `/` with `G_DIR_SEPARATOR` or `PATH_SEPARATOR`
  2. Use GLib path functions: `g_build_filename()`, `g_get_user_config_dir()`
  3. Change config path from `~/.config/pihpsdr/` to `%APPDATA%\piHPSDR\`
  4. Update `src/property.c` for Windows paths

#### TASK-015: Verify WDSP Builds on Windows
- **Description:** Ensure WDSP library compiles cleanly on Windows
- **Dependencies:** TASK-002, TASK-005
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Notes:** WDSP already has `wdsp/linux_port.h` which maps Windows APIs - should work
- **Test:** Compile `wdsp/` subdirectory standalone on Windows

#### TASK-016: Initial Compilation Attempt
- **Description:** Try to compile entire project on Windows, document all errors
- **Dependencies:** TASK-007 through TASK-015
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Deliverable:** List of compilation errors and plan to fix them
- **Goal:** Get project to compile (may not link or run yet)

---

## PHASE 2: CORE FUNCTIONALITY (Weeks 4-6)

### Milestone 2.1: Audio Implementation

#### TASK-017: Enable PortAudio in Build
- **Description:** Configure build to use `src/portaudio.c` instead of `src/audio.c`
- **Dependencies:** TASK-004, TASK-016
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Makefile changes:**
  - Add `-DPORTAUDIO` flag for Windows
  - Link against PortAudio library
  - Exclude `src/audio.c` and `src/pulseaudio.c` from Windows build

#### TASK-018: Test Audio Device Enumeration
- **Description:** Verify PortAudio can list Windows audio devices
- **Dependencies:** TASK-017
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Test program:**
  ```c
  // Simple test to list PortAudio devices on Windows
  Pa_Initialize();
  int numDevices = Pa_GetDeviceCount();
  for(int i=0; i<numDevices; i++) {
      const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
      printf("%d: %s\n", i, info->name);
  }
  ```

#### TASK-019: Test RX Audio Output (Mono/Single RX)
- **Description:** Get basic RX audio playing through Windows speakers
- **Dependencies:** TASK-018
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Steps:**
  1. Connect to a radio (or use simulator)
  2. Start single receiver
  3. Verify audio plays through default Windows device
  4. Check for dropouts, glitches
  5. Monitor latency

#### TASK-020: Test TX Microphone Input
- **Description:** Verify microphone input works on Windows
- **Dependencies:** TASK-019
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Steps:**
  1. Select microphone device in piHPSDR
  2. Enable transmit (PTT)
  3. Verify mic audio reaches radio
  4. Check for clipping, distortion
  5. Test with USB and onboard microphones

#### TASK-021: Test Multiple Audio Backends (WDM, DirectSound, WASAPI)
- **Description:** Test all PortAudio backends available on Windows
- **Dependencies:** TASK-019, TASK-020
- **Estimate:** 1 day
- **Priority:** P1
- **Status:** Not Started
- **Test matrix:**
  - WDM: Basic compatibility test
  - DirectSound: Compatibility and latency test
  - WASAPI Shared: Default mode test
  - WASAPI Exclusive: Low-latency CW test
- **Document:** Which backend works best for each use case

#### TASK-022: Optimize Audio Buffer Sizes
- **Description:** Tune buffer sizes for optimal latency without dropouts
- **Dependencies:** TASK-021
- **Estimate:** 1 day
- **Priority:** P1
- **Status:** Not Started
- **Test configurations:**
  - Buffer size: 64, 128, 256, 512 samples
  - Measure latency with loopback test
  - Find sweet spot: lowest latency without glitches
- **Target:** <20ms for CW sidetone

#### TASK-023: Test Dual Receiver Audio
- **Description:** Verify two independent audio streams work
- **Dependencies:** TASK-019
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Test:**
  - Start two receivers
  - Assign different audio output devices (if available)
  - Verify both play simultaneously without interference

---

### Milestone 2.2: Radio Protocol Support

#### TASK-024: Test Old Protocol (P1) Discovery
- **Description:** Verify P1 radios can be discovered on Windows network
- **Dependencies:** TASK-010, TASK-016
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Radios:** Hermes, Metis, legacy HPSDR hardware
- **Test:** Run discovery, verify radios appear in list

#### TASK-025: Test New Protocol (P2) Discovery
- **Description:** Verify P2 radios can be discovered on Windows network
- **Dependencies:** TASK-010, TASK-016
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Radios:** Angelia, Orion, Orion2, HermesLite-II, ANAN radios
- **Test:** Run discovery, verify radios appear with correct capabilities

#### TASK-026: Test RX Functionality (Single Receiver)
- **Description:** End-to-end test of receiving on Windows
- **Dependencies:** TASK-019, TASK-024, TASK-025
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Test steps:**
  1. Connect to radio
  2. Tune to active frequency
  3. Verify spectrum/waterfall display
  4. Verify audio output
  5. Change bands - verify correct filters
  6. Change modes (USB, LSB, CW, AM, FM)

#### TASK-027: Test TX Functionality
- **Description:** End-to-end test of transmitting on Windows
- **Dependencies:** TASK-020, TASK-026
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Test steps:**
  1. Configure microphone
  2. Set low power (5W for safety)
  3. Key PTT (software button or CAT)
  4. Verify TX indicator
  5. Monitor transmitted signal (2nd receiver or spectrum analyzer)
  6. Test voice modes (USB, LSB, AM, FM)

#### TASK-028: Test CW Keying
- **Description:** Test CW transmission on Windows
- **Dependencies:** TASK-027
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Test methods:**
  - Software CW keyer (if available)
  - CAT control CW keying
  - Test dit/dah timing
  - Monitor transmitted signal

#### TASK-029: Test Mode Switching
- **Description:** Verify all modes work correctly on Windows
- **Dependencies:** TASK-026, TASK-027
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Modes to test:**
  - CWU, CWL
  - USB, LSB
  - AM (with carrier control)
  - FM (wide and narrow)
  - DIGU, DIGL (if supported)

#### TASK-030: Test Band Switching and Filters
- **Description:** Verify band changes work correctly
- **Dependencies:** TASK-026
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Tests:**
  - Switch through all amateur bands (160m - 6m)
  - Verify correct filters engage
  - Verify VFO frequency limits
  - Test band stacking memory

---

### Milestone 2.3: UI Testing

#### TASK-031: Launch GTK3 Main Window on Windows
- **Description:** Get piHPSDR GUI to appear on Windows
- **Dependencies:** TASK-003, TASK-016
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Verify:**
  - Window appears without crashes
  - Title bar shows "piHPSDR"
  - Window is resizable
  - All widgets visible (no missing elements)

#### TASK-032: Test All Menus
- **Description:** Click through every menu item, verify functionality
- **Dependencies:** TASK-031
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Menus to test:**
  - Radio menu (start/stop/discover)
  - Band menu (band selection)
  - Mode menu (mode selection)
  - Filter menu (filter selection)
  - VFO menu
  - Settings menus
  - About dialog

#### TASK-033: Test Spectrum and Waterfall Display
- **Description:** Verify spectrum/waterfall renders correctly on Windows
- **Dependencies:** TASK-026, TASK-031
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Verify:**
  - Spectrum panadapter displays
  - Waterfall scrolls smoothly
  - Signal peaks visible
  - Filter overlay displayed
  - No graphical glitches or tearing

#### TASK-034: Test VFO Controls
- **Description:** Verify VFO tuning works on Windows
- **Dependencies:** TASK-031
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Test:**
  - Mouse wheel tuning
  - Click-to-tune on spectrum
  - Direct frequency entry
  - VFO A/B switching
  - VFO split mode
  - RIT/XIT controls

#### TASK-035: Test Meters (S-meter, Power meter)
- **Description:** Verify all meters display correctly
- **Dependencies:** TASK-026, TASK-031
- **Estimate:** 2 hours
- **Priority:** P1
- **Status:** Not Started
- **Meters:**
  - S-meter (RX signal strength)
  - Power output meter (TX)
  - SWR meter (TX)
  - ALC meter

#### TASK-036: Test Screen Resizing and Full-Screen Mode
- **Description:** Verify UI adapts to different window sizes
- **Dependencies:** TASK-031
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Tests:**
  - Resize window - verify elements reflow
  - Maximize window
  - Enter/exit full-screen mode (F11)
  - Test different screen resolutions
  - Test with high-DPI displays (if available)

#### TASK-037: Test Sliders and Controls
- **Description:** Verify all interactive controls work
- **Dependencies:** TASK-031
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Controls to test:**
  - AF Gain slider
  - RF Gain slider
  - AGC control
  - Preamp/Attenuator buttons
  - NB/NR/ANF controls
  - Drive/Tune power controls
  - Toolbar buttons

---

## PHASE 3: ADVANCED FEATURES (Weeks 7-9)

### Milestone 3.1: Multi-Receiver Support

#### TASK-038: Test Dual Receiver Configuration
- **Description:** Verify two receivers work simultaneously on Windows
- **Dependencies:** TASK-023, TASK-026
- **Estimate:** 1 day
- **Priority:** P1
- **Status:** Not Started
- **Test:**
  - Start second receiver
  - Tune to different frequencies
  - Verify both audio outputs work
  - Switch active receiver
  - Verify independent controls

#### TASK-039: Test Receiver Switching
- **Description:** Verify switching between RX1 and RX2 works
- **Dependencies:** TASK-038
- **Estimate:** 2 hours
- **Priority:** P1
- **Status:** Not Started

---

### Milestone 3.2: CW Optimization

#### TASK-040: Measure CW Sidetone Latency
- **Description:** Accurately measure CW sidetone latency on Windows
- **Dependencies:** TASK-022, TASK-028
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Method:**
  - Audio loopback test with oscilloscope
  - Software latency measurement tool
  - Compare with macOS baseline (~15ms)
- **Target:** <20ms, ideally 15ms

#### TASK-041: Optimize CW Buffer Sizes
- **Description:** Tune audio buffers specifically for CW mode
- **Dependencies:** TASK-040
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Strategy:**
  - Use existing CW ring buffer logic from portaudio.c
  - Test with WASAPI Exclusive mode
  - Adjust MY_CW_LOW_WATER and MY_CW_HIGH_WATER constants
  - Test with actual CW keying

#### TASK-042: Test CW Keying Responsiveness
- **Description:** Subjective test of CW feel and timing
- **Dependencies:** TASK-041
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Get feedback from CW operators on:**
  - Sidetone delay perception
  - Dit/dah timing accuracy
  - QSK (full break-in) if supported

---

### Milestone 3.3: Remote Operation

#### TASK-043: Test Client/Server Mode - Server Side
- **Description:** Run piHPSDR as server on Windows
- **Dependencies:** TASK-010, TASK-026
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Test:**
  - Start piHPSDR in server mode
  - Verify listens on network port
  - Connect client from another machine
  - Verify client can control radio

#### TASK-044: Test Client/Server Mode - Client Side
- **Description:** Run piHPSDR as client on Windows connecting to remote server
- **Dependencies:** TASK-010
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Test:**
  - Connect to remote piHPSDR server (Linux or macOS)
  - Verify RX audio streams over network
  - Verify TX audio sent to server
  - Test controls work remotely

#### TASK-045: Test Rigctl Protocol
- **Description:** Verify Hamlib rigctl compatibility on Windows
- **Dependencies:** TASK-026
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Test with:**
  - WSJT-X on Windows
  - fldigi on Windows
  - gpredict (if Windows version available)
  - Manual rigctl commands

#### TASK-046: Test TCI Protocol
- **Description:** Verify TCI (Transceiver Control Interface) works on Windows
- **Dependencies:** TASK-026
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Test with:** Compatible logging/contest software

---

### Milestone 3.4: Optional Features

#### TASK-047: Implement Windows MIDI Support (Optional)
- **Description:** Add MIDI controller support using Windows MIDI API
- **Dependencies:** TASK-031
- **Estimate:** 3 days
- **Priority:** P2
- **Status:** Not Started
- **Approach:**
  - Create `src/windows_midi.c` (analogous to `src/alsa_midi.c` and `src/mac_midi.c`)
  - Use Windows Multimedia API (mmsystem.h)
  - Test with USB MIDI controller

#### TASK-048: Test SoapySDR on Windows (Optional)
- **Description:** If SoapySDR Windows builds available, test integration
- **Dependencies:** TASK-026
- **Estimate:** 1 day
- **Priority:** P2
- **Status:** Not Started
- **Radios to test:**
  - RTL-SDR
  - HackRF
  - SDRPlay
  - PlutoSDR

#### TASK-049: Test USB OZY Support (Optional)
- **Description:** Test legacy USB OZY radios on Windows
- **Dependencies:** TASK-026
- **Estimate:** 1 day
- **Priority:** P3
- **Status:** Not Started
- **Requires:** libusb-1.0 for Windows

---

## PHASE 4: POLISH & RELEASE (Weeks 10-12)

### Milestone 4.1: Testing & Bug Fixes

#### TASK-050: Extended Stability Testing (24-Hour Test)
- **Description:** Run piHPSDR continuously for 24+ hours monitoring for crashes/leaks
- **Dependencies:** All Phase 2 and 3 tasks
- **Estimate:** 3 days (mostly automated)
- **Priority:** P0
- **Status:** Not Started
- **Monitor:**
  - Memory usage over time (check for leaks)
  - CPU usage stability
  - Audio dropouts
  - UI responsiveness
  - Log all errors/warnings

#### TASK-051: Test on Windows 10
- **Description:** Full test suite on Windows 10 (multiple versions if possible)
- **Dependencies:** All core functionality tasks
- **Estimate:** 2 days
- **Priority:** P0
- **Status:** Not Started
- **Versions:**
  - Windows 10 21H2 (minimum recommended)
  - Windows 10 22H2 (latest)
  - Windows 10 LTSC (if available)

#### TASK-052: Test on Windows 11
- **Description:** Full test suite on Windows 11
- **Dependencies:** All core functionality tasks
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Versions:**
  - Windows 11 22H2
  - Windows 11 23H2

#### TASK-053: Test with Various Audio Hardware
- **Description:** Test with different audio interfaces
- **Dependencies:** TASK-019, TASK-020
- **Estimate:** 2 days
- **Priority:** P0
- **Status:** Not Started
- **Hardware to test:**
  - Built-in Realtek audio
  - USB audio interface (generic)
  - Professional USB interface (if available)
  - HDMI audio output
  - Bluetooth audio (stretch - likely high latency)

#### TASK-054: Test with Multiple Radio Types
- **Description:** Verify compatibility with all supported HPSDR radios
- **Dependencies:** TASK-024, TASK-025
- **Estimate:** 3 days
- **Priority:** P0
- **Status:** Not Started
- **Radios (if available):**
  - Hermes (P1)
  - HermesLite-II (P2)
  - Angelia (P2)
  - Orion/Orion2 (P2)
  - ANAN-10/100/200 (P2)
  - Use hpsdrsim simulator for unavailable hardware

#### TASK-055: Fix All Critical Bugs
- **Description:** Address all P0 and P1 bugs found during testing
- **Dependencies:** TASK-050 through TASK-054
- **Estimate:** 2 weeks (ongoing)
- **Priority:** P0
- **Status:** Not Started
- **Process:**
  - Triage bugs by severity
  - Fix crashes first
  - Fix data corruption issues
  - Fix major functional issues
  - Document known minor issues for future releases

#### TASK-056: Performance Profiling and Optimization
- **Description:** Profile CPU and memory usage, optimize bottlenecks
- **Dependencies:** TASK-050
- **Estimate:** 3 days
- **Priority:** P1
- **Status:** Not Started
- **Tools:**
  - Visual Studio Profiler (if using MSVC)
  - gprof (if using MinGW)
  - Windows Performance Analyzer
- **Target:** <25% CPU single RX, <50% dual RX

---

### Milestone 4.2: Documentation

#### TASK-057: Write Windows Installation Guide
- **Description:** Step-by-step guide for installing piHPSDR on Windows
- **Dependencies:** TASK-064 (installer ready)
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **File:** `docs/Windows_Installation_Guide.md`
- **Contents:**
  1. System requirements
  2. Download links
  3. Installation steps (with screenshots)
  4. First run setup
  5. Connecting to a radio
  6. Basic operation

#### TASK-058: Write Windows Configuration Guide
- **Description:** Document Windows-specific configuration options
- **Dependencies:** TASK-031
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **File:** `docs/Windows_Configuration.md`
- **Contents:**
  1. Audio device selection (WDM vs WASAPI)
  2. Firewall configuration
  3. Network adapter selection
  4. Config file location (%APPDATA%\piHPSDR\)
  5. Performance tuning tips

#### TASK-059: Write Windows Troubleshooting Guide
- **Description:** Common issues and solutions for Windows users
- **Dependencies:** TASK-050 through TASK-054
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **File:** `docs/Windows_Troubleshooting.md`
- **Contents:**
  1. Radio not discovered (firewall, wrong network adapter)
  2. No audio (device selection, sample rate)
  3. Audio dropouts (buffer size, WASAPI mode)
  4. High CPU usage
  5. DLL errors / missing dependencies
  6. GTK theme issues

#### TASK-060: Update Main README with Windows Support
- **Description:** Update project README to reflect Windows support
- **Dependencies:** TASK-057, TASK-058, TASK-059
- **Estimate:** 2 hours
- **Priority:** P0
- **Status:** Not Started
- **Changes:**
  - Add Windows to supported platforms list
  - Add Windows installation section
  - Link to Windows-specific docs
  - Update screenshots (include Windows)

#### TASK-061: Create CHANGELOG Entry for Windows Port
- **Description:** Document Windows port in CHANGELOG
- **Dependencies:** All Phase 4 tasks
- **Estimate:** 1 hour
- **Priority:** P1
- **Status:** Not Started
- **Entry should include:**
  - Major features working on Windows
  - Known limitations (GPIO, I2C excluded)
  - Performance notes
  - Installation instructions reference

---

### Milestone 4.3: Packaging

#### TASK-062: Create NSIS Installer Script
- **Description:** Write NSIS script to create Windows installer
- **Dependencies:** All Phase 2 and 3 tasks complete
- **Estimate:** 2 days
- **Priority:** P0
- **Status:** Not Started
- **File:** `windows/pihpsdr-installer.nsi`
- **Installer must include:**
  1. piHPSDR executable
  2. All GTK3 DLLs (gtk-3-0.dll, glib-2.0.dll, cairo.dll, etc.)
  3. PortAudio DLL
  4. FFTW3 DLL
  5. WDSP library
  6. pthread DLL
  7. Any other runtime dependencies
  8. Default configuration files
  9. README and license
  10. Start menu shortcuts
  11. Desktop shortcut (optional)
  12. Uninstaller

#### TASK-063: Bundle All DLL Dependencies
- **Description:** Identify and collect all required DLLs for distribution
- **Dependencies:** TASK-016 (compilation complete)
- **Estimate:** 1 day
- **Priority:** P0
- **Status:** Not Started
- **Method:**
  - Run `ldd pihpsdr.exe` or use Dependency Walker
  - Copy all non-system DLLs to installer directory
  - Verify on clean Windows VM (no MSYS2/dev tools)
- **Common DLLs:**
  - GTK: gtk-3-0.dll, gdk-3-0.dll, glib-2.0.dll, gobject-2.0.dll, gio-2.0.dll
  - Cairo: cairo.dll, cairo-gobject.dll
  - Pango: pango-1.0.dll, pangocairo-1.0.dll, pangowin32-1.0.dll
  - Graphics: gdk-pixbuf-2.0.dll, libpng16.dll, libjpeg-8.dll
  - Other: libfftw3-3.dll, portaudio.dll, libwinpthread-1.dll
  - MinGW runtime: libgcc_s_seh-1.dll, libstdc++-6.dll

#### TASK-064: Test Installer on Clean Windows 10 VM
- **Description:** Install on clean Windows 10 with no dev tools
- **Dependencies:** TASK-062, TASK-063
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **VM Setup:**
  - Fresh Windows 10 21H2 or later
  - No MSYS2, no Visual Studio, no development tools
  - Standard user account (not admin)
- **Test:**
  1. Run installer
  2. Launch piHPSDR from Start menu
  3. Verify all features work
  4. Uninstall and verify clean removal

#### TASK-065: Test Installer on Clean Windows 11 VM
- **Description:** Install on clean Windows 11
- **Dependencies:** TASK-062, TASK-063
- **Estimate:** 4 hours
- **Priority:** P0
- **Status:** Not Started
- **Same as TASK-064 but on Windows 11**

#### TASK-066: Create Portable ZIP Distribution
- **Description:** Create portable .zip for users who don't want installer
- **Dependencies:** TASK-063
- **Estimate:** 4 hours
- **Priority:** P1
- **Status:** Not Started
- **Contents:**
  - pihpsdr.exe
  - All DLLs in same directory
  - README.txt (portable instructions)
  - LICENSE.txt
  - Sample configuration (optional)
- **Test:** Extract on clean machine, run from arbitrary location

#### TASK-067: Sign Binaries (Optional)
- **Description:** Code-sign executable and installer to avoid SmartScreen warnings
- **Dependencies:** TASK-062, TASK-066
- **Estimate:** 1 day
- **Priority:** P2
- **Status:** Not Started
- **Requires:**
  - Code signing certificate (costs money or use free options)
  - SignTool.exe from Windows SDK
- **Note:** Can be deferred to post-release if budget/time limited

---

### Milestone 4.4: Release

#### TASK-068: Create GitHub Release Tag
- **Description:** Tag release in Git repository
- **Dependencies:** All Phase 4 tasks complete
- **Estimate:** 1 hour
- **Priority:** P0
- **Status:** Not Started
- **Version:** e.g., `v1.0-windows` or follow existing scheme
- **Tag message:** Windows port release notes

#### TASK-069: Upload Windows Binaries to GitHub Releases
- **Description:** Attach installer and portable ZIP to GitHub release
- **Dependencies:** TASK-068, TASK-064, TASK-065, TASK-066
- **Estimate:** 1 hour
- **Priority:** P0
- **Status:** Not Started
- **Files to upload:**
  - `piHPSDR-1.0-Windows-Setup.exe` (installer)
  - `piHPSDR-1.0-Windows-Portable.zip` (portable)
  - `piHPSDR-1.0-Windows-Setup.exe.sha256` (checksum)
  - `piHPSDR-1.0-Windows-Portable.zip.sha256` (checksum)

#### TASK-070: Write Release Announcement
- **Description:** Prepare announcement text for Windows release
- **Dependencies:** TASK-068
- **Estimate:** 2 hours
- **Priority:** P0
- **Status:** Not Started
- **Contents:**
  - What's new (Windows support!)
  - Download links
  - Installation instructions
  - Known limitations (GPIO/I2C excluded)
  - Link to documentation
  - Call for testing and feedback

#### TASK-071: Announce on piHPSDR Mailing List / Forums
- **Description:** Post release announcement to community
- **Dependencies:** TASK-070
- **Estimate:** 1 hour
- **Priority:** P0
- **Status:** Not Started
- **Channels:**
  - piHPSDR Google Group (if exists)
  - HermesLite user group
  - TAPR HPSDR mailing list
  - Reddit r/amateurradio (if appropriate)
  - QRZ forums

#### TASK-072: Monitor Initial Feedback and Bug Reports
- **Description:** Actively monitor for post-release issues
- **Dependencies:** TASK-071
- **Estimate:** Ongoing (first 2 weeks critical)
- **Priority:** P0
- **Status:** Not Started
- **Actions:**
  - Check GitHub Issues daily
  - Respond to mailing list questions
  - Triage new bug reports
  - Create hotfix releases if critical bugs found

#### TASK-073: Create Post-Release Bug Fix Plan
- **Description:** Plan for addressing post-release issues
- **Dependencies:** TASK-072
- **Estimate:** Ongoing
- **Priority:** P1
- **Status:** Not Started
- **Strategy:**
  - Hotfix (1.0.1) for critical bugs within 1 week
  - Minor release (1.1) with bug fixes and improvements within 1-3 months
  - Maintain Windows support parity with Linux/macOS going forward

---

## ADDITIONAL TASKS (Not in PRD Phases)

### Build System and CI/CD

#### TASK-074: Set Up GitHub Actions for Windows Builds
- **Description:** Automate Windows builds using GitHub Actions CI/CD
- **Dependencies:** TASK-062, TASK-063
- **Estimate:** 2 days
- **Priority:** P2
- **Status:** Not Started
- **Workflow:**
  1. Trigger on push/pull request
  2. Set up MSYS2 environment
  3. Install dependencies
  4. Compile piHPSDR
  5. Run basic tests
  6. Create installer artifact
  7. Upload artifact for download

#### TASK-075: Create Windows Build Instructions for Developers
- **Description:** Document how to build from source on Windows
- **Dependencies:** TASK-007
- **Estimate:** 1 day
- **Priority:** P1
- **Status:** Not Started
- **File:** `docs/Building_on_Windows.md`
- **Contents:**
  1. Install MSYS2
  2. Install dependencies (step-by-step pacman commands)
  3. Clone repository
  4. Build WDSP
  5. Build piHPSDR
  6. Troubleshooting common build errors

---

## SUMMARY STATISTICS

### By Priority
- **P0 (Critical):** 50 tasks
- **P1 (High):** 18 tasks
- **P2 (Medium):** 5 tasks
- **P3 (Low):** 2 tasks

### By Phase
- **Phase 1 (Foundation):** 16 tasks
- **Phase 2 (Core Functionality):** 21 tasks
- **Phase 3 (Advanced Features):** 10 tasks
- **Phase 4 (Polish & Release):** 27 tasks
- **Additional:** 2 tasks

### Total Estimated Effort
- **Total tasks:** 76
- **Estimated effort:** ~12 weeks for 1 developer (as per PRD)
- **Can be parallelized:** Some tasks can run concurrently with 2+ developers

---

## DEPENDENCY GRAPH (Critical Path)

```
TASK-001 (Build eval)
  └─> TASK-002 (Dev env)
       ├─> TASK-003 (GTK3)
       ├─> TASK-004 (PortAudio)
       ├─> TASK-005 (FFTW3)
       ├─> TASK-006 (pthreads)
       └─> TASK-007 (Makefile)
            ├─> TASK-009 (compat.h)
            │    └─> TASK-010 (Winsock)
            ├─> TASK-011 (disable GPIO)
            ├─> TASK-012 (disable I2C)
            ├─> TASK-013 (disable SATURN)
            ├─> TASK-014 (paths)
            ├─> TASK-015 (WDSP)
            └─> TASK-016 (compile!)
                 ├─> TASK-017 (enable PortAudio)
                 │    └─> TASK-018 (audio enum)
                 │         └─> TASK-019 (RX audio)
                 │              └─> TASK-026 (RX test)
                 │                   └─> TASK-050 (stability)
                 │                        └─> TASK-062 (installer)
                 │                             └─> TASK-068 (release!)
                 └─> TASK-024/025 (discovery)
```

---

## TRACKING SPREADSHEET TEMPLATE

| Task ID | Description | Priority | Estimate | Assignee | Status | Completed Date | Notes |
|---------|-------------|----------|----------|----------|--------|----------------|-------|
| TASK-001 | Evaluate build options | P0 | 4h | | Not Started | | |
| TASK-002 | Set up dev environment | P0 | 8h | | Not Started | | |
| ... | ... | ... | ... | ... | ... | ... | ... |

---

## RISK MITIGATION TASKS

### If WASAPI Latency is Too High
- **Fallback:** Research alternative low-latency audio APIs
- **Task:** TASK-ALT-01: Investigate Windows Audio Session API (WASAPI2)
- **Estimate:** 3 days

### If GTK3 Has Rendering Issues
- **Fallback:** Consider GTK2 (older but more tested on Windows)
- **Task:** TASK-ALT-02: Test GTK2 compatibility
- **Estimate:** 1 week

### If Winsock Compatibility is Hard
- **Fallback:** Use cross-platform socket library (e.g., SDL_net, ZeroMQ)
- **Task:** TASK-ALT-03: Evaluate socket abstraction libraries
- **Estimate:** 2 days

---

## NOTES

- **Iterative Approach:** Each phase builds on the previous; don't skip ahead
- **Testing is Critical:** Allocate adequate time for testing on real hardware
- **Community Involvement:** Engage beta testers early (Phase 3)
- **Document Everything:** Future maintainers will thank you
- **Expect the Unexpected:** Buffer 20% extra time for unforeseen issues

---

**END OF TASK LIST**

**Last Updated:** 2025-11-06
**Status:** Ready for execution
**Next Step:** Assign TASK-001 and begin Phase 1
