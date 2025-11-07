# TASK-007: Windows-Specific Makefile Configuration

**Date:** 2025-11-06
**Task ID:** TASK-007
**Dependencies:** TASK-001, TASK-002
**Status:** In Progress
**Estimated Time:** 1 day

---

## Overview

This document describes the modifications needed to support Windows builds in the existing Makefile.

**Strategy:** Minimal, surgical changes following the macOS pattern.

---

## Changes Summary

We need to add Windows support in **5 key locations**:

1. **Platform Detection** (after line 51)
2. **Disable Hardware Features** (after line 121)
3. **Audio Backend Selection** (around line 300)
4. **MIDI Support** (around line 142) - Optional for MVP
5. **System Libraries** (after line 404)

---

## Detailed Changes

### Change 1: Platform Detection (Lines 49-56)

**Current code:**
```makefile
# get the OS Name
UNAME_S := $(shell uname -s)
UNAME_R := $(shell uname -r)
```

**Add after line 51:**
```makefile
# Detect Windows (MSYS2/MinGW sets OS=Windows_NT environment variable)
# On Windows with MSYS2, uname -s returns something like "MINGW64_NT-10.0-xxxxx"
# We normalize this to "Windows" for consistency
ifeq ($(OS),Windows_NT)
  UNAME_S := Windows
endif
# Alternative detection: check if UNAME_S contains MINGW or MSYS
ifneq (,$(findstring MINGW,$(UNAME_S)))
  UNAME_S := Windows
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
  UNAME_S := Windows
endif
```

**Rationale:**
- MSYS2 sets `OS=Windows_NT` environment variable
- `uname -s` in MSYS2 returns strings like `MINGW64_NT-10.0-19045` or `MSYS_NT-10.0`
- We normalize to `UNAME_S := Windows` for clean conditionals

---

### Change 2: Disable Hardware-Specific Features (Lines 112-121)

**Current code:**
```makefile
##############################################################################
#
# disable GPIO and SATURN for MacOS, simply because it is not there
#
##############################################################################

ifeq ($(UNAME_S), Darwin)
GPIO=
SATURN=
endif
```

**Replace with:**
```makefile
##############################################################################
#
# Disable GPIO and SATURN for MacOS and Windows (not available)
#
##############################################################################

ifeq ($(UNAME_S), Darwin)
GPIO=
SATURN=
TTS=        # TTS deferred for macOS? (check current state)
endif

ifeq ($(UNAME_S), Windows)
GPIO=
SATURN=
TTS=        # Defer Windows TTS (SAPI) to post-MVP
endif
```

**Rationale:**
- Windows PCs don't have GPIO pins (Raspberry Pi specific)
- SATURN requires Linux kernel module
- TTS on Windows would need Windows SAPI implementation (defer to later)

---

### Change 3: Audio Backend Selection (Lines 298-307)

**Current code:**
```makefile
##############################################################################
#
# Options for audio module
#  - MacOS: only PORTAUDIO
#  - Linux: either PULSEAUDIO (default) or ALSA (upon request)
#
##############################################################################

ifeq ($(UNAME_S), Darwin)
  AUDIO=PORTAUDIO
endif
ifeq ($(UNAME_S), Linux)
  ifneq ($(AUDIO) , ALSA)
    AUDIO=PULSE
  endif
endif
```

**Replace with:**
```makefile
##############################################################################
#
# Options for audio module
#  - MacOS: only PORTAUDIO
#  - Windows: only PORTAUDIO
#  - Linux: either PULSEAUDIO (default) or ALSA (upon request)
#
##############################################################################

ifeq ($(UNAME_S), Darwin)
  AUDIO=PORTAUDIO
endif

ifeq ($(UNAME_S), Windows)
  AUDIO=PORTAUDIO
endif

ifeq ($(UNAME_S), Linux)
  ifneq ($(AUDIO) , ALSA)
    AUDIO=PULSE
  endif
endif
```

**Rationale:**
- Windows doesn't have ALSA or PulseAudio
- PortAudio works well on Windows (WASAPI backend)
- Same strategy as macOS

