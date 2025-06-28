# piHPSDR Web Interface Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Create a web-based remote interface for piHPSDR that provides full control over Internet connections with moderate latency.

**Architecture:** Python asyncio gateway bridges TCP (piHPSDR protocol) to WebSocket (browser). Svelte frontend renders spectrum via Canvas 2D and plays audio via Web Audio API.

**Tech Stack:** Python 3.11+ (asyncio, websockets, struct, numpy), Svelte 4, Vite, Web Audio API

---

## Phase 1: Gateway Foundation

### Task 1: Project Structure Setup

**Files:**
- Create: `webserver/gateway/requirements.txt`
- Create: `webserver/gateway/protocol.py`
- Create: `webserver/frontend/package.json`

**Step 1: Create directory structure**

```bash
mkdir -p webserver/gateway webserver/frontend/src/lib webserver/frontend/src/components
```

**Step 2: Create requirements.txt**

```
websockets>=12.0
numpy>=1.24.0
```

**Step 3: Create protocol.py with header parsing**

```python
"""piHPSDR protocol definitions and parsing utilities."""
import struct
from dataclasses import dataclass
from enum import IntEnum
from typing import Optional

# Sync bytes
SYNC_BYTES = bytes([0xFA, 0xFA, 0xAF, 0xAF])

class CmdType(IntEnum):
    """Command types (client -> server)."""
    CMD_ADC = 34
    CMD_AGC = 35
    CMD_AGC_GAIN = 36
    CMD_FREQ = 58
    CMD_HEARTBEAT = 59
    CMD_MODE = 63
    CMD_MOX = 66
    CMD_MUTE_RX = 67
    CMD_FILTER_SEL = 56
    CMD_VOLUME = 122
    CMD_ZOOM = 126
    CMD_START_RADIO = 103
    CMD_RX_SPECTRUM = 93
    CMD_TX_SPECTRUM = 117

class InfoType(IntEnum):
    """Info types (server -> client)."""
    INFO_ADC = 127
    INFO_BAND = 128
    INFO_BANDSTACK = 129
    INFO_RADIO = 133
    INFO_RECEIVER = 134
    INFO_RXAUDIO = 135
    INFO_SPECTRUM = 136
    INFO_TRANSMITTER = 138
    INFO_VFO = 140

# Header: sync(4) + data_type(2) + b1(1) + b2(1) + s1(2) + s2(2) = 12 bytes
HEADER_SIZE = 12
HEADER_FORMAT = '>4sHBBHH'  # Big-endian

@dataclass
class Header:
    """Packet header."""
    sync: bytes
    data_type: int
    b1: int
    b2: int
    s1: int
    s2: int

    @classmethod
    def parse(cls, data: bytes) -> 'Header':
        sync, dtype, b1, b2, s1, s2 = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
        return cls(sync, dtype, b1, b2, s1, s2)

    def pack(self) -> bytes:
        return struct.pack(HEADER_FORMAT, SYNC_BYTES, self.data_type,
                          self.b1, self.b2, self.s1, self.s2)

def encode_double(value: float) -> int:
    """Encode double to uint64 per piHPSDR protocol."""
    return int((value + 9e8) * 1e10)

def decode_double(encoded: int) -> float:
    """Decode uint64 to double per piHPSDR protocol."""
    return encoded * 1e-10 - 9e8

# Mode names
MODES = ['LSB', 'USB', 'DSB', 'CWL', 'CWU', 'FM', 'AM', 'DIGU', 'SPEC', 'DIGL', 'SAM', 'DRM']
```

**Step 4: Commit**

```bash
git add webserver/
git commit -m "feat: initial project structure with protocol definitions"
```

---

### Task 2: TCP Client for piHPSDR

**Files:**
- Create: `webserver/gateway/pihpsdr_client.py`
- Create: `webserver/gateway/test_protocol.py`

**Step 1: Write failing test for header parsing**

```python
# webserver/gateway/test_protocol.py
import pytest
from protocol import Header, SYNC_BYTES, HEADER_SIZE, CmdType

def test_header_parse():
    # Construct a valid header: sync + CMD_FREQ(58) + b1=0 + b2=1 + s1=0 + s2=0
    data = SYNC_BYTES + bytes([0x00, 0x3A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00])
    header = Header.parse(data)
    assert header.data_type == CmdType.CMD_FREQ
    assert header.b1 == 0
    assert header.b2 == 1
```

**Step 2: Run test to verify it passes (protocol.py already has implementation)**

```bash
cd webserver/gateway
python -m pytest test_protocol.py -v
```

Expected: PASS

**Step 3: Create pihpsdr_client.py**

