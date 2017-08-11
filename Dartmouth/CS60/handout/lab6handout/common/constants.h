//FILE: common/constants.h

//Description: this file contains constants

//Date: April 29,2008

#ifndef CONSTANTS_H
#define CONSTANTS_H

/*******************************************************************/
//overlay parameters
/*******************************************************************/

//this is the port number that is used for nodes to interconnect each other to form an overlay, you should change this to a random value to avoid conflicts with other students
#define CONNECTION_PORT 3000

//this is the port number that is opened by overlay process and connected by the network layer process, you should change this to a random value to avoid conflicts with other students
#define OVERLAY_PORT 3500

//max packet data length
#define MAX_PKT_LEN 1488 


/*******************************************************************/
//network layer parameters
/*******************************************************************/
//max node number supported by the overlay network 
#define MAX_NODE_NUM 10

//infinite link cost value, if two nodes are disconnected, they will have link cost INFINITE_COST
#define INFINITE_COST 999

//the network layer process opens this port, and waits for connection from transport layer process, you should change this to a random value to avoid conflicts with other students
#define NETWORK_PORT 4002

//this is the broadcasting nodeID. If the overlay layer process receives a packet destined to BROADCAST_NODEID from the network layer process, it should send this packet to all the neighbors
#define BROADCAST_NODEID 9999

//route update broadcasting interval in seconds
#define ROUTEUPDATE_INTERVAL 5
#endif
