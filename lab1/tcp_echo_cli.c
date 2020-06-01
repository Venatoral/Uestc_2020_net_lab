#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CMD_STR 100
#define SOCKADDR_SIZE sizeof(struct sockaddr)
int echo_rqt(int fd) {
    int res = 0;
    char* buf = malloc(MAX_CMD_STR + 1);
    int len = 0;
    while (1) {
        memset(buf, 0, MAX_CMD_STR + 1);
        res = read(0, buf, MAX_CMD_STR + 1);
        // if input exit then exit
        if( strncmp(buf, "exit", 4) == 0) {
            break;
        }
        buf[res - 1] = '\0';
        len = strlen(buf) + 1;
        // send to server ( size then msg)
        write(fd, &len, sizeof(len));
        write(fd, buf, len);
        // read msg length from server
        memset(buf, 0, MAX_CMD_STR + 1);
        res = read(fd, &len, sizeof(len));
        if(res <= 0) {
            break;
        }
        // read msg
        res = read(fd, buf, len);
        if(res <= 0) {
            break;
        }
        printf("[echo_rep] %s\n", buf);
    }
    return 0;
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
    int res;
    while(1) {
        res = connect(client_fd, (struct sockaddr *)&client, SOCKADDR_SIZE); 
        if (res == -1 && errno == EINTR)
            continue;
        if (res == -1)
            break;
        // after connecton 
        printf("[cli] server[%s:%d] is connected!\n", ip, p);
        res = echo_rqt(client_fd);
        if(res == 0)
            break;
    }

    close(client_fd);
    printf("[cli] connfd is closed\n");
    printf("[cli] client is going to exit!\n");
    return 0;
}