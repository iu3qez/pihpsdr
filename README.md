# pihpsdr
**SDR host program**,
supporting both the old (P1) and new (P2) HPSDR protocols, as well as the SoapySDR framework.
It runs on Linux (Raspberry Pi but also Desktop or Laptop computers running LINUX), MacOS (Intel or AppleSilicon CPUs, using  "Homebrew" or "MacPorts"), and Windows (via the MinGW-w64 toolchain).

**piHPSDR Manual: (Release 2.4 Version)** https://github.com/dl1ycf/pihpsdr/releases/download/v2.4/piHPSDR-Manual.pdf

**piHPSDR Manual: (current Version)** https://github.com/dl1ycf/pihpsdr/releases/download/v2.4/piHPSDR-Manual-current.pdf

***
piHPSDR should be compiled from the sources, consult the Manual (**link given above**) on how to compile/install piHPSDR on your machine
***

Latest features:

- **client/server model for remote operation** (including transmitting in phone and CW)
- full support for Anan G2-Ultra radios, including customizable panel button/encoder functions
- added continuous frequency compressor (**CFC**) and downward expander (**DEXP**) to the TX chain
- in-depth manual (**link given above**)
- HermesLite-II I/O-board support
- audio recording (RX capture) and playback (TX)
- automatic installation procedures for compilation from the sources, for Linux (including RaspPi) and MacOS
  (Appendix J, K).
- dynamic screen resizing in the "Screen" menu, including transitions
  between full-screen and window mode
- experimental Windows build using MinGW-w64; see `WINDOWS/build_mingw.sh` and `Makefile.win` for prerequisites and cross compilation steps (MSYS2, GTK+, PortAudio, FFTW, libusb).
  Build the `wdsp` library and copy the resulting DLLs beside `pihpsdr.exe`.



