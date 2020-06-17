/*
	套接字编程实验2-TCP多进程并发服务器与多进程客户端参考模版——服务器端
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <assert.h>

#define BACKLOG 1024
#define bprintf(fp, format, ...) \
	if(fp == NULL){printf(format, ##__VA_ARGS__);} 	\
	else{printf(format, ##__VA_ARGS__);	\
			fprintf(fp, format, ##__VA_ARGS__);fflush(fp);}	//后面的输出到文件操作，建议使用这个宏，还可同时在屏幕上显示出来

int sig_type = 0, sig_to_exit = 0;
FILE * fp_res = NULL;	//文件指针

void sig_chld(int signo) {
	sig_type = SIGCHLD;
	printf("[src](%d) SIGCHLD is coming!\n", getpid());
	pid_t pid_chld;
	while ((pid_chld = waitpid(-1, NULL, WNOHANG)) > 0)
		;
}

void sig_int(int signo) {	
	// TODO 记录本次系统信号编号到sig_type中;通过getpid()获取进程ID，按照指导书上的要求打印相关信息，并设置sig_to_exit的值
	sig_type = SIGINT;
	sig_to_exit = true;
	printf("[srv](%d) SIGINT is coming!\n", getpid());
}

void sig_pipe(int signo) {	
	// TODO 记录本次系统信号编号到sig_type中;通过getpid()获取进程ID，按照指导书上的要求打印相关信息，并设置sig_to_exit的值
	sig_type = SIGPIPE;
	printf("[src](%d) SIGPIPE is coming!\n", getpid());
}


/*
int install_sig_handlers()
功能：安装SIGPIPE,SIGCHLD,SIGINT三个信号的处理函数
	返回值：
		-1，安装SIGPIPE失败；
		-2，安装SIGCHLD失败；
		-3，安装SIGINT失败；
		0，都成功
*/
int install_sig_handlers(){
	int res = -1;
	struct sigaction sigact_pipe, old_sigact_pipe;
	sigact_pipe.sa_handler = sig_pipe;//sig_pipe()，信号处理函数
	sigact_pipe.sa_flags = 0;
	sigact_pipe.sa_flags |= SA_RESTART;//设置受影响的慢系统调用重启
	sigemptyset(&sigact_pipe.sa_mask);
	res = sigaction(SIGPIPE, &sigact_pipe, &old_sigact_pipe);
	if(res)
		return -1;

	// TODO 安装SIGCHLD信号处理器,若失败返回-2.这里可直接将handler设为SIG_IGN，忽略SIGCHLD信号即可，
	// 注意和上述SIGPIPE一样，也要设置受影响的慢系统调用重启。也可以按指导书说明用一个自定义的sig_chld
	// 函数来处理SIGCHLD信号(复杂些)
	struct sigaction sigact_chld, old_sigact_chld;
	sigact_chld.sa_handler = sig_chld;
	sigact_chld.sa_flags = 0;
	sigact_chld.sa_flags |= SA_RESTART; //设置受影响的慢系统调用重启
	sigemptyset(&sigact_chld.sa_mask);
	res = sigaction(SIGCHLD, &sigact_chld, &old_sigact_chld);
	if(res)
		return -2;
	// TODO 安装SIGINT信号处理器,若失败返回-3
	struct sigaction sigact_int, old_sigact_int;
	sigact_int.sa_handler = sig_int;
	sigact_int.sa_flags = 0;
	sigemptyset(&sigact_int.sa_mask);
	res = sigaction(SIGINT, &sigact_int, &old_sigact_int);
	if(res)
		return -3;
	return 0;
}