---

### Change 4: MIDI Support (Lines 130-146) - OPTIONAL for MVP

**Current code:**
```makefile
ifeq ($(MIDI),ON)
MIDI_OPTIONS=-D MIDI
MIDI_HEADERS= src/midi.h src/midi_menu.h
ifeq ($(UNAME_S), Darwin)
MIDI_SOURCES= src/mac_midi.c src/midi2.c src/midi3.c src/midi_menu.c
MIDI_OBJS= src/mac_midi.o src/midi2.o src/midi3.o src/midi_menu.o
MIDI_LIBS= -framework CoreMIDI -framework Foundation
endif
ifeq ($(UNAME_S), Linux)
MIDI_SOURCES= src/alsa_midi.c src/midi2.c src/midi3.c src/midi_menu.c
MIDI_OBJS= src/alsa_midi.o src/midi2.o src/midi3.o src/midi_menu.o
MIDI_LIBS= -lasound
endif
endif
```

**Add after line 142 (before `endif` on line 143):**
```makefile
ifeq ($(UNAME_S), Windows)
# Windows MIDI implementation (optional for MVP, can disable MIDI entirely)
# Option 1: Disable MIDI on Windows for MVP
MIDI=OFF
# Option 2: Use Windows MIDI API (requires src/windows_midi.c - create later)
# MIDI_SOURCES= src/windows_midi.c src/midi2.c src/midi3.c src/midi_menu.c
# MIDI_OBJS= src/windows_midi.o src/midi2.o src/midi3.o src/midi_menu.o
# MIDI_LIBS= -lwinmm
endif
```

**Recommendation for MVP:** Disable MIDI on Windows initially (`MIDI=OFF`), implement later.

---

### Change 5: System Libraries (Lines 396-404)

**Current code:**
```makefile
##############################################################################
#
# Specify additional OS-dependent system libraries
#
##############################################################################

ifeq ($(UNAME_S), Linux)
SYSLIBS=-lrt
endif

ifeq ($(UNAME_S), Darwin)
SYSLIBS=-framework IOKit
endif
```

**Add after line 404:**
```makefile

ifeq ($(UNAME_S), Windows)
# Winsock2 for network sockets, iphlpapi for network interface enumeration
SYSLIBS=-lws2_32 -liphlpapi
endif
```

**Rationale:**
- `ws2_32.dll` = Winsock2 (Windows sockets API)
- `iphlpapi.dll` = IP Helper API (network adapter enumeration for discovery)
- Both are standard Windows system libraries

---

## Complete Patch File

Here's the complete set of changes in unified diff format:

