#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<signal.h>
#include<time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define BUF_SIZE 100

void error_handling(char *msg);

int clnt_no = 0;
int clnt_socks[10][2];
int matched = 0;
int defender_selected = 0;
int role[10];
char buf[BUF_SIZE];

struct sigaction sigact;

pthread_mutex_t mutx;

int main(int argc, char* argv[])
{
	int serv_sock_r; // Readable server socket
	int serv_sock_w; // Writable server socket
	int clnt_sock_r; // Readable client socket
	int clnt_sock_w; // Writable client socket
	struct sockaddr_in serv_addr_r;
	struct sockaddr_in serv_addr_w;
	struct sockaddr_in clnt_addr_r;
	struct sockaddr_in clnt_addr_w;
	int clnt_addr_r_size;
	int clnt_addr_w_size;
	pthread_t thread;

	signal(SIGPIPE, SIG_IGN); // Ignore PIPE signal

	if(pthread_mutex_init(&mutx, NULL)) // start mutex
		error_handling("mutex init error");

	serv_sock_r = socket(PF_INET, SOCK_STREAM, 0); // open read server socket as tcp
	if(serv_sock_r < 0)
		error_handling("socket error");

	serv_sock_w = socket(PF_INET, SOCK_STREAM, 0); // open write server socket as tcp
	if(serv_sock_w < 0)
		error_handling("socket error");

	memset(role, 0, sizeof(int)*10);
	memset(&serv_addr_r, 0, sizeof(serv_addr_r));
	serv_addr_r.sin_family = AF_INET;
	serv_addr_r.sin_addr.s_addr= htonl(INADDR_ANY);
	serv_addr_r.sin_port = htons(6000);

	memset(&serv_addr_w, 0, sizeof(serv_addr_w));
	serv_addr_w.sin_family = AF_INET;
	serv_addr_w.sin_addr.s_addr= htonl(INADDR_ANY);
	serv_addr_w.sin_port = htons(6001);


	if(bind(serv_sock_r, (struct sockaddr*)&serv_addr_r, sizeof(serv_addr_r)) == -1)
		error_handling("bind() error");

	if(bind(serv_sock_w, (struct sockaddr*)&serv_addr_w, sizeof(serv_addr_w)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock_r, 5) == -1)
		error_handling("listen() error");

	if(listen(serv_sock_w, 5) == -1)
		error_handling("listen() error");
	
	while(1){
		clnt_addr_r_size = sizeof(clnt_addr_r);
		clnt_sock_r = accept(serv_sock_r, (struct sockaddr *)&clnt_addr_r, &clnt_addr_r_size);

		clnt_addr_w_size = sizeof(clnt_addr_w);
		clnt_sock_w = accept(serv_sock_w, (struct sockaddr *)&clnt_addr_w, &clnt_addr_w_size);

		int readfrom = recv(clnt_sock_r, buf, BUF_SIZE, 0);
		if(readfrom > 0){
			printf("%dth client, IP : %s\n", clnt_no, inet_ntoa(clnt_addr_r.sin_addr));
			printf("got message : %s\n", buf);
			int sendto = send(clnt_sock_w, buf, readfrom, 0);
		}
	}

	return 0;
}

void error_handling(char * msg){
	printf("%s\n",msg);
}
