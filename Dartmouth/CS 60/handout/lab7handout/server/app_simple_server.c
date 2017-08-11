//FILE: server/app_simple_server.c

//Description: this is the simple server application code. The server first connects to the local SNP process. Then it initializes the SRT server by calling srt_svr_init(). It creates 2 sockets and waits for connection from the client by calling srt_svr_sock() and srt_svr_connect() twice. The server then receives short strings sent from the client from both connections. Finally the server closes the socket by calling srt_server_close(). The server disconnects from the local SNP process.

//Date: May 6,2008

//Input: None

//Output: SRT server states

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "../common/constants.h"
#include "srt_server.h"

//Two connection are created. One uses client port CLIENTPORT1 and server port SVRPORT1. The other uses client port CLIENTPORT2 and server port SVRPORT2.
#define CLIENTPORT1 87
#define SVRPORT1 88
#define CLIENTPORT2 89
#define SVRPORT2 90
//After the strings are received, the server waits WAITTIME seconds, and then closes the connections
#define WAITTIME 15

//This function connects to the local SNP process on port NETWORK_PORT. If the TCP connection fails, return -1. The TCP socket desciptor returned will be used by SRT to send segments.
int connectToNetwork() {
	struct sockaddr_in servaddr;
	
	servaddr.sin_family = AF_INET;	
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(NETWORK_PORT);

	int network_conn = socket(AF_INET,SOCK_STREAM,0);  
	if(network_conn<0)
		return -1;
	if(connect(network_conn, (struct sockaddr*)&servaddr, sizeof(servaddr))!=0) 
		return -1;

	//succefully connected
	return network_conn;
}

//This function is used to disconnect from the local SNP process by closing the TCP connection. 
void disconnectToNetwork(int network_conn) {
	close(network_conn);
}


int main() {
	//random seed for segment loss
	srand(time(NULL));

	//connect to local SNP process and get the TCP socket descriptor
	int network_conn = connectToNetwork();
	if(network_conn<0) {
		printf("can not start overlay\n");
	}

	//initialize srt server
	srt_server_init(network_conn);

	//create a srt server sock at port SVRPORT1 
	int sockfd= srt_server_sock(SVRPORT1);
	if(sockfd<0) {
		printf("can't create srt server\n");
		exit(1);
	}
	//listen and accept connection from a srt client 
	srt_server_accept(sockfd);

	//create a srt server sock at port SVRPORT2
	int sockfd2= srt_server_sock(SVRPORT2);
	if(sockfd2<0) {
		printf("can't create srt server\n");
		exit(1);
	}
	//listen and accept connection from a srt client 
	srt_server_accept(sockfd2);


	char buf1[6];
	char buf2[7];
	int i;
	//receive strings from first connection
	for(i=0;i<5;i++) {
		srt_server_recv(sockfd,buf1,6);
		printf("recv string: %s from connection 1\n",buf1);
	}
	//receive strings from second connection
	for(i=0;i<5;i++) {
		srt_server_recv(sockfd2,buf2,7);
		printf("recv string: %s from connection 2\n",buf2);
	}

	sleep(WAITTIME);

	//close srt server 
	if(srt_server_close(sockfd)<0) {
		printf("can't destroy srt server\n");
		exit(1);
	}				
	if(srt_server_close(sockfd2)<0) {
		printf("can't destroy srt server\n");
		exit(1);
	}				

	//disconnect from the local SNP process
	disconnectToNetwork(network_conn);
}
