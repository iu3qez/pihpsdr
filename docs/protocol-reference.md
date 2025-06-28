# piHPSDR Client-Server Protocol Reference

## Overview

The piHPSDR client-server protocol enables remote operation where:
- **Server:** piHPSDR running locally, attached to radio hardware
- **Client:** Remote instance (or web gateway) without radio

The server handles all DSP (WDSP) and sends processed data to the client. The client only sends commands.

## Connection

- **Transport:** TCP
- **Default Port:** 45454
- **Authentication:** SHA512 password hash with salt (100,000 iterations)

## Packet Structure

All packets begin with a 12-byte header:

```
Offset  Size  Field       Description
0       4     sync        Magic bytes: 0xFA 0xFA 0xAF 0xAF
4       2     data_type   Command/Info type (big-endian)
6       1     b1          Optional byte 1
7       1     b2          Optional byte 2
8       2     s1          Optional short 1 (big-endian)
10      2     s2          Optional short 2 (big-endian)
```

Some commands use only the header; others append additional packed data structures.

## Data Encoding

All multi-byte integers use **big-endian** (network byte order).

### Double Encoding

Doubles are encoded as uint64 with special transformation:
```
encoded = (double_value + 9e8) * 1e10
```

Decoding:
```
double_value = encoded * 1e-10 - 9e8
```

Range: approximately ±9×10⁸ with 10⁻¹⁰ resolution.

## Command Types (Client → Server)

### Frequency Control

| Type | Value | Description |
|------|-------|-------------|
| CMD_FREQ | 58 | Set VFO frequency |
| CMD_MODE | 63 | Set operating mode |
| CMD_FILTER_SEL | 56 | Select filter preset |
| CMD_FILTER_VAR | 57 | Set variable filter |
| CMD_BAND_SEL | 40 | Select band |
| CMD_BANDSTACK | 41 | Select bandstack entry |
| CMD_STEP | 104 | VFO step |
| CMD_ZOOM | 126 | Set zoom level |
| CMD_PAN | 69 | Set pan position |

### CMD_FREQ Structure
```
Header only, with:
- b1: VFO index (0 or 1)
- Followed by U64_COMMAND with frequency in Hz
```

### Audio/Receiver Control

| Type | Value | Description |
|------|-------|-------------|
| CMD_VOLUME | 122 | Set audio volume |
| CMD_AGC | 35 | Set AGC mode |
| CMD_AGC_GAIN | 36 | Set AGC parameters |
| CMD_SQUELCH | 102 | Set squelch |
| CMD_NOISE | 68 | Set noise reduction |
| CMD_MUTE_RX | 67 | Mute receiver |

### TX Control

| Type | Value | Description |
|------|-------|-------------|
| CMD_MOX | 66 | Set MOX state |
| CMD_TOGGLE_MOX | 106 | Toggle MOX |
| CMD_TUNE | 108 | Set tune state |
| CMD_TOGGLE_TUNE | 107 | Toggle tune |
| CMD_DRIVE | 53 | Set drive level |
| CMD_TWOTONE | 109 | Two-tone test |

### Spectrum Control

| Type | Value | Description |
|------|-------|-------------|
| CMD_RX_SPECTRUM | 93 | Enable/disable RX spectrum |
| CMD_TX_SPECTRUM | 117 | Enable/disable TX spectrum |
| CMD_RX_FPS | 91 | Set RX spectrum FPS |
| CMD_TX_FPS | 116 | Set TX spectrum FPS |

### System

| Type | Value | Description |
|------|-------|-------------|
| CMD_START_RADIO | 103 | Start radio operation |
| CMD_HEARTBEAT | 59 | Keep-alive |
| CMD_RESTART | 82 | Restart radio |

## Info Types (Server → Client)

### INFO_SPECTRUM (136)

Periodic spectrum/waterfall data with embedded meter readings.

```c
struct SPECTRUM_DATA {
    HEADER header;
    mydouble meter;           // S-meter reading
    mydouble alc;             // ALC level
    mydouble fwd;             // Forward power
    mydouble swr;             // SWR
    mydouble cA, cB, cAp, cBp; // PureSignal coefficients
    uint64_t vfo_a_freq;      // VFO A frequency
    uint64_t vfo_b_freq;      // VFO B frequency
    uint64_t vfo_a_ctun_freq;
    uint64_t vfo_b_ctun_freq;
    uint64_t vfo_a_offset;
    uint64_t vfo_b_offset;
    uint16_t width;           // Spectrum width in pixels
    uint8_t id;               // Receiver ID (0,1) or TX (8)
    uint8_t avail;            // Data available flag
    uint8_t sample[4096];     // Spectrum samples (dBm + offset)
};
```

