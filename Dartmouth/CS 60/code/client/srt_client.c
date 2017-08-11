#include "srt_client.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>     // memset
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>


/*interfaces to application layer*/

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
//  GOAL: Your goal for this assignment is to design and implement the 
//  protoypes below - fill the code.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// srt client initialization
//
// This function initializes the TCB table marking all entries NULL. It also initializes
// a global variable for the overlay TCP socket descriptor ‘‘conn’’ used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to
// handle the incoming segments. There is only one seghandler for the client side which
// handles call connections for the client.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

// transport control block for each socket
static client_tcb_t* tcb_list[MAX_TRANSPORT_CONNECTIONS];
// the actual tcp socket
static int overlay_conn;

static pthread_t seghandler_thread;

void srt_client_init(int conn) {
    for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
        tcb_list[i] = NULL;
    }
    overlay_conn = conn;

    int rc = pthread_create(&seghandler_thread, NULL, seghandler, NULL);
    assert(rc == 0);
}

// Create a client tcb, return the sock
//
// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized
// e.g., TCB state is set to CLOSED and the client port set to the function call parameter
// client port. 
//
// Returns:
// The TCB table entry index should be returned as the new socket ID to the client
// and be used to identify the connection on the client side. If no entry in the TC table
// is available the function returns -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// NOT THREAD SAFE

int srt_client_sock(unsigned int client_port) {
    for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
        if (tcb_list[i] == NULL) {
            client_tcb_t* tcb = (client_tcb_t*) malloc(sizeof(client_tcb_t));
            memset(tcb, 0, sizeof(client_tcb_t));
            pthread_mutex_init(&(tcb->mu), NULL);
            
            tcb->client_portNum = client_port;
            tcb->svr_portNum = -1;
            tcb->state = CLOSED;
            init_event_with_mu(&(tcb->ctrl_ev), &(tcb->mu));

            tcb_list[i] = tcb;
            return i;
        }
    }
    return -1;
}

#define UNLOCK_RETURN(mu_ptr, rc) \
    do { \
        pthread_mutex_unlock(mu_ptr); \
        return rc; \
    } while (0)

// Connect to a srt server
//
// This function is used to connect to the server. It takes the socket ID and the
// server’s port number as input parameters. The socket ID is used to find the TCB entry.
// This function sets up the TCB’s server port number and a SYN segment to send to
// the server using snp_sendseg(). After the SYN segment is sent, a timer is started.
// If no SYNACK is received after SYNSEG_TIMEOUT timeout, then the SYN is
// retransmitted. If SYNACK is received, return 1. Otherwise, if the number of SYNs
// sent > SYN_MAX_RETRY,  transition to CLOSED state and return -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

static void populate_syn_seg(const client_tcb_t* tcb, seg_t* seg) {
    memset(seg, 0, sizeof(seg_t));
    srt_hdr_t* hdr = &(seg->header);
    hdr->src_port = tcb->client_portNum;
    hdr->dest_port = tcb->svr_portNum;
    hdr->type = SYN;
}

int srt_client_connect(int sockfd, unsigned int server_port) {
    client_tcb_t* tcb = tcb_list[sockfd];
    // This socket has not been allocated yet
    if (tcb == NULL) return -1;

    pthread_mutex_t* mu = &(tcb->mu);
    pthread_mutex_lock(mu);
    // Wrong state
    if (tcb->state != CLOSED) 
        UNLOCK_RETURN(mu, -2);
    // This socket is already occupied by other server port before
    if (tcb->svr_portNum != -1) 
        UNLOCK_RETURN(mu, -2);

    tcb->svr_portNum = server_port;
    tcb->state = SYNSENT;
    pthread_mutex_unlock(mu);

    event_t* ctrl_ev = &(tcb->ctrl_ev);
    seg_t syn_seg;
    populate_syn_seg(tcb, &syn_seg);
    int send_count = 0;
    int result = 0;
    while (result == 0) {
        snp_sendseg(overlay_conn, &syn_seg);
        ++send_count;
        printf("client sockfd=%d, svr_port=%d, sending SYN, iter=%d\n", sockfd, server_port, send_count);

        int wait_rc = wait_timeout_event(ctrl_ev, 0, SYN_TIMEOUT);
        // |ctrl_ev->cv_mutex| is locked at this point.
        if (wait_rc == 1) {
            assert(tcb->state = CONNECTED);
            result = 1;
        } else if (send_count >= SYN_MAX_RETRY) {
            // exceeds #maximum retry
            tcb->state = CLOSED;
            result = -1;
        }
        unlock_event(ctrl_ev);
    }
    return result;
}

// Send data to a srt server
//
// Send data to a srt server. You do not need to implement this for Lab4.
// We will use this in Lab5 when we implement a Go-Back-N sliding window.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_send(int sockfd, void* data, unsigned int length) {
	return 1;
}

