# PRD: piHPSDR Windows Porting

**Document Version:** 1.0
**Date:** 2025-11-06
**Author:** Claude (AI Assistant)
**Status:** Draft

---

## 1. EXECUTIVE SUMMARY

### 1.1 Project Overview
This document defines requirements for porting **piHPSDR** (High Performance Software Defined Radio host program) to the Windows platform. piHPSDR is currently a mature Linux/macOS application written in C with GTK3 UI, supporting multiple HPSDR radio protocols and extensive hardware integration.

### 1.2 Current State
- **Primary Platforms:** Linux (Raspberry Pi + desktop), macOS (Intel/Apple Silicon)
- **Codebase:** ~84,000 lines of C code (106 .c files)
- **UI Framework:** GTK+ 3.0 (4,478+ function calls across 56 files)
- **Build System:** GNU Make with conditional compilation
- **No existing Windows support:** Zero WIN32/MinGW code paths in main application

### 1.3 Business Case
Windows porting will:
- Expand user base to Windows radio amateurs (largest desktop OS market share)
- Reduce barrier to entry (many HPSDR users operate Windows-only stations)
- Maintain feature parity with Linux/macOS versions
- Enable cross-platform development and testing

---

## 2. PROJECT SCOPE

### 2.1 In Scope

#### Core Functionality (MUST HAVE)
1. **Radio Protocol Support**
   - Old Protocol (P1) - legacy HPSDR
   - New Protocol (P2) - current HPSDR with DDC/DUC
   - SoapySDR framework integration

2. **Hardware Discovery & Communication**
   - Network-based radio discovery
   - TCP/IP communication with HPSDR devices
   - Support for all current radio types (Hermes, Angelia, Orion, HermesLite-II, etc.)

3. **Signal Processing**
   - RX/TX DSP chain (DDC, DUC, AGC, filters)
   - WDSP library integration (already has Windows compatibility layer)
   - Multiple receiver support
   - CW/SSB/AM/FM modes
   - PureSignal TX feedback correction

4. **User Interface**
   - All GTK3-based menus, dialogs, controls
   - Spectrum/waterfall visualization
   - VFO controls and displays
   - S-meter and power meter
   - Screen resizing and full-screen mode
   - Customizable sliders and toolbar

5. **Audio Subsystem** ⭐ CRITICAL
   - RX audio output (stereo, 48kHz)
   - TX microphone input (mono, 48kHz)
   - Low-latency CW sidetone (<20ms target)
   - Audio device enumeration and selection
   - Ring buffer management for RX/TX
   - **Windows Implementation:** Use PortAudio library (already proven on macOS)

6. **Remote Operation**
   - Client/server architecture
   - Network audio streaming
   - Rigctl protocol compatibility
   - TCI (Transceiver Control Interface)

7. **Build System**
   - Windows-native build configuration
   - MinGW-w64 or MSVC toolchain support
   - Automated dependency management
   - Installer creation (NSIS or WiX)

#### Nice to Have
- MIDI controller support (Windows MIDI API)
- SoapySDR support (if Windows builds available)
- USB OZY support (libusb-1.0 for Windows)

### 2.2 Out of Scope / Explicitly Excluded

#### ⛔ Hardware-Specific Features (MUST BE DISABLED ON WINDOWS)

1. **GPIO (General Purpose I/O) - EXCLUDED**
   - Raspberry Pi hardware control
   - Encoder/knob support via GPIO
   - PTT input/output via GPIO pins
   - CW paddle input via GPIO
   - **Rationale:** Windows PCs lack GPIO hardware; feature is RasPi-specific
   - **Implementation:** Compile-time flag `-DGPIO=OFF` for Windows builds

2. **I2C (Inter-Integrated Circuit) - EXCLUDED**
   - MCP23017 expander support
   - Switch control matrix
   - Direct hardware access via `/dev/i2c-*`
   - **Rationale:** Windows lacks I2C device interfaces; feature is Linux embedded-specific
   - **Implementation:** Exclude all I2C code from Windows builds

3. **SATURN/G2 XDMA Native Driver - EXCLUDED**
   - Direct XDMA kernel driver access
   - **Rationale:** Requires Linux kernel module
   - **Implementation:** `-DSATURN=OFF` for Windows