**Spectrum samples:** Each byte represents dBm level. Typical range 0-255 mapping to signal strength.

### INFO_RXAUDIO (135)

Periodic audio data from receiver.

```c
struct RXAUDIO_DATA {
    HEADER header;
    uint16_t numsamples;      // Number of samples (max 512)
    uint16_t samples[512];    // Signed 16-bit PCM samples
    uint8_t rx;               // Receiver ID
};
```

**Audio format:**
- Sample rate: Matches receiver sample_rate (typically 48000 Hz)
- Channels: Mono (interleaved L/R summed or single channel)
- Bit depth: 16-bit signed PCM

### INFO_VFO (140)

VFO state information.

```c
struct VFO_DATA {
    HEADER header;
    uint64_t frequency;       // Current frequency (Hz)
    uint64_t ctun_frequency;  // CTUN frequency
    uint64_t rit;             // RIT offset
    uint64_t xit;             // XIT offset
    uint64_t lo;              // LO frequency (transverter)
    uint64_t offset;          // Frequency offset
    uint64_t step;            // Tuning step
    uint16_t rit_step;        // RIT step
    uint16_t deviation;       // FM deviation
    uint8_t vfo;              // VFO index (0=A, 1=B)
    uint8_t band;             // Band index
    uint8_t bandstack;        // Bandstack index
    uint8_t mode;             // Operating mode
    uint8_t filter;           // Filter index
    uint8_t ctun;             // CTUN enabled
    uint8_t rit_enabled;
    uint8_t xit_enabled;
    uint8_t cwAudioPeakFilter;
};
```

### INFO_RECEIVER (134)

Complete receiver configuration.

```c
struct RECEIVER_DATA {
    HEADER header;
    // ... many fields, see client_server.h
    uint32_t sample_rate;     // Audio sample rate
    uint16_t fps;             // Spectrum FPS
    uint16_t filter_low;      // Filter low edge
    uint16_t filter_high;     // Filter high edge
    uint16_t width;           // Panadapter width
    uint8_t id;               // Receiver ID
    uint8_t agc;              // AGC mode
    uint8_t zoom;             // Zoom level
    // ... noise, EQ, display settings
};
```

### INFO_TRANSMITTER (138)

Transmitter configuration.

### INFO_RADIO (133)

Initial radio configuration sent once at connection.

## Operating Modes

```
0  = LSB
1  = USB
2  = DSB
3  = CWL
4  = CWU
5  = FM
6  = AM
7  = DIGU
8  = SPEC
9  = DIGL
10 = SAM
11 = DRM
```

## Typical Message Flow

### Connection Sequence

1. Client connects TCP
2. Server sends challenge (if password protected)
3. Client sends password hash
4. Server sends INFO_RADIO (radio configuration)
5. Server sends INFO_BAND, INFO_BANDSTACK for all bands
6. Server sends INFO_RECEIVER for each receiver
7. Server sends INFO_TRANSMITTER
8. Server sends INFO_VFO for each VFO
9. Client sends CMD_START_RADIO
10. Server begins periodic INFO_SPECTRUM and INFO_RXAUDIO

### Steady State

- Server sends INFO_SPECTRUM at configured FPS (typically 15-30)
- Server sends INFO_RXAUDIO as available (~47 packets/sec at 48kHz)
- Client sends CMD_* for any user action
- Client sends CMD_HEARTBEAT periodically
- Server sends INFO_* updates when state changes

## Constants

```c
#define SPECTRUM_DATA_SIZE 4096  // Max spectrum width
#define AUDIO_DATA_SIZE 512      // Samples per audio packet
#define HPSDR_PWD_LEN 64         // Max password length
```

## Error Handling

- If TCP recv() fails or returns 0 repeatedly (10 times), connection is dead
- Server sets `remoteclient.running = FALSE` on client death
- Server should force TX off when client disconnects

## Gateway Implementation Notes

For a WebSocket gateway:

1. **TCP → WebSocket:** Parse binary packets, convert to JSON for control data
2. **Audio:** Forward as binary WebSocket frames (PCM or transcode to Opus)
3. **Spectrum:** Forward sample array as binary, let browser render
4. **Commands:** Accept JSON from browser, encode as binary TCP packets
5. **Heartbeat:** Maintain both TCP and WebSocket heartbeats
6. **Fail-safe:** Force CMD_MOX off on WebSocket disconnect
