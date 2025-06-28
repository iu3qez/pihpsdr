# piHPSDR Web Interface Project

This branch (`webserver`) implements a web-based remote interface for piHPSDR using the existing client-server protocol.

## Project Overview

**Goal:** Create a web interface that provides full remote control of piHPSDR over Internet connections with moderate latency (50-200ms).

**Architecture:**
- **Gateway (Python asyncio):** Bridges TCP protocol to WebSocket
- **Frontend (Svelte):** Browser-based UI with Canvas 2D waterfall

**Key Decisions:**
- No modifications to piHPSDR C code (external gateway only)
- Audio: PCM 8kHz mono (Opus deferred)
- Spectrum: Canvas 2D rendering
- TX audio: Phase 2

## Protocol Reference

See `docs/protocol-reference.md` for complete protocol documentation.

### Quick Reference

**Sync bytes:** `0xFA 0xFA 0xAF 0xAF`

**Header structure (12 bytes):**
```c
struct HEADER {
    uint8_t sync[4];    // 0xFA 0xFA 0xAF 0xAF
    uint16_t data_type; // Command/Info type (big-endian)
    uint8_t b1, b2;     // Optional bytes
    uint16_t s1, s2;    // Optional shorts
};
```

**Key Commands (client → server):**
- `CMD_FREQ` (58): Set VFO frequency
- `CMD_MODE` (63): Set operating mode
- `CMD_VOLUME` (122): Set audio volume
- `CMD_MOX` (66): Set TX state
- `CMD_FILTER_SEL` (56): Select filter
- `CMD_ZOOM` (126): Set zoom level
- `CMD_START_RADIO` (103): Start radio operation

**Key Info (server → client):**
- `INFO_SPECTRUM` (136): Spectrum/waterfall data
- `INFO_RXAUDIO` (135): RX audio samples
- `INFO_VFO` (140): VFO state
- `INFO_RECEIVER` (134): Receiver parameters
- `INFO_TRANSMITTER` (138): Transmitter parameters

**Data encoding:**
- All multi-byte values: **big-endian** (network byte order)
- Doubles: encoded as `(value + 9e8) * 1e10` as uint64

## File Structure

```
webserver/
├── gateway/                 # Python gateway
│   ├── main.py
│   ├── pihpsdr_client.py   # TCP connection to piHPSDR
│   ├── websocket_server.py # WebSocket server
│   ├── protocol.py         # Protocol definitions
│   ├── audio_buffer.py     # Adaptive audio buffering
│   └── message_bridge.py   # Binary ↔ JSON translation
├── frontend/               # Svelte frontend
│   ├── src/
│   │   ├── App.svelte
│   │   ├── lib/
│   │   └── components/
│   └── package.json
└── docs/
    ├── protocol-reference.md
    └── plans/
```

## Development Commands

```bash
# Gateway
cd webserver/gateway
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python main.py --host <pihpsdr-ip> --port 45454

# Frontend
cd webserver/frontend
npm install
npm run dev
```

## Safety Notes

piHPSDR server has built-in fail-safe:
- TCP disconnect → `remoteclient.running = FALSE`
- Server forces TX off when client dies
- Gateway adds additional watchdog layer
