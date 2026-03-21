#include "ksocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 512

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
    
    // FIX: Must use a variable to pass by reference to recvfrom
    socklen_t addr_len = sizeof(server_addr); 

    // 1. Open a new file in Binary Write mode
    FILE *file = fopen("recv.txt", "w");
    if (!file) {
        perror("Could not create output file");
        exit(EXIT_FAILURE);
    }

    char buffer[CHUNK_SIZE];
    int total_received = 0;
    
    printf("Waiting for file data...\n");

    // 2. Loop continuously to receive chunks
    while (1) {
        int n = -1;
        while (n < 0) {
            // Passing &addr_len here!
            n = k_recvfrom(M2, buffer, CHUNK_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
            if (n < 0) usleep(10000); // Wait 10ms if no message
        }
        
        // 3. If we receive a 0-byte message, it means the sender finished
        if (n == 0) {
            printf("\nEOF signal received. File transfer complete.\n");
            break; 
        }

        // 4. Write the received chunk to the file
        fwrite(buffer, 1, n, file);
        total_received += n;
        
        // Optional: Print a dot for every chunk so you can see progress
        printf("."); 
        fflush(stdout);
    }
    
    printf("Total bytes received and saved: %d\n", total_received);

    fclose(file);
    k_close(M2);
    return 0;
}