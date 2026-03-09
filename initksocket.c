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

#define N 100
#define SEND_BUF_SIZE 100
#define RECV_BUF_SIZE 10
#define TIMEOUT_T_MS 1000

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
    int free;//1 if free, 0 if not free
    pid_t ppid;//parent process pid
    int fd_udp;//underlying udp socket
    char* IP;//other end IP
    int port;//other end port
    message send_buffer[SEND_BUF_SIZE];//fixed size array of messages
    message recv_buffer[RECV_BUF_SIZE];
    swnd_struct swnd;
    rwnd_struct rwnd;
    long long send_times[10];
    int nospace;
}sock_info;


sock_info* SM;
int shmid;


void garbage_collecter(){
    shmdt(SM);
    shmctl(shmid, IPC_RMID, NULL);
}



void R_func(){
    //handles receiving messages from udp sockets
    fd_set read_set;
    struct timeval timeout;
    int max_fd;
    // int active_sockets=0;

    while(1){
        FD_ZERO(&read_set);
        max_fd=-1;

        for(int i=0;i<N;i++){
            if (SM[i].free == 0) { 
                // active_sockets++;
                FD_SET(SM[i].fd_udp, &read_set);
                if (SM[i].fd_udp > max_fd) {
                    max_fd = SM[i].fd_udp;
                }
            }
        }

        if (max_fd == -1) {
            usleep(100000); //100ms
            continue;
        }

        //timeout value is 0.5s
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        int status=select(max_fd+1,&read_set,NULL,NULL,&timeout);

        if(status<0){
            perror("Error in select");
            continue;
        }


        if(status==0){
            //timeout occurred
            // int new_sockets=0;
            // for(int i=0;i<N;i++){
            //     if(SM[i].free==0){
            //         new_sockets++;
            //     }
            // }

            for (int i = 0; i < N; i++) {
                if (SM[i].free == 0 && SM[i].nospace==1) {
                    if (SM[i].rwnd.rwnd_size > 0) {  
                        message dup_ack;
                        dup_ack.type = 1;
                        dup_ack.ack_no = SM[i].rwnd.expected[0];
                        dup_ack.rwnd_size = SM[i].rwnd.rwnd_size;
                        
                        struct sockaddr_in dest_addr;
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(SM[i].port);
                        inet_pton(AF_INET, SM[i].IP, &dest_addr.sin_addr);

                        sendto(SM[i].fd_udp, &dup_ack, sizeof(dup_ack), 0, 
                              (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    }
                }
            }
        continue;
        }


        //status > 0, socks are ready for activity(reading)
        for(int i=0;i<N;i++){
            //if the socket is active and it is ready to read data
            if(SM[i].free==0 && FD_ISSET(SM[i].fd_udp,&read_set)){
                //rread the data 
                struct sockaddr_in sender_addr;
                socklen_t addr_len = sizeof(sender_addr);
                message recv_pkt;

                int recv_len = recvfrom(SM[i].fd_udp, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr*)&sender_addr, &addr_len);
                
                if (recv_len > 0) {
                    if (recv_pkt.type == 0) { 
                        //if the received message is a DATA message(receiver side)
                        
                        //if the msg is received, then the recv window size decreases byt 1
                        SM[i].nospace = 0; 
                        if (SM[i].rwnd.rwnd_size > 0) {
                            SM[i].rwnd.rwnd_size--; 
                        }

                        //if after recving message, rwnd size is 0, then set nospace flag=1(no space left in recv buffer)
                        if (SM[i].rwnd.rwnd_size == 0) {
                            SM[i].nospace = 1;
                        }

                        //make an acknowledge message
                        message ack_pkt;
                        ack_pkt.type = 1;
                        ack_pkt.ack_no = recv_pkt.seq_no;
                        ack_pkt.rwnd_size = SM[i].rwnd.rwnd_size;
                        
                        //send ACK message to sender
                        sendto(SM[i].fd_udp, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&sender_addr, addr_len);

                    }

                    else if (recv_pkt.type == 1) { 
                        //if the received msg is an ACK message(sender side)

                        //update sender window size based on receiver's  window
                        SM[i].swnd.swnd_size = recv_pkt.rwnd_size;

                        // Check if it's a new ACK or Duplicate ACK
                        // (You need logic here to check if recv_pkt.ack_no matches an unacked packet)
                        //SM[i].swnd.unacked.size=10
                        int is_new_ack = 0; 
                        for (int j = 0; j <10 ;j++)
                        {
                            if(SM[i].swnd.unacked[j]==-1 && SM[i].swnd.unacked[j] == recv_pkt.ack_no){
                                is_new_ack=1;
                                SW[i].swnd.unacked[j]=-1;
                                break;
                            }
                        }
                        
                       if (is_new_ack) {
                            //remove message from sender-side buffer
                            for (int k = 0; k < SEND_BUF_SIZE; k++) {
                                if (SM[i].send_buffer[k].seq_no == recv_pkt.ack_no) {
                                    //mark the slot as empty (-1)
                                    // you would increment your 'start index of window' here instead.
                                    SM[i].send_buffer[k].seq_no = -1; 
                                    break;
                                }
                            }

                            // Note: If your assignment expects "Cumulative ACKs" (where an ACK for 
                            // seq 5 means 1, 2, 3, 4, and 5 are ALL received), you would change 
                            // the '==' checks above to '<=' to clear out multiple packets at once.
                            
                        } else {
                            // Duplicate ACK 
                        }
                    }
                }

            }
        }

    }
}


long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}


