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
    while (fgets(buf, MAX_CMD_STR, stdin)) {
        // if input exit then exit
        if( strncmp(buf, "exit", 4) == 0) {
            break;
        }
        buf[strlen(buf) - 1] = '\0';
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
        fprintf(stderr, "<Usage> ./%s [IP] [PORT]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in srv_addr;
    int connfd =  socket(AF_INET, SOCK_STREAM, 0);
    char* ip = argv[1];
    in_port_t p = atoi(argv[2]);
    memset(&srv_addr, 0, sizeof(struct sockaddr_in));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(p);
    inet_pton(AF_INET, ip, &srv_addr.sin_addr);
    int res;
    while(1) {
        res = connect(connfd, (struct sockaddr *)&srv_addr, SOCKADDR_SIZE); 
        if (res == 0) {
            // after connecton 
            printf("[cli] server[%s:%d] is connected!\n", ip, p);
            res = echo_rqt(connfd);
            if(res == 0)
                break;
        } else if( res == -1 && errno == EINTR)
            continue;
    }

    close(connfd);
    printf("[cli] connfd is closed\n");
    printf("[cli] client is going to exit!\n");
    return 0;
}