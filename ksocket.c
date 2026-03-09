//implemented as library
#include<sys/socket.h>



int k_socket(int domain, int type, int protocol){
    int sock=socket(domain, SOCK_DGRAM, protocol);
    //check free space available in SM

}


//parameters and return are diff for only this function
void k_bind(){


}


void k_sendto(){


}


void k_recvfrom(){

}

void k_close(){


}

int drop_message(float p){
    //generate random number between 0 and 1
    float r=(float)rand();
    if(r<p){
        return 1;//drop message
    }
    return 0;//do not drop message
}