```python
"""TCP client for piHPSDR server."""
import asyncio
import struct
import logging
from typing import Callable, Optional, Any
from protocol import (
    Header, HEADER_SIZE, SYNC_BYTES, CmdType, InfoType,
    encode_double, decode_double
)

logger = logging.getLogger(__name__)

# Packet sizes (after header)
SPECTRUM_EXTRA_SIZE = 8*8 + 6*8 + 2 + 2 + 4096  # doubles + uint64s + width + id/avail + samples
RXAUDIO_EXTRA_SIZE = 2 + 512*2 + 1  # numsamples + samples + rx
VFO_EXTRA_SIZE = 7*8 + 2*2 + 9  # frequencies + shorts + bytes
RECEIVER_EXTRA_SIZE = 800  # approximate, varies
RADIO_EXTRA_SIZE = 2500  # approximate

class PihpsdrClient:
    """Async TCP client for piHPSDR."""

    def __init__(self, host: str, port: int = 45454):
        self.host = host
        self.port = port
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.running = False
        self.on_spectrum: Optional[Callable] = None
        self.on_audio: Optional[Callable] = None
        self.on_vfo: Optional[Callable] = None
        self.on_receiver: Optional[Callable] = None
        self.on_radio: Optional[Callable] = None

    async def connect(self) -> bool:
        """Connect to piHPSDR server."""
        try:
            self.reader, self.writer = await asyncio.open_connection(
                self.host, self.port
            )
            self.running = True
            logger.info(f"Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            logger.error(f"Connection failed: {e}")
            return False

    async def disconnect(self):
        """Disconnect from server."""
        self.running = False
        if self.writer:
            self.writer.close()
            await self.writer.wait_closed()

    async def send_raw(self, data: bytes):
        """Send raw bytes to server."""
        if self.writer:
            self.writer.write(data)
            await self.writer.drain()

    async def send_header_only(self, cmd: CmdType, b1: int = 0, b2: int = 0,
                                s1: int = 0, s2: int = 0):
        """Send a header-only command."""
        header = Header(SYNC_BYTES, cmd, b1, b2, s1, s2)
        await self.send_raw(header.pack())

    async def send_start_radio(self):
        """Send CMD_START_RADIO."""
        await self.send_header_only(CmdType.CMD_START_RADIO)

    async def send_heartbeat(self):
        """Send CMD_HEARTBEAT."""
        await self.send_header_only(CmdType.CMD_HEARTBEAT)

    async def send_frequency(self, vfo: int, freq_hz: int):
        """Send CMD_FREQ to set VFO frequency."""
        header = Header(SYNC_BYTES, CmdType.CMD_FREQ, vfo, 0, 0, 0)
        data = header.pack() + struct.pack('>Q', freq_hz)
        await self.send_raw(data)

    async def send_volume(self, rx: int, volume: float):
        """Send CMD_VOLUME (0.0 to 1.0)."""
        header = Header(SYNC_BYTES, CmdType.CMD_VOLUME, rx, 0, 0, 0)
        data = header.pack() + struct.pack('>Q', encode_double(volume))
        await self.send_raw(data)

    async def send_mox(self, state: bool):
        """Send CMD_MOX to set TX state."""
        await self.send_header_only(CmdType.CMD_MOX, b1=1 if state else 0)

    async def send_mode(self, vfo: int, mode: int):
        """Send CMD_MODE."""
        await self.send_header_only(CmdType.CMD_MODE, b1=vfo, b2=mode)

    async def send_rx_spectrum(self, rx: int, enable: bool):
        """Enable/disable spectrum for receiver."""
        await self.send_header_only(CmdType.CMD_RX_SPECTRUM, b1=rx, b2=1 if enable else 0)

    async def _read_exact(self, n: int) -> bytes:
        """Read exactly n bytes."""
        data = await self.reader.readexactly(n)
        return data

    async def receive_loop(self):
        """Main receive loop - parse incoming packets."""
        while self.running:
            try:
                # Read header
                header_data = await self._read_exact(HEADER_SIZE)
                header = Header.parse(header_data)

                if header.sync != SYNC_BYTES:
                    logger.warning("Invalid sync bytes, resynchronizing...")
                    continue

                # Dispatch based on type
                await self._dispatch_packet(header)

            except asyncio.IncompleteReadError:
                logger.info("Connection closed by server")
                self.running = False
            except Exception as e:
                logger.error(f"Receive error: {e}")
                self.running = False

    async def _dispatch_packet(self, header: Header):
        """Dispatch packet to appropriate handler."""
        dtype = header.data_type

        if dtype == InfoType.INFO_SPECTRUM:
            await self._handle_spectrum(header)
        elif dtype == InfoType.INFO_RXAUDIO:
            await self._handle_audio(header)
        elif dtype == InfoType.INFO_VFO:
            await self._handle_vfo(header)
        elif dtype == InfoType.INFO_RECEIVER:
            await self._handle_receiver(header)
        elif dtype == InfoType.INFO_RADIO:
            await self._handle_radio(header)
        else:
            # Skip unknown packets - read based on type
            await self._skip_packet(dtype)

    async def _handle_spectrum(self, header: Header):
        """Parse INFO_SPECTRUM packet."""
        data = await self._read_exact(SPECTRUM_EXTRA_SIZE)

        # Parse key fields
        offset = 0
        meter = decode_double(struct.unpack_from('>Q', data, offset)[0]); offset += 8
        alc = decode_double(struct.unpack_from('>Q', data, offset)[0]); offset += 8
        fwd = decode_double(struct.unpack_from('>Q', data, offset)[0]); offset += 8
        swr = decode_double(struct.unpack_from('>Q', data, offset)[0]); offset += 8
        offset += 4*8  # Skip cA, cB, cAp, cBp

        vfo_a_freq = struct.unpack_from('>Q', data, offset)[0]; offset += 8
        vfo_b_freq = struct.unpack_from('>Q', data, offset)[0]; offset += 8
        offset += 4*8  # Skip ctun and offset fields

        width = struct.unpack_from('>H', data, offset)[0]; offset += 2
        rx_id = data[offset]; offset += 1
        avail = data[offset]; offset += 1

        samples = data[offset:offset + width]

        if self.on_spectrum:
            await self.on_spectrum({
                'id': rx_id,
                'width': width,
                'samples': samples,
                'meter': meter,
                'vfo_a_freq': vfo_a_freq,
                'vfo_b_freq': vfo_b_freq,
            })

    async def _handle_audio(self, header: Header):
        """Parse INFO_RXAUDIO packet."""
        data = await self._read_exact(RXAUDIO_EXTRA_SIZE)

        numsamples = struct.unpack_from('>H', data, 0)[0]
        samples = struct.unpack_from(f'>{numsamples}h', data, 2)
        rx = data[2 + numsamples*2]

        if self.on_audio:
            await self.on_audio({
                'rx': rx,
                'samples': samples,
            })

    async def _handle_vfo(self, header: Header):
        """Parse INFO_VFO packet."""
        data = await self._read_exact(VFO_EXTRA_SIZE)

        freq = struct.unpack_from('>Q', data, 0)[0]
        ctun_freq = struct.unpack_from('>Q', data, 8)[0]
        mode = data[56 + 3]  # Offset to mode byte

        if self.on_vfo:
            await self.on_vfo({
                'vfo': header.b1,
                'frequency': freq,
                'ctun_frequency': ctun_freq,
                'mode': mode,
            })

    async def _handle_receiver(self, header: Header):
        """Parse INFO_RECEIVER packet (simplified)."""
        data = await self._read_exact(RECEIVER_EXTRA_SIZE)
        # Full parsing would extract all fields
        if self.on_receiver:
            await self.on_receiver({'raw': data})

    async def _handle_radio(self, header: Header):
        """Parse INFO_RADIO packet (simplified)."""
        data = await self._read_exact(RADIO_EXTRA_SIZE)
        if self.on_radio:
            await self.on_radio({'raw': data})

    async def _skip_packet(self, dtype: int):
        """Skip unknown packet types."""
        # For unknown types, we need a size map or protocol knowledge
        # This is a simplified approach
        pass
```

