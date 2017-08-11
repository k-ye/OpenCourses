//FILE: network/dvtable.h
//
//Description: this file defines the data structures and functions for distance vector table. 
//
//Date: April 29,2008



#ifndef DVTABLE_H
#define DVTABLE_H

#include "../common/pkt.h"

//dv_entry_t structure definition
typedef struct distancevectorentry {
	int nodeID;		//destnation nodeID	
	unsigned int cost;	//cost to the destination
} dv_entry_t;


//A distance vector table contains the n+1 dv_t entries, where n is the number of the neighbors of this node, and the rest one is for this node itself. 
typedef struct distancevector {
	int nodeID;		//source nodeID
	dv_entry_t* dvEntry;	//an array of N dv_entry_ts, each of which contains the destination node ID and the cost to the destination from the source node. N is the total number of nodes in the overlay.
} dv_t;


//This function creates a dvtable(distance vector table) dynamically.
//A distance vector table contains the n+1 entries, where n is the number of the neighbors of this node, and the rest one is for this node itself. 
//Each entry in distance vector table is a dv_t structure which contains a source node ID and an array of N dv_entry_t structures where N is the number of all the nodes in the overlay.
//Each dv_entry_t contains a destination node address the the cost from the source node to this destination node.
//The dvtable is initialized in this function.
//The link costs from this node to its neighbors are initialized using direct link cost retrived from topology.dat. 
//Other link costs are initialized to INFINITE_COST.
//The dynamically created dvtable is returned.
dv_t* dvtable_create();

//This function destroys a dvtable. 
//It frees all the dynamically allocated memory for the dvtable.
void dvtable_destroy(dv_t* dvtable);

//This function sets the link cost between two nodes in dvtable.
//If those two nodes are found in the table and the link cost is set, return 1.
//Otherwise, return -1.
int dvtable_setcost(dv_t* dvtable,int fromNodeID,int toNodeID, unsigned int cost);

//This function returns the link cost between two nodes in dvtable
//If those two nodes are found in dvtable, return the link cost. 
//otherwise, return INFINITE_COST.
unsigned int dvtable_getcost(dv_t* dvtable, int fromNodeID, int toNodeID);

//This function prints out the contents of a dvtable.
void dvtable_print(dv_t* dvtable);

#endif
