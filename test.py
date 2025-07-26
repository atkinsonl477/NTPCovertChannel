import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 123))
print("Listening on UDP 123")

while True:
    packet, addr = sock.recvfrom(1024)
    print(f"Received from {addr}: {packet}")