**Step 4: Commit**

```bash
git add webserver/gateway/pihpsdr_client.py webserver/gateway/test_protocol.py
git commit -m "feat: TCP client for piHPSDR protocol"
```

---

### Task 3: WebSocket Server

**Files:**
- Create: `webserver/gateway/websocket_server.py`

**Step 1: Create websocket_server.py**

```python
"""WebSocket server for browser clients."""
import asyncio
import json
import logging
from typing import Set, Dict, Any, Optional
import websockets
from websockets.server import WebSocketServerProtocol

logger = logging.getLogger(__name__)

class WebSocketServer:
    """WebSocket server for browser clients."""

    def __init__(self, host: str = '0.0.0.0', port: int = 8765):
        self.host = host
        self.port = port
        self.clients: Set[WebSocketServerProtocol] = set()
        self.on_command: Optional[callable] = None
        self.server = None

    async def start(self):
        """Start WebSocket server."""
        self.server = await websockets.serve(
            self._handle_client,
            self.host,
            self.port
        )
        logger.info(f"WebSocket server listening on {self.host}:{self.port}")

    async def stop(self):
        """Stop WebSocket server."""
        if self.server:
            self.server.close()
            await self.server.wait_closed()

    async def _handle_client(self, websocket: WebSocketServerProtocol):
        """Handle a connected client."""
        self.clients.add(websocket)
        client_addr = websocket.remote_address
        logger.info(f"Client connected: {client_addr}")

        try:
            async for message in websocket:
                await self._process_message(websocket, message)
        except websockets.exceptions.ConnectionClosed:
            logger.info(f"Client disconnected: {client_addr}")
        finally:
            self.clients.discard(websocket)

    async def _process_message(self, websocket: WebSocketServerProtocol, message: str):
        """Process incoming message from browser."""
        try:
            data = json.loads(message)
            cmd = data.get('cmd')

            if self.on_command:
                await self.on_command(cmd, data)
        except json.JSONDecodeError:
            logger.warning(f"Invalid JSON: {message}")

    async def broadcast_json(self, data: Dict[str, Any]):
        """Broadcast JSON to all clients."""
        if not self.clients:
            return
        message = json.dumps(data)
        await asyncio.gather(
            *[client.send(message) for client in self.clients],
            return_exceptions=True
        )

    async def broadcast_binary(self, data: bytes):
        """Broadcast binary data to all clients."""
        if not self.clients:
            return
        await asyncio.gather(
            *[client.send(data) for client in self.clients],
            return_exceptions=True
        )

    async def send_spectrum(self, rx_id: int, samples: bytes, width: int):
        """Send spectrum data as binary: [0x01][rx_id][width:u16][samples...]."""
        header = bytes([0x01, rx_id]) + width.to_bytes(2, 'big')
        await self.broadcast_binary(header + samples)

    async def send_audio(self, rx_id: int, samples: tuple):
        """Send audio data as binary: [0x02][rx_id][count:u16][samples...]."""
        import struct
        header = bytes([0x02, rx_id]) + len(samples).to_bytes(2, 'big')
        audio_data = struct.pack(f'>{len(samples)}h', *samples)
        await self.broadcast_binary(header + audio_data)
```

