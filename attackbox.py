import socket
import ntplib
import struct
import math
PRIVATE_KEY_WORD='Bruh'
PRIVATE_KEY=PRIVATE_KEY_WORD.encode('ascii')

def pack_fixed32(float_value : float):
    fraction, integer = math.modf(float_value)
    integer = int(integer)
    fraction = int(fraction * (2**16))

    #!: Big endian
    #H: unsigned short (2 bytes == 16 bits)
    return struct.pack('!HH', integer, fraction)

def pack_fixed64(float_value : float):
    fraction, integer = math.modf(float_value)
    integer = int(integer)
    fraction = int(fraction * (2**32))

    #!: Big endian
    #I: unsigned int (4 bytes == 32 bits)
    return struct.pack('!II', integer, fraction)

def pack_data

def send_ntp_response(sock : socket.socket, client_address : str, client_port : int):

    c = ntplib.NTPClient()
    response = c.request('time.google.com', version=3)

    #Hard-coded fields
    flags = b'\x24'
    peer_clock_stratum = b'\x02'
    peer_polling_interval = b'\x00'
    peer_clock_precision = b'\xe7'
    reference_id = b'\xc2\x79\xcf\xf9'

    #Fixed-32 values
    root_delay = pack_fixed32(response.root_delay)
    root_dispersion = pack_fixed32(response.root_dispersion)

    #Fixed-64 values
    reference_timestamp = pack_fixed64(response.ref_timestamp)
    origin_timestamp = pack_fixed64(response.orig_timestamp)
    receive_timestamp = pack_fixed64(response.recv_timestamp)

    #transmit_timestamp = pack_fixed64(response.tx_timestamp)
    transmit_timestamp_int = struct.pack('!I', int(response.tx_timestamp))
    data = b'\x65\x76\x61\x6e'
    xored = bytes([b ^ k for b, k in zip(data, PRIVATE_KEY)])
    transmit_timestamp_message = xored

    #Build packet
    packet = b''
    packet += flags
    packet += peer_clock_stratum
    packet += peer_polling_interval
    packet += peer_clock_precision
    packet += root_delay
    packet += root_dispersion
    packet += reference_id
    packet += reference_timestamp
    packet += origin_timestamp
    packet += receive_timestamp
    packet += transmit_timestamp_int
    packet += transmit_timestamp_message

    sock.sendto(packet, (client_address, client_port))

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", 123))

client_address = "127.0.0.1"
client_port = 8080

send_ntp_response(sock, client_address, client_port)