#define p 0.5
#define T 5
#define SOCK_KTP 100

#define ENOSPACE 256
#define ENOTBOUND 257
#define ENOMESSAGE 258
#define ENOTSUP 259


int errno = 0;


int k_socket(int domain, int type, int protocol);
int k_bind(int sock_KTP, char* src_IP, int src_port, char* dest_IP, int dest_port);
int k_sendto(int sock_KTP, const void buf[.size], size_t size, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int k_recvfrom(int sock_KTP, const void buf[.size], size_t size, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int k_close(int sock_ktp);
int drop_message(float p);