// Disconnect from a srt server
//
// This function is used to disconnect from the server. It takes the socket ID as
// an input parameter. The socket ID is used to find the TCB entry in the TCB table.
// This function sends a FIN segment to the server. After the FIN segment is sent
// the state should transition to FINWAIT and a timer started. If the
// state == CLOSED after the timeout the FINACK was successfully received. Else,
// if after a number of retries FIN_MAX_RETRY the state is still FINWAIT then
// the state transitions to CLOSED and -1 is returned.

static void populate_fin_seg(const client_tcb_t* tcb, seg_t* seg) {
    memset(seg, 0, sizeof(seg_t));
    srt_hdr_t* hdr = &(seg->header);
    hdr->src_port = tcb->client_portNum;
    hdr->dest_port = tcb->svr_portNum;
    hdr->type = FIN;
}


int srt_client_disconnect(int sockfd) {
    client_tcb_t* tcb = tcb_list[sockfd];
    // This socket has not been allocated yet
    if (tcb == NULL)
        return -1;

    pthread_mutex_t* mu = &(tcb->mu);
    pthread_mutex_lock(mu);
    // Wrong state
    if (tcb->state != CONNECTED) 
        UNLOCK_RETURN(mu, -2);

    tcb->state = FINWAIT;
    pthread_mutex_unlock(mu);

    event_t* ctrl_ev = &(tcb->ctrl_ev);
    seg_t syn_seg;
    populate_fin_seg(tcb, &syn_seg);
    int send_count = 0;
    int result = 0;
    while (result == 0) {
        snp_sendseg(overlay_conn, &syn_seg);
        ++send_count;

        int wait_rc = wait_timeout_event(ctrl_ev, 0, FIN_TIMEOUT);
        // |ctrl_ev->cv_mutex| is locked at this point.
        if (wait_rc == 1) {
            assert(tcb->state = CLOSED);
            result = 1;
        } else if (send_count >= FIN_MAX_RETRY) {
            // exceeds #maximum retry
            tcb->state = CLOSED;
            result = -1;
        }
        unlock_event(ctrl_ev);
    }
    return result;
}


// Close srt client

// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1
// if fails (i.e., in the wrong state).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_close(int sockfd) {
    client_tcb_t* tcb = tcb_list[sockfd];
    // This socket has not been allocated yet
    if (tcb == NULL) return -1;

    pthread_mutex_t* mu = &(tcb->mu);
    pthread_mutex_lock(mu);
    // Wrong state
    if (tcb->state != CLOSED) 
        UNLOCK_RETURN(mu, -1);

    pthread_mutex_unlock(mu);
    destroy_event(&(tcb->ctrl_ev));
    pthread_mutex_destroy(mu);
    free(tcb);
    tcb_list[sockfd] = NULL;

	return 1;
}

// The thread handles incoming segments
//
// This is a thread  started by srt_client_init(). It handles all the incoming
// segments from the server. The design of seghandler is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the overlay connection is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
//

int find_sockfd_by_response(const srt_hdr_t* response_hdr) {
    for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
        client_tcb_t* tcb = tcb_list[i];
        if (tcb == NULL)
            continue;
        // note that since this is the header of a response message, the
        // src/dest port are flipped.
        if ((tcb->client_portNum == response_hdr->dest_port) && 
            (tcb->svr_portNum == response_hdr->src_port))
            return i;
    }
    return -1;
}

void* seghandler(void* arg) {
    seg_t seg;
    int rc = 0;
    while (1) {
        rc = snp_recvseg(overlay_conn, &seg);
        if (rc == -1)
            break;

        srt_hdr_t* hdr = &(seg.header);
        int sockfd = find_sockfd_by_response(hdr);
        if (sockfd == -1) {
            printf("Received a segment that nobody would handle: src=%d, dst=%d.\n",
                hdr->src_port, hdr->dest_port);
            continue;
        }
        
        client_tcb_t* tcb = tcb_list[sockfd];
        event_t* ctrl_ev = &(tcb->ctrl_ev);
        lock_event(ctrl_ev);
        switch(tcb->state) {
            case SYNSENT: {
                if (hdr->type == SYNACK) {
                    tcb->state = CONNECTED;
                    signal_event(ctrl_ev);
                }
                break;
            }
            case FINWAIT: {
                if (hdr->type == FINACK) {
                    tcb->state = CLOSED;
                    signal_event(ctrl_ev);
                }
                break;
            }
            case CONNECTED: {
                // ignored
                break;
            }
            case CLOSED: {
                // ignored
                break;
            }
            default: {
                printf("Unkonwn client state: %d\n", tcb->state);
                break;
            }
        }
        unlock_event(ctrl_ev);
    }

    // close(overlay_conn);
    pthread_exit(NULL);
}



