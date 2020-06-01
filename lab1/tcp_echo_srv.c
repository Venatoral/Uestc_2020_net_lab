#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define SOCKADDR_SIZE sizeof(struct sockaddr)

bool to_exit;
int sig_type;
void handle(int fd) {
    char *buf = malloc(512);
    int res = 1;
    while (1) {
        memset(buf, 0, 512);
        res = read(fd, buf, 512);
        if (res < 0) {
            printf("[srv] read len return %d and errno is %d\n", res, errno);
            if (errno == EINTR && sig_type != SIGINT) {
                continue;
            }
            free(buf);
            return;
        }
        if(res == 0)
            return;
        buf[res - 1] = '\0';
        printf("[echo_rqt] %s\n", buf);
        // send back
        write(fd, buf, 512);
    }
}

void sigInt(int signo) {
    sig_type = SIGINT;
}
void sigPipe() {
    printf("[src] SIGPIPE is comming!\n");
}


int main(int argc, char* argv[]) {
    if( argc != 3 ) {
        fprintf(stderr, "<Usage>: ./server [IP] [PORT]\n");
        exit(EXIT_FAILURE);
    }
    // set signals
    to_exit = false;
    struct sigaction sigact_pipe, old_sigact_pipe;
    sigact_pipe.sa_handler = sigPipe;
    sigemptyset(&sigact_pipe.sa_mask);
    sigact_pipe.sa_flags = 0;
    sigact_pipe.sa_flags |= SA_RESTART;
    sigaction(SIGPIPE, &sigact_pipe, &old_sigact_pipe);

    char* bind_ip = argv[1];
    in_port_t bind_port = atoi(argv[2]);

    struct sockaddr_in server;
    // set server struct 
    memset(&server, 0, sizeof(server));
    server.sin_addr.s_addr = inet_addr(bind_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(bind_port);
    // init socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // bind
    printf("[srv] server[%s:%d] is initializing!\n", bind_ip, bind_port);
    if( bind(listen_fd, (struct sockaddr*)&server, SOCKADDR_SIZE) == -1 ) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    // start listening (max 5 client)
    if (listen(listen_fd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // start handling request
    struct      sockaddr_in client;
    socklen_t   addr_len = SOCKADDR_SIZE;
    int         client_fd;
    char*       client_ip;
    int         client_port;
    while(1) {
        client_fd = accept(listen_fd, (struct sockaddr *)&client, &addr_len); 
        if( client_fd == -1 && errno == EINTR && sig_type == SIGINT)
            break;
        if(client_fd == -1)
            continue;
        client_ip = inet_ntoa(client.sin_addr);
        client_port = ntohs(client.sin_port);
        printf("[srv] client[%s:%d] is accepted!\n", client_ip, client_port);
        // start reading
        handle(client_fd);
        close(client_fd);
        printf("[srv] connfd is closed!\n");
    }
    close(listen_fd);
    printf("[srv] listenfd is closed\n");
    printf("[srv] server is going to exit\n");
    return 0;
}