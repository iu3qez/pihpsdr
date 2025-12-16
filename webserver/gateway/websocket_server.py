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
