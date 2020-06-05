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

// FILE pointer
FILE* log_file_ptr;
// PDU struct
typedef struct _PDU {
    int pin;
    int len;
    char data[512];
}PDU;

int echo_rqt(int fd, int pin) {
    int res = 0;
    PDU buf;
    buf.pin = pin;
    int len = 0;
    // open data file
    char data_file_name[32];
    sprintf(data_file_name,  "td%d.txt", pin);
    FILE* data_file_ptr = fopen(data_file_name, "rb");
    // if fail to open
    if(!data_file_ptr) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    while (fgets(buf.data, MAX_CMD_STR, data_file_ptr)) {
        // if input exit then exit
        if( strncmp(buf.data, "exit", 4) == 0) {
            break;
        }
        buf.data[strlen(buf.data) - 1] = '\0';
        buf.len = strlen(buf.data) + 1;
        // send msg
        write(fd, &buf, sizeof(buf));
        // read msg
        memset(&buf, 0, sizeof(PDU));
        res = read(fd, &buf, sizeof(buf));
        if(res <= 0) {
            break;
        }
        fprintf(log_file_ptr, "[echo_rep](%d) %s\n", getpid(), buf.data);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 3 && argc != 4) {
        fprintf(stderr, "<Usage> ./%s [IP] [PORT] [MAX_PROCESS_NUM]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // get param
    int pin = 0;
    char *ip = argv[1];
    in_port_t p = atoi(argv[2]);
    int max_pin = atoi(argv[3]);
    // create process by max_pin
    pid_t pid;
    for(int i = 1; i < max_pin; i++) {
        pin++;
        pid = fork();
        if(pid == 0) {
            break;
        }
    }
    // for main process reset pin to 0
    if(pid != 0) {
        pin = 0;
    }
    // get socket
    int connfd =  socket(AF_INET, SOCK_STREAM, 0);
    // open file and show infomation
    char log_file_name[32];
    sprintf(log_file_name, "stu_cli_res_%d.txt", pin);
    log_file_ptr = fopen(log_file_name, "wb");
    if(!log_file_ptr) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    printf("[cli](%d) %s is created!\n", getpid(), log_file_name);
    // set server addr
    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(struct sockaddr_in));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(p);
    inet_pton(AF_INET, ip, &srv_addr.sin_addr);
    // open file and show info
    int res;
    while(1) {
        res = connect(connfd, (struct sockaddr *)&srv_addr, SOCKADDR_SIZE); 
        if (res == 0) {
            // after connecton 
            printf("[cli](%d) server[%s:%d] is connected!\n", getpid(), ip, p);
            res = echo_rqt(connfd, pin);
            if(res == 0)
                break;
        } else if( res == -1 && errno == EINTR)
            continue;
    }
    close(connfd);
    fprintf(log_file_ptr, "[cli](%d) connfd is closed\n", getpid());
    fprintf(log_file_ptr, "[cli](%d) client process is going to exit!\n", getpid());
    fclose(log_file_ptr);
    printf("[cli](%d) %s is closed!\n", getpid(), log_file_name);
    return 0;
}