//implemented as library
#include<sys/socket.h>
#include<sys/shm.h>
#include <stdlib.h>
#include "ksocket.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include<stdio.h>
#define N 100
#define SEND_BUF_SIZE 100
#define RECV_BUF_SIZE 10
int k_errno;
typedef struct swnd_struct{
    int swnd_size;
    int unacked[10];
}swnd_struct;

typedef struct rwnd_struct{
    int rwnd_size;
    int expected[10];
}rwnd_struct;


typedef struct message{
    int type;//0-Data,1-ack
    int seq_no;
    int ack_no;
    int rwnd_size;
    char msg_data[512];//each msg is 512 bytes(fixed)
}message;

typedef struct sock_info{
    int not_free;//0 if free, 1 if not free
    pid_t ppid;//parent process pid
    int fd_udp;//underlying udp socket
    char* IP;//other end IP
    int port;//other end port
    int cur_seq_no;//current sequence number
    int send_buffer_sz;// index for send buffer
    int recv_buffer_sz;// index for recv buffer
    message send_buffer[SEND_BUF_SIZE];//fixed size array of messages
    message recv_buffer[RECV_BUF_SIZE];
    swnd_struct swnd;
    rwnd_struct rwnd;
    long long send_times[10];
    int nospace;
}sock_info;

int shmid;
sock_info* SM;

int k_socket(int domain, int type, int protocol){
    if(type != SOCK_KTP){
        k_errno = ENOTSUP;
        return -1;//only datagram sockets supported
    }
    int sock=socket(domain, SOCK_DGRAM, protocol);
    //check free space available in SM
    shmid=shmget(100,N*sizeof(sock_info),IPC_CREAT|0666);
    SM = shmat(shmid,NULL,0);

    for(int i=0;i<N;i++){
        if(SM[i].not_free==0){
            SM[i].not_free=1;
            SM[i].ppid=getpid();
            SM[i].fd_udp=sock;
            SM[i].send_buffer_sz=0;
            SM[i].recv_buffer_sz=0;
            SM[i].cur_seq_no=1;
            // inka cheyali
            return i;
        }
    }
    k_errno = ENOSPACE;
    return -1;//no free space available in SM
}


//parameters and return are diff for only this function
int k_bind(int sock_KTP, char* src_IP, int src_port, char* dest_IP, int dest_port){
    struct sockaddr_in src_addr;
    src_addr.sin_family=AF_INET;
    src_addr.sin_port=htons(src_port);
    inet_pton(AF_INET, src_IP, &src_addr.sin_addr);
    
    int return_val = bind(SM[sock_KTP].fd_udp, (struct sockaddr*)&src_addr, sizeof(src_addr));
    if(return_val == -1){
        perror("Bind Failed\n");
        return -1;
    }

    SM[sock_KTP].IP=dest_IP;
    SM[sock_KTP].port=dest_port;

    return return_val;
}


int k_sendto(int sock_KTP, const void* buf, size_t size, int flags, struct sockaddr *dest_addr, socklen_t addrlen){
    printf("Inside k_sendto for socket %d\n", sock_KTP);
    char dest_ip[100];
    inet_ntop(AF_INET, &((struct sockaddr_in*)dest_addr)->sin_addr, dest_ip, sizeof(dest_ip));
    int dest_port = ntohs(((struct sockaddr_in*)dest_addr)->sin_port);
    printf("Destination IP: %s, Destination Port: %d\n", dest_ip, dest_port);
    if(strcmp(SM[sock_KTP].IP, dest_ip) == 0 && SM[sock_KTP].port == dest_port){
        printf("Destination IP and port match for socket %d. Adding message to send buffer.\n", sock_KTP);
        if((SM[sock_KTP].send_buffer_sz+1) < SEND_BUF_SIZE){
            printf("Adding message to send buffer of socket %d with seq_no %d\n", sock_KTP, SM[sock_KTP].cur_seq_no);
            //add message to send buffer
            // printf("Adding message to send buffer of socket %d with seq_no %d\n", sock_KTP, SM[sock_KTP].cur_seq_no);
            message* new_msg = (message*)malloc(sizeof(message));

            new_msg->seq_no = SM[sock_KTP].cur_seq_no;
            printf("Message seq_no set to %d\n", new_msg->seq_no);
            strncpy(new_msg->msg_data, buf, size);  
            printf("Message data set to: %s\n", new_msg->msg_data);
            new_msg->type = 0; // Data message
            SM[sock_KTP].cur_seq_no++;

            SM[sock_KTP].send_buffer[SM[sock_KTP].send_buffer_sz] = *new_msg;
            SM[sock_KTP].send_buffer_sz++;
            free(new_msg);
        }
        else{
            k_errno = ENOSPACE;
            return -1;// send buffer full
        }
        return 0; // no error;
    }
    else{
        printf("Destination IP and port do not match for socket %d. Expected IP: %s, Expected Port: %d\n", sock_KTP, SM[sock_KTP].IP, SM[sock_KTP].port);
        k_errno = ENOTBOUND;
        return -1;// socket not bound to this dest IP and port
    }
}


int k_recvfrom(int sock_KTP, void* buf, size_t size, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    if(SM[sock_KTP].recv_buffer_sz > 0){
        //copy message from recv buffer to buf
        message* msg = &SM[sock_KTP].recv_buffer[0];
        strncpy(buf, msg->msg_data, size);
        //remove message from recv buffer
        for(int i=1; i<SM[sock_KTP].recv_buffer_sz; i++){
            SM[sock_KTP].recv_buffer[i-1] = SM[sock_KTP].recv_buffer[i];
        }
        SM[sock_KTP].recv_buffer_sz--;
        return 0; // no error
    }
    else{
        k_errno = ENOMESSAGE;
        return -1;// no message to receive
    }
}

int k_close(int sock_ktp){
    int rval = close(SM[sock_ktp].fd_udp);
    SM[sock_ktp].not_free = 0;//mark as free
    SM[sock_ktp].ppid = 0;
    SM[sock_ktp].fd_udp = 0;
    SM[sock_ktp].IP = NULL;
    SM[sock_ktp].port = 0;
    SM[sock_ktp].cur_seq_no = 0;
    SM[sock_ktp].send_buffer_sz = 0;
    SM[sock_ktp].recv_buffer_sz = 0;
    return rval;
}

int drop_message(float p){
    //generate random number between 0 and 1
    float r=(float)rand();
    if(r<p){
        return 1;//drop message
    }
    return 0;//do not drop message
}