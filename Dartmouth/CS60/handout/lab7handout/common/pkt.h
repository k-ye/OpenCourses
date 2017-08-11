#ifndef PKT_H
#define PKT_H

#include "constants.h"



//packet type definition, used for type field in packet header
#define	ROUTE_UPDATE 1
#define SNP 2	

//SNP packet format definition
typedef struct snpheader {
  int src_nodeID;		          //source node ID
  int dest_nodeID;		        //destination node ID
  unsigned short int length;	//length of the data in the packet
  unsigned short int type;	  //type of the packet 
} snp_hdr_t;

typedef struct packet {
  snp_hdr_t header;
  char data[MAX_PKT_LEN];
} snp_pkt_t;


//route update packet definition
//for a route update packet, the route update information will be stored in the data field of a packet 

//a route update entry
typedef struct routeupdate_entry {
        unsigned int nodeID;	//destination nodeID
        unsigned int cost;	//link cost from the source node(src_nodeID in header) to destination node
} routeupdate_entry_t;

//route update packet format
typedef struct pktrt{
        unsigned int entryNum;	//number of entries contained in this route update packet
        routeupdate_entry_t entry[MAX_NODE_NUM];
} pkt_routeupdate_t;



// sendpkt_arg_t data structure is used in the overlay_sendpkt() function. 
// overlay_sendpkt() is called by the SNP process to request 
// the ON process to send a packet out to the overlay network. 
// 
// The ON process and SNP process are connected with a 
// local TCP connection, in overlay_sendpkt(), the SNP process 
// sends this data structure over this TCP connection to the ON process. 
// The ON process receives this data structure by calling getpktToSend().
// Then the ON process sends the packet out to the next hop by calling sendpkt().
typedef struct sendpktargument {
  int nextNodeID;    //node ID of the next hop
  snp_pkt_t pkt;         //the packet to be sent
} sendpkt_arg_t;


// overlay_sendpkt() is called by the SNP process to request the ON 
// process to send a packet out to the overlay network. The 
// ON process and SNP process are connected with a local TCP connection. 
// In overlay_sendpkt(), the packet and its next hop's nodeID are encapsulated 
// in a sendpkt_arg_t data structure and sent over this TCP connection to the ON process. 
// The parameter overlay_conn is the TCP connection's socket descriptior 
// between the SNP process and the ON process.
// When sending the sendpkt_arg_t data structure over the TCP connection between the SNP 
// process and the ON process, use '!&'  and '!#' as delimiters. 
// Send !& sendpkt_arg_t structure !# over the TCP connection.
// Return 1 if sendpkt_arg_t data structure is sent successfully, otherwise return -1.
int overlay_sendpkt(int nextNodeID, snp_pkt_t* pkt, int overlay_conn);




// overlay_recvpkt() function is called by the SNP process to receive a packet 
// from the ON process. The parameter overlay_conn is the TCP connection's socket 
// descriptior between the SNP process and the ON process. The packet is sent over 
// the TCP connection between the SNP process and the ON process, and delimiters 
// !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if a packet is received successfully, otherwise return -1.
int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn);



// This function is called by the ON process to receive a sendpkt_arg_t data structure.
// A packet and the next hop's nodeID is encapsulated  in the sendpkt_arg_t structure.
// The parameter network_conn is the TCP connection's socket descriptior between the
// SNP process and the ON process. The sendpkt_arg_t structure is sent over the TCP 
// connection between the SNP process and the ON process, and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data
// Return 1 if a sendpkt_arg_t structure is received successfully, otherwise return -1.
int getpktToSend(snp_pkt_t* pkt, int* nextNode,int network_conn);




// forwardpktToSNP() function is called after the ON process receives a packet from 
// a neighbor in the overlay network. The ON process calls this function 
// to forward the packet to SNP process. 
// The parameter network_conn is the TCP connection's socket descriptior between the SNP 
// process and ON process. The packet is sent over the TCP connection between the SNP process 
// and ON process, and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection.
// Return 1 if the packet is sent successfully, otherwise return -1.
int forwardpktToSNP(snp_pkt_t* pkt, int network_conn);



// sendpkt() function is called by the ON process to send a packet 
// received from the SNP process to the next hop.
// Parameter conn is the TCP connection's socket descritpor to the next hop node.
// The packet is sent over the TCP connection between the ON process and a neighboring node,
// and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection
// Return 1 if the packet is sent successfully, otherwise return -1.
int sendpkt(snp_pkt_t* pkt, int conn);



// recvpkt() function is called by the ON process to receive 
// a packet from a neighbor in the overlay network.
// Parameter conn is the TCP connection's socket descritpor to a neighbor.
// The packet is sent over the TCP connection  between the ON process and the neighbor,
// and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if the packet is received successfully, otherwise return -1.
int recvpkt(snp_pkt_t* pkt, int conn);



#endif
