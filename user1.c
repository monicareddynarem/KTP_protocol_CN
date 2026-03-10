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

int main(){
    int M1 = k_socket(AF_INET, SOCK_KTP, 0);

    if(M1<0){
        perror("Error creating KTP socket",errno);
        exit(EXIT_FAILURE);
    }
    char* src_ip = "127.0.0.1";
    char* dest_ip = "127.0.0.2";
    int src_port = 8080;
    int dest_port = 9090;

    if(k_bind(M1, src_ip, src_port, dest_ip, dest_port)<0){
        perror("Error binding KTP socket",errno);
        exit(EXIT_FAILURE);
    }
    char* msg = "Hello, KTP!";

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);

    k_sendto(M1, msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    return 0;
}