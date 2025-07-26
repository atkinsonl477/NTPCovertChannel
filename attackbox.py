import socket
import ntplib
import time
import sys
import struct
from fxpmath import Fxp

def send_ntp_response():
    c = ntplib.NTPClient()
    response = c.request('time.google.com', version=3)

    fixed_point_format = Fxp(None, signed=True, n_word=64, n_frac=32)
    float_value = .001068
    fixed_point_value = Fxp(float_value, like=fixed_point_format)
    #print(struct.pack('>d', fixed_point_value))
    print(fixed_point_value.hex())

    #build NTP packet
    flags = 0x24
    peer_clock_stratum = 0x02
    peer_polling_interval = 0x00
    peer_clock_precision = 0xe7
    root_delay = struct.pack('>d', response.root_delay)
    root_dispersion = struct.pack('>d', response.root_dispersion)
    reference_id = 0xc279cff9
    reference_timestamp = struct.pack('>d', response.ref_timestamp)
    origin_timestamp = struct.pack('>d', response.orig_timestamp)
    receive_timestamp = struct.pack('>d', response.recv_timestamp)
    transmit_timestamp = struct.pack('>d', response.tx_timestamp)

    #print(type(response.root_dispersion))

def receive_ntp_request():
    y = 0

send_ntp_response()