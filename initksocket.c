#include<sys/shm.h>
#include<sys/types.h>
#include<stdlib.h>
#include<pthread.h>
#define N 100
#define SEND_BUF_SIZE 100
#define RECV_BUF_SIZE 10

typedef struct swnd_struct{
    int swnd_size;
    int unacked[10];
}swnd_struct;

typedef struct rwnd_struct{
    int rwnd_size;
    int expected[10];
}rwnd_struct;


typedef struct message{
    int seq_no;
    char msg_data[512];//each msg is 512 bytes(fixed)
}message;

typedef struct sock_info{
    int free;//1 if free, 0 if not free
    pid_t ppid;//parent process pid
    int fd_udp;//underlying udp socket
    char* IP;//other end IP
    int port;//other end port
    message send_buffer[SEND_BUF_SIZE];//fixed size array of messages
    message recv_buffer[RECV_BUF_SIZE];
    swnd_struct swnd;
    rwnd_struct rwnd;
}sock_info;

void garbage_collecter(){


}

void R_func(){
    
    //receiver thread function
}
void S_func(){
    //sender thread function
}
int main(){
    //implement 2 threads R and S

    int shmid=shmget(100,N*sizeof(sock_info),IPC_CREAT|0666);
    sock_info* SM = shmat(shmid,NULL,0);

    pthread_t R,S;

    // create threads R and S
    pthread_create(&R,NULL,R_func,NULL);
    pthread_create(&S,NULL,S_func,NULL);
    return 0;
}