//FILE: server/app_simple_server.c

//Description: this is the simple server application code. The server first starts the overlay by creating a direct TCP link between the client and the server. Then it initializes the SRT server by calling srt_svr_init(). It creates 2 sockets and waits for connection from the client by calling srt_svr_sock() and srt_svr_connect() twice. The server then receives short strings sent from the client from both connections. Finally the server closes the socket by calling srt_server_close(). Overlay is stopped by calling overlay_end().

//Date: April 26,2008

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

//two connection are created, one uses client port CLIENTPORT1 and server port SVRPORT1. The other uses client port CLIENTPORT2 and server port SVRPORT2
#define CLIENTPORT1 87
#define SVRPORT1 88
#define CLIENTPORT2 89
#define SVRPORT2 90
//after the strings are received, the server waits WAITTIME seconds, and then closes the connections
#define WAITTIME 10

//this function starts the overlay by creating a direct TCP connection between the client and the server. The TCP socket descriptor is returned. If the TCP connection fails, return -1. The TCP socket desciptor returned will be used by SRT to send segments.
int overlay_start() {

  // Your code here.

}

//this function stops the overlay by closing the TCP connection between the server and the client
void overlay_stop(int connection) {

  // Your code here.

}

int main() {
	//random seed for segment loss
	srand(time(NULL));

	//start overlay and get the overlay TCP socket descriptor
	int overlay_conn = overlay_start();
	if(overlay_conn<0) {
		printf("can not start overlay\n");
	}

	//initialize srt server
	srt_server_init(overlay_conn);

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

	//stop the overlay
	overlay_stop(overlay_conn);
}
