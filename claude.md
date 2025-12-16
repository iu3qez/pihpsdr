# piHPSDR Windows Porting - Guida e Regole

## Stato Attuale

Il codice nel repository è quello **originale Linux**, non portato. Un precedente tentativo di porting è stato abbandonato perché i branch si erano allontanati troppo e non era possibile riconciliarli.

Nella cartella `Windows/` esistono già:
- Layer di compatibilità funzionanti (`windows_compat.h`, `windows_compat.c`)
- Header wrapper per intercettare include POSIX
- MIDI implementation per Windows (`windows_midi.c`)
- Script di build e documentazione

---

## Regole di Portabilità (FONDAMENTALI)

### 1. Minimo Impatto sul Codice Originale
- **NON modificare** il codice sorgente originale se non strettamente necessario
- Preferire **sempre** i wrapper rispetto alle modifiche dirette
- Se servono `#ifdef _WIN32`, usarli ma **senza modificare la logica** del codice

### 2. Uso dei Wrapper
- Gli header wrapper in `Windows/` intercettano gli include POSIX
- Pattern: `#include <pthread.h>` → viene intercettato da `Windows/pthread.h` → include `windows_compat.h`
- Il codice sorgente **non deve sapere** di essere su Windows

### 3. Documentazione Obbligatoria
- **Ogni modifica** al codice originale deve essere documentata in questo file
- Specificare: file, riga, motivo, alternativa considerata

### 4. Esclusione Codice Non Necessario
- Alcune parti del codice sono "difficili" e **non servono su Windows**
- Queste vanno **escluse completamente** dalla compilazione Windows (non solo via config)
- Esempio: **GPIO** - specifico Raspberry Pi, da escludere del tutto

| Componente | File | Motivo Esclusione |
|------------|------|-------------------|
| GPIO | `gpio.c`, `i2c.c`, `andromeda.c` | Hardware Raspberry Pi (libgpiod, i2c-dev) |
| SATURN | `saturnmain.c`, `saturnserver.c`, `saturndrivers.c`, `saturnregisters.c`, `saturn_menu.c` | Driver kernel Linux PCIe |
| udplistener | `udplistener.c` | Usa fork(), execl(), posix_openpt() - non portabile |
| Simulatori | `hpsdrsim.c`, `bootldrsim.c` | Usano mmap(), signal handlers complessi |
| ALSA MIDI | `alsa_midi.c` | Specifico Linux, usare windows_midi.c |
| ALSA Audio | `audio.c` (quando AUDIO=ALSA) | Specifico Linux, usare PortAudio |
| PulseAudio | `pulseaudio.c` | Specifico Linux, usare PortAudio |

### 5. Stub Richiedono Approvazione
- **IMPORTANTE**: Eventuali stub (implementazioni vuote/parziali) devono essere **APPROVATI PREVENTIVAMENTE**
- Prima di creare uno stub, chiedere conferma con: cosa fa, perché serve, alternativa

### 6. Build System
- **CMake** per il porting Windows (cross-compile da Linux)
- Il Makefile originale rimane per Linux/macOS
- CMake semplifica la cross-compilazione con MinGW

---

## Architettura del Layer di Compatibilità

### Header Wrapper Pattern
```
Windows/
├── pthread.h          → #include "windows_compat.h"
├── semaphore.h        → #include "windows_compat.h"
├── unistd.h           → #include "windows_compat.h"
├── fcntl.h            → #include "windows_compat.h"
├── poll.h             → #include "windows_compat.h"
├── netdb.h            → #include "windows_compat.h"
├── termios.h          → #include "windows_compat.h"
├── pwd.h              → #include "windows_compat.h"
├── endian.h           → #include "windows_compat.h"
├── ifaddrs.h          → #include "windows_compat.h"
├── arpa/
│   └── inet.h         → #include "../windows_compat.h"
├── net/
│   ├── if.h           → #include "../windows_compat.h"
│   └── if_arp.h       → #include "../windows_compat.h"
├── netinet/
│   ├── in.h           → #include "../windows_compat.h"
│   ├── ip.h           → #include "../windows_compat.h"
│   └── tcp.h          → #include "../windows_compat.h"
└── sys/
    ├── socket.h       → #include "../windows_compat.h"
    ├── ioctl.h        → #include "../windows_compat.h"
    ├── mman.h         → #include "../windows_compat.h"
    ├── select.h       → #include "../windows_compat.h"
    └── utsname.h      → #include "../windows_compat.h"
```

### Mappature POSIX → Windows (in windows_compat.h/c)

| POSIX | Windows | Note |
|-------|---------|------|
| `pthread_t` | `HANDLE` | CreateThread |
| `pthread_mutex_t` | `CRITICAL_SECTION` | |
| `sem_t` | `struct { HANDLE }` | CreateSemaphore |
| `close(socket)` | `closesocket()` | Macro |
| `sleep(x)` | `Sleep(x*1000)` | Macro |
| `usleep(x)` | `Sleep(x/1000)` | Macro |
| `clock_gettime()` | Implementazione custom | |
| `getifaddrs()` | `GetAdaptersAddresses()` | |
| `realpath()` | `GetFullPathNameA()` | |
| `fcntl()` | `ioctlsocket()` | Per O_NONBLOCK |

---

## Features Supportate su Windows

- GTK3 GUI
- PortAudio (audio)
- Windows MIDI API (WinMM)
- Network protocols (P1/P2 HPSDR)
- Client/server
- USB OZY (libusb)
- SoapySDR (opzionale)
- STEMlab/RedPitaya
- WDSP DSP library

## Features NON Supportate su Windows

