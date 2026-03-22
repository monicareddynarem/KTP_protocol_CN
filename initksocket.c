// Mini Project 1 Submission
// Group Details:
// Member 1 Name: Ramakurthi Ashok Chandra
// Member 1 Roll No: 23CS10059
// Member 2 Name: Monica Reddy Narem
// Member 2 Roll No: 23CS10046

#include "ksocket.h"
#include <sys/shm.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

void garbage_collecter(){
    if(SM != NULL) shmdt(SM);
    shmctl(shmid, IPC_RMID, NULL);
}

void* G_func(void* arg){
    while(1) {
        sleep(2);
        for(int i = 0; i < N; i++) {
            if (SM[i].not_free == 1 && SM[i].ppid > 0) {
                if (kill(SM[i].ppid, 0) == -1) {
                    printf("Garbage Collector: Process %d died unexpectedly. Cleaning socket %d.\n", SM[i].ppid, i);
                    pthread_mutex_lock(&SM[i].mutex);
                    if (SM[i].bind_done == 2) close(SM[i].fd_udp);
                    SM[i].not_free = 0;
                    SM[i].bind_done = 0;
                    pthread_mutex_unlock(&SM[i].mutex);
                }
            }
        }
    }
    return NULL;
}

void* R_func(void* arg){
    fd_set read_set;
    struct timeval timeout;
    int max_fd;

    while(1){
        for (int i = 0; i < N; i++) {
            if (SM[i].not_free == 1 && SM[i].bind_done == 1) {
                pthread_mutex_lock(&SM[i].mutex);
                SM[i].fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
                struct sockaddr_in src_addr;
                src_addr.sin_family = AF_INET;
                src_addr.sin_port = htons(SM[i].src_port);
                inet_pton(AF_INET, SM[i].src_IP, &src_addr.sin_addr);
                
                if (bind(SM[i].fd_udp, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
                    perror("Init daemon failed to bind UDP socket");
                    SM[i].bind_done = -1;
                } else {
                    printf("Daemon bound socket for user to %s:%d\n", SM[i].src_IP, SM[i].src_port);
                    SM[i].bind_done = 2;
                }
                pthread_mutex_unlock(&SM[i].mutex);
            }
            if (SM[i].not_free == 1 && SM[i].bind_done == -1) {
                pthread_mutex_lock(&SM[i].mutex);
                close(SM[i].fd_udp);
                SM[i].bind_done = 0;
                SM[i].not_free = 0;
                printf("Daemon closed socket for user.\n");
                pthread_mutex_unlock(&SM[i].mutex);
            }
        }

        FD_ZERO(&read_set);
        max_fd = -1;

        for(int i = 0; i < N; i++){
            if (SM[i].not_free == 1 && SM[i].bind_done == 2) { 
                FD_SET(SM[i].fd_udp, &read_set);
                if (SM[i].fd_udp > max_fd) max_fd = SM[i].fd_udp;
            }
        }

        if (max_fd == -1) {
            usleep(100000); 
            continue;
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        int status = select(max_fd + 1, &read_set, NULL, NULL, &timeout);

        if(status < 0){
            perror("Error in select");
            continue;
        }

        if(status == 0){
            for (int i = 0; i < N; i++) {
                if (SM[i].not_free == 1 && SM[i].bind_done == 2 && SM[i].nospace == 1) {
                    pthread_mutex_lock(&SM[i].mutex);
                    if (SM[i].rwnd.rwnd_size > 0) {  
                        message dup_ack;
                        dup_ack.type = 1;
                        dup_ack.ack_no = SM[i].rwnd.expected[0] - 1;
                        dup_ack.rwnd_size = SM[i].rwnd.rwnd_size;
                        
                        struct sockaddr_in dest_addr;
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(SM[i].port);
                        inet_pton(AF_INET, SM[i].IP, &dest_addr.sin_addr);

                        sendto(SM[i].fd_udp, &dup_ack, sizeof(dup_ack), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    }
                    pthread_mutex_unlock(&SM[i].mutex);
                }
            }
            continue;
        }

        for(int i = 0; i < N; i++){
            if(SM[i].not_free == 1 && SM[i].bind_done == 2 && FD_ISSET(SM[i].fd_udp, &read_set)){
                struct sockaddr_in sender_addr;
                socklen_t addr_len = sizeof(sender_addr);
                message recv_pkt;

                int recv_len = recvfrom(SM[i].fd_udp, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr*)&sender_addr, &addr_len);
                
                if (recv_len > 0) {
                    if(drop_message(DROP_PROB) == 1){
                        continue; 
                    }
                    
                    pthread_mutex_lock(&SM[i].mutex);
                    
                    if (recv_pkt.type == 0) { 
                        int is_duplicate = 0;
                        if (recv_pkt.seq_no < SM[i].rwnd.expected[0]) {
                            is_duplicate = 1; 
                        } else {
                            for(int k = 0; k < SM[i].recv_buffer_sz; k++) {
                                if(SM[i].recv_buffer[k].seq_no == recv_pkt.seq_no) {
                                    is_duplicate = 1; 
                                    break;
                                }
                            }
                        }

                        if (is_duplicate) {
                            if (recv_pkt.seq_no <= SM[i].rwnd.expected[0]) {
                                message ack_pkt;
                                ack_pkt.type = 1;
                                ack_pkt.ack_no = SM[i].rwnd.expected[0] - 1;
                                ack_pkt.rwnd_size = SM[i].rwnd.rwnd_size;
                                sendto(SM[i].fd_udp, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&sender_addr, addr_len);
                            }
                            pthread_mutex_unlock(&SM[i].mutex);
                            continue; 
                        }

                        SM[i].nospace = 0;
                        if (SM[i].rwnd.rwnd_size > 0) {
                            SM[i].recv_buffer[SM[i].recv_buffer_sz++] = recv_pkt;
                            SM[i].rwnd.rwnd_size--; 
                            
                            if (SM[i].rwnd.rwnd_size == 0) {
                                SM[i].nospace = 1;
                            }
                        } else {
                            SM[i].nospace = 1;
                            pthread_mutex_unlock(&SM[i].mutex);
                            continue; 
                        }

                        if (recv_pkt.seq_no == SM[i].rwnd.expected[0]) {
                            SM[i].rwnd.expected[0]++;
                            
                            int found_next = 1;
                            while (found_next) {
                                found_next = 0;
                                for(int k = 0; k < SM[i].recv_buffer_sz; k++) {
                                    if(SM[i].recv_buffer[k].seq_no == SM[i].rwnd.expected[0]) {
                                        SM[i].rwnd.expected[0]++;
                                        found_next = 1;
                                        break; 
                                    }
                                }
                            }

                            message ack_pkt;
                            ack_pkt.type = 1;
                            ack_pkt.ack_no = SM[i].rwnd.expected[0] - 1;
                            ack_pkt.rwnd_size = SM[i].rwnd.rwnd_size;
                            sendto(SM[i].fd_udp, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&sender_addr, addr_len);
                        }
                        
                        pthread_mutex_unlock(&SM[i].mutex);
                    }
                    else if (recv_pkt.type == 1) { 
                        SM[i].swnd.swnd_size = recv_pkt.rwnd_size;
                        for (int j = 0; j < 10; j++) {
                            if(SM[i].swnd.unacked[j] != -1 && SM[i].swnd.unacked[j] <= recv_pkt.ack_no){
                                int acked_seq = SM[i].swnd.unacked[j];
                                SM[i].swnd.unacked[j] = -1;
                                for (int k = 0; k < SM[i].send_buffer_sz; k++) {
                                    if (SM[i].send_buffer[k].seq_no == acked_seq) {
                                        for(int x = k; x < SM[i].send_buffer_sz - 1; x++){
                                            SM[i].send_buffer[x] = SM[i].send_buffer[x+1];
                                        }
                                        SM[i].send_buffer_sz--;
                                        break;
                                    }
                                }
                            }
                        }
                        pthread_mutex_unlock(&SM[i].mutex);
                    }
                }
            }
        }
    }
    return NULL;
}

long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

void* S_func(void* arg){
    long sleep_time_us = (T * 1000000) / 2;

    while (1) {
        usleep(sleep_time_us);
        long long current_time = get_current_time_ms();

        for (int i = 0; i < N; i++) {
            if (SM[i].not_free == 1 && SM[i].bind_done == 2) { 
                pthread_mutex_lock(&SM[i].mutex);

                struct sockaddr_in dest_addr;
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(SM[i].port);
                inet_pton(AF_INET, SM[i].IP, &dest_addr.sin_addr);

                int timeout_occurred = 0;
                for (int j = 0; j < 10; j++) {
                    if (SM[i].swnd.unacked[j] != -1) { 
                        if ((current_time - SM[i].send_times[j]) > (T * 1000)) {
                            timeout_occurred = 1;
                            break; 
                        }
                    }
                }

                if (timeout_occurred) {
                    for (int j = 0; j < 10; j++) {
                        if (SM[i].swnd.unacked[j] != -1) {
                            int seq_to_resend = SM[i].swnd.unacked[j];
                            for (int k = 0; k < SM[i].send_buffer_sz; k++) {
                                if (SM[i].send_buffer[k].seq_no == seq_to_resend) {
                                    sendto(SM[i].fd_udp, &SM[i].send_buffer[k], sizeof(message), 0,
                                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                                    SM[i].send_times[j] = get_current_time_ms();
                                    SM[i].total_transmissions++;
                                    break;
                                }
                            }
                        }
                    }
                }

                int current_unacked_count = 0;
                for (int j = 0; j < 10; j++) {
                    if (SM[i].swnd.unacked[j] != -1) current_unacked_count++;
                }

                if (current_unacked_count < SM[i].swnd.swnd_size) {
                    int available_slots = SM[i].swnd.swnd_size - current_unacked_count;
                    for (int k = 0; k < SM[i].send_buffer_sz && available_slots > 0; k++) {
                        int already_sent = 0;
                        for (int j = 0; j < 10; j++) {
                            if (SM[i].swnd.unacked[j] == SM[i].send_buffer[k].seq_no) {
                                already_sent = 1;
                                break;
                            }
                        }

                        if (!already_sent) {
                            sendto(SM[i].fd_udp, &SM[i].send_buffer[k], sizeof(message), 0,
                                   (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                            SM[i].total_transmissions++;
                                
                            for (int j = 0; j < 10; j++) {
                                if (SM[i].swnd.unacked[j] == -1) {
                                    SM[i].swnd.unacked[j] = SM[i].send_buffer[k].seq_no;
                                    SM[i].send_times[j] = get_current_time_ms(); 
                                    break;
                                }
                            }
                            available_slots--;
                        }
                    }
                }
                pthread_mutex_unlock(&SM[i].mutex);  
            }
        }
    }
    return NULL;
}

int main(){
    srand(time(NULL));

    shmid = shmget(100, N * sizeof(sock_info), IPC_CREAT | 0666);
    SM = shmat(shmid, NULL, 0);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    
    for(int i = 0; i < N; i++) {
        SM[i].not_free = 0;
        SM[i].bind_done = 0;
        if (pthread_mutex_init(&SM[i].mutex, &attr) != 0)
        {
            perror("Mutex init failed");
        }
    }

    pthread_t R, S, G;

    printf("Initializing KTP protocol...\n");
    pthread_create(&R, NULL, R_func, NULL);
    pthread_create(&S, NULL, S_func, NULL);
    pthread_create(&G, NULL, G_func, NULL); 

    printf("KTP protocol initialized. Running...\n");
    pthread_join(R, NULL);
    pthread_join(S, NULL);
    pthread_join(G, NULL);

    garbage_collecter();
    return 0;
}