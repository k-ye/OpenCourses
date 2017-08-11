//FILE: server/srt_server.c
//
//Description: this file contains the SRT server interface implementation 
//
//Date: April 18,2008
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/select.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include "srt_server.h"
#include "../topology/topology.h"
#include "../common/constants.h"


//declare tcbtable as global variable
svr_tcb_t* tcbtable[MAX_TRANSPORT_CONNECTIONS];
//declare the connection to the SNP process as global variable
int network_conn;


/*********************************************************************/
//
//help functions for tcbtable operations
//
/*********************************************************************/

//get the tcb indexed by sockfd
//return 0 if no tcb found
svr_tcb_t* tcbtable_gettcb(int sockfd) {
	if(tcbtable[sockfd]!=NULL)
		return tcbtable[sockfd];
	else
		return 0;
}

//get the tcb with given server port 
//return 0 is no tcb found
svr_tcb_t* tcbtable_gettcbFromPort(unsigned int serverPort)
{
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable[i]!=NULL && tcbtable[i]->svr_portNum==serverPort) {
			return tcbtable[i];
		}
	}
	return 0;
}


//get a new tcb from tcbtable
//assign the server port with the given port number
//return the index of the new tcb
//return -1 if the all tcbs in tcbtable are used or the given port number is used
int tcbtable_newtcb(unsigned int port) {
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable[i]!=NULL&&tcbtable[i]->svr_portNum==port) {
			return -1;
		}
	}

	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable[i]==NULL) {
			tcbtable[i] = (svr_tcb_t*) malloc(sizeof(svr_tcb_t));
			tcbtable[i]->svr_portNum = port;
			return i;
		}
	}
	return -1;
}


/*********************************************************************/
//
//SRT APIs implementation
//
/*********************************************************************/

// This function initializes the TCB table marking all entries NULL. It also initializes 
// a global variable for the TCP socket descriptor ``conn'' used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to 
// handle the incoming segments. There is only one seghandler for the server side which
// handles call connections for the client.
void srt_server_init(int conn) {
	//initialize global variables
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++)
		tcbtable[i] = NULL;
	network_conn = conn;

	//create seghandler thread 
	pthread_t seghandler_thread;
	pthread_create(&seghandler_thread,NULL,seghandler, (void*)0);
}

// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized 
// e.g., TCB state is set to CLOSED and the server port set to the function call parameter 
// server port.  The TCB table entry index should be returned as the new socket ID to the server 
// and be used to identify the connection on the server side. If no entry in the TCB table  
// is available the function returns -1.
int srt_server_sock(unsigned int port) {
	//get a tcb from tcb table
	int sockfd = tcbtable_newtcb(port);
	if(sockfd<0)
		return -1;

	//create a receive buffer 
	char* recvBuf;
	recvBuf = (char*) malloc(RECEIVE_BUF_SIZE);
	assert(recvBuf!=NULL);

	//create a mutex for receive buffer	
	pthread_mutex_t* recvBuf_mutex;
	recvBuf_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	assert(recvBuf_mutex!=NULL);
	pthread_mutex_init(recvBuf_mutex,NULL);

	// initialize  server tcb
	svr_tcb_t* my_servertcb = tcbtable_gettcb(sockfd);
	my_servertcb->svr_nodeID = topology_getMyNodeID();
	my_servertcb->state = CLOSED;
	my_servertcb->usedBufLen = 0;
	my_servertcb->bufMutex = recvBuf_mutex;
	my_servertcb->recvBuf = recvBuf;
	return sockfd;
}

// This function gets the TCB pointer using the sockfd and changes the state of the connection to 
// LISTENING. It then starts a timer to ``busy wait'' until the TCB's state changes to CONNECTED 
// (seghandler does this when a SYN is received). It waits in an infinite loop for the state 
// transition before proceeding and to return 1 when the state change happens, dropping out of
// the busy wait loop.
int srt_server_accept(int sockfd) {
	//get tcb indexed by sockfd
	svr_tcb_t* my_servertcb;
	my_servertcb = tcbtable_gettcb(sockfd);
	if(!my_servertcb)
		return -1;

	switch(my_servertcb->state) {
		case CLOSED:
			//state transition
			my_servertcb->state = LISTENING;
			//busy wait until state transitions to CONNECTED
			while(1) {
				if (my_servertcb->state == CONNECTED)
					break;
				else {
					select(0,0,0,0,& (struct timeval) {.tv_usec = ACCEPT_POLLING_INTERVAL/1000} );
				}
			}
			return 1;
		case LISTENING:
			return -1;
		case CONNECTED:
			return -1;
		case CLOSEWAIT:
			return -1;
		default: 
			return -1;
	}

}

// Receive data from a srt client. 
// This function keeps polling the receive buffer every RECVBUF_POLLING_INTERVAL
// until the requested data is available, then it stores the data and returns 1
// If the function fails, return -1 
int srt_server_recv(int sockfd, void* buf, unsigned int length) {
	svr_tcb_t* servertcb;
	servertcb = tcbtable_gettcb(sockfd);
	if(!servertcb)
		return -1;
	
	switch(servertcb->state) {
		case CLOSED:
			return -1;
		case LISTENING:
			return -1;
		case CONNECTED:
			//continuously poll receive buffer to see if there is enough data 
			while(1) {
				if(servertcb->usedBufLen<length) {
					sleep(RECVBUF_POLLING_INTERVAL);
				}
				else {
					pthread_mutex_lock(servertcb->bufMutex);
					char* dest = (char*) buf;
					memcpy(dest,servertcb->recvBuf,length);
					memcpy(servertcb->recvBuf, servertcb->recvBuf+length,servertcb->usedBufLen-length);
					servertcb->usedBufLen = servertcb->usedBufLen-length;	
					pthread_mutex_unlock(servertcb->bufMutex);
					break;
				}
			}
			return 1;
		case CLOSEWAIT:
			return -1;
		default: 
			return -1;
	}


}

// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1 
// if fails (i.e., in the wrong state).
int srt_server_close(int sockfd) {
	//get tcb indexed by sockfd
	svr_tcb_t* servertcb;
	servertcb = tcbtable_gettcb(sockfd);
	if(!servertcb)
		return -1;

	switch(servertcb->state) {
		case CLOSED:
			free(servertcb->bufMutex);
			free(servertcb->recvBuf);
			free(tcbtable[sockfd]);
			tcbtable[sockfd]=NULL;
			return 1;
		case LISTENING:
			return -1;
		case CONNECTED:
			return -1;
		case CLOSEWAIT:
			return -1;
		default: 
			return -1;
	}
}

// This is a thread  started by srt_server_init(). It handles all the incoming 
// segments from the client. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the connection to the SNP process is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
void* seghandler(void* arg) {
	seg_t segBuf;
	svr_tcb_t* my_servertcb;
	int src_nodeID;

	while(1) {
		//receive a segment
		if(snp_recvseg(network_conn, &src_nodeID, &segBuf)<0) {
			close(network_conn);
			pthread_exit(NULL);
		}
		//find the tcb to handle the segment
		my_servertcb = tcbtable_gettcbFromPort(segBuf.header.dest_port);
		if(!my_servertcb) {
			printf("SERVER: NO PORT FOR RECEIVED SEGMENT\n");
			continue;
		}
		
		//segment handling
		switch(my_servertcb->state) {
			case CLOSED:
				break;
			case LISTENING:
				//waiting for SYN segment from client
				if(segBuf.header.type==SYN) {
					// SYN received
					printf("SERVER: SYN RECEIVED\n");
					//update servertcb and send SYNACK back
					my_servertcb->client_nodeID = src_nodeID;
					my_servertcb->client_portNum = segBuf.header.src_port;
					syn_received(my_servertcb,&segBuf);
					//state transition
					my_servertcb->state=CONNECTED;
					printf("SERVER: CONNECTED\n");
				}
				else
					printf("SERVER: IN LISTENING, NON SYN SEG RECEIVED\n");
				break;
			case CONNECTED:	
				if(segBuf.header.type==SYN&&my_servertcb->client_portNum==segBuf.header.src_port&&my_servertcb->client_nodeID==src_nodeID) {
					// SYN received
					printf("SERVER: DUPLICATE SYN RECEIVED\n");
					//update servertcb
					syn_received(my_servertcb,&segBuf);
				}
				else if(segBuf.header.type==DATA&&my_servertcb->client_portNum==segBuf.header.src_port&&my_servertcb->client_nodeID==src_nodeID) {
					data_received(my_servertcb,&segBuf);
				}
				else if(segBuf.header.type==FIN&&my_servertcb->client_portNum==segBuf.header.src_port&&my_servertcb->client_nodeID==src_nodeID) {
					//state transition
					printf("SERVER: FIN RECEIVED\n");
		 			my_servertcb->state = CLOSEWAIT;	
					printf("SERVER: CLOSEWAIT\n");
					//start a closewait timer
					pthread_t cwtimer;
					pthread_create(&cwtimer,NULL,closewait, (void*)my_servertcb);
					//send FINACK back
					fin_received(my_servertcb,&segBuf);
				}		
				break;
			case CLOSEWAIT:
				if(segBuf.header.type==FIN&&my_servertcb->client_portNum==segBuf.header.src_port&&my_servertcb->client_nodeID==src_nodeID) {
					printf("SERVER: DUPLICATE FIN RECEIVED\n");
					//send FINACK back
					fin_received(my_servertcb,&segBuf);
				}
				else
					printf("SERVER: IN CLOSEWAIT, NON FIN SEG RECEIVED\n");
				break;
		}
	}
}




/**********************************************/
//
//some other help functions
//
/**********************************************/

//this is for closewait timer implementation
//when a closewait timer is started, this thread is started
//it waits for CLOSEWAIT_TIMEOUT
//and transitions the state to CLOSED state
void* closewait(void* servertcb) {
	svr_tcb_t* my_servertcb = (svr_tcb_t*)servertcb;
	sleep(CLOSEWAIT_TIMEOUT);

	//timerout, state transitions to CLOSED
	pthread_mutex_lock(my_servertcb->bufMutex);
	my_servertcb->usedBufLen= 0;
	pthread_mutex_unlock(my_servertcb->bufMutex);
	my_servertcb->state = CLOSED;
	printf("SERVER: CLOSED\n");
	pthread_exit(NULL);
}

//this function handles SYN segment
//it updates expect_seqNum and send a SYNACK back
void syn_received(svr_tcb_t* svrtcb, seg_t* syn) {
	//update expected sequence
	svrtcb->expect_seqNum = syn->header.seq_num;
	//send SYNACK back
	seg_t synack;
	bzero(&synack,sizeof(synack));
	synack.header.type=SYNACK;
	synack.header.src_port = svrtcb->svr_portNum;
	synack.header.dest_port = svrtcb->client_portNum;
	synack.header.length = 0;
	snp_sendseg(network_conn,svrtcb->client_nodeID,&synack);
	printf("SERVER: SYNACK SENT,%d,%d\n",synack.header.src_port,synack.header.dest_port);
}

//This function handles DATA segment
//if it's expected DATA segment,
//extract the data and save data to send buffer and update expect_seqNum
//wheather its expected DATA segment, send DATAACK back with new or old expect_seqNum
void data_received(svr_tcb_t* svrtcb, seg_t* data) {
	if(data->header.seq_num == svrtcb->expect_seqNum) {
		//save data into receive buffer, update expect sequence number
		if(savedata(svrtcb,data)<0)
			return;
	}
	//send DATAACK back
	seg_t dataack;
	bzero(&dataack,sizeof(dataack));
	dataack.header.type = DATAACK;
	dataack.header.src_port = svrtcb->svr_portNum;
	dataack.header.dest_port = svrtcb->client_portNum;
	dataack.header.ack_num = svrtcb->expect_seqNum;
	dataack.header.length = 0;
	snp_sendseg(network_conn,svrtcb->client_nodeID,&dataack);
}

//This function handles FIN segment by sending a FINACK back 
void fin_received(svr_tcb_t* svrtcb, seg_t* fin) {
	seg_t finack;
	bzero(&finack,sizeof(finack));
	finack.header.type=FINACK;
	finack.header.src_port = svrtcb->svr_portNum;
	finack.header.dest_port = svrtcb->client_portNum;
	finack.header.length = 0;
	snp_sendseg(network_conn,svrtcb->client_nodeID,&finack);
	printf("SERVER: FINACK SENT\n");
}



//save received data to receive buffer and update the corresponding tcb fields
//it is called by data_received when a DATA segment with expect sequence number is received
int savedata(svr_tcb_t* svrtcb, seg_t* segment) {
	if(segment->header.length + svrtcb->usedBufLen < RECEIVE_BUF_SIZE) {
		pthread_mutex_lock(svrtcb->bufMutex);
		memcpy(&svrtcb->recvBuf[svrtcb->usedBufLen],segment->data,segment->header.length);
		svrtcb->usedBufLen= svrtcb->usedBufLen + segment->header.length;
		svrtcb->expect_seqNum = segment->header.length+segment->header.seq_num;
		pthread_mutex_unlock(svrtcb->bufMutex);
		return 1;
	}
	else return -1;
}