```diff
--- Makefile.orig	2025-11-06
+++ Makefile	2025-11-06
@@ -49,6 +49,18 @@
 # get the OS Name
 UNAME_S := $(shell uname -s)
 UNAME_R := $(shell uname -r)
+
+# Detect Windows (MSYS2/MinGW environment)
+# MSYS2 sets OS=Windows_NT, and uname -s returns MINGW64_NT or MSYS_NT
+ifeq ($(OS),Windows_NT)
+  UNAME_S := Windows
+endif
+ifneq (,$(findstring MINGW,$(UNAME_S)))
+  UNAME_S := Windows
+endif
+ifneq (,$(findstring MSYS,$(UNAME_S)))
+  UNAME_S := Windows
+endif

 # Get git commit version and date
 GIT_DATE := $(firstword $(shell git --no-pager show --date=short --format="%ai" --name-only))
@@ -112,11 +124,17 @@

 ##############################################################################
 #
-# disable GPIO and SATURN for MacOS, simply because it is not there
+# Disable GPIO and SATURN for MacOS and Windows (not available on these platforms)
 #
 ##############################################################################

 ifeq ($(UNAME_S), Darwin)
+GPIO=
+SATURN=
+endif
+
+ifeq ($(UNAME_S), Windows)
+# Disable hardware-specific features for Windows
 GPIO=
 SATURN=
 endif
@@ -138,6 +156,11 @@
 MIDI_SOURCES= src/alsa_midi.c src/midi2.c src/midi3.c src/midi_menu.c
 MIDI_OBJS= src/alsa_midi.o src/midi2.o src/midi3.o src/midi_menu.o
 MIDI_LIBS= -lasound
+endif
+ifeq ($(UNAME_S), Windows)
+# Disable MIDI on Windows for MVP (implement later with Windows MIDI API)
+MIDI=OFF
+# TODO: Implement src/windows_midi.c using Windows Multimedia API (mmsystem.h)
 endif
 endif
 CPP_DEFINES += -DMIDI
@@ -297,10 +320,14 @@
 ##############################################################################
 #
 # Options for audio module
-#  - MacOS: only PORTAUDIO
+#  - MacOS and Windows: only PORTAUDIO
 #  - Linux: either PULSEAUDIO (default) or ALSA (upon request)
 #
 ##############################################################################

 ifeq ($(UNAME_S), Darwin)
+  AUDIO=PORTAUDIO
+endif
+
+ifeq ($(UNAME_S), Windows)
   AUDIO=PORTAUDIO
 endif
 ifeq ($(UNAME_S), Linux)
@@ -401,6 +428,11 @@

 ifeq ($(UNAME_S), Darwin)
 SYSLIBS=-framework IOKit
+endif
+
+ifeq ($(UNAME_S), Windows)
+# Winsock2 for network sockets, iphlpapi for network interface info
+SYSLIBS=-lws2_32 -liphlpapi
 endif

 ##############################################################################
```

---

## Testing the Changes

After applying these changes, test compilation:

### Step 1: Backup Original Makefile
```bash
cd ~/Projects/pihpsdr
cp Makefile Makefile.backup
```

### Step 2: Apply Changes
Apply the patch (or manually edit the Makefile with the changes above).

### Step 3: Verify Platform Detection
```bash
make -n | head -20
```

Look for confirmation that `UNAME_S` is set to `Windows`.

### Step 4: Check Variables
```bash
make -n 2>&1 | grep -E "(UNAME_S|GPIO|SATURN|AUDIO|SYSLIBS)" | head -20
```

Expected output should show:
- `UNAME_S = Windows`
- `GPIO` and `SATURN` empty (disabled)
- `AUDIO = PORTAUDIO`
- `SYSLIBS = -lws2_32 -liphlpapi`

### Step 5: Attempt Compilation
```bash
make clean
make 2>&1 | tee windows_build_log.txt
```

**Expected at this stage:**
- Still compilation errors (normal - need more changes)
- But fewer errors than before
- GPIO/SATURN errors should be gone
- Audio should use PortAudio

---

## Expected Remaining Issues After TASK-007

After these Makefile changes, we'll still have compilation errors related to:

1. **Socket/Network code** (needs Winsock2 compatibility) → TASK-010
2. **File paths** (needs `/` vs `\` handling) → TASK-014
3. **Missing `windows_compat.h`** → TASK-009
4. **I2C code** (needs to be excluded) → TASK-012
5. **Platform-specific headers** (e.g., `<unistd.h>`, `<sys/socket.h>`)

These will be addressed in subsequent tasks.

---

## Verification Checklist

After applying TASK-007 changes:

- [ ] Makefile detects Windows correctly (`UNAME_S = Windows`)
- [ ] GPIO is disabled on Windows
- [ ] SATURN is disabled on Windows
- [ ] AUDIO is set to PORTAUDIO on Windows
- [ ] SYSLIBS includes `-lws2_32 -liphlpapi`
- [ ] MIDI is disabled on Windows (for MVP)
- [ ] Compilation produces fewer errors than before
- [ ] No errors related to GPIO/SATURN/ALSA/PulseAudio

---

## Next Steps

After TASK-007:
1. **TASK-009:** Create `src/windows_compat.h` (socket/path compatibility)
2. **TASK-010:** Port network code to Winsock2
3. **TASK-012:** Exclude I2C code
4. **TASK-016:** Achieve full compilation

---

**END OF DOCUMENT**
