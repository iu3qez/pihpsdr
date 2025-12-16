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
        except Exception:
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
