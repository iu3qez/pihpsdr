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
