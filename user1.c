#include "ksocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 1024

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

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);

    sleep(2); // Give the receiver a moment to start up

    // 1. Open the file to send in Binary Read mode
    char* filename = "cat.jpg"; // Change this to whatever file you want to send
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Could not open file to send");
        exit(EXIT_FAILURE);
    }

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    int total_sent = 0;

    printf("Starting file transfer for '%s'...\n", filename);

    // 2. Read the file in chunks and send them
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        int sent = -1;
        while (sent < 0) {
            sent = k_sendto(M1, buffer, bytes_read, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            if (sent < 0) usleep(10000); // Wait 10ms if buffer is full
        }
        total_sent += sent;
    }

    // 3. Send a 0-byte packet to signal End of File (EOF)
    int sent_eof = -1;
    while (sent_eof < 0) {
        sent_eof = k_sendto(M1, buffer, 0, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (sent_eof < 0) usleep(10000);
    }

    printf("File successfully added to send buffer. Total bytes: %d\n", total_sent);
    
    fclose(file);
    sleep(5); // Keep alive to allow background transmission
    k_close(M1);
    printf("Socket closed. Exiting.\n");

    return 0;
}