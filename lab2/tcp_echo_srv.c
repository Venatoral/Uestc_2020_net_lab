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
#include <sys/wait.h>
#define SOCKADDR_SIZE   sizeof(struct sockaddr)
#define PDU_SIZE        sizeof(PDU)
bool to_exit;
int sig_type;

typedef struct _PDU {
    int pin;
    int len;
    char data[512];
}PDU;

int echo_rep(int fd, FILE* fp) {
    int pin = 0;
    PDU buf;
    memset(&buf, 0, PDU_SIZE);
    int res = 1;
    while (1) {
        res = read(fd, &buf, PDU_SIZE);
        if (res < 0) {
            printf("[srv] read len return %d and errno is %d\n", res, errno);
            if (errno == EINTR && sig_type != SIGINT) {
                continue;
            }
            break;
        }
        if(res == 0)
            break;
        pin = buf.pin;
        buf.data[buf.len] = '\0';
        fprintf(fp, "[echo_rqt](%d) %s\n", getpid(),buf.data);
        // send back
        write(fd, &buf, PDU_SIZE);
    }
    return pin;
}
// signal handlers 
void sig_int(int signo) {
    sig_type = SIGINT;
    to_exit = true;
    printf("[srv](%d) SIGINT is coming!\n", getpid());
}
void sig_pipe() {
    sig_type = SIGPIPE;
    printf("[src](%d) SIGPIPE is coming!\n", getpid());
}

void sig_chld(int signo) {
    printf("[src](%d) SIGCHLD is coming!\n", getpid());
    pid_t pid_chld;
    while( (pid_chld  = waitpid(-1, NULL, WNOHANG) ) > 0 );
}

int main(int argc, char* argv[]) {
    if( argc != 3 ) {
        fprintf(stderr, "<Usage>: ./server [IP] [PORT]\n");
        exit(EXIT_FAILURE);
    }
    // open txt file and show info
    FILE* fp = fopen("stu_srv_res_p.txt", "wb");
    if(!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    to_exit = false;
    printf("[srv](%d) stu_srv_res_p.txt is opened!", getpid());
    // SIGPIPE
    struct sigaction sigact_pipe, old_sigact_pipe;
    sigact_pipe.sa_handler = sig_pipe;
    sigemptyset(&sigact_pipe.sa_mask);
    sigact_pipe.sa_flags = 0;
    sigact_pipe.sa_flags |= SA_RESTART;
    sigaction(SIGPIPE, &sigact_pipe, &old_sigact_pipe);
    // SIGCHLD
    struct sigaction sigact_chld, old_sigact_chld;
    sigact_chld.sa_handler = sig_chld;
    sigemptyset(&sigact_chld.sa_mask);
    sigact_chld.sa_flags = 0;
    sigaction(SIGCHLD, &sigact_chld, &old_sigact_chld);
    // SIGINT
    struct sigaction sigact_int, old_sigact_int;
    sigact_int.sa_handler = sig_int;
    sigemptyset(&sigact_int.sa_mask);
    sigact_int.sa_flags = 0;
    
    sigaction(SIGINT, &sigact_int, &old_sigact_int);

    // get ip and port
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
    fprintf(fp, "[srv](%d) server[%s:%d] is initializing!\n", getpid(), bind_ip, bind_port);
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
    while(!to_exit) {
        client_fd = accept(listen_fd, (struct sockaddr *)&client, &addr_len); 
        if( client_fd == -1 && errno == EINTR && sig_type == SIGINT)
            break;
        if(client_fd == -1)
            continue;
        client_ip = inet_ntoa(client.sin_addr);
        client_port = ntohs(client.sin_port);
        // show accept info
        fprintf(fp, "[srv](%d) client[%s:%d] is accepted!\n", getpid(), client_ip, client_port);
        // fork and start reading
        pid_t pid = fork();
        // for child process
        if(pid == 0) {
            // open file
            char fn_res[32] = {0};
            sprintf(fn_res, "stu_srv_res_%d.txt", getpid());
            FILE* cfp = fopen(fn_res, "wb");
            if(!cfp) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            // after open file
            printf("[srv](%d) %s is opened!\n", getpid(), fn_res);
            fprintf(cfp, "[srv](%d) child process is created!\n", getpid());
            // for safety beacause child don't need listen_fd
            close(listen_fd);
            fprintf(cfp, "[srv](%d) listenfd is closed!\n", getpid());
            // start to handle req
            int pin = echo_rep(client_fd, cfp);
            close(client_fd);
            fprintf(cfp, "[srv](%d) connfd is closed!\n", getpid());
            // close file
            fprintf(cfp, "[srv](%d) child process is going to exit!\n", getpid());
            fclose(cfp);
            printf("[srv](%d) %s is closed!\n", getpid(), fn_res);
            // rename file
            char fn_new_name[32] = {0};
            sprintf(fn_new_name, "stu_srv_res_%d.txt", pin);
            rename(fn_res, fn_new_name);
            printf("[srv](%d) res file rename done!\n", getpid());
            exit(EXIT_SUCCESS);
        }
        close(client_fd);
    }
    // close listen socket
    close(listen_fd);
    fprintf(fp, "[srv](%d) listenfd is closed!\n", getpid());
    fprintf(fp,"[srv](%d) parent process is going to exit\n", getpid());
    // close file
    fclose(fp);
    printf("[srv](%d) stu_srv_res_p.txt is closed!\n", getpid());
    return 0;
}