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
#include<mysql/mysql.h>

#define BUF_SIZE 100

void* clnt_connection(void *arg);
void send_msg(char* msg, int len);
void matching_seq();
void error_handling(char *msg);
void signal_handler(int signo);

int clnt_no = 0;
int clnt_socks[10][2];
int matched = 0;
int defender_selected = 0;
int role[10];

//mysql info
const char *host = "iitgamedb.cvmjrshfk7ix.us-east-2.rds.amazonaws.com";
const char *username = "juwonkim";
const char *pw = "5gkrsus5qks";
const char *dbname = "record";

MYSQL *connection = NULL;
MYSQL conn;
MYSQL_RES *sql_result;
MYSQL_ROW sql_row;  

struct sigaction sigact;

pthread_mutex_t mutx;

int main(int argc, char* argv[]){
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
	serv_addr_r.sin_port = htons(6007);

	memset(&serv_addr_w, 0, sizeof(serv_addr_w));
	serv_addr_w.sin_family = AF_INET;
	serv_addr_w.sin_addr.s_addr= htonl(INADDR_ANY);
	serv_addr_w.sin_port = htons(6008);


	if(bind(serv_sock_r, (struct sockaddr*)&serv_addr_r, sizeof(serv_addr_r)) == -1)
		error_handling("bind() error");

	if(bind(serv_sock_w, (struct sockaddr*)&serv_addr_w, sizeof(serv_addr_w)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock_r, 5) == -1)
		error_handling("listen() error");

	if(listen(serv_sock_w, 5) == -1)
		error_handling("listen() error");

	srand((unsigned)time(NULL));

	if(mysql_init(&conn) == NULL){
		error_handling("mysql_init() error");
	}

	connection = mysql_real_connect(&conn,host, username, pw, dbname, 3306, (const char*)NULL, 0);
	if(connection == NULL){
		error_handling("mysql connection failed");
	}else{
		printf("mysql conncection success\n");
	}

	if(mysql_select_db(&conn, dbname)){
		error_handling("mysql database select failed");
	}

	while(1){
		clnt_addr_r_size = sizeof(clnt_addr_r);
		clnt_sock_r = accept(serv_sock_r, (struct sockaddr *)&clnt_addr_r, &clnt_addr_r_size);

		clnt_addr_w_size = sizeof(clnt_addr_w);
		clnt_sock_w = accept(serv_sock_w, (struct sockaddr *)&clnt_addr_w, &clnt_addr_w_size);

		pthread_mutex_lock(&mutx);
		
		clnt_socks[clnt_no][0] = clnt_sock_r; // allocate readable socket
		pthread_mutex_unlock(&mutx);

		pthread_create(&thread, NULL, clnt_connection, (void*) clnt_sock_r); // create thread

		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_no++][1] = clnt_sock_w; // allocate writable socket
		pthread_mutex_unlock(&mutx);
	
		printf("%dth client, IP : %s\n", clnt_no, inet_ntoa(clnt_addr_r.sin_addr));
	}

	return 0;
}

void *clnt_connection(void *arg)
{
	int clnt_sock_r = (int)arg;
	int str_len = 0;
	char msg[256];
	int i;
	
	while((str_len = read(clnt_sock_r, msg, 256)) != 0){ // if socket get message, send this to other socket
		if(mysql_query(connection, msg)){
			printf("failed query : %s\n", msg);
			error_handling("mysql query failed");
		}else{
			printf("query : %s\n", msg);
		}
		str_len = strlen(msg);
		send_msg(msg, str_len);
		sleep(1);
	}
	
	pthread_mutex_lock(&mutx);
	for(i = 0 ; i < clnt_no ; i++){ //  if client number decrease, rid out disconnected socket from queue
		for(; i < clnt_no-1 ; i++){
			clnt_socks[i][0] = clnt_socks[i+1][0];
			clnt_socks[i][1] = clnt_socks[i+1][1];
			break;
		}
	}
	clnt_no--;
	
	pthread_mutex_unlock(&mutx);
	
	close(clnt_sock_r);
	return 0;
}

void send_msg(char *msg, int len)
{
	int i;
	int field;
	char game_type[1];
	char query[128];
	char *query1 = "SELECT username, foodpoint FROM rank WHERE game_type=";
	char *query2 = " ORDER BY foodpoint DESC;";
	char temp[8];
	char result[32];

	query[0] = 0;
	strcpy(temp,strrchr(msg,'='));
	game_type[0] = temp[2];
	game_type[1] = 0;
	strcat(strcat(strcat(query, query1), game_type), query2);

	pthread_mutex_lock(&mutx);

	if(mysql_query(connection, query)){
		printf("faild query : %s\n", query);
		error_handling("select mysql query failed");
	}

	sql_result = mysql_store_result(connection);
	if(sql_result == NULL){
		error_handling("select data store error");
	}

	field = mysql_num_fields(sql_result);

	while((sql_row = mysql_fetch_row(sql_result))){
		for(int j = 0 ; j < field ; j++){
			strcpy(result, sql_row[j]);
			printf("query_result : %s\n", result);
			//write(clnt_socks[i][1], msg, 4);a
			result[0] = 0;
		}
	}
	query[0] =0;
	temp[0] = 0;
	game_type[0] = 0;
	sql_result = NULL;
	pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void signal_handling(int signo)
{

}
