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

#define SOCKADDR_SIZE sizeof(struct sockaddr)
void echo_rqt(int fd) {
    int res = 0;
    char* buf = malloc(512);
    int len = 0;
    while (1) {
        memset(buf, 0, 512);
        res = read(0, buf, 512);
        buf[res - 1] = '\0';
        // if input exit then exit
        if( strncmp(buf, "exit", 4) == 0) {
            return;
        }
        len = strlen(buf) + 1;
        // send to server ( size then msg)
        write(fd, &len, sizeof(len));
        write(fd, buf, len);

        // read msg from server
        memset(buf, 0, 512);
        res = read(fd, &len, sizeof(len));
        if(res <= 0) {
            return;
        }

        res = read(fd, buf, len);

        if(res <= 0) {
            return;
        }

        printf("[echo_rep] %s\n", buf);
    }
}
int main(int argc, char* argv[]) {
    if(argc != 3) {
        fprintf(stderr, "<Usage> ./client [IP] [PORT]\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in client;
    int client_fd =  socket(AF_INET, SOCK_STREAM, 0);
    char* ip = argv[1];
    in_port_t p = atoi(argv[2]);
    memset(&client, 0, sizeof(struct sockaddr_in));
    client.sin_family = AF_INET;
    client.sin_port = htons(p);
    client.sin_addr.s_addr = inet_addr(ip);

    if( connect(client_fd, (struct sockaddr*)&client, SOCKADDR_SIZE) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    // after connecton 
    printf("[cli] server[%s:%d] is connected!\n", ip, p);
    echo_rqt(client_fd);

    close(client_fd);
    printf("[cli] connfd is closed\n");
    printf("[cli] client is going to exit!\n");
    return 0;
}