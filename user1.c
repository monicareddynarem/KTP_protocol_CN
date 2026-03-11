#include "ksocket.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>
#include<sys/time.h>
#include<stddef.h>
int main(){
    int M1 = k_socket(AF_INET, SOCK_KTP, 0);

    if(M1<0){
        perror("Error creating KTP socket");
        exit(EXIT_FAILURE);
    }
    else {
        printf("KTP socket created with fd: %d\n", M1);
    }
    char* src_ip = "127.0.0.1";
    char* dest_ip = "127.0.0.2";
    int src_port = 8080;
    int dest_port = 9090;

    if(k_bind(M1, src_ip, src_port, dest_ip, dest_port)<0){
        perror("Error binding KTP socket");
        exit(EXIT_FAILURE);
    }
    else {
        printf("KTP socket bound to src %s:%d and dest %s:%d\n", src_ip, src_port, dest_ip, dest_port); 
    }
    char* msg = "Hello, KTP!";
    printf("%s\n",msg);

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);

    sleep(10); // Sleep for a while to allow the receiver to be ready

    printf("Sending message to %s:%d...\n", dest_ip, dest_port);
    k_sendto(M1, msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    printf("Exitting\n");

    return 0;
}