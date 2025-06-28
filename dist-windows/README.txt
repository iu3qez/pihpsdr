piHPSDR for Windows
===================

This package contains piHPSDR cross-compiled for Windows using MinGW.

Contents:
- pihpsdr.exe         : Main executable
- setup-env.bat       : Launcher script (RECOMMENDED)
- *.dll               : Required runtime libraries
- share/              : GTK3 runtime data files (icons, schemas)
- lib/                : GTK3 modules (pixbuf loaders)
- etc/                : GTK3 configuration

To run:
1. Extract this entire folder to a location on your Windows machine
2. Double-click setup-env.bat to run (RECOMMENDED)
   OR double-click pihpsdr.exe directly

Note: Keep all files in the same directory structure. The program needs
the DLLs and data files to function properly.

The setup-env.bat script sets up the correct environment variables for
GTK3 to find its resources. Using it is recommended for best compatibility.

Configuration Files:
--------------------
piHPSDR saves all its files in:
  %LOCALAPPDATA%\piHPSDR\
  (typically: C:\Users\<username>\AppData\Local\piHPSDR\)

This directory will contain:

Configuration files (.props):
- Radio settings: Named by MAC address (e.g., 02-03-04-05-06-07.props)
- gpio.props       : GPIO settings
- ipaddr.props     : IP address settings
- protocols.props  : Protocol settings
- remote.props     : Remote settings
- midi.props       : MIDI settings

DSP optimization:
- wdspWisdom00     : FFTW wisdom file (FFT optimization data)
                     Created automatically on first run, speeds up DSP processing

Log files:
- pihpsdr.stdout   : Standard output log
- pihpsdr.stderr   : Error log (useful for troubleshooting)

The .props and wisdom files are created automatically. The wisdom file creation
can take a few minutes on first run while it optimizes FFT sizes up to 262144.

IMPORTANT: Backup the entire %LOCALAPPDATA%\piHPSDR\ folder to preserve
all your configurations and DSP optimizations.

Note: Using %LOCALAPPDATA% (AppData\Local) instead of Documents avoids issues
with cloud sync services (OneDrive, Google Drive) trying to sync binary files
and logs. This is the recommended location for application data on Windows.

For more information, see the main piHPSDR documentation.
