#include "ksocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int M1 = k_socket(AF_INET, SOCK_KTP, 0);
    if (M1 < 0) {
        perror("Error creating KTP socket");
        exit(EXIT_FAILURE);
    }
    printf("KTP socket created with index: %d\n", M1);

    char* src_ip = "127.0.0.1";
    char* dest_ip = "127.0.0.2";
    int src_port = 8080;
    int dest_port = 9090;

    if (k_bind(M1, src_ip, src_port, dest_ip, dest_port) < 0) {
        perror("Error binding KTP socket");
        exit(EXIT_FAILURE);
    }
    printf("KTP socket requested bind to src %s:%d\n", src_ip, src_port);

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);

    sleep(2); // Give the receiver a moment to start up

    char msg[] = "Hello, KTP!";
    printf("Sending message: '%s' to %s:%d...\n", msg, dest_ip, dest_port);
    
    int sent = -1;
    while (sent < 0) {
        sent = k_sendto(M1, msg, strlen(msg) + 1, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (sent < 0) usleep(50000); 
    }

    printf("Message successfully added to send buffer.\n");
    
    sleep(5); // Keep alive to allow background transmission
    k_close(M1);
    printf("Socket closed. Exiting.\n");

    return 0;
}