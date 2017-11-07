#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define BUF_SIZE 100

void error_handling(char *msg);
void signal_handler(int signo);

char buffer[BUF_SIZE];
char sender[13];

int main(int argc, char* argv[])
{
	int serv_sock_r;
	int clnt_cnt = 0;
	int clnt_check[5];
	int nbytes, addr_len = sizeof(struct sockaddr);
	struct sockaddr_in serv_addr_r;
	struct sockaddr_in clnt_addr;
	struct sockaddr_in clnts[5];

	signal(SIGPIPE, SIG_IGN);

	serv_sock_r = socket(PF_INET, SOCK_DGRAM, 0);
	if(serv_sock_r < 0)
		error_handling("socket error");

	memset(clnts, 0, sizeof(struct sockaddr)*5);
	memset(clnt_check, 0, sizeof(int)*5);
	memset(buffer, 0, BUF_SIZE);
	memset(sender, ',', 13);
	memset(&clnt_addr, 0, sizeof(serv_addr_r));
	memset(&serv_addr_r, 0, sizeof(serv_addr_r));
	serv_addr_r.sin_family = AF_INET;
	serv_addr_r.sin_addr.s_addr= htonl(INADDR_ANY);
	serv_addr_r.sin_port = htons(6003);

	if(bind(serv_sock_r, (struct sockaddr *)&serv_addr_r, addr_len) < 0)
		error_handling("bind error");

	while(1){
		nbytes = recvfrom(serv_sock_r, buffer, BUF_SIZE, 0, (struct sockaddr *)&clnt_addr, &addr_len);
		//printf("reciver : %s\n",buffer);
		for(int i = 0 ; i < 2 ; i++){
			if(buffer[0] == i+48){ // if client connected first time, allocate client info to array
				if(clnt_check[i] == 0){
					memcpy(&clnts[i], &clnt_addr, addr_len);
					clnt_check[i] = 1;
					printf("udp client %s ip: %s\n",buffer, inet_ntoa(clnts[i].sin_addr));
					clnt_cnt++;
				}
			}
		}
		int fd = 0;
		for(int i = 0 ; i < clnt_cnt ; i++){
			//if(clnt_check[i] && buffer[0] == i){ // send buffer to other clients except myself
				fd = sendto(serv_sock_r, buffer, BUF_SIZE, MSG_DONTWAIT, (struct sockaddr *)&clnts[i], sizeof(struct sockaddr));
				printf("udp buffer %s ip: %s stat : %d\n",buffer, inet_ntoa(clnts[i].sin_addr),fd);
			//}
		}
		memset(buffer, 0, BUF_SIZE);
	}

	return 0;
}

void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
}
