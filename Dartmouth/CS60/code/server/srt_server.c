#include "srt_server.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>

#include "../common/util.h"

/*interfaces to application layer*/

//
//
//  SRT socket API for the server side application.
//  ===================================
//
//  In what follows, we provide the prototype definition for each call and
//  limited pseudo code representation of the function. This is not meant to be
//  comprehensive - more a guideline.
//
//  You are free to design the code as you wish.
//
//  NOTE: When designing all functions you should consider all possible states
//  of the FSM using a switch statement (see the Lab4 assignment for an
//  example). Typically, the FSM has to be
// in a certain state determined by the design of the FSM to carry out a certain
// action.
//
//  GOAL: Your job is to design and implement the prototypes below - fill in the
//  code.
//

// transport control block for each socket
static svr_tcb_t* tcb_list[MAX_TRANSPORT_CONNECTIONS];
// the actual tcp socket
static int overlay_conn;

static pthread_t seghandler_thread;

// srt server initialization
//
// This function initializes the TCB table marking all entries NULL. It also
// initializes a global variable for the overlay TCP socket descriptor ‘‘conn’’
// used as input parameter for snp_sendseg and snp_recvseg. Finally, the
// function starts the seghandler thread to handle the incoming segments. There
// is only one seghandler for the server side which handles call connections for
// the client.
//

void srt_server_init(int conn) {
  for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
    tcb_list[i] = NULL;
  }
  overlay_conn = conn;

  int rc = pthread_create(&seghandler_thread, NULL, seghandler, NULL);
  assert(rc == 0);
}

// Create a server sock
//
// This function looks up the client TCB table to find the first NULL entry, and
// creates a new TCB entry using malloc() for that entry. All fields in the TCB
// are initialized e.g., TCB state is set to CLOSED and the server port set to
// the function call parameter server port.  The TCB table entry index should be
// returned as the new socket ID to the server and be used to identify the
// connection on the server side. If no entry in the TCB table is available the
// function returns -1.

int srt_server_sock(unsigned int port) {
  for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
    if (tcb_list[i] == NULL) {
      svr_tcb_t* tcb = (svr_tcb_t*)malloc(sizeof(svr_tcb_t));
      memset(tcb, 0, sizeof(svr_tcb_t));
      pthread_mutex_init(&(tcb->mu), NULL);
      // init state
      tcb->client_portNum = -1;
      tcb->svr_portNum = port;
      tcb->state = CLOSED;
      init_event_with_mu(&(tcb->ctrl_ev), &(tcb->mu));
      // init receive buffer
      tcb->recvBuf = (char*)malloc(RECEIVE_BUF_SIZE);
      memset(tcb->recvBuf, 0, RECEIVE_BUF_SIZE);
      tcb->bufMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
      pthread_mutex_init(tcb->bufMutex, NULL);
      init_event_with_mu(&(tcb->recvBuf_ev), tcb->bufMutex);
      tcb->usedBufLen = 0;

      tcb_list[i] = tcb;
      return i;
    }
  }
  return -1;
}

#define UNLOCK_RETURN(mu_ptr, rc) \
  do {                            \
    pthread_mutex_unlock(mu_ptr); \
    return rc;                    \
  } while (0)

// Accept connection from srt client
//
// This function gets the TCB pointer using the sockfd and changes the state of
// the connetion to LISTENING. It then starts a timer to ‘‘busy wait’’ until the
// TCB’s state changes to CONNECTED (seghandler does this when a SYN is
// received). It waits in an infinite loop for the state transition before
// proceeding and to return 1 when the state change happens, dropping out of the
// busy wait loop. You can implement this blocking wait in different ways, if
// you wish.
//

int srt_server_accept(int sockfd) {
  svr_tcb_t* tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  pthread_mutex_t* mu = &(tcb->mu);
  pthread_mutex_lock(mu);
  // Wrong state
  if (tcb->state != CLOSED) UNLOCK_RETURN(mu, -2);
  // This socket is already occupied by other client port before
  if (tcb->client_portNum != -1) UNLOCK_RETURN(mu, -2);

  tcb->state = LISTENING;
  tcb->expect_seqNum = 1;
  pthread_mutex_unlock(mu);

  event_t* ctrl_ev = &(tcb->ctrl_ev);
  wait_event(ctrl_ev);
  assert(tcb->state == CONNECTED);
  unlock_event(ctrl_ev);

  return 1;
}

