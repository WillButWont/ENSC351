#include "udp_client.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 7070
#define SERVER_IP "127.0.0.1"

static int sockfd = -1;
static struct sockaddr_in server_addr;

int udp_init(void) {
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    printf("[UDP] Client Initialized targeting %s:%d\n", SERVER_IP, SERVER_PORT);
    return 0;
}

void udp_send(const char* message) {
    if (sockfd < 0) return;
    sendto(sockfd, message, strlen(message), 0, 
           (const struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("[UDP] Sent: %s\n", message);
}

void udp_cleanup(void) {
    if (sockfd >= 0) close(sockfd);
}