/*
int echo_rep(int sockfd)
功能：业务处理函数
	返回：
		-1，未获取客户端PIN
		>1，有效的客户端PIN
*/
int echo_rep(int sockfd)
{
	int len_h = -1, len_n = -1;	//h后缀的用来存放主机字节序的结果，n的用来存放网络上读入的网络字节序结果，下同
	int pin_h = -1, pin_n = -1;
	int res = 0;
	char *buf = NULL;
	pid_t pid = getpid();

	// 读取客户端PDU并执行echo回复
    do {
		// 读取客户端PIN（Process Index， 0-最大并发数），并创建记录文件
		do {
			//TODO 用read读取PIN（网络字节序）到pin_n中；返回值赋给res
			res = read(sockfd, &pin_n, 4);
			if(res < 0){
				bprintf(fp_res, "[srv](%d) read pin_n return %d and errno is %d!\n", pid, res, errno);
				if(errno == EINTR){
					if(sig_type == SIGINT)
						return pin_h;
					continue;
				}
				return pin_h;
			}
			if(!res){
				return pin_h;
			}
			//TODO 将pin_n字节序转换后存放到pin_h中
			pin_h = ntohl(pin_n);
			break;		
		} while(1);

		// 读取客户端echo_rqt数据长度
		do{
			//TODO 用read读取客户端echo_rqt数据长度（网络字节序）到len_n中:返回值赋给res
			res = read(sockfd, &len_n, 4);			
			if(res < 0){
				bprintf(fp_res, "[srv](%d) read len_n return %d and errno is %d\n", pid, res, errno);
				if(errno == EINTR){
					if(sig_type == SIGINT)
						return len_h;
					continue;
				}
				return len_h;
			}
			if(!res){
				return len_h;
			}
			//TODO 将len_n字节序转换后存放到len_h中
			len_h = ntohl(len_n);
			break;
		}while(1);
		// 读取客户端echo_rqt数据
		buf = (char*)malloc(len_h * + 8); // 预留PID与数据长度的存储空间，为后续回传做准备
		
		int read_amnt = 0, len_to_read = len_h;
		do{
			//TODO 使用read读取客户端数据，返回值赋给res。注意read第2、3个参数，即每次存放的缓冲区的首地址及所需读取的长度如何设定
			res = read(sockfd, buf + 8 + read_amnt, len_to_read);
			if(res < 0){
				bprintf(fp_res, "[srv](%d) read data return %d and errno is %d,\n", pid, res, errno);
				if(errno == EINTR){
					if(sig_type == SIGINT){
						free(buf);
						return pin_h;
					}
					continue;
				}
				free(buf);
				return pin_h;
			}
			if(!res){
				free(buf);
				return pin_h;
			}
			//TODO 此处计算read_amnt及len_to_read的值，注意考虑已读完和未读完两种情况，以及其它情况（此时直接用上面的代码操作，free(buf),并 return pin_h）
			read_amnt += res;
			len_to_read = len_h - read_amnt;
			if(len_to_read <= 0)
				break;
		}while(1);
		buf[len_h + 8 - 1] = 0;
		//TODO 解析客户端echo_rqt数据并写入res文件；注意缓冲区起始位置哦
		fprintf(fp_res, "[echo_rqt](%d) %s\n", getpid(), buf + 8);
		// 将客户端PIN写入PDU缓存（网络字节序）
		memcpy(buf, &pin_n, 4);
		// 将echo_rep数据长度写入PDU缓存（网络字节序）
		memcpy(buf + 4, &len_n, 4);
		//TODO 用write发送echo_rep数据并释放buf:
		write(sockfd, buf, len_h + 8);
    }while(1);
	return pin_h;
}

