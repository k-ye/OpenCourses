//FILE: network/routingtable.h
//
//Description: this file defines the data structures and functions for routing table. 
//A routing table is a hash table containing MAX_RT_ENTRY slot entries.  
//
//Date: April 29,2008



#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

//routingtable_entry_t is the routing entry contained in the routing table.
typedef struct routingtable_entry {
	int destNodeID;		//destination node ID
	int nextNodeID;		//next node ID to which the packet should be forwarded
	struct routingtable_entry* next;	//pointer to the next routingtable_entry_t in the same routing table slot
} routingtable_entry_t;

//A routing table is a hash table containing MAX_ROUTINGTABLE_SLOTS slots. Each slot is a linked list of routing entries.
typedef struct routingtable {
	routingtable_entry_t* hash[MAX_ROUTINGTABLE_SLOTS];
} routingtable_t;

//This is the hash function used the by the routing table
//It takes the hash key - destination node ID as input, 
//and returns the hash value - slot number for this destination node ID.
//
//You can copy makehash() implementation below directly to routingtable.c:
//int makehash(int node) {
//	return node%MAX_ROUTINGTABLE_ENTRIES;
//}
//
int makehash(int node); 

//This function creates a routing table dynamically.
//All the entries in the table are initialized to NULL pointers.
//Then for all the neighbors with a direct link, create a routing entry using the neighbor itself as the next hop node, and insert this routing entry into the routing table. 
//The dynamically created routing table structure is returned.
routingtable_t* routingtable_create();

//This funtion destroys a routing table. 
//All dynamically allocated data structures for this routing table are freed.
void routingtable_destroy(routingtable_t* routingtable);

//This function updates the routing table using the given destination node ID and next hop's node ID.
//If the routing entry for the given destination already exists, update the existing routing entry.
//If the routing entry of the given destination is not there, add one with the given next node ID.
//Each slot in routing table contains a linked list of routing entries due to conflicting hash keys (differnt hash keys (destination node ID here) may have same hash values (slot entry number here)).
//To add an routing entry to the hash table:
//First use the hash function makehash() to get the slot number in which this routing entry should be stored. 
//Then append the routing entry to the linked list in that slot.
void routingtable_setnextnode(routingtable_t* routingtable, int destNodeID, int nextNodeID);

//This function looks up the destNodeID in the routing table.
//Since routing table is a hash table, this opeartion has O(1) time complexity.
//To find a routing entry for a destination node, you should first use the hash function makehash() to get the slot number and then go through the linked list in that slot to search the routing entry.
//If the destNodeID is found, return the nextNodeID for this destination node.
//If the destNodeID is not found, return -1.
int routingtable_getnextnode(routingtable_t* routingtable, int destNodeID);

//This function prints out the contents of the routing table
void routingtable_print(routingtable_t* routingtable);

#endif