// Returns the remaining number of bytes wanted by the client.
// precondition: |tcb->bufMutex| is locked
static unsigned copy_data_from_buf(void* buf, unsigned want_sz,
                                   svr_tcb_t* tcb) {
  unsigned copy_sz = umin(want_sz, tcb->usedBufLen);
  memcpy(buf, tcb->recvBuf, copy_sz);
  tcb->usedBufLen -= copy_sz;
  // dst and src could overrlap, hence we use `memmove` here.
  memset(tcb->recvBuf, 0, copy_sz);  // this is unncessary
  memmove(tcb->recvBuf, tcb->recvBuf + copy_sz, tcb->usedBufLen);

  return copy_sz;
  // return (want_sz - copy_sz);
}

// Receive data from a srt client
//
// Receive data to a srt client. Recall this is a unidirectional transport
// where DATA flows from the client to the server. Signaling/control messages
// such as SYN, SYNACK, etc.flow in both directions. You do not need to
// implement this for Lab4. We will use this in Lab5 when we implement a
// Go-Back-N sliding window.
//
int srt_server_recv(int sockfd, void* buf, unsigned int length) {
  svr_tcb_t* tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  event_t* buf_ev = &(tcb->recvBuf_ev);
  lock_event(buf_ev);
  int copy_sz = copy_data_from_buf(buf, length, tcb);
  length -= copy_sz;
  buf += copy_sz;
  unlock_event(buf_ev);

  while (length) {
    wait_event(buf_ev);
    // |buf_ev->cv_mutex| is locked at this point.
    if (tcb->state != CONNECTED) {
      return -1;
    }
    // has something in the buffer
    copy_sz = copy_data_from_buf(buf, length, tcb);
    length -= copy_sz;
    buf += copy_sz;
    unlock_event(buf_ev);
  }

  return 1;
}

// Close the srt server
//
// This function calls free() to free the TCB entry. It marks that entry in TCB
// as NULL and returns 1 if succeeded (i.e., was in the right state to complete
// a close) and -1 if fails (i.e., in the wrong state).
//

int srt_server_close(int sockfd) {
  svr_tcb_t* tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  pthread_mutex_t* mu = &(tcb->mu);
  pthread_mutex_lock(mu);
  // Wrong state
  if (tcb->state != CLOSED) UNLOCK_RETURN(mu, -1);

  pthread_mutex_unlock(mu);
  destroy_event(&(tcb->ctrl_ev));
  pthread_mutex_destroy(mu);
  free(tcb);
  tcb_list[sockfd] = NULL;

  return 1;
}

// Thread handles incoming segments
//
// This is a thread  started by srt_server_init(). It handles all the incoming
// segments from the client. The design of seghanlder is an infinite loop that
// calls snp_recvseg(). If snp_recvseg() fails then the overlay connection is
// closed and the thread is terminated. Depending on the state of the connection
// when a segment is received  (based on the incoming segment) various actions
// are taken. See the client FSM for more details.
//

int find_sockfd_by_request(const srt_hdr_t* request_hdr) {
  for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
    svr_tcb_t* tcb = tcb_list[i];
    if (tcb == NULL) continue;

    if ((tcb->svr_portNum == request_hdr->dest_port) &&
        (tcb->client_portNum == request_hdr->src_port)) {
      return i;
    }
  }
  // for SYN, it could be that the client_portNum is not set yet. Search again
  if (request_hdr->type == SYN) {
    for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
      svr_tcb_t* tcb = tcb_list[i];
      if (tcb == NULL) continue;

      if ((tcb->svr_portNum == request_hdr->dest_port) &&
          (tcb->client_portNum == -1)) {
        return i;
      }
    }
  }
  return -1;
}

static void populate_seg_base(const svr_tcb_t* tcb, unsigned short type,
                              seg_t* seg) {
  memset(seg, 0, sizeof(seg_t));
  srt_hdr_t* hdr = &(seg->header);
  hdr->src_port = tcb->svr_portNum;
  hdr->dest_port = tcb->client_portNum;
  hdr->type = type;
}

static void send_synack(const svr_tcb_t* tcb) {
  seg_t synack;
  populate_seg_base(tcb, SYNACK, &synack);
  snp_sendseg(overlay_conn, &synack);
}

static void send_finack(const svr_tcb_t* tcb) {
  seg_t finack;
  populate_seg_base(tcb, FINACK, &finack);
  snp_sendseg(overlay_conn, &finack);
}

