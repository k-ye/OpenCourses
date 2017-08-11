//FILE: overlay/neighbortable.h
//
//Description: this file defines the data structures and APIs for neighbor table
//
//Date: April 28,2008

#ifndef NEIGHBORTABLE_H 
#define NEIGHBORTABLE_H
#include <arpa/inet.h>

//neighbor table entry definition
//a neighbor table contains n entries where n is the number of neighbors
//Each Node has a Overlay Network (ON) process running, each ON process maintains the neighbor table for the node that the process is running on.

typedef struct neighborentry {
  int nodeID;	        //neighbor's node ID
  in_addr_t nodeIP;     //neighbor's IP address
  int conn;	        //TCP connection's socket descriptor to the neighbor
} nbr_entry_t;


//This function first creates a neighbor table dynamically. It then parses the topology/topology.dat file and fill the nodeID and nodeIP fields in all the entries, initialize conn field as -1 .
//return the created neighbor table
nbr_entry_t* nt_create();

//This function destroys a neighbortable. It closes all the connections and frees all the dynamically allocated memory.
void nt_destroy(nbr_entry_t* nt);

//This function is used to assign a TCP connection to a neighbor table entry for a neighboring node. If the TCP connection is successfully assigned, return 1, otherwise return -1
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn);

#endif
