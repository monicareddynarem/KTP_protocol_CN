// Mini Project 1 Submission
// Group Details:
// Member 1 Name: Ramakurthi Ashok Chandra
// Member 1 Roll No: 23CS10059
// Member 2 Name: Monica Reddy Narem
// Member 2 Roll No: 23CS10046

#ifndef KSOCKET_H
#define KSOCKET_H

#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#define DROP_PROB 0.05
#define T 5
#define SOCK_KTP 100

#define ENOSPACE 256
#define ENOTBOUND 257
#define ENOMESSAGE 258
#define ENOTSUP 259

#define N 100
#define SEND_BUF_SIZE 20
#define RECV_BUF_SIZE 10

extern int k_errno;

typedef struct swnd_struct {
    int swnd_size;
    int unacked[10];
} swnd_struct;

typedef struct rwnd_struct {
    int rwnd_size;
    int expected[10];
} rwnd_struct;

typedef struct message {
    int type; 
    int seq_no;
    int ack_no;
    int rwnd_size;
    int msg_len;
    char msg_data[512]; 
} message;

typedef struct sock_info {
    int not_free; 
    pid_t ppid;   
    int fd_udp;   
    
    char src_IP[16]; 
    int src_port;    
    
    char IP[16];     
    int port;        
    
    int bind_done;   

    int cur_seq_no;
    int send_buffer_sz;
    int app_read_seq_no;
    int recv_buffer_sz;
    message send_buffer[SEND_BUF_SIZE];
    message recv_buffer[RECV_BUF_SIZE];
    swnd_struct swnd;
    rwnd_struct rwnd;
    long long send_times[10];
    int nospace;

    int total_transmissions;
    int total_messages;

    pthread_mutex_t mutex; 
} sock_info;

extern int shmid;
extern sock_info* SM;

int k_socket(int domain, int type, int protocol);
int k_bind(int sock_KTP, char* src_IP, int src_port, char* dest_IP, int dest_port);
int k_sendto(int sock_KTP, const void* buf, size_t size, int flags, struct sockaddr *dest_addr, socklen_t addrlen);
int k_recvfrom(int sock_KTP, void* buf, size_t size, int flags, const struct sockaddr *dest_addr, socklen_t *addrlen);
int k_close(int sock_ktp);
int drop_message(float p);

#endif