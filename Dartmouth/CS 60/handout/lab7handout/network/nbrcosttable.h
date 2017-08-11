//FILE: network/nbrcosttable.h
//
//Description: this file defines the data structures and operations for neighbor cost table.  
//
//Date: April 28,2008

#ifndef NBRCOSTTABLE_H 
#define NBRCOSTTABLE_H 
#include <arpa/inet.h>

//neighbor cost table entry definition
typedef struct neighborcostentry {
	unsigned int nodeID;	//neighbor's node ID
	unsigned int cost;	//direct link cost to the neighbor
} nbr_cost_entry_t;

//This function creates a neighbor cost table dynamically 
//and initialize the table with all its neighbors' node IDs and direct link costs.
//The neighbors' node IDs and direct link costs are retrieved from topology.dat file. 
nbr_cost_entry_t* nbrcosttable_create();

//This function destroys a neighbor cost table. 
//It frees all the dynamically allocated memory for the neighbor cost table.
void nbrcosttable_destroy(nbr_cost_entry_t* nct);

//This function is used to get the direct link cost from neighbor.
//The direct link cost is returned if the neighbor is found in the table.
//INFINITE_COST is returned if the node is not found in the table.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID);

//This function prints out the contents of a neighbor cost table.
void nbrcosttable_print(nbr_cost_entry_t* nct); 

#endif