int main(int argc, char* argv[])
{
	// 基于argc简单判断命令行指令输入是否正确；
	if(argc != 3)
	{
		printf("Usage:%s <IP> <PORT>\n", argv[0]);
		return -1;
	}
 	// 获取当前进程PID，用于后续父进程信息打印；
	pid_t pid = getpid();
 	// 定义IP地址字符串（点分十进制）缓存，用于后续IP地址转换；
	char ip_str[20]={0};//用于IP地址转换
	// 定义res文件名称缓存，用于后续文档名称构建；
	char fn_res[20]={0};//
	// 定义结果变量，用于承接后续系统调用返回结果；
	int res = -1;
	// TODO 调用install_sig_handlers函数，安装信号处理器，包括SIGPIPE，SIGCHLD以及SIGINT；如果返回的不是0，就打印一个出错信息然后返回res值
	res = install_sig_handlers();
	if(res != 0) {
		printf("[srv](%d) failed to install signal handkers!\n", pid);
		return res;
	}
	// 打开文件"stu_srv_res_p.txt"，用于后续父进程信息记录；
	fp_res = fopen("stu_srv_res_p.txt", "wb");
	if(!fp_res){
		printf("[srv](%d) failed to open file \"stu_srv_res_p.txt\"!\n", pid);
		return res;
	}

    //TODO 定义如下变量：
	// 服务器Socket地址srv_addr，客户端Socket地址cli_addr；
	// 客户端Socket地址长度cli_addr_len（类型为socklen_t）；
	// Socket监听描述符listenfd，以及Socket连接描述符connfd；
	struct sockaddr_in srv_addr, cli_addr;
	socklen_t cli_addr_len = sizeof(struct sockaddr);
	int listenfd, connfd;
	//TODO 初始化服务器Socket地址srv_addr，其中会用到argv[1]、argv[2]
		/* IP地址转换推荐使用inet_pton()；端口地址转换推荐使用atoi(); */
	char *bind_ip = argv[1];
	in_port_t bind_port = atoi(argv[2]);
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_addr.s_addr = inet_addr(bind_ip);
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(bind_port);
	//TODO 按题设要求打印服务器端地址server[ip:port]到fp_res文件中，推荐使用inet_ntop();
	fprintf(fp_res, "[srv](%d) server[%s:%d] is initializing!\n", getpid(), bind_ip, bind_port);
	fflush(fp_res);
	//TODO 获取Socket监听描述符: listenfd = socket(x,x,x);
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	//TODO 绑定服务器Socket地址: res = bind(x,x,x);
	if (bind(listenfd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	// TODO 开启服务监听: res = listen(x,x);
	if (listen(listenfd, BACKLOG) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	// 开启accpet()主循环，直至sig_to_exit指示服务器退出；
	char *client_ip;
	int client_port;
	while(!sig_to_exit) {
		//TODO 获取cli_addr长度，执行accept()：connfd = accept(x,x,x);
		connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_addr_len);
		// 以下代码紧跟accept()，用于判断accpet()是否因SIG_INT信号退出（本案例中只关心SIGINT）；也可以不做此判断，直接执行 connfd<0 时continue，因为此时sig_to_exit已经指明需要退出accept()主循环，两种方式二选一即可。
		if(connfd == -1 && errno == EINTR){
			if(sig_type == SIGINT)
				break;
			continue;
		}
		if(connfd == -1)
			break;
		//TODO 按题设要求打印客户端端地址client[ip:port]到fp_res文件中，推荐使用inet_ntop();
		client_ip = inet_ntoa(cli_addr.sin_addr);
		client_port = ntohs(cli_addr.sin_port);
		// show accept info
		fprintf(fp_res, "[srv](%d) client[%s:%d] is accepted!\n", getpid(), client_ip, client_port);
		fflush(fp_res);
		// 派生子进程对接客户端开展业务交互
		if(!fork()){// 子进程
			// 获取当前子进程PID,用于后续子进程信息打印
			pid = getpid();
			// 打开res文件，首先基于PID命名，随后在子进程退出前再根据echo_rep()返回的PIN值对文件更名；
			sprintf(fn_res, "stu_srv_res_%d.txt", pid);
			fp_res = fopen(fn_res, "wb");// Write only， append at the tail. Open or create a binary file;
			if(!fp_res){
				printf("[srv](%d) child exits, failed to open file \"stu_srv_res_%d.txt\"!\n", pid, pid);
				exit(-1);
			}
			//TODO 按指导书要求，将文件被打开的提示信息打印到stdout
			printf("[srv](%d) %s is opened!\n", getpid(), fn_res);
			bprintf(fp_res, "[srv](%d) child process is created!\n", getpid());
			//TODO 关闭监听描述符，打印提示信息到文件中
			close(listenfd);
			bprintf(fp_res, "[srv](%d) listenfd is closed!\n", getpid());
			//TODO 执行业务函数echo_rep（返回客户端PIN到变量pin中，以便用于后面的更名操作）
			int pin = echo_rep(connfd);
			if(pin < 0){
				bprintf(fp_res, "[srv](%d) child exits, client PIN returned by echo_rqt() error!\n", pid);
				exit(-1);
			}
			// 更名子进程res文件（PIN替换PID）
			char fn_res_n[20]={0};
			sprintf(fn_res_n, "stu_srv_res_%d.txt", pin);
			if(!rename(fn_res, fn_res_n)){
				bprintf(fp_res, "[srv](%d) res file rename done!\n", pid);
			}
			else{			
				bprintf(fp_res, "[srv](%d) child exits, res file rename failed!\n", pid);
			}
			//TODO 关闭连接描述符，输出信息到res文件中
			close(connfd);
			fprintf(fp_res, "[srv](%d) connfd is closed!\n", getpid());
			//TODO 关闭子进程res文件,并按指导书要求打印提示信息到stdout,然后exit
			fprintf(fp_res, "[srv](%d) child process is going to exit!\n", getpid());
			fclose(fp_res);
			printf("[srv](%d) %s is closed!\n", getpid(), fn_res);
			exit(EXIT_SUCCESS);
		}
		else{// 父进程		
			//TODO 关闭连接描述符
			close(connfd);
		}
	}

	//TODO 关闭监听描述符
	close(listenfd);
	bprintf(fp_res, "[srv](%d) listenfd is closed!\n", pid);
	bprintf(fp_res, "[srv](%d) parent process is going to exit!\n", pid);
	//TODO 关闭父进程res文件,并按指导书要求打印提示信息至stdout
	fclose(fp_res);
	printf("[srv](%d) stu_srv_res_p.txt is closed!\n", getpid());
	return 0;
}