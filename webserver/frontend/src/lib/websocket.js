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
