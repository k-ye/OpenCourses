//FILE: server/app_stress_server.c

//Description: this is the stress server application code. The server first connects to the local SNP process.  Then it initializes the SRT server by calling srt_svr_init(). It creates a sockets and waits for connection from the client by calling srt_svr_sock() and srt_svr_connect(). It then receives the length of the file to be received. After that, it creates a buffer, receives the file data and saves the file data to receivedtext.txt file. Finally the server closes the socket by calling srt_server_close(). The server disconnects from the local SNP process.

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

//One SRT connection is created using client port CLIENTPORT1 and server port SVRPORT1. 
#define CLIENTPORT1 87
#define SVRPORT1 88
//After the received file data is saved, the server waits WAITTIME seconds, and then closes the connection.
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

	//start overlay and get the overlay TCP socket descriptor
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

	//receive the file size first 
	//and then receive the file data
	int fileLen;
	srt_server_recv(sockfd,&fileLen,sizeof(int));
	char* buf = (char*) malloc(fileLen);
	srt_server_recv(sockfd,buf,fileLen);

	//save the received file data in receivedtext.txt
	FILE* f;
	f = fopen("receivedtext.txt","a");
	fwrite(buf,fileLen,1,f);
	fclose(f);
	free(buf);

	//wait for a while
	sleep(WAITTIME);

	//close srt server 
	if(srt_server_close(sockfd)<0) {
		printf("can't destroy srt server\n");
		exit(1);
	}				

	//disconnect from the local SNP process
	disconnectToNetwork(network_conn);
}
