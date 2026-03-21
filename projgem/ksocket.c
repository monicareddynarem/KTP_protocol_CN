#include "ksocket.h"
#include <sys/socket.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

int k_errno;
int shmid = 0;
sock_info* SM = NULL;

int k_socket(int domain, int type, int protocol){
    if(type != SOCK_KTP){
        k_errno = ENOTSUP;
        return -1;
    }
    
    // Attach to shared memory if not already attached in this process
    if (SM == NULL) {
        shmid = shmget(100, N * sizeof(sock_info), IPC_CREAT | 0666);
        SM = shmat(shmid, NULL, 0);
    }

    for(int i = 0; i < N; i++){
        if(SM[i].not_free == 0){
            SM[i].not_free = 1;
            SM[i].ppid = getpid();
            SM[i].bind_done = 0;
            SM[i].send_buffer_sz = 0;
            SM[i].recv_buffer_sz = 0;
            SM[i].cur_seq_no = 1;
            SM[i].swnd.swnd_size = 10;
            SM[i].rwnd.rwnd_size = 10;
            SM[i].rwnd.expected[0] = 1; // Start expecting sequence number 1
            
            for(int j=0; j<10; j++) SM[i].swnd.unacked[j] = -1;

            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&SM[i].mutex, &attr);

            return i;
        }
    }
    k_errno = ENOSPACE;
    return -1;
}

int k_bind(int sock_KTP, char* src_IP, int src_port, char* dest_IP, int dest_port){
    pthread_mutex_lock(&SM[sock_KTP].mutex);
    
    strncpy(SM[sock_KTP].src_IP, src_IP, 15);
    SM[sock_KTP].src_IP[15] = '\0';
    SM[sock_KTP].src_port = src_port;

    strncpy(SM[sock_KTP].IP, dest_IP, 15);
    SM[sock_KTP].IP[15] = '\0';
    SM[sock_KTP].port = dest_port;
    
    SM[sock_KTP].bind_done = 1; // Signal the daemon to bind the socket
    pthread_mutex_unlock(&SM[sock_KTP].mutex);

    // Wait for the daemon to finish binding the OS socket
    while (SM[sock_KTP].bind_done == 1) {
        usleep(10000); 
    }

    if (SM[sock_KTP].bind_done == -1) {
        return -1; // Daemon failed to bind
    }

    return 0;
}

int k_sendto(int sock_KTP, const void* buf, size_t size, int flags, struct sockaddr *dest_addr, socklen_t addrlen){
    char dest_ip[100];
    inet_ntop(AF_INET, &((struct sockaddr_in*)dest_addr)->sin_addr, dest_ip, sizeof(dest_ip));
    int dest_port = ntohs(((struct sockaddr_in*)dest_addr)->sin_port);

    pthread_mutex_lock(&SM[sock_KTP].mutex);

    if(strcmp(SM[sock_KTP].IP, dest_ip) == 0 && SM[sock_KTP].port == dest_port){
        if((SM[sock_KTP].send_buffer_sz + 1) < SEND_BUF_SIZE){
            message new_msg;
            new_msg.seq_no = SM[sock_KTP].cur_seq_no;
            memset(new_msg.msg_data, 0, 512);
            memcpy(new_msg.msg_data, buf, size > 512 ? 512 : size);  
            new_msg.type = 0; 

            SM[sock_KTP].cur_seq_no++;
            SM[sock_KTP].send_buffer[SM[sock_KTP].send_buffer_sz] = new_msg;
            SM[sock_KTP].send_buffer_sz++;

            pthread_mutex_unlock(&SM[sock_KTP].mutex);
            return size;
        }
        else{
            pthread_mutex_unlock(&SM[sock_KTP].mutex);
            k_errno = ENOSPACE;
            return -1;
        }
    }
    else{
        pthread_mutex_unlock(&SM[sock_KTP].mutex);
        k_errno = ENOTBOUND;
        return -1;
    }
}

int k_recvfrom(int sock_KTP, void* buf, size_t size, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    pthread_mutex_lock(&SM[sock_KTP].mutex);
    
    if(SM[sock_KTP].recv_buffer_sz > 0){
        message* msg = &SM[sock_KTP].recv_buffer[0];
        memcpy(buf, msg->msg_data, size > 512 ? 512 : size);
        
        for(int i=1; i<SM[sock_KTP].recv_buffer_sz; i++){
            SM[sock_KTP].recv_buffer[i-1] = SM[sock_KTP].recv_buffer[i];
        }
        SM[sock_KTP].recv_buffer_sz--;
        pthread_mutex_unlock(&SM[sock_KTP].mutex);
        return size;
    }
    else{
        pthread_mutex_unlock(&SM[sock_KTP].mutex);
        k_errno = ENOMESSAGE;
        return -1;
    }
}

int k_close(int sock_ktp){
    pthread_mutex_lock(&SM[sock_ktp].mutex);
    SM[sock_ktp].bind_done = -1; // Signal daemon to close OS socket
    pthread_mutex_unlock(&SM[sock_ktp].mutex);
    
    // Wait for daemon to finish closing
    while(SM[sock_ktp].not_free == 1) {
        usleep(10000);
    }
    return 0;
}

int drop_message(float p){
    float r = (float)rand() / (float)RAND_MAX;
    if(r < p) return 1;
    return 0;
}