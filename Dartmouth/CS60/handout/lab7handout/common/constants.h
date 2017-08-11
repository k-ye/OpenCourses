//FILE: common/constants.h

//Description: this file contains constants used by srt protocol

//Date: April 29,2008

#ifndef CONSTANTS_H
#define CONSTANTS_H

/*******************************************************************/
//transport layer parameters
/*******************************************************************/

//this is the MAX connections can be supported by SRT. You TCB table should contain MAX_TRANSPORT_CONNECTIONS entries
#define MAX_TRANSPORT_CONNECTIONS 10
//Maximum segment length
//MAX_SEG_LEN = 1500 - sizeof(seg header) - sizeof(ip header)
//#define MAX_SEG_LEN  1464
#define MAX_SEG_LEN 200
//The packet loss rate is 10%
#define PKT_LOSS_RATE 0.1
//SYN_TIMEOUT value in nano seconds
#define SYN_TIMEOUT 500000000
//SYN_TIMEOUT value in nano seconds
#define FIN_TIMEOUT 500000000
//max number of SYN retransmissions in srt_client_connect()
#define SYN_MAX_RETRY 5
//max number of FIN retransmission in srt_client_disconnect()
#define FIN_MAX_RETRY 5
//server close wait timeout value in seconds
#define CLOSEWAIT_TIMEOUT 5
//sendBuf_timer thread's polling interval in nanoseconds
#define SENDBUF_POLLING_INTERVAL 500000000
//srt client polls the receive buffer with this time interval in order 
//to check if requested data is available in srt_srv_recv() function
//in seconds
#define RECVBUF_POLLING_INTERVAL 1
//srt_svr_accept() function uses this interval to busy wait on the tcb state
#define ACCEPT_POLLING_INTERVAL 500000000
//size of receive buffer
#define RECEIVE_BUF_SIZE 1000000
//DATA segment timeout value in microseconds
#define DATA_TIMEOUT 500000
//GBN window size
#define GBN_WINDOW 10

/*******************************************************************/
//overlay parameters
/*******************************************************************/

//this is port number that used for nodes to interconnect each other to form an overlay 
//you should change this to a random value to avoid conflictions with other students
#define CONNECTION_PORT 3022

//this is port number that opened by overlay process. network layer process should connect to the overlay process on this port  
//you should change this to a random value to avoid conflictions with other students
#define OVERLAY_PORT 3522

//max packet data length
#define MAX_PKT_LEN 1488 



/*******************************************************************/
//network layer parameters
/*******************************************************************/
//max node number support by the overlay  
#define MAX_NODE_NUM 10

//max routing table slots 
#define MAX_ROUTINGTABLE_SLOTS 10

//infinite link cost value
//if two nodes are unconnected, they will have link cost INFINITE_COST
#define INFINITE_COST 999

//network layer process opens this port, and waits for connection from transport layer process,
//you should change this to a random value to avoid conflictions with other students
#define NETWORK_PORT 4022

//this is the broadcasting nodeID address
#define BROADCAST_NODEID 9999

//route update broadcasting interval in seconds
#define ROUTEUPDATE_INTERVAL 5
#endif
