import socket

data = []
finalSize = 0
#Inserting 
def insertSorted(byteCode: bytes, number: int):
    newItem = (byteCode, number)
    i = 0
    while i < len(data) and data[i][1] < number:
        i += 1
    data.insert(i, newItem)

def constructFinalMessage() -> bytes:
    finalBytes = b''
    for byteCode in data:
        b, num = byteCode
        finalBytes += b
        
    return finalBytes


KEY = "Bruh".encode()
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 123))
packetsReceived = 0

print("Listening on UDP port 123...")
while True:
    constructedPacket = []
    currentByteOnKey = 0
    lastPacket = 0
    isDone = False
    while not isDone or (len(data) < finalSize + 1):
        print("Receiving Packet")
        packet, addr = sock.recvfrom(1024)
        #print(packet)
        ip, port = addr
    
        #print("Ports Binary:",bin(port))
        # Encoded Data
        bytesToDecode = packet[44:48]
        # Finished Bit (15)
        LastSeq = port & 0x1
        # Bytes To Read from this packet (13-14)
        # print (bin(port & 0x6))
        bytesToRead = (int)((port & 0x6) >> 1) + 1
        # Sequence Number (0-12) (32 kB max per message)
        seqNum = (int)(port & 0xFFF8) >> 3
        
        print("Sequence done?", LastSeq, "bytesToRead:", bytesToRead, "Sequence Number:", seqNum)
        
        # Last packet is being sent here
        if LastSeq == 0x1:
            print("Got to end of packet, packets we need to read:", seqNum)
            isDone = True
            finalSize = seqNum

        rawBytes = b''
        for i in range(bytesToRead):
            #print(i, end=" ")
            #print("Xoring these values", KEY[i], bytesToDecode[i])
            rawBytes += bytes([KEY[i] ^ bytesToDecode[i]])
        insertSorted(rawBytes, seqNum)
        packetsReceived += 1
        
       #print("__New Packet__",data,"___")
        
    finalMessage = constructFinalMessage()
    print(packetsReceived, "Packets Received, message decoded is")
    print(finalMessage)
    packetsReceived = 0
    data = []
    
        
        
    
    
    
    
    
    