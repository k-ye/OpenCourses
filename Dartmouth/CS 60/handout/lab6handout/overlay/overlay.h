//FILE: overlay/overlay.h
//
//Description: this file defines the data structures and functions used by ON process 
//
//Date: April 28,2008

#ifndef OVERLAY_H 
#define OVERLAY_H
#include "../common/constants.h"
#include "../common/pkt.h"
#include "neighbortable.h"

// This thread opens a TCP port on CONNECTION_PORT and waits for the incoming connection from all the neighbors that have a larger node ID than my nodeID,
// After all the incoming connections are established, this thread terminates 
void* waitNbrs();

// This function connects to all the neighbors that have a smaller node ID than my nodeID
// After all the outgoing connections are established, return 1, otherwise return -1
int connectNbrs();


//This function opens a TCP port on OVERLAY_PORT, and waits for the incoming connection from local SNP process. After the local SNP process is connected, this function keeps getting send_arg_t structures from SNP process, and sends the packets to the next hop in the overlay network.
void waitNetwork();

//Each listen_to_neighbor thread keeps receiving packets from a neighbor. It handles the received packets by forwarding the packets to the SNP process.
//all listen_to_neighbor threads are started after all the TCP connections to the neighbors are established 
void* listen_to_neighbor(void* arg);

//this function stops the overlay
//it closes all the connections and frees all the dynamically allocated memory
//it is called when receiving a signal SIGINT
void overlay_stop(); 

#endif
