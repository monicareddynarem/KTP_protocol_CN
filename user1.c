#include<ksocket.h>
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
    int sockfd = k_socket(AF_INET, SOCK_DGRAM, 0);
}