void S_func(){
    //handles timeout and retransmissions
    long sleep_time_us = (TIMEOUT_T_MS / 2) * 1000;

    while (1) {
        // 1. Sleep for T/2
        usleep(sleep_time_us);

        long long current_time = get_current_time_ms();

        // 2. Loop through all KTP sockets
        for (int i = 0; i < N; i++) {
            if (SM[i].free == 0) { // If socket is active
                
                struct sockaddr_in dest_addr;
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(SM[i].port);
                inet_pton(AF_INET, SM[i].IP, &dest_addr.sin_addr);

                // ==========================================
                // PHASE A: Check for Timeouts & Retransmit
                // ==========================================
                int timeout_occurred = 0;

                // Check if ANY unacked message has exceeded timeout T
                for (int j = 0; j < 10; j++) {
                    if (SM[i].swnd.unacked[j] != -1) { // Assuming -1 means empty slot
                        if ((current_time - SM[i].swnd.send_times[j]) > TIMEOUT_T_MS) {
                            timeout_occurred = 1;
                            break; // One timeout triggers retransmission for the whole window
                        }
                    }
                }

                if (timeout_occurred) {
                    printf("Timeout occurred on socket %d. Retransmitting window...\n", SM[i].fd_udp);
                    
                    // Retransmit all unacked messages in the current window
                    for (int j = 0; j < 10; j++) {
                        if (SM[i].swnd.unacked[j] != -1) {
                            int seq_to_resend = SM[i].swnd.unacked[j];
                            
                            // Find the message in the send buffer
                            for (int k = 0; k < SEND_BUF_SIZE; k++) {
                                if (SM[i].send_buffer[k].seq_no == seq_to_resend) {
                                    
                                    // Retransmit the message
                                    sendto(SM[i].fd_udp, &SM[i].send_buffer[k], sizeof(message), 0,
                                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                                    
                                    // Update the timestamp for this retransmission
                                    SM[i].swnd.send_times[j] = get_current_time_ms();
                                    break;
                                }
                            }
                        }
                    }
                }

                // ==========================================
                // PHASE B: Send New Pending Messages
                // ==========================================
                
                // First, count how many messages are currently unacked
                int current_unacked_count = 0;
                for (int j = 0; j < 10; j++) {
                    if (SM[i].swnd.unacked[j] != -1) {
                        current_unacked_count++;
                    }
                }

                // If we have room in the sender window (and the receiver's window isn't 0)
                if (current_unacked_count < SM[i].swnd.swnd_size) {
                    
                    int available_slots = SM[i].swnd.swnd_size - current_unacked_count;

                    // Search the send_buffer for valid messages that HAVEN'T been sent yet
                    for (int k = 0; k < SEND_BUF_SIZE && available_slots > 0; k++) {
                        if (SM[i].send_buffer[k].seq_no != -1) { 
                            
                            // Check if this message is already in the unacked array
                            int already_sent = 0;
                            for (int j = 0; j < 10; j++) {
                                if (SM[i].swnd.unacked[j] == SM[i].send_buffer[k].seq_no) {
                                    already_sent = 1;
                                    break;
                                }
                            }

                            // If it hasn't been sent, SEND IT!
                            if (!already_sent) {
                                sendto(SM[i].fd_udp, &SM[i].send_buffer[k], sizeof(message), 0,
                                       (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                                
                                // Find an empty slot in unacked array to store it
                                for (int j = 0; j < 10; j++) {
                                    if (SM[i].swnd.unacked[j] == -1) {
                                        SM[i].swnd.unacked[j] = SM[i].send_buffer[k].seq_no;
                                        SM[i].swnd.send_times[j] = get_current_time_ms(); // Set timestamp
                                        break;
                                    }
                                }
                                available_slots--;
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}


int main(){
    //implement 2 threads R and S

    shmid=shmget(100,N*sizeof(sock_info),IPC_CREAT|0666);
    SM = shmat(shmid,NULL,0);

    pthread_t R,S;

    // create threads R and S
    pthread_create(&R,NULL,R_func,NULL);
    pthread_create(&S,NULL,S_func,NULL);


    pthread_join(R,NULL);
    pthread_join(S,NULL);


    return 0;
}