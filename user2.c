#include "ksocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 512

int main(int argc, char* argv[]) {
    char* src_ip = "127.0.0.2";
    char* dest_ip = "127.0.0.1";
    int src_port = 9090;
    int dest_port = 8080;
    char* filename="received.jpg";
    if(argc == 6) {
        src_ip = argv[1];
        dest_ip = argv[2];
        src_port = atoi(argv[3]);
        dest_port = atoi(argv[4]);
        filename = argv[5];
    }

    int M2 = k_socket(AF_INET, SOCK_KTP, 0);
    if (M2 < 0) {
        perror("Error creating KTP socket");
        exit(EXIT_FAILURE);
    }

    
    if (k_bind(M2, src_ip, src_port, dest_ip, dest_port) < 0) {
        perror("Bind failed user2");
        exit(EXIT_FAILURE);
    }
    printf("Receiver bound and listening on %s:%d...\n", src_ip, src_port);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &server_addr.sin_addr);
    
    socklen_t addr_len = sizeof(server_addr); 

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Could not create output file");
        exit(EXIT_FAILURE);
    }

    char buffer[CHUNK_SIZE];
    int total_received = 0;
    
    printf("Waiting for file data...\n");

    while (1) {
        int n = -1;
        while (n < 0) {
            n = k_recvfrom(M2, buffer, CHUNK_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
            if (n < 0) usleep(10000); 
        }
        
        if (n == 0) {
            printf("\nEOF signal received. File transfer complete.\n");
            break; 
        }

        fwrite(buffer, 1, n, file);
        total_received += n;
        
        // a dot for every chunk
        printf("."); 
        fflush(stdout);
    }
    
    printf("Total bytes received and saved: %d\n", total_received);

    fclose(file);
    sleep(7);
    k_close(M2);
    return 0;
}