4. **Text-to-Speech (TTS) - DEFERRED**
   - Linux espeak integration
   - **Note:** Could be implemented later with Windows SAPI, but not priority

### 2.3 Dependencies to Port

#### Required Libraries
| Library | Linux | macOS | Windows Strategy |
|---------|-------|-------|------------------|
| **GTK+ 3.0** | Native | Homebrew | MSYS2/vcpkg |
| **Audio** | ALSA/PulseAudio | PortAudio | **PortAudio** (proven) |
| **WDSP** | Internal build | Internal | Internal (has Windows compat layer) |
| **FFTW3** | System | System | MSYS2/vcpkg |
| **Pthreads** | Native | Native | pthreads-win32 or C11 threads |
| **Networking** | POSIX sockets | POSIX | Winsock2 (minor changes) |
| **MIDI** | ALSA MIDI | CoreMIDI | Windows MIDI API (optional) |

---

## 3. TECHNICAL REQUIREMENTS

### 3.1 Architecture Requirements

#### AR-01: Cross-Platform Build System
- **Priority:** P0 (Critical)
- **Description:** Extend Makefile or migrate to CMake to support Windows builds
- **Acceptance Criteria:**
  - Single command builds on Windows (e.g., `make` or `cmake --build`)
  - Automatic detection of Windows platform
  - Conditional compilation flags working correctly
  - All dependencies auto-detected or documented

#### AR-02: Audio Abstraction Layer
- **Priority:** P0 (Critical)
- **Description:** Use PortAudio consistently across all platforms
- **Rationale:** macOS already uses PortAudio successfully; proven cross-platform
- **Acceptance Criteria:**
  - `src/portaudio.c` compiled on Windows with `-DPORTAUDIO` flag
  - ALSA code (`src/audio.c`) excluded on Windows
  - Audio device enumeration works on Windows
  - Latency targets: RX <200ms, CW sidetone <20ms
  - No audio dropouts or buffer overruns under normal operation

#### AR-03: Network Socket Compatibility
- **Priority:** P0 (Critical)
- **Description:** Abstract POSIX sockets to support Winsock2
- **Changes Required:**
  - Replace `#include <sys/socket.h>` with Windows compatibility header
  - Use `closesocket()` instead of `close()` for sockets
  - Initialize Winsock with `WSAStartup()` on Windows
  - Handle `SOCKET` type (UINT_PTR) vs `int` file descriptor
- **Files Affected:** Network protocol files (`old_protocol.c`, `new_protocol.c`, `discovery.c`, etc.)

#### AR-04: Threading Compatibility
- **Priority:** P0 (Critical)
- **Description:** Ensure pthread code works on Windows
- **Options:**
  1. Use pthreads-win32 library (drop-in replacement)
  2. Migrate to C11 `<threads.h>` (modern, cross-platform)
- **Recommendation:** pthreads-win32 (minimal code changes)