- GPIO (specifico Raspberry Pi)
- SATURN/G2 XDMA (driver kernel Linux)
- PulseAudio/ALSA (usare PortAudio)
- ALSA MIDI (usare WinMM)

---

## Dipendenze Windows (MSYS2/MinGW64)

```
mingw-w64-x86_64-gcc
mingw-w64-x86_64-gtk3
mingw-w64-x86_64-portaudio
mingw-w64-x86_64-fftw
mingw-w64-x86_64-openssl
mingw-w64-x86_64-libusb
mingw-w64-x86_64-curl
mingw-w64-x86_64-cmake
```

---

## Modifiche al Codice Originale

### Regola: Conflitti Macro/Definizioni Windows

Windows definisce già alcune macro che confliggono con quelle del progetto.

**Soluzione per selezioni utente**: Rinominare con prefisso `selection_`

| Originale | Rinominato | Motivo |
|-----------|------------|--------|
| `SNB` | `selection_SNB` | Conflitto con Windows headers |

### Conflitti WDSP Identificati

La libreria WDSP in `wdsp/` ha conflitti con Windows headers:

| Macro | File | Riga | Gravità | Gestione |
|-------|------|------|---------|----------|
| `DWORD` | wdsp/wdsp.h | 11 | CRITICA | Gestito in windows_compat.h |
| `DWORD` | wdsp/linux_port.h | 41 | CRITICA | Gestito in wdsp_wrapper.h |
| `TRUE` | wdsp/linux_port.h | 45 | CRITICA | Da gestire |
| `FALSE` | wdsp/linux_port.h | 44 | CRITICA | Da gestire |
| `HANDLE` | wdsp/linux_port.h | 42 | CRITICA | Parzialmente gestito |
| `LONG` | wdsp/linux_port.h | 40 | MODERATA | Da gestire |
| `LPCRITICAL_SECTION` | wdsp/wdsp.h | 67 | MODERATA | Gestito in wdsp_wrapper.h |
| `__stdcall` | wdsp/wdsp.h | 66 | MINORE | Da verificare |

**Nota**: `wdsp/linux_port.h` è SOLO per Linux/macOS. Su Windows serve un equivalente `windows_port.h`.

### REGISTRO MODIFICHE
Ogni modifica al codice originale deve essere registrata qui.

| File | Tipo | Descrizione | Data |
|------|------|-------------|------|
| (da popolare durante il porting) | | | |

---

## Analisi WDSP

La libreria WDSP (`wdsp/`) è **Linux-first** con supporto Windows parziale (~25-30%).

### Stato Attuale
- `linux_port.h/c`: Layer compatibilità SOLO per Linux/macOS
- 7 blocchi `#ifdef _WIN32` sparsi in 3 file
- Supporto audio real-time Windows (AVRT) presente
- **MANCA**: `windows_port.h/c` equivalente

### Cosa Serve per Windows
1. Creare `wdsp/windows_port.h` e `wdsp/windows_port.c`
2. Aggiornare `wdsp/comm.h` per includere windows_port.h su Windows
3. Mappare funzioni POSIX → Windows API

### File WDSP Critici
- `wdsp/comm.h` - Include management (riga 27-33)
- `wdsp/linux_port.h` - Modello per windows_port.h
- `wdsp/main.c` - Audio priority (già ha #ifdef _WIN32)
- `wdsp/wisdom.c` - Console allocation (già ha #ifdef _WIN32)

---

## File Sorgente per Categoria

### Categoria A: Network (Alta Priorità)
File che usano socket, richiedono Winsock2 wrapper:
- `main.c`, `new_protocol.c`, `old_protocol.c`
- `discovery.c`, `new_discovery.c`, `old_discovery.c`
- `rigctl.c`, `protocols.c`, `client_thread.c`
- `bootloader.c`

### Categoria B: Threading/Audio (Media Priorità)
File che usano pthread/semaphore:
- `portaudio.c` - Audio (usare questo su Windows)
- `iambic.c` - CW keyer
- `receiver.c`, `transmitter.c`
- `waterfall.c`, `rx_panadapter.c`, `tx_panadapter.c`

### Categoria C: Casi Speciali (Richiedono Attenzione)
- `startup.c` - usa `getpwuid()`, `getuid()` → implementare wrapper
- `ozyio.c` - usa `readlink("/proc/self/exe")` → usare `GetModuleFileName()`

### Categoria D: Da Escludere (vedi sezione 4)
GPIO, SATURN, simulatori, ALSA

---

## TODO Porting

- [ ] Creare CMakeLists.txt per Windows (cross-compile)
- [ ] Creare `wdsp/windows_port.h` e `wdsp/windows_port.c`
- [ ] Verificare wrapper in `Windows/` coprono tutti i casi
- [ ] Gestire conflitti TRUE/FALSE/HANDLE in WDSP
- [ ] Implementare wrapper per `getpwuid()`, `readlink()`
- [ ] Testare compilazione cross-compile
- [ ] Documentare ogni modifica nel REGISTRO MODIFICHE

---

## Note Tecniche

### Cross-Compilazione
- Host: Linux (Ubuntu/Debian)
- Target: Windows x86_64
- Toolchain: MinGW-w64 (x86_64-w64-mingw32-gcc)
- CMake toolchain file necessario

### Configurazione Directory Windows
- Config: `%LOCALAPPDATA%/piHPSDR` (non Documents per evitare sync cloud)
- Eseguibile: directory corrente con DLL

### Problemi Noti Risolti in Passato
- `#include <Windows.h>` → `#include <windows.h>` (case-sensitive su Linux)
- `#include <mmeapi.h>` → `#include <mmsystem.h>` (MinGW compatibility)
- `OnLinkPrefixLength` non disponibile in MinGW GCC 10