**Step 2: Commit**

```bash
git add webserver/gateway/websocket_server.py
git commit -m "feat: WebSocket server for browser clients"
```

---

### Task 4: Main Gateway Entry Point

**Files:**
- Create: `webserver/gateway/main.py`

**Step 1: Create main.py**

```python
"""piHPSDR Web Gateway - main entry point."""
import asyncio
import argparse
import logging
import signal
from pihpsdr_client import PihpsdrClient
from websocket_server import WebSocketServer

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class Gateway:
    """Bridge between piHPSDR and web browsers."""

    def __init__(self, pihpsdr_host: str, pihpsdr_port: int = 45454,
                 ws_host: str = '0.0.0.0', ws_port: int = 8765):
        self.pihpsdr = PihpsdrClient(pihpsdr_host, pihpsdr_port)
        self.ws_server = WebSocketServer(ws_host, ws_port)
        self.running = False
        self.heartbeat_task = None

        # Wire up callbacks
        self.pihpsdr.on_spectrum = self._on_spectrum
        self.pihpsdr.on_audio = self._on_audio
        self.pihpsdr.on_vfo = self._on_vfo
        self.ws_server.on_command = self._on_browser_command

    async def start(self):
        """Start the gateway."""
        logger.info("Starting gateway...")

        # Start WebSocket server
        await self.ws_server.start()

        # Connect to piHPSDR
        if not await self.pihpsdr.connect():
            logger.error("Failed to connect to piHPSDR")
            return False

        self.running = True

        # Start receive loop
        receive_task = asyncio.create_task(self.pihpsdr.receive_loop())

        # Start heartbeat
        self.heartbeat_task = asyncio.create_task(self._heartbeat_loop())

        # Wait a moment for initial data, then start radio
        await asyncio.sleep(1)
        await self.pihpsdr.send_start_radio()
        await self.pihpsdr.send_rx_spectrum(0, True)

        logger.info("Gateway started successfully")

        # Wait for receive loop to end
        await receive_task

        return True

    async def stop(self):
        """Stop the gateway."""
        logger.info("Stopping gateway...")
        self.running = False

        # Force MOX off for safety
        try:
            await self.pihpsdr.send_mox(False)
        except:
            pass

        if self.heartbeat_task:
            self.heartbeat_task.cancel()

        await self.pihpsdr.disconnect()
        await self.ws_server.stop()

    async def _heartbeat_loop(self):
        """Send periodic heartbeats."""
        while self.running:
            try:
                await self.pihpsdr.send_heartbeat()
                await asyncio.sleep(2)
            except asyncio.CancelledError:
                break

    async def _on_spectrum(self, data: dict):
        """Handle spectrum data from piHPSDR."""
        await self.ws_server.send_spectrum(
            data['id'],
            data['samples'],
            data['width']
        )
        # Also send meter data as JSON
        await self.ws_server.broadcast_json({
            'type': 'meter',
            's': data['meter'],
            'vfo_a': data['vfo_a_freq'],
            'vfo_b': data['vfo_b_freq'],
        })

    async def _on_audio(self, data: dict):
        """Handle audio data from piHPSDR."""
        await self.ws_server.send_audio(data['rx'], data['samples'])

    async def _on_vfo(self, data: dict):
        """Handle VFO update from piHPSDR."""
        await self.ws_server.broadcast_json({
            'type': 'vfo',
            'id': data['vfo'],
            'freq': data['frequency'],
            'mode': data['mode'],
        })

    async def _on_browser_command(self, cmd: str, data: dict):
        """Handle command from browser."""
        logger.debug(f"Browser command: {cmd} {data}")

        if cmd == 'freq':
            vfo = data.get('vfo', 0)
            hz = data.get('hz', 0)
            await self.pihpsdr.send_frequency(vfo, hz)

        elif cmd == 'mode':
            vfo = data.get('vfo', 0)
            mode = data.get('mode', 0)
            await self.pihpsdr.send_mode(vfo, mode)

        elif cmd == 'volume':
            rx = data.get('rx', 0)
            vol = data.get('value', 0.5)
            await self.pihpsdr.send_volume(rx, vol)

        elif cmd == 'mox':
            state = data.get('state', False)
            await self.pihpsdr.send_mox(state)


async def main():
    parser = argparse.ArgumentParser(description='piHPSDR Web Gateway')
    parser.add_argument('--host', required=True, help='piHPSDR server IP')
    parser.add_argument('--port', type=int, default=45454, help='piHPSDR server port')
    parser.add_argument('--ws-port', type=int, default=8765, help='WebSocket port')
    args = parser.parse_args()

    gateway = Gateway(args.host, args.port, ws_port=args.ws_port)

    # Handle signals
    loop = asyncio.get_event_loop()
    for sig in (signal.SIGTERM, signal.SIGINT):
        loop.add_signal_handler(sig, lambda: asyncio.create_task(gateway.stop()))

    try:
        await gateway.start()
    except KeyboardInterrupt:
        pass
    finally:
        await gateway.stop()


if __name__ == '__main__':
    asyncio.run(main())
```