#### AR-05: File Path Handling
- **Priority:** P1 (High)
- **Description:** Handle Windows path separators and conventions
- **Changes:**
  - Use `G_DIR_SEPARATOR` from GLib instead of hardcoded `/`
  - Config files in `%APPDATA%\piHPSDR\` instead of `~/.config/`
  - Handle both `/` and `\` gracefully

### 3.2 User Interface Requirements

#### UI-01: GTK3 on Windows
- **Priority:** P0 (Critical)
- **Description:** GTK3 must render correctly on Windows 10/11
- **Dependencies:**
  - GTK 3.24.x from MSYS2 or vcpkg
  - GLib, Cairo, Pango, GDK-Pixbuf
- **Acceptance Criteria:**
  - All menus, dialogs, widgets render correctly
  - Native Windows theme integration (optional but nice)
  - No graphical glitches or artifacts
  - Resizing and full-screen mode work

#### UI-02: High-DPI Display Support
- **Priority:** P2 (Medium)
- **Description:** Support Windows high-DPI scaling (125%, 150%, 200%)
- **Implementation:** GTK3 handles this automatically if configured correctly

### 3.3 Audio Requirements

#### AU-01: PortAudio Integration
- **Priority:** P0 (Critical)
- **Description:** Use existing `src/portaudio.c` implementation
- **Testing:**
  - Test with various Windows audio devices (Realtek, USB, ASIO)
  - Verify 48kHz sample rate support
  - Test with WDM, WASAPI, and DirectSound backends

#### AU-02: Low-Latency CW Operation
- **Priority:** P0 (Critical)
- **Description:** CW sidetone latency must be <20ms (as on macOS)
- **Implementation:**
  - Use existing ring buffer logic from `portaudio.c:96-97`
  - Use small audio buffer size (128 samples as in macOS)
  - Prefer WASAPI exclusive mode if available

#### AU-03: Audio Device Persistence
- **Priority:** P1 (High)
- **Description:** Remember selected audio devices across sessions
- **Implementation:** Save device names in configuration file

### 3.4 Build and Distribution Requirements

#### BD-01: Installer Creation
- **Priority:** P0 (Critical)
- **Description:** Windows installer for easy deployment
- **Options:**
  1. NSIS (Nullsoft Scriptable Install System)
  2. WiX Toolset (MSI installer)
  3. Inno Setup
- **Recommendation:** NSIS (simple, widely used)
- **Must Include:**
  - All GTK3 DLLs and dependencies
  - WDSP library
  - PortAudio DLL
  - FFTW3 library
  - Default configuration
  - Uninstaller

#### BD-02: Portable Build
- **Priority:** P1 (High)
- **Description:** Optionally create portable .zip distribution
- **Contents:** All DLLs + executable in single folder (no installation)

#### BD-03: Debug Symbols
- **Priority:** P2 (Medium)
- **Description:** Separate PDB files for crash debugging
- **Distribution:** Optional download for developers

---

## 4. FUNCTIONAL REQUIREMENTS

### 4.1 Core Functionality Matrix

| Feature | Linux | macOS | Windows Target | Priority |
|---------|-------|-------|----------------|----------|
| Old Protocol (P1) | ✅ | ✅ | ✅ | P0 |
| New Protocol (P2) | ✅ | ✅ | ✅ | P0 |
| SoapySDR | ✅ | ✅ | ✅ (if available) | P1 |
| Multi-RX support | ✅ | ✅ | ✅ | P0 |
| CW/SSB/AM/FM modes | ✅ | ✅ | ✅ | P0 |
| Spectrum/Waterfall | ✅ | ✅ | ✅ | P0 |
| RX audio output | ✅ | ✅ | ✅ | P0 |
| TX microphone input | ✅ | ✅ | ✅ | P0 |
| CW sidetone | ✅ | ✅ | ✅ | P0 |
| PureSignal | ✅ | ✅ | ✅ | P0 |
| Remote operation | ✅ | ✅ | ✅ | P0 |
| Rigctl protocol | ✅ | ✅ | ✅ | P1 |
| TCI support | ✅ | ✅ | ✅ | P1 |
| MIDI support | ✅ | ✅ | ⚠️ (optional) | P2 |
| **GPIO support** | ✅ | ❌ | **❌ EXCLUDED** | N/A |
| **I2C support** | ✅ | ❌ | **❌ EXCLUDED** | N/A |
| **SATURN driver** | ✅ | ❌ | **❌ EXCLUDED** | N/A |
| USB OZY | ✅ | ❌ | ⚠️ (optional) | P2 |

**Legend:**
- ✅ Fully supported
- ⚠️ Optional/conditional
- ❌ Not supported/excluded

### 4.2 Configuration Management

#### CM-01: Windows-Specific Config Location
- **Path:** `%APPDATA%\piHPSDR\` (e.g., `C:\Users\<name>\AppData\Roaming\piHPSDR\`)
- **Files:**
  - `pihpsdr.props` - main configuration
  - `wisdom` - FFTW wisdom file
  - Radio-specific configs

#### CM-02: Default Configuration
- **Requirement:** Ship sensible defaults for Windows
- **Audio:** Detect default Windows audio devices on first run
- **Network:** Auto-discovery enabled by default

---

## 5. NON-FUNCTIONAL REQUIREMENTS

### 5.1 Performance

#### NF-01: Audio Latency
- **RX Audio:** <200ms end-to-end latency
- **CW Sidetone:** <20ms latency (target: 15ms as on macOS)
- **Justification:** Critical for CW operation user experience

#### NF-02: CPU Usage
- **Idle:** <5% CPU on modern processor (i5/Ryzen 5 class)
- **Active RX:** <25% CPU single receiver, <50% dual receiver
- **TX:** <40% CPU during transmission

#### NF-03: Memory Usage
- **Baseline:** <200MB RAM
- **With dual RX:** <400MB RAM

### 5.2 Compatibility

#### NF-04: Windows Versions
- **Minimum:** Windows 10 (64-bit) version 1809 or later
- **Recommended:** Windows 10 21H2 or Windows 11
- **Rationale:** GTK3 on Windows requires modern OS

#### NF-05: Hardware Requirements
- **CPU:** Dual-core 2.0GHz minimum, quad-core recommended
- **RAM:** 4GB minimum, 8GB recommended
- **Network:** Gigabit Ethernet (for HPSDR radios)
- **Audio:** Any Windows-compatible sound card (48kHz support required)

### 5.3 Reliability

#### NF-06: Stability
- **Uptime:** 8+ hours continuous operation without crashes
- **Radio disconnection:** Graceful handling of network errors
- **Audio device removal:** No crash when device unplugged

### 5.4 Usability

#### NF-07: Installation
- **Time:** <5 minutes for typical installation
- **User Interaction:** Minimal (click "Next" installer)
- **Dependencies:** All included (no separate downloads)

#### NF-08: First Run Experience
- **Auto-discovery:** Automatically find radios on network
- **Audio setup:** Detect default audio devices
- **Sane defaults:** Program works "out of the box"

---

## 6. IMPLEMENTATION STRATEGY

### 6.1 Phase 1: Foundation (Weeks 1-3)

#### Milestone 1.1: Build System
- [ ] Evaluate build options: Makefile + MSYS2 vs CMake migration
- [ ] Set up Windows development environment
- [ ] Create Windows-specific Makefile or CMakeLists.txt
- [ ] Add platform detection (`_WIN32`, `_WIN64`)
- [ ] Configure conditional compilation flags

#### Milestone 1.2: Dependency Setup
- [ ] Install GTK3 development environment (MSYS2 recommended)
- [ ] Build or obtain PortAudio for Windows
- [ ] Build FFTW3 for Windows
- [ ] Set up pthreads-win32
- [ ] Verify WDSP builds on Windows (should already work)

#### Milestone 1.3: Core Porting
- [ ] Create Windows compatibility header (`windows_compat.h`)
  - Socket abstraction (Winsock2)
  - File path helpers
  - Platform-specific includes
- [ ] Modify all network code for Winsock2
- [ ] Disable GPIO code (`#ifndef GPIO` guards)
- [ ] Disable I2C code
- [ ] Disable SATURN code

