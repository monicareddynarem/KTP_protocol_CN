#include<sys/shm.h>
#include<sys/types.h>
#include<stdlib.h>
#include<pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<ksocket.h>

//receiver

int main(){
    int M2=k_socket(AF_INET,SOCK_KTP,0);

    char* src_ip="127.0.0.2";
    char* dest_ip="127.0.0.1";
    int src_port=9090;
    int dest_port=8080;

    if(k_bind(M2,src_ip,src_port,dest_ip,dest_port)<0){
        perror("Bind failed user2");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(dest_port);
    inet_pton(AF_INET,dest_ip,&server_addr.sin_addr);


    char buffer[512];

    int n=k_recvfrom(M2,buffer,sizeof(buffer),0,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(n<0){
        perror("Receive failed user2");    }
    else{
        printf("Received message: %s\n", buffer);
    }       

    return 0;
}