**Step 2: Commit**

```bash
git add webserver/gateway/main.py
git commit -m "feat: main gateway entry point with command bridging"
```

---

## Phase 2: Frontend Foundation

### Task 5: Svelte Project Setup

**Files:**
- Create: `webserver/frontend/package.json`
- Create: `webserver/frontend/vite.config.js`
- Create: `webserver/frontend/index.html`
- Create: `webserver/frontend/src/main.js`
- Create: `webserver/frontend/src/App.svelte`

**Step 1: Create package.json**

```json
{
  "name": "pihpsdr-web",
  "version": "0.1.0",
  "private": true,
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "vite build",
    "preview": "vite preview"
  },
  "devDependencies": {
    "@sveltejs/vite-plugin-svelte": "^3.0.0",
    "svelte": "^4.0.0",
    "vite": "^5.0.0"
  }
}
```

**Step 2: Create vite.config.js**

```javascript
import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';

export default defineConfig({
  plugins: [svelte()],
  server: {
    port: 5173,
    host: true
  }
});
```

**Step 3: Create index.html**

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>piHPSDR Web</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #1a1a1a;
      color: #e0e0e0;
    }
  </style>
</head>
<body>
  <div id="app"></div>
  <script type="module" src="/src/main.js"></script>
</body>
</html>
```

**Step 4: Create src/main.js**

```javascript
import App from './App.svelte';

const app = new App({
  target: document.getElementById('app')
});

export default app;
```

**Step 5: Create src/App.svelte (minimal)**

```svelte
<script>
  let connected = false;
  let frequency = 7074000;
</script>

<main>
  <h1>piHPSDR Web</h1>
  <p>Status: {connected ? 'Connected' : 'Disconnected'}</p>
  <p>Frequency: {(frequency / 1e6).toFixed(6)} MHz</p>
</main>

<style>
  main {
    padding: 20px;
    max-width: 1200px;
    margin: 0 auto;
  }
  h1 {
    color: #4CAF50;
    margin-bottom: 20px;
  }
</style>
```

**Step 6: Commit**

```bash
git add webserver/frontend/
git commit -m "feat: Svelte frontend project setup"
```

---

### Task 6: WebSocket Client Module

**Files:**
- Create: `webserver/frontend/src/lib/websocket.js`
- Create: `webserver/frontend/src/lib/stores.js`

**Step 1: Create stores.js**

```javascript
import { writable } from 'svelte/store';

export const connected = writable(false);
export const vfoA = writable({ freq: 7074000, mode: 1 });
export const vfoB = writable({ freq: 7074000, mode: 1 });
export const meter = writable({ s: -120 });
export const spectrum = writable({ samples: new Uint8Array(800), width: 800 });
```

**Step 2: Create websocket.js**

```javascript
import { connected, vfoA, meter, spectrum } from './stores.js';

class WebSocketClient {
  constructor() {
    this.ws = null;
    this.reconnectDelay = 1000;
    this.maxReconnectDelay = 30000;
    this.onAudio = null;
  }

  connect(url) {
    this.url = url;
    this._connect();
  }

  _connect() {
    console.log(`Connecting to ${this.url}...`);
    this.ws = new WebSocket(this.url);
    this.ws.binaryType = 'arraybuffer';

    this.ws.onopen = () => {
      console.log('WebSocket connected');
      connected.set(true);
      this.reconnectDelay = 1000;
    };

    this.ws.onclose = () => {
      console.log('WebSocket disconnected');
      connected.set(false);
      this._scheduleReconnect();
    };

    this.ws.onerror = (err) => {
      console.error('WebSocket error:', err);
    };

    this.ws.onmessage = (event) => {
      if (event.data instanceof ArrayBuffer) {
        this._handleBinary(event.data);
      } else {
        this._handleJson(event.data);
      }
    };
  }

  _scheduleReconnect() {
    setTimeout(() => {
      this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.maxReconnectDelay);
      this._connect();
    }, this.reconnectDelay);
  }

  _handleBinary(buffer) {
    const view = new DataView(buffer);
    const type = view.getUint8(0);

    if (type === 0x01) {
      // Spectrum: [0x01][rx_id][width:u16][samples...]
      const rxId = view.getUint8(1);
      const width = view.getUint16(2);
      const samples = new Uint8Array(buffer, 4, width);
      spectrum.set({ samples, width, rxId });
    } else if (type === 0x02) {
      // Audio: [0x02][rx_id][count:u16][samples...]
      const rxId = view.getUint8(1);
      const count = view.getUint16(2);
      const samples = new Int16Array(buffer, 4, count);
      if (this.onAudio) {
        this.onAudio(rxId, samples);
      }
    }
  }

  _handleJson(data) {
    try {
      const msg = JSON.parse(data);

      if (msg.type === 'vfo') {
        if (msg.id === 0) {
          vfoA.set({ freq: msg.freq, mode: msg.mode });
        } else {
          // vfoB.set(...)
        }
      } else if (msg.type === 'meter') {
        meter.set({ s: msg.s, vfo_a: msg.vfo_a, vfo_b: msg.vfo_b });
      }
    } catch (e) {
      console.error('Invalid JSON:', e);
    }
  }

  send(cmd, data = {}) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ cmd, ...data }));
    }
  }

  setFrequency(vfo, hz) {
    this.send('freq', { vfo, hz });
  }

  setMode(vfo, mode) {
    this.send('mode', { vfo, mode });
  }

  setVolume(rx, value) {
    this.send('volume', { rx, value });
  }

  setMox(state) {
    this.send('mox', { state });
  }
}

export const wsClient = new WebSocketClient();
```

**Step 3: Commit**

```bash
git add webserver/frontend/src/lib/
git commit -m "feat: WebSocket client with Svelte stores"
```

---

### Task 7: Waterfall Component

**Files:**
- Create: `webserver/frontend/src/components/Waterfall.svelte`

**Step 1: Create Waterfall.svelte**

```svelte
<script>
  import { onMount, onDestroy } from 'svelte';
  import { spectrum } from '../lib/stores.js';

  export let width = 800;
  export let height = 300;
  export let waterfallHeight = 200;

  let canvas;
  let ctx;
  let waterfallCanvas;
  let waterfallCtx;
  let waterfallImageData;
  let animationId;

  // Color gradient for spectrum (blue -> green -> yellow -> red)
  const gradient = [];
  for (let i = 0; i < 256; i++) {
    if (i < 64) {
      gradient.push([0, 0, i * 4]);
    } else if (i < 128) {
      gradient.push([0, (i - 64) * 4, 255 - (i - 64) * 4]);
    } else if (i < 192) {
      gradient.push([(i - 128) * 4, 255, 0]);
    } else {
      gradient.push([255, 255 - (i - 192) * 4, 0]);
    }
  }

  onMount(() => {
    ctx = canvas.getContext('2d');
    waterfallCtx = waterfallCanvas.getContext('2d');
    waterfallImageData = waterfallCtx.createImageData(width, waterfallHeight);

    // Subscribe to spectrum updates
    const unsubscribe = spectrum.subscribe(draw);

    return () => {
      unsubscribe();
      if (animationId) cancelAnimationFrame(animationId);
    };
  });

  function draw(data) {
    if (!ctx || !data.samples) return;

    const { samples } = data;
    const spectrumHeight = height - waterfallHeight;

    // Clear spectrum area
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, width, spectrumHeight);

    // Draw spectrum line
    ctx.strokeStyle = '#4CAF50';
    ctx.lineWidth = 1;
    ctx.beginPath();

    for (let x = 0; x < width && x < samples.length; x++) {
      const dbm = samples[x];
      const y = spectrumHeight - (dbm / 255) * spectrumHeight;
      if (x === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    ctx.stroke();

    // Fill under curve
    ctx.lineTo(width, spectrumHeight);
    ctx.lineTo(0, spectrumHeight);
    ctx.closePath();
    ctx.fillStyle = 'rgba(76, 175, 80, 0.3)';
    ctx.fill();

    // Update waterfall
    updateWaterfall(samples);
  }

  function updateWaterfall(samples) {
    // Scroll waterfall down
    const imgData = waterfallImageData.data;
    const rowSize = width * 4;

    // Move existing data down one row
    for (let y = waterfallHeight - 1; y > 0; y--) {
      const destOffset = y * rowSize;
      const srcOffset = (y - 1) * rowSize;
      for (let i = 0; i < rowSize; i++) {
        imgData[destOffset + i] = imgData[srcOffset + i];
      }
    }

    // Draw new row at top
    for (let x = 0; x < width && x < samples.length; x++) {
      const dbm = samples[x];
      const [r, g, b] = gradient[dbm] || [0, 0, 0];
      const offset = x * 4;
      imgData[offset] = r;
      imgData[offset + 1] = g;
      imgData[offset + 2] = b;
      imgData[offset + 3] = 255;
    }

    waterfallCtx.putImageData(waterfallImageData, 0, 0);
  }

  function handleClick(event) {
    // Calculate frequency from click position
    const rect = canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const ratio = x / width;
    // Dispatch event for frequency change
    canvas.dispatchEvent(new CustomEvent('freqclick', {
      detail: { ratio },
      bubbles: true
    }));
  }
</script>

<div class="waterfall-container">
  <canvas
    bind:this={canvas}
    {width}
    height={height - waterfallHeight}
    on:click={handleClick}
  />
  <canvas
    bind:this={waterfallCanvas}
    {width}
    height={waterfallHeight}
    on:click={handleClick}
  />
</div>

<style>
  .waterfall-container {
    display: flex;
    flex-direction: column;
    background: #1a1a1a;
    border: 1px solid #333;
    border-radius: 4px;
    overflow: hidden;
  }
  canvas {
    display: block;
    cursor: crosshair;
  }
</style>
```

**Step 2: Commit**

```bash
git add webserver/frontend/src/components/Waterfall.svelte
git commit -m "feat: Canvas 2D waterfall component"
```

---

### Task 8: Audio Player Module

**Files:**
- Create: `webserver/frontend/src/lib/audioPlayer.js`

**Step 1: Create audioPlayer.js**

```javascript
/**
 * Audio player using Web Audio API with adaptive buffering.
 */
export class AudioPlayer {
  constructor(sampleRate = 48000) {
    this.sampleRate = sampleRate;
    this.ctx = null;
    this.bufferQueue = [];
    this.isPlaying = false;
    this.nextPlayTime = 0;
    this.bufferDuration = 0.1; // 100ms buffer
    this.minBufferSize = 3;    // Minimum buffers before starting
  }

  async init() {
    if (this.ctx) return;

    this.ctx = new (window.AudioContext || window.webkitAudioContext)({
      sampleRate: this.sampleRate
    });

    // Resume context (required by browsers)
    if (this.ctx.state === 'suspended') {
      await this.ctx.resume();
    }

    console.log('AudioContext initialized, sample rate:', this.ctx.sampleRate);
  }

  /**
   * Add audio samples to the queue.
   * @param {Int16Array} samples - Signed 16-bit PCM samples
   */
  addSamples(samples) {
    if (!this.ctx) return;

    // Convert Int16 to Float32
    const floatSamples = new Float32Array(samples.length);
    for (let i = 0; i < samples.length; i++) {
      floatSamples[i] = samples[i] / 32768;
    }

    // Create audio buffer
    const buffer = this.ctx.createBuffer(1, floatSamples.length, this.sampleRate);
    buffer.getChannelData(0).set(floatSamples);

    this.bufferQueue.push(buffer);

    // Start playback if we have enough buffers
    if (!this.isPlaying && this.bufferQueue.length >= this.minBufferSize) {
      this._startPlayback();
    }
  }

  _startPlayback() {
    if (this.isPlaying) return;

    this.isPlaying = true;
    this.nextPlayTime = this.ctx.currentTime + this.bufferDuration;
    this._scheduleBuffers();
  }

  _scheduleBuffers() {
    while (this.bufferQueue.length > 0) {
      const buffer = this.bufferQueue.shift();

      const source = this.ctx.createBufferSource();
      source.buffer = buffer;
      source.connect(this.ctx.destination);

      // Schedule playback
      const playTime = Math.max(this.nextPlayTime, this.ctx.currentTime);
      source.start(playTime);

      this.nextPlayTime = playTime + buffer.duration;
    }

    // Check for underrun
    if (this.bufferQueue.length === 0) {
      this.isPlaying = false;
    }
  }

  setVolume(value) {
    // For future: add gain node
  }

  stop() {
    if (this.ctx) {
      this.ctx.close();
      this.ctx = null;
    }
    this.bufferQueue = [];
    this.isPlaying = false;
  }
}

export const audioPlayer = new AudioPlayer();
```

**Step 2: Commit**

```bash
git add webserver/frontend/src/lib/audioPlayer.js
git commit -m "feat: Web Audio API player with buffering"
```

---

### Task 9: VFO Display Component

**Files:**
- Create: `webserver/frontend/src/components/VfoDisplay.svelte`

**Step 1: Create VfoDisplay.svelte**

```svelte
<script>
  import { vfoA } from '../lib/stores.js';
  import { wsClient } from '../lib/websocket.js';

  const MODES = ['LSB', 'USB', 'DSB', 'CWL', 'CWU', 'FM', 'AM', 'DIGU', 'SPEC', 'DIGL', 'SAM', 'DRM'];

  let frequency = 7074000;
  let mode = 1;

  vfoA.subscribe(v => {
    frequency = v.freq;
    mode = v.mode;
  });

  function formatFrequency(hz) {
    const mhz = hz / 1e6;
    return mhz.toFixed(6);
  }

  function handleWheel(event) {
    event.preventDefault();
    const step = event.shiftKey ? 1000 : 100;
    const delta = event.deltaY > 0 ? -step : step;
    const newFreq = frequency + delta;
    wsClient.setFrequency(0, newFreq);
  }

  function handleModeChange(event) {
    const newMode = parseInt(event.target.value);
    wsClient.setMode(0, newMode);
  }
</script>

<div class="vfo-display" on:wheel={handleWheel}>
  <div class="frequency">
    <span class="mhz">{formatFrequency(frequency).split('.')[0]}</span>
    <span class="decimal">.</span>
    <span class="khz">{formatFrequency(frequency).split('.')[1]}</span>
    <span class="unit">MHz</span>
  </div>

  <select class="mode-select" value={mode} on:change={handleModeChange}>
    {#each MODES as modeName, i}
      <option value={i}>{modeName}</option>
    {/each}
  </select>
</div>

<style>
  .vfo-display {
    display: flex;
    align-items: center;
    gap: 20px;
    padding: 15px 20px;
    background: #2a2a2a;
    border-radius: 8px;
    user-select: none;
  }

  .frequency {
    font-family: 'Courier New', monospace;
    font-size: 2.5rem;
    font-weight: bold;
    cursor: ns-resize;
  }

  .mhz {
    color: #4CAF50;
  }

  .decimal {
    color: #666;
  }

  .khz {
    color: #8BC34A;
  }

  .unit {
    font-size: 1rem;
    color: #666;
    margin-left: 8px;
  }

  .mode-select {
    font-size: 1.2rem;
    padding: 8px 16px;
    background: #333;
    color: #fff;
    border: 1px solid #444;
    border-radius: 4px;
    cursor: pointer;
  }

  .mode-select:hover {
    background: #3a3a3a;
  }
</style>
```

**Step 2: Commit**

```bash
git add webserver/frontend/src/components/VfoDisplay.svelte
git commit -m "feat: VFO display component with wheel tuning"
```

---

### Task 10: Integrate Components in App.svelte

**Files:**
- Modify: `webserver/frontend/src/App.svelte`

**Step 1: Update App.svelte**

```svelte
<script>
  import { onMount } from 'svelte';
  import { connected, meter } from './lib/stores.js';
  import { wsClient } from './lib/websocket.js';
  import { audioPlayer } from './lib/audioPlayer.js';
  import Waterfall from './components/Waterfall.svelte';
  import VfoDisplay from './components/VfoDisplay.svelte';

  let gatewayUrl = 'ws://localhost:8765';
  let sMeter = -120;

  meter.subscribe(m => {
    sMeter = m.s;
  });

  onMount(async () => {
    // Initialize audio
    await audioPlayer.init();

    // Connect audio callback
    wsClient.onAudio = (rxId, samples) => {
      audioPlayer.addSamples(samples);
    };

    // Connect to gateway
    wsClient.connect(gatewayUrl);
  });

  function formatSMeter(dbm) {
    if (dbm >= -73) return 'S9+' + Math.round(dbm + 73) + 'dB';
    const s = Math.max(0, Math.round((dbm + 127) / 6));
    return 'S' + s;
  }
</script>

<main>
  <header>
    <h1>piHPSDR Web</h1>
    <div class="status" class:connected={$connected}>
      {$connected ? 'Connected' : 'Disconnected'}
    </div>
  </header>

  <section class="controls">
    <VfoDisplay />
    <div class="meter">
      <span class="label">S-Meter:</span>
      <span class="value">{formatSMeter(sMeter)}</span>
      <span class="dbm">({sMeter.toFixed(0)} dBm)</span>
    </div>
  </section>

  <section class="display">
    <Waterfall width={800} height={400} waterfallHeight={250} />
  </section>
</main>

<style>
  main {
    padding: 20px;
    max-width: 1000px;
    margin: 0 auto;
  }

  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
  }

  h1 {
    color: #4CAF50;
    margin: 0;
  }

  .status {
    padding: 6px 12px;
    border-radius: 4px;
    background: #ff5252;
    color: white;
    font-weight: bold;
  }

  .status.connected {
    background: #4CAF50;
  }

  .controls {
    display: flex;
    gap: 20px;
    align-items: center;
    margin-bottom: 20px;
    flex-wrap: wrap;
  }

  .meter {
    background: #2a2a2a;
    padding: 15px 20px;
    border-radius: 8px;
  }

  .meter .label {
    color: #888;
    margin-right: 10px;
  }

  .meter .value {
    font-size: 1.5rem;
    font-weight: bold;
    color: #4CAF50;
  }

  .meter .dbm {
    color: #666;
    margin-left: 10px;
  }

  .display {
    margin-top: 20px;
  }
</style>
```

**Step 2: Commit**

```bash
git add webserver/frontend/src/App.svelte
git commit -m "feat: integrate all components in main App"
```

---

## Phase 3: Testing & Polish

### Task 11: End-to-End Test

**Step 1: Install frontend dependencies**

```bash
cd webserver/frontend
npm install
```

**Step 2: Create Python virtual environment**

```bash
cd webserver/gateway
python -m venv venv
source venv/bin/activate  # or venv\Scripts\activate on Windows
pip install -r requirements.txt
```

**Step 3: Start gateway (with piHPSDR running)**

```bash
python main.py --host <pihpsdr-ip>
```

**Step 4: Start frontend dev server**

```bash
cd webserver/frontend
npm run dev
```

**Step 5: Open browser at http://localhost:5173**

Expected:
- Connection status shows "Connected"
- Waterfall displays spectrum
- Audio plays
- VFO wheel changes frequency

**Step 6: Commit any fixes**

```bash
git add -A
git commit -m "fix: end-to-end integration fixes"
```

---

## Summary

| Phase | Tasks | Focus |
|-------|-------|-------|
| 1 | 1-4 | Gateway (Python TCP/WebSocket bridge) |
| 2 | 5-10 | Frontend (Svelte, Canvas, Web Audio) |
| 3 | 11 | Integration testing |

**Total tasks:** 11

**Key files:**
- `webserver/gateway/main.py` - Gateway entry point
- `webserver/gateway/pihpsdr_client.py` - TCP protocol client
- `webserver/frontend/src/App.svelte` - Main UI
- `webserver/frontend/src/components/Waterfall.svelte` - Spectrum display