**Deliverable:** Application compiles on Windows (may not run yet)

### 6.2 Phase 2: Core Functionality (Weeks 4-6)

#### Milestone 2.1: Audio Implementation
- [ ] Enable PortAudio build (`-DPORTAUDIO`)
- [ ] Test audio device enumeration
- [ ] Test RX audio output (single receiver)
- [ ] Test TX microphone input
- [ ] Optimize audio buffer sizes for low latency
- [ ] Test with multiple audio backends (WDM, WASAPI, DirectSound)

#### Milestone 2.2: Radio Protocols
- [ ] Test Old Protocol (P1) radio discovery
- [ ] Test New Protocol (P2) radio discovery
- [ ] Verify RX functionality (single receiver)
- [ ] Verify TX functionality
- [ ] Test mode switching (CW, SSB, AM, FM)

#### Milestone 2.3: UI Testing
- [ ] Launch GTK3 main window on Windows
- [ ] Test all menus and dialogs
- [ ] Verify spectrum/waterfall rendering
- [ ] Test VFO controls
- [ ] Test screen resizing and full-screen

**Deliverable:** Basic RX/TX functionality working on Windows

### 6.3 Phase 3: Advanced Features (Weeks 7-9)

#### Milestone 3.1: Multi-Receiver
- [ ] Test dual-receiver configuration
- [ ] Verify independent audio streams
- [ ] Test RX switching

#### Milestone 3.2: CW Optimization
- [ ] Test CW sidetone latency (measure with oscilloscope/audio loopback)
- [ ] Optimize buffer sizes to achieve <20ms
- [ ] Test CW keying (if GPIO excluded, test via network or CAT)

#### Milestone 3.3: Remote Operation
- [ ] Test client/server mode
- [ ] Verify network audio streaming
- [ ] Test Rigctl protocol
- [ ] Test TCI protocol

