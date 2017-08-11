//
// FILE: srt_client.c
//
// Description: this file contains client states' definition, some important data structures
// and the client SRT socket interface definitions. You need to implement all these interfaces
//
// Date: April 18, 2008
//       April 21, 2008 **Added more detailed description of prototypes fixed ambiguities** ATC
//       April 26, 2008 ** Added GBN and send buffer function descriptions **
//

#include "srt_client.h"

//
//
//  SRT socket API for the client side application. 
//  ===================================
//
//  In what follows, we provide the prototype definition for each call and limited pseudo code representation
//  of the function. This is not meant to be comprehensive - more a guideline. 
// 
//  You are free to design the code as you wish.
//
//  NOTE: When designing all functions you should consider all possible states of the FSM using
//  a switch statement (see the Lab4 assignment for an example). Typically, the FSM has to be
// in a certain state determined by the design of the FSM to carry out a certain action. 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

// This function initializes the TCB table marking all entries NULL. It also initializes 
// a global variable for the overlay TCP socket descriptor ``conn'' used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to 
// handle the incoming segments. There is only one seghandler for the client side which
// handles call connections for the client.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
void srt_client_init(int conn)
{
  return;
}


// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized 
// e.g., TCB state is set to CLOSED and the client port set to the function call parameter 
// client port.  The TCB table entry index should be returned as the new socket ID to the client 
// and be used to identify the connection on the client side. If no entry in the TC table  
// is available the function returns -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_sock(unsigned int client_port)
{
  return 0;
}


// This function is used to connect to the server. It takes the socket ID and the 
// server's port number as input parameters. The socket ID is used to find the TCB entry.  
// This function sets up the TCB's server port number and a SYN segment to send to
// the server using snp_sendseg(). After the SYN segment is sent, a timer is started. 
// If no SYNACK is received after SYNSEG_TIMEOUT timeout, then the SYN is 
// retransmitted. If SYNACK is received, return 1. Otherwise, if the number of SYNs 
// sent > SYN_MAX_RETRY,  transition to CLOSED state and return -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_connect(int socked, unsigned int server_port)
{
  return 0;
}


// Send data to a srt server. This function should use the socket ID to find the TCP entry. 
// Then It should create segBufs using the given data and append them to send buffer linked list. 
// If the send buffer was empty before insertion, a thread called sendbuf_timer 
// should be started to poll the send buffer every SENDBUF_POLLING_INTERVAL time
// to check if a timeout event should occur. If the function completes successfully, 
// it returns 1. Otherwise, it returns -1.
// 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_send(int sockfd, void* data, unsigned int length)
{
  return 0;
}


// This function is used to disconnect from the server. It takes the socket ID as 
// an input parameter. The socket ID is used to find the TCB entry in the TCB table.  
// This function sends a FIN segment to the server. After the FIN segment is sent
// the state should transition to FINWAIT and a timer started. If the 
// state == CLOSED after the timeout the FINACK was successfully received. Else,
// if after a number of retries FIN_MAX_RETRY the state is still FINWAIT then
// the state transitions to CLOSED and -1 is returned.


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_disconnect(int sockfd)
{
  return 0;
}


// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1 
// if fails (i.e., in the wrong state).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_close(int sockfd)
{
  return 0;
}


// This is a thread  started by srt_client_init(). It handles all the incoming 
// segments from the server. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the overlay connection is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void *seghandler(void* arg)
{
  return 0;
}



// This thread continuously polls send buffer to trigger timeout events
// It should always be running when the send buffer is not empty
// If the current time -  first sent-but-unAcked segment's sent time > DATA_TIMEOUT, a timeout event occurs
// When timeout, resend all sent-but-unAcked segments
// When the send buffer is empty, this thread terminates
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void* sendBuf_timer(void* clienttcb)
{
  return 0;
}
