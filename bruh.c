#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define SRC_PORT 1234
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
}




int main() {
    char packet[4096];
    memset(packet, 0, 4096);

    struct iphdr *iph = (struct iphdr *) packet;
    struct udphdr *udph = (struct udphdr *) (packet + sizeof(struct iphdr));
    char *data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
   
    time_t unix_time = time(NULL);  // Seconds since 1970
    
    struct ntp_timestamp ntp_seconds = {(uint32_t)(unix_time + 2208988800U), 0};
    

    memcpy(data + 16, &ntp_seconds, sizeof(uint32_t) * 2);
    memcpy(data + 24, &ntp_seconds, sizeof(uint32_t) * 2);
    memcpy(data + 32, &ntp_seconds, sizeof(uint32_t) * 2);
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
    iph->saddr = inet_addr("192.168.15.20"); // change to your IP
    iph->daddr = inet_addr("192.168.13.166");   // target IP
    iph->check = checksum(iph, sizeof(struct iphdr));

    // Fill in UDP header
    udph->source = htons(SRC_PORT);
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


    while (1) {
        printf("Enter Input to send to other end\n");
        char buf[2048];
        memset(buf, 0, 2048);
        fgets(buf, sizeof(buf), stdin);
        int length = strcspn(buf, "\n");
        buf[length] = 0;  // remove trailing newline if present

        printf("Sending %s\n", &buf[0]);
        int currentIndex = 0;
        while (buf[currentIndex * 4] != '\0') {
            uint16_t finishByte = 0;
            char pack[4];
            memset(pack, 0, 4);
            bool smallPacket = false;
            for (uint8_t i = 0; i < 4; i++) {
                //printf("%d Comparing buf[currentIndex (%d) * 4 + i + 1\n", i, currentIndex);
                if (buf[currentIndex * 4 + i] == '\0') {
                    
                    finishByte =  0b1 | ((i - 1) << 1) | (currentIndex << 3);
                    udph->source = htons(finishByte);
                    printf("Ending The Message\n");
                    smallPacket = true;
                    break;
                }
                else {
                    //udph->source = htons(SRC_PORT);
                    printf("XORing %c with %c\n", buf[currentIndex * 4 + i], KEY[i]);
                    pack[i] = KEY[i] ^ buf[currentIndex * 4 + i];
                    //memcpy(data + 44 + i, &buf + (currentIndex * 4) + i, 1);
                }

            }

            if (buf[currentIndex * 4 + 4] == '\0' && !smallPacket) {
                        printf("Reached End of Line On Last byte in chunk\n");
                        uint16_t finishByte = 0b111 | (currentIndex << 3);
                        udph->source = htons(finishByte);
                        printf("Ending The Message\n");
                        
                    }
            else if (!smallPacket) {
                finishByte |= 0b110 | (currentIndex << 3);
                udph->source = htons(finishByte);
            }
            

            //printf("Sending Packet here\n");
            memcpy(data + 44, &pack, sizeof(uint32_t));
            //printf(buf[currentIndex * 4]);
            //printf("\n");
            //udph->check = checksum(udph, sizeof(struct udphdr));
            printf("Sending Packet\n");
            if (sendto(s, packet, ntohs(iph->tot_len), 0,
               (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                perror("sendto");
                return 1;
            }
            currentIndex++;

        
        }
        printf("Total Packets Sent: %d\n", currentIndex);
    }

    
    

    //printf("Packet sent.\n");
    close(s);
    return 0;
}