#### Milestone 3.4: Optional Features
- [ ] Implement Windows MIDI support (if desired)
- [ ] Test SoapySDR on Windows (if available)
- [ ] Test USB OZY support with libusb-win32

**Deliverable:** Feature-complete Windows port

### 6.4 Phase 4: Polish & Release (Weeks 10-12)

#### Milestone 4.1: Testing & Bug Fixes
- [ ] Extended stability testing (24+ hour runs)
- [ ] Test on multiple Windows versions (Win10, Win11)
- [ ] Test with various audio hardware
- [ ] Test with multiple radio types
- [ ] Fix all critical bugs

#### Milestone 4.2: Documentation
- [ ] Write Windows installation guide
- [ ] Document Windows-specific configuration
- [ ] Create troubleshooting guide
- [ ] Update main README with Windows support

#### Milestone 4.3: Packaging
- [ ] Create NSIS installer script
- [ ] Bundle all dependencies (GTK3 DLLs, etc.)
- [ ] Test installer on clean Windows system
- [ ] Create portable .zip distribution
- [ ] Sign binaries (optional, requires code signing certificate)

#### Milestone 4.4: Release
- [ ] Create GitHub release with Windows binaries
- [ ] Announce on piHPSDR mailing list / forums
- [ ] Gather user feedback
- [ ] Address post-release issues

**Deliverable:** Public Windows release of piHPSDR

---

## 7. TESTING REQUIREMENTS

### 7.1 Unit Testing
- Network socket abstraction layer
- Audio buffer management
- Configuration file parsing

### 7.2 Integration Testing
- Radio discovery on Windows network stack
- Audio input/output with PortAudio
- UI rendering with GTK3
- Multi-threading with pthreads-win32

### 7.3 System Testing

#### Test Cases
1. **TC-01: Fresh Installation**
   - Clean Windows 10 VM
   - Run installer
   - Launch application
   - Verify auto-discovery
   - Verify audio device detection

2. **TC-02: Radio Connection**
   - Connect to HPSDR radio (P1 and P2)
   - Verify RX audio
   - Verify spectrum display
   - Change bands and modes

3. **TC-03: Transmit**
   - Configure microphone
   - Key transmitter (PTT via software or CAT)
   - Verify TX audio
   - Verify power output control

4. **TC-04: CW Operation**
   - Select CW mode
   - Test sidetone latency (measure)
   - Verify acceptable delay (<20ms target)

5. **TC-05: Stability**
   - 24-hour continuous RX operation
   - Monitor memory usage (no leaks)
   - Monitor CPU usage (stable)
   - No crashes or freezes

6. **TC-06: Audio Device Switching**
   - Change output device while running
   - Verify audio continues correctly
   - Unplug USB audio device
   - Verify graceful error handling

### 7.4 Compatibility Testing

#### Test Matrix
| Windows Version | Audio Backend | Radio Type | Result |
|-----------------|---------------|------------|--------|
| Win 10 21H2     | WDM           | Hermes     | ✅/❌   |
| Win 10 21H2     | WASAPI        | Hermes     | ✅/❌   |
| Win 11 22H2     | WASAPI        | HL2        | ✅/❌   |
| Win 10 LTSC     | DirectSound   | Angelia    | ✅/❌   |

---

## 8. RISKS AND MITIGATION

### 8.1 Technical Risks

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| **GTK3 rendering issues on Windows** | High | Medium | Early testing; fallback to GTK2 if necessary |
| **PortAudio latency too high** | High | Low | Use WASAPI exclusive mode; optimize buffers |
| **Winsock compatibility issues** | Medium | Low | Thorough testing; compatibility layer |
| **Threading bugs on Windows** | Medium | Medium | Use proven pthreads-win32; extensive testing |
| **DLL dependency hell** | Medium | High | Bundle all DLLs; static linking where possible |
| **Performance degradation** | Medium | Low | Profile and optimize; Windows CPU scheduler behaves differently |

### 8.2 Project Risks

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| **Scope creep** | High | High | Strict adherence to PRD; defer nice-to-haves |
| **Insufficient testing hardware** | Medium | Medium | Borrow radios from community; use simulator |
| **Community feedback overwhelming** | Low | Medium | Staged beta release; prioritize critical bugs |

---

## 9. SUCCESS CRITERIA

### 9.1 Minimum Viable Product (MVP)

The Windows port is considered successful if:

1. ✅ **Builds and installs** on Windows 10/11 via installer
2. ✅ **Discovers HPSDR radios** on local network
3. ✅ **Receives audio** with acceptable latency (<200ms)
4. ✅ **Transmits audio** with working microphone input
5. ✅ **CW sidetone** works with low latency (<25ms acceptable, <20ms target)
6. ✅ **All modes functional** (CW, SSB, AM, FM)
7. ✅ **Spectrum/waterfall** displays correctly
8. ✅ **Runs for 8+ hours** without crashes
9. ✅ **GPIO and I2C code disabled** and excluded from build
10. ✅ **Documentation** covers Windows installation and setup

### 9.2 Stretch Goals

- ⭐ MIDI controller support via Windows MIDI API
- ⭐ ASIO audio support for ultra-low latency
- ⭐ Windows 11 native theme integration
- ⭐ Code signing certificate for installer
- ⭐ SoapySDR support on Windows

---

## 10. MAINTENANCE AND SUPPORT

### 10.1 Post-Release Support

- **Bug Fixes:** Critical bugs fixed within 1 week, minor bugs within 1 month
- **Updates:** Quarterly releases with bug fixes and minor improvements
- **Community:** GitHub Issues for bug reports, discussions for questions

### 10.2 Documentation

- Installation guide for Windows
- Troubleshooting guide (common audio issues, firewall, etc.)
- Build instructions for developers
- Contribution guidelines

### 10.3 Long-Term Considerations

- **GTK4 Migration:** Eventually migrate to GTK4 (better Windows support)
- **CMake Migration:** Replace Makefile with CMake for better cross-platform builds
- **CI/CD:** Automated Windows builds via GitHub Actions
- **Code Signing:** Obtain certificate to avoid Windows SmartScreen warnings

---

## 11. APPENDICES

### Appendix A: Build Tools Comparison

| Tool | Pros | Cons | Recommendation |
|------|------|------|----------------|
| **MSYS2** | Native-like experience; pacman package manager; easy GTK3 | Large install; Unix-style paths | ⭐ Recommended for development |
| **vcpkg** | Modern; Microsoft-backed; good library coverage | Slower builds; less GTK3 experience | Alternative |
| **Cygwin** | Complete POSIX environment | Heavyweight; Cygwin DLL dependency | Not recommended |
| **MinGW-w64 standalone** | Lightweight | Manual dependency management | For advanced users |

### Appendix B: Audio Backend Comparison

| Backend | Latency | Compatibility | Notes |
|---------|---------|---------------|-------|
| **WDM (Windows Driver Model)** | High (~50ms) | Universal | Fallback option |
| **DirectSound** | Medium (~30ms) | Universal | Good compatibility |
| **WASAPI Shared** | Medium (~20ms) | Win Vista+ | Recommended default |
| **WASAPI Exclusive** | Low (~5-10ms) | Win Vista+ | Best for CW; requires exclusive device access |
| **ASIO** | Very Low (~2-5ms) | Requires driver | For audiophile cards; optional |

**Recommendation:** Default to WASAPI Shared, allow user to select WASAPI Exclusive for CW.

### Appendix C: Excluded Features Rationale

#### GPIO (General Purpose I/O)
- **Purpose:** Direct hardware control on Raspberry Pi (encoders, PTT, paddles)
- **Why excluded:** Windows PCs do not have GPIO pins; feature is SBC-specific
- **Workaround:** Users can use external USB encoders, CAT control for PTT

#### I2C (Inter-Integrated Circuit)
- **Purpose:** Communicate with MCP23017 chip for switch matrix on embedded systems
- **Why excluded:** Windows has no standard I2C interface; requires specialized hardware
- **Workaround:** Not needed on typical Windows desktop; use MIDI or USB HID devices

#### SATURN/G2 Native Driver
- **Purpose:** Direct XDMA driver for SATURN radios on Linux
- **Why excluded:** Requires Linux kernel module; no Windows equivalent
- **Note:** SATURN radios can still use network protocols

---

## 12. REVISION HISTORY

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-11-06 | Claude | Initial PRD creation |

---

## 13. APPROVALS

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Project Owner | ___________ | ___________ | ________ |
| Lead Developer | ___________ | ___________ | ________ |
| Community Representative | ___________ | ___________ | ________ |

---

**END OF DOCUMENT**
