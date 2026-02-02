#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

// Initialize UDP socket
int udp_init(void);

// Send message to localhost (where JS server lives)
void udp_send(const char* message);

void udp_cleanup(void);

#endif