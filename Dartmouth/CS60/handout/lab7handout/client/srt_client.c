//FILE: client/srt_client.c
//
//Description: this file contains the SRT client interface implementation 
//
//Date: April 18,2008

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <assert.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "../topology/topology.h"
#include "srt_client.h"
#include "../common/seg.h"

//declare tcbtable as global variable
client_tcb_t* tcbtable[MAX_TRANSPORT_CONNECTIONS];
//declare the TCP connection to the SNP process as global variable
int network_conn;


/*********************************************************************/
//
//help functions for tcbtable operations
//
/*********************************************************************/

//get the sock tcb indexed by sockfd
//return 0 if no tcb found
client_tcb_t* tcbtable_gettcb(int sockfd) {
       	if(tcbtable[sockfd]!=NULL)
		return tcbtable[sockfd];
	else
		return 0;
}

//get the sock tcb from the given client port number 
//return 0 if no tcb found 
client_tcb_t* tcbtable_gettcbFromPort(unsigned int clientPort)
{
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable[i]!=NULL && tcbtable[i]->client_portNum==clientPort) {
			return tcbtable[i];
		}
	}
	return 0;
}

//get a new tcb from tcbtable
//assign the client port with the given port number
//return the index of the new tcb
//return -1 if the all tcbs in tcbtable are used or the given port number is used
int tcbtable_newtcb(unsigned int port) {
	int i;

	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable[i]!=NULL&&tcbtable[i]->client_portNum==port) {
			return -1;
		}
	}

	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable[i]==NULL) {
			tcbtable[i] = (client_tcb_t*) malloc(sizeof(client_tcb_t));
			tcbtable[i]->client_portNum = port;
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
// handle the incoming segments. There is only one seghandler for the client side which
// handles call connections for the client.
void srt_client_init(int conn) {
	//initialize global variables
        int i;
        for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		tcbtable[i] = NULL;
	}
	network_conn = conn;

	//create the seghandler 
	pthread_t seghandler_thread;
	pthread_create(&seghandler_thread,NULL,seghandler, (void*)0);
}


// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized 
// e.g., TCB state is set to CLOSED and the client port set to the function call parameter 
// client port.  The TCB table entry index should be returned as the new socket ID to the client 
// and be used to identify the connection on the client side. If no entry in the TC table  
// is available the function returns -1.
int srt_client_sock(unsigned int client_port) {
	// get a new tcb
	int sockfd = tcbtable_newtcb(client_port);
        if(sockfd<0)
                return -1;

	client_tcb_t* my_clienttcb = tcbtable_gettcb(sockfd);

	//initialize tcb
	my_clienttcb->client_nodeID = topology_getMyNodeID();
	my_clienttcb->svr_nodeID = -1;
	my_clienttcb->next_seqNum = 0;
	my_clienttcb->state = CLOSED;	
	my_clienttcb->sendBufHead = 0;
	my_clienttcb->sendBufunSent = 0;
	my_clienttcb->sendBufTail = 0;
	my_clienttcb->unAck_segNum = 0;
	//create the mutex for send buffer
	pthread_mutex_t* sendBuf_mutex;
	sendBuf_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	assert(sendBuf_mutex!=NULL);
	pthread_mutex_init(sendBuf_mutex,NULL);
	my_clienttcb->bufMutex = sendBuf_mutex;

	return sockfd;

}

// This function is used to connect to the server. It takes the socket ID and the 
// server's port number as input parameters. The socket ID is used to find the TCB entry.  
// This function sets up the TCB's server port number and a SYN segment to send to
// the server using snp_sendseg(). After the SYN segment is sent, a timer is started. 
// If no SYNACK is received after SYNSEG_TIMEOUT timeout, then the SYN is 
// retransmitted. If SYNACK is received, return 1. Otherwise, if the number of SYNs 
// sent > SYN_MAX_RETRY,  transition to CLOSED state and return -1.
int srt_client_connect(int sockfd, int nodeID, unsigned int server_port) {
	//get tcb indexed by sockfd
	client_tcb_t* clienttcb;
        clienttcb = tcbtable_gettcb(sockfd);
        if(!clienttcb)
                return -1;
	switch(clienttcb->state) {
		case CLOSED:
			//assigned the given server port
			clienttcb->svr_portNum = server_port;
			clienttcb->svr_nodeID = nodeID;
			//send SYN to server
			seg_t syn;
			bzero(&syn,sizeof(syn));
			syn.header.type = SYN;
			syn.header.src_port = clienttcb->client_portNum;
			syn.header.dest_port = clienttcb->svr_portNum;
			syn.header.seq_num = 0;
			syn.header.length = 0;
			snp_sendseg(network_conn, clienttcb->svr_nodeID, &syn);	
			printf("CLIENT: SYN SENT\n");
	
			//state transition
			clienttcb->state = SYNSENT;
	
			//retry in case  timeout
			int retry = SYN_MAX_RETRY;
			while(retry>0) {
				select(0,0,0,0,&(struct timeval){.tv_usec = SYN_TIMEOUT/1000});
				if(clienttcb->state == CONNECTED) {
					return 1;
				}
				else { 
					snp_sendseg(network_conn, clienttcb->svr_nodeID, &syn);	
					retry--;
				}
			}	
			//state transition
			clienttcb->state = CLOSED;
			return -1;		
		case SYNSENT:
			return -1;
		case CONNECTED:
			return -1;
		case FINWAIT:
			return -1;
		default:
			return -1;
	}
}

// Send data to a srt server. This function should use the socket ID to find the TCP entry. 
// Then It should create segBufs using the given data and append them to send buffer linked list. 
// If the send buffer was empty before insertion, a thread called sendbuf_timer 
// should be started to poll the send buffer every SENDBUF_POLLING_INTERVAL time
// to check if a timeout event should occur. If the function completes successfully, 
// it returns 1. Otherwise, it returns -1.
int srt_client_send(int sockfd, void* data, unsigned int length) {
	//get tcb indexed by sockfd
	client_tcb_t* clienttcb;
        clienttcb = tcbtable_gettcb(sockfd);
        if(!clienttcb)
                return -1;

	int segNum;
	int i;
	switch(clienttcb->state) {
		case CLOSED:
			return -1;		
		case SYNSENT:
			return -1;
		case CONNECTED:
			//create segments using the given data
			segNum = length/MAX_SEG_LEN;
			if(length%MAX_SEG_LEN)
			segNum++;
	
			for(i=0;i<segNum;i++) {
				segBuf_t* newBuf = (segBuf_t*) malloc(sizeof(segBuf_t));	
				assert(newBuf!=NULL);
				bzero(newBuf,sizeof(segBuf_t));
				newBuf->seg.header.src_port = clienttcb->client_portNum;
				newBuf->seg.header.dest_port = clienttcb->svr_portNum;
				if(length%MAX_SEG_LEN!=0 && i==segNum-1)
					newBuf->seg.header.length = length%MAX_SEG_LEN;
				else
					newBuf->seg.header.length = MAX_SEG_LEN;
				newBuf->seg.header.type = DATA;
				char* datatosend = (char*)data;
				memcpy(newBuf->seg.data,&datatosend[i*MAX_SEG_LEN],newBuf->seg.header.length);
				sendBuf_addSeg(clienttcb,newBuf);
			}

			//send segments until unACKed segments reaches GBN_WINDOW,sendBuf_timer is also started if no sendBuf_timer is already running
			sendBuf_send(clienttcb);
			return 1;
		case FINWAIT:
			return -1;
		default:
			return -1;
	}

}

// This function is used to disconnect from the server. It takes the socket ID as 
// an input parameter. The socket ID is used to find the TCB entry in the TCB table.  
// This function sends a FIN segment to the server. After the FIN segment is sent
// the state should transition to FINWAIT and a timer started. If the 
// state == CLOSED after the timeout the FINACK was successfully received. Else,
// if after a number of retries FIN_MAX_RETRY the state is still FINWAIT then
// the state transitions to CLOSED and -1 is returned.
//disconnect from a srt server
int srt_client_disconnect(int sockfd) {
	//get the tcb indexed by sockfd
	client_tcb_t* clienttcb;
        clienttcb = tcbtable_gettcb(sockfd);
        if(!clienttcb)
                return -1;

	seg_t fin;

	switch(clienttcb->state) {
		case CLOSED:
			return -1;
		case SYNSENT:
			return -1;
		case CONNECTED:
			//send fin
			bzero(&fin,sizeof(fin));
			fin.header.type = FIN;
			fin.header.src_port = clienttcb->client_portNum;
			fin.header.dest_port = clienttcb->svr_portNum;
			fin.header.length = 0;
			snp_sendseg(network_conn, clienttcb->svr_nodeID, &fin);
			printf("CLIENT: FIN SENT\n");
			//state transition
			clienttcb->state = FINWAIT;
			printf("CLIENT: FINWAIT\n");
	
			//resend in case of timeout
			int retry = FIN_MAX_RETRY;
			while(retry>0) {
				select(0,0,0,0,&(struct timeval){.tv_usec = FIN_TIMEOUT/1000});
				if(clienttcb->state == CLOSED) {
					clienttcb->svr_nodeID = -1;
					clienttcb->svr_portNum = 0;
					clienttcb->next_seqNum = 0;
					sendBuf_clear(clienttcb);
					return 1;
				}
				else {
					printf("CLIENT: FIN RESENT\n");
					snp_sendseg(network_conn, clienttcb->svr_nodeID, &fin);
					retry--;
				}	
			}
			clienttcb->state = CLOSED;
			return -1;
		case FINWAIT:
			return -1;
		default:
			return -1;
	}
	

}

// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1 
// if fails (i.e., in the wrong state).
int srt_client_close(int sockfd) {
	//get tcb indexed by sockfd
	client_tcb_t* clienttcb;
        clienttcb = tcbtable_gettcb(sockfd);
        if(!clienttcb)
                return -1;

	switch(clienttcb->state) {
		case CLOSED:
			free(clienttcb->bufMutex);
			free(tcbtable[sockfd]);
			tcbtable[sockfd]=NULL;
			return 1;
		case SYNSENT:
			return -1;
		case CONNECTED:
			return -1;
		case FINWAIT:
			return -1;
		default:
			return -1;
	}
}

// This is a thread  started by srt_client_init(). It handles all the incoming 
// segments from the server. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the connection to the SNP process is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
void* seghandler(void* arg) {
	seg_t segBuf;
	int src_nodeID;
	while(1) {
		//receive a segment
		if(snp_recvseg(network_conn,&src_nodeID, &segBuf)<0) {
			close(network_conn);
			pthread_exit(NULL);
		}

		//find the tcb to handle the segment
		client_tcb_t* my_clienttcb = tcbtable_gettcbFromPort(segBuf.header.dest_port);
		if(!my_clienttcb) {
			printf("CLIENT: NO PORT FOR RECEIVED SEGMENT\n");
			continue;
		}

		//segment handling
		switch(my_clienttcb->state) {
			case CLOSED:
				break;
			case SYNSENT:
				if(segBuf.header.type==SYNACK&&my_clienttcb->svr_portNum==segBuf.header.src_port&&my_clienttcb->svr_nodeID==src_nodeID) {
					printf("CLIENT: SYNACK RECEIVED\n");
					my_clienttcb->state = CONNECTED;
					printf("CLIENT: CONNECTED\n");
				}
				else
					printf("CLIENT: IN SYNSENT, NON SYNACK SEG RECEIVED\n");
				break;
			case CONNECTED:	
				if(segBuf.header.type==DATAACK&&my_clienttcb->svr_portNum==segBuf.header.src_port&&my_clienttcb->svr_nodeID==src_nodeID) {
					if(my_clienttcb->sendBufHead!=NULL&&segBuf.header.ack_num >= my_clienttcb->sendBufHead->seg.header.seq_num) {
						//received ack, update send buffer
						sendBuf_recvAck(my_clienttcb, segBuf.header.ack_num);
						//send new segments in send buffer
						sendBuf_send(my_clienttcb);
					}
				}
				else {
					printf("CLIENT: IN CONNECTED, NON DATAACK SEG RECEIVED\n");
				}
				break;
			case FINWAIT:
				if(segBuf.header.type==FINACK&&my_clienttcb->svr_portNum==segBuf.header.src_port&&my_clienttcb->svr_nodeID==src_nodeID) {
					printf("CLIENT: FINACK RECEIVED\n");
					my_clienttcb->state = CLOSED;
					printf("CLIENT: CLOSED\n");
				}
				else
					printf("CLIENT: IN FINWAIT, NON FINACK SEG RECEIVED\n");
				break;
		}
	}
}

/*******************************************************/
//
// help functions: send buffer operations 
//
/*******************************************************/

//add a segBuf to send buffer
//initialize the necessary fields in segBuf
//and add segBuf in to the send buffer linked list
void sendBuf_addSeg(client_tcb_t* clienttcb, segBuf_t* newSegBuf) {
	pthread_mutex_lock(clienttcb->bufMutex);
	if(clienttcb->sendBufHead==0) {
		newSegBuf->seg.header.seq_num = clienttcb->next_seqNum;
		clienttcb->next_seqNum += newSegBuf->seg.header.length;
		newSegBuf->sentTime = 0;
		clienttcb->sendBufHead= newSegBuf;
		clienttcb->sendBufunSent = newSegBuf;
		clienttcb->sendBufTail = newSegBuf;
	} else {
	//	newSegBuf->seg.header.seq_num = clienttcb->sendBufTail->seg.header.seq_num+clienttcb->sendBufTail->seg.header.length;	
		newSegBuf->seg.header.seq_num = clienttcb->next_seqNum;
		clienttcb->next_seqNum += newSegBuf->seg.header.length;
		newSegBuf->sentTime = 0;
		clienttcb->sendBufTail->next = newSegBuf;
		clienttcb->sendBufTail = newSegBuf;
		if(clienttcb->sendBufunSent == 0)
			clienttcb->sendBufunSent = newSegBuf;
	}
	pthread_mutex_unlock(clienttcb->bufMutex);
}

//send segments in send buffer until sent-but-unAcked segments reaches GBN_WINDOW 
//sendBuf_timer is started if needed  
void sendBuf_send(client_tcb_t* clienttcb) {
	pthread_mutex_lock(clienttcb->bufMutex);
	
	while(clienttcb->unAck_segNum<GBN_WINDOW && clienttcb->sendBufunSent!=0) {
		snp_sendseg(network_conn, clienttcb->svr_nodeID, (seg_t*)clienttcb->sendBufunSent);
		struct timeval currentTime;
		gettimeofday(&currentTime,NULL);
		clienttcb->sendBufunSent->sentTime = currentTime.tv_sec*1000+ currentTime.tv_usec;
		//segBuf_timer should be started after sending out the first Data segment	
		if(clienttcb->unAck_segNum ==0) {
			pthread_t timer;
			pthread_create(&timer,NULL,sendBuf_timer, (void*)clienttcb);
		}
		clienttcb->unAck_segNum++;
		

		if(clienttcb->sendBufunSent != clienttcb->sendBufTail)
			clienttcb->sendBufunSent= clienttcb->sendBufunSent->next;
		else
			clienttcb->sendBufunSent = 0; 
	}
	pthread_mutex_unlock(clienttcb->bufMutex);
}

//this function is called when timeout event occurs
//resend all sent-but-unAcked segments in send buffer
void sendBuf_timeout(client_tcb_t* clienttcb) {
	pthread_mutex_lock(clienttcb->bufMutex);
	segBuf_t* bufPtr=clienttcb->sendBufHead;
	int i;
	for(i=0;i<clienttcb->unAck_segNum;i++) {
		snp_sendseg(network_conn, clienttcb->svr_nodeID, (seg_t*)bufPtr);
		struct timeval currentTime;
		gettimeofday(&currentTime,NULL);
		bufPtr->sentTime = currentTime.tv_sec*1000000+ currentTime.tv_usec;
		bufPtr = bufPtr->next; 
	}
	pthread_mutex_unlock(clienttcb->bufMutex);

}

//this function is called when a DATAACK is received ack received 
//update send buffer pointers in clinet tcb structure and free all the acked segBufs
void sendBuf_recvAck(client_tcb_t* clienttcb, unsigned int ack_seqnum) {
	pthread_mutex_lock(clienttcb->bufMutex);

	//if all segments are Acked	
	if(ack_seqnum>clienttcb->sendBufTail->seg.header.seq_num)
		clienttcb->sendBufTail = 0;

	segBuf_t* bufPtr= clienttcb->sendBufHead;
	while(bufPtr && bufPtr->seg.header.seq_num<ack_seqnum) {
		clienttcb->sendBufHead = bufPtr->next;
			segBuf_t* temp = bufPtr;
		bufPtr = bufPtr->next;
		free(temp);
		clienttcb->unAck_segNum--;
	}
	pthread_mutex_unlock(clienttcb->bufMutex);
}

//delete clienttcb's send buffer linked list
//and clear the send buffer pointers in clienttcb
//this function is called when clienttcb transitions to CLOSED state
void sendBuf_clear(client_tcb_t* clienttcb) {
	pthread_mutex_lock(clienttcb->bufMutex);
    	segBuf_t* bufPtr = clienttcb->sendBufHead;
	while(bufPtr!=clienttcb->sendBufTail) {
		segBuf_t* temp = bufPtr;
		bufPtr = bufPtr->next;
		free(temp);	
	}
	free(clienttcb->sendBufTail);
	clienttcb->sendBufunSent = 0;
	clienttcb->sendBufHead = 0;
  	clienttcb->sendBufTail = 0;
	clienttcb->unAck_segNum = 0;
	pthread_mutex_unlock(clienttcb->bufMutex);	
}

//thread that continuously polls send buffer to trigger timeout events
//if the first sent-but-unAcked segment times out, call sendBuf_timeout()
void* sendBuf_timer(void* clienttcb) {
	client_tcb_t* my_clienttcb = (client_tcb_t*) clienttcb;
	while(1) {
		select(0,0,0,0,&(struct timeval){.tv_usec = SENDBUF_POLLING_INTERVAL/1000});

		struct timeval currentTime;
		gettimeofday(&currentTime,NULL);
		
		//if unAck_segNum is 0, means no segments left in the send buffer, exit
		if(my_clienttcb->unAck_segNum == 0) {
			pthread_exit(NULL);
		}
		else if(my_clienttcb->sendBufHead->sentTime>0 && my_clienttcb->sendBufHead->sentTime<currentTime.tv_sec*1000000+currentTime.tv_usec-DATA_TIMEOUT) {
			sendBuf_timeout(my_clienttcb);
		}
	}
}

