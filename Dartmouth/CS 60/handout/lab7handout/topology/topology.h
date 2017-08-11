//FILE: topology/topology.h
//
//Description: this file declares some help functions used to parse the topology file 
//
//Date: April 29,2008


#ifndef TOPOLOGY_H 
#define TOPOLOGY_H
#include <netdb.h>

//this function returns node ID of the given hostname
//the node ID is an integer of the last 8 digit of the node's IP address
//for example, a node with IP address 202.120.92.3 will have node ID 3
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromname(char* hostname); 

//this function returns node ID from the given IP address
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromip(struct in_addr* addr);

//this function returns my node ID
//if my node ID can't be retrieved, return -1
int topology_getMyNodeID();

//this functions parses the topology information stored in topology.dat
//returns the number of neighbors
int topology_getNbrNum(); 

//this functions parses the topology information stored in topology.dat
//returns the number of total nodes in the overlay 
int topology_getNodeNum();

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the nodes' IDs in the overlay network  
int* topology_getNodeArray(); 

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the neighbors'IDs  
int* topology_getNbrArray(); 

//this functions parses the topology information stored in topology.dat
//returns the cost of the direct link between the two given nodes 
//if no direct link between the two given nodes, INFINITE_COST is returned
unsigned int topology_getCost(int fromNodeID, int toNodeID);
#endif
