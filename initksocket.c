#include<sys/shm.h>
#include<sys/types.h>

#define N 100

typedef struct swnd{
    int sender_ws;
    int* msgs;
}swnd;

typedef struct rwnd{
    int receiver_ws;
    int* msgs;
}rwnd;


typedef struct sock_info{
    int free;//1 if free, 0 if not free
    pid_t ppid;//parent process pid
    int fd_udp;//underlying udp socket
    char IP[50];//other end IP
    int port;//other end port
    int* send_buffer;
    int* recv_buffer;
}sock_info;

void garbage_collecter(){


}


int main(){
    
    int shmid=shmget(100,N*sizeof(sock_info),0);


    return 0;
}