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