void* closewait_func(void* tcb_arg) {
  struct timeval tv;
  tv.tv_sec = CLOSEWAIT_TIMEOUT;
  tv.tv_usec = 0;
  select(0, NULL, NULL, NULL, &tv);

  svr_tcb_t* tcb = (svr_tcb_t*)tcb_arg;
  pthread_mutex_lock(&(tcb->mu));
  // change to CLOSED
  tcb->state = CLOSED;
  // clear buffer
  pthread_mutex_lock(tcb->bufMutex);
  tcb->usedBufLen = 0;
  pthread_mutex_unlock(tcb->bufMutex);
  
  pthread_mutex_unlock(&(tcb->mu));

  pthread_exit(NULL);
}

static inline int can_buffer_data(const svr_tcb_t* tcb, const seg_t* seg) {
  const srt_hdr_t* hdr = &(seg->header);
  DPRINTF("checking if we can buffer data...\n");
  DPRINTF("  tcb->expect_seqNum=%d, hdr->seq_num=%d ", tcb->expect_seqNum, hdr->seq_num);
  DPRINTF("  tcb->usedBufLen=%d, hdr->length=%d\n", tcb->usedBufLen, hdr->length);
  return ((checkchecksum(seg) == 1) && 
          (tcb->expect_seqNum == hdr->seq_num) &&
          (tcb->usedBufLen + hdr->length <= RECEIVE_BUF_SIZE));
}

// precondition:
// - |tcb->mu| is locked
// - |tcb->bufMutex| is unlocked
static void handle_data_seg(svr_tcb_t* tcb, const seg_t* seg) {
  event_t* buf_ev = &(tcb->recvBuf_ev);
  const srt_hdr_t* hdr = &(seg->header);
  DPRINTF("received DATA seg! src_port=%d dest_port=%d seq_num=%d data_sz=%d\n", hdr->src_port, hdr->dest_port, hdr->seq_num, hdr->length);
  lock_event(buf_ev);
  if (can_buffer_data(tcb, seg)) {
    const unsigned short data_sz = hdr->length;
    memcpy(tcb->recvBuf + tcb->usedBufLen, seg->data, data_sz);
    tcb->usedBufLen += data_sz;
    tcb->expect_seqNum += data_sz;
    DPRINTF("can buf, after buf, usedBufLen=%d!\n", tcb->usedBufLen);

    signal_event(buf_ev);
  } else {
    DPRINTF("cannot buf :( !\n");
  }

  seg_t dataack;
  populate_seg_base(tcb, DATAACK, &dataack);
  dataack.header.ack_num = tcb->expect_seqNum;
  DPRINTF("send DATAACK! src_port=%d dest_port=%d ack_num=%d\n", dataack.header.src_port, dataack.header.dest_port, dataack.header.ack_num);
  snp_sendseg(overlay_conn, &dataack);

  unlock_event(buf_ev);
}

void* seghandler(void* arg) {
  seg_t seg;
  int rc = 0;
  while (1) {
    rc = snp_recvseg(overlay_conn, &seg);
    if (rc == -1) break;

    srt_hdr_t* hdr = &(seg.header);
    int sockfd = find_sockfd_by_request(hdr);
    if (sockfd == -1) {
      DPRINTF("Received a segment that nobody would handle: src=%d, dst=%d.\n",
             hdr->src_port, hdr->dest_port);
      continue;
    }
    svr_tcb_t* tcb = tcb_list[sockfd];
    event_t* ctrl_ev = &(tcb->ctrl_ev);
    lock_event(ctrl_ev);
    DPRINTF("Received a segment for sockfd: %d, state=%d.\n", sockfd,
           tcb->state);
    switch (tcb->state) {
      case LISTENING: {
        if (hdr->type == SYN) {
          assert(tcb->client_portNum == -1);
          tcb->client_portNum = hdr->src_port;
          tcb->expect_seqNum = hdr->seq_num;

          // lock_event(ctrl_ev);
          send_synack(tcb);
          tcb->state = CONNECTED;
          signal_event(ctrl_ev);
        }
        break;
      }
      case CONNECTED: {
        if (hdr->type == SYN) {
          send_synack(tcb);
        } else if (hdr->type == FIN) {
          // lock_event(ctrl_ev);
          send_finack(tcb);
          tcb->state = CLOSEWAIT;
          signal_event(ctrl_ev);

          pthread_t closewait_thr;
          pthread_create(&closewait_thr, NULL, closewait_func, (void*)tcb);
        } else if (hdr->type == DATA) {
          handle_data_seg(tcb, &seg);
        }
        break;
      }
      case CLOSEWAIT: {
        send_finack(tcb);
        break;
      }
      case CLOSED: {
        // ignored
        break;
      }
      default: {
        DPRINTF("Unkonwn server state: %d\n", tcb->state);
        break;
      }
    }
    unlock_event(ctrl_ev);
  }

  // close(overlay_conn);
  pthread_exit(NULL);
}
