"""piHPSDR protocol definitions and parsing utilities."""
import struct
from dataclasses import dataclass
from enum import IntEnum

# Sync bytes
SYNC_BYTES = bytes([0xFA, 0xFA, 0xAF, 0xAF])

class CmdType(IntEnum):
    """Command types (client -> server)."""
    CMD_ADC = 0
    CMD_AGC = 1
    CMD_AGC_GAIN = 2
    CMD_FREQ = 25
    CMD_HEARTBEAT = 26
    CMD_MODE = 30
    CMD_MOX = 33
    CMD_MUTE_RX = 34
    CMD_FILTER_SEL = 23
    CMD_VOLUME = 89
    CMD_ZOOM = 93
    CMD_START_RADIO = 70
    CMD_RX_SPECTRUM = 60
    CMD_TX_SPECTRUM = 84

class InfoType(IntEnum):
    """Info types (server -> client)."""
    INFO_ADC = 94
    INFO_BAND = 95
    INFO_BANDSTACK = 96
    INFO_RADIO = 100
    INFO_RECEIVER = 101
    INFO_RXAUDIO = 102
    INFO_RX_SPECTRUM = 103
    INFO_TRANSMITTER = 105
    INFO_VFO = 107

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
