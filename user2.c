#include "ksocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int M2 = k_socket(AF_INET, SOCK_KTP, 0);
    if (M2 < 0) {
        perror("Error creating KTP socket");
        exit(EXIT_FAILURE);
    }

    char* src_ip = "127.0.0.2";
    char* dest_ip = "127.0.0.1";
    int src_port = 9090;
    int dest_port = 8080;

    if (k_bind(M2, src_ip, src_port, dest_ip, dest_port) < 0) {
        perror("Bind failed user2");
        exit(EXIT_FAILURE);
    }
    printf("Receiver bound and listening on %s:%d...\n", src_ip, src_port);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &server_addr.sin_addr);

    char buffer[512];
    int n = -1;
    
    while (n < 0) {
        n = k_recvfrom(M2, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (n < 0) usleep(50000); 
    }
    
    printf("Success! Received message: %s\n", buffer);

    k_close(M2);
    return 0;
}