//FILE: client/app_simple_client.c
//
//Description: this is the simple client application code. The client first connects to the local SNP process. Then it initializes the SRT client by calling srt_client_init(). It creates 2 sockets and connects to the server  by calling srt_client_sock() and srt_client_connect() twice. It then sends short strings to the server from these two connections. After some time, the client disconnects from the server by calling srt_client_disconnect(). Finally the client closes the socket by calling srt_client_close(). The client then disconnects from the local SNP process.

//Date: May 6, 2008

//Input: None

//Output: SRT client states

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/constants.h"
#include "../topology/topology.h"
#include "srt_client.h"

//Two connection are created. One uses client port CLIENTPORT1 and server port SVRPORT1. The other uses client port CLIENTPORT2 and server port SVRPORT2.
#define CLIENTPORT1 87
#define SVRPORT1 88
#define CLIENTPORT2 89
#define SVRPORT2 90

//After connecting to the SNP process, wait STARTDELAY for server to start.
#define STARTDELAY 1
//After the strings are sent, wait WAITTIME seconds, and then close the connections.
#define WAITTIME 5

//This function connects to the local SNP process on port NETWORK_PORT. If TCP connection fails, return -1. The TCP socket desciptor returned will be used by SRT to send segments.
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

//This function disconnects from the local SNP process by closing the local TCP connection to the local SNP process. 
void disconnectToNetwork(int network_conn) {
	close(network_conn);
}

int main() {
	//random seed for loss rate
	srand(time(NULL));

	//connect to SNP process and get the TCP socket descriptor	
	int network_conn = connectToNetwork();
	if(network_conn<0) {
		printf("fail to connect to the local SNP process\n");
		exit(1);
	}

	//initialize srt client
	srt_client_init(network_conn);
	sleep(STARTDELAY);

	char hostname[50];
	printf("Enter server name to connect:");
	scanf("%s",hostname);
	int svr_nodeID = topology_getNodeIDfromname(hostname);
	if(svr_nodeID == -1) {
		printf("host name error!\n");
		exit(1);
	} else {
		printf("connecting to node %d\n",svr_nodeID);
	}

	//create a srt client sock on port CLIENTPORT1 and connect to srt server port SVRPORT1
	int sockfd = srt_client_sock(CLIENTPORT1);
	if(sockfd<0) {
		printf("fail to create srt client sock");
		exit(1);
	}
	if(srt_client_connect(sockfd,svr_nodeID,SVRPORT1)<0) {
		printf("fail to connect to srt server\n");
		exit(1);
	}
	printf("client connected to server, client port:%d, server port %d\n",CLIENTPORT1,SVRPORT1);
	
	//create a srt client sock on port CLIENTPORT2 and connect to srt server port SVRPORT2
	int sockfd2 = srt_client_sock(CLIENTPORT2);
	if(sockfd2<0) {
		printf("fail to create srt client sock");
		exit(1);
	}
	if(srt_client_connect(sockfd2,svr_nodeID,SVRPORT2)<0) {
		printf("fail to connect to srt server\n");
		exit(1);
	}
	printf("client connected to server, client port:%d, server port %d\n",CLIENTPORT2, SVRPORT2);

	//send strings through the first connection
      	char mydata[6] = "hello";
	int i;
	for(i=0;i<5;i++){
      		srt_client_send(sockfd, mydata, 6);
		printf("send string:%s to connection 1\n",mydata);	
      	}
	//send strings through the second connection
      	char mydata2[7] = "byebye";
	for(i=0;i<5;i++){
      		srt_client_send(sockfd2, mydata2, 7);
		printf("send string:%s to connection 2\n",mydata2);	
      	}

	//wait for a while and close the connections
	sleep(WAITTIME);

	if(srt_client_disconnect(sockfd)<0) {
		printf("fail to disconnect from srt server\n");
		exit(1);
	}
	if(srt_client_close(sockfd)<0) {
		printf("fail to close srt client\n");
		exit(1);
	}
	
	if(srt_client_disconnect(sockfd2)<0) {
		printf("fail to disconnect from srt server\n");
		exit(1);
	}
	if(srt_client_close(sockfd2)<0) {
		printf("fail to close srt client\n");
		exit(1);
	}

	//disconnect from the SNP process
	disconnectToNetwork(network_conn);
}
