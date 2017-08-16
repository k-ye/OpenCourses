#include "srt_client.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memset, memcpy
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#include "../common/util.h"

/*interfaces to application layer*/

//
//
//  SRT socket API for the client side application.
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
//  GOAL: Your goal for this assignment is to design and implement the
//  protoypes below - fill the code.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// srt client initialization
//
// This function initializes the TCB table marking all entries NULL. It also
// initializes a global variable for the overlay TCP socket descriptor ‘‘conn’’
// used as input parameter for snp_sendseg and snp_recvseg. Finally, the
// function starts the seghandler thread to handle the incoming segments. There
// is only one seghandler for the client side which handles call connections for
// the client.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

// transport control block for each socket
static client_tcb_t *tcb_list[MAX_TRANSPORT_CONNECTIONS];
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
// This function looks up the client TCB table to find the first NULL entry, and
// creates a new TCB entry using malloc() for that entry. All fields in the TCB
// are initialized e.g., TCB state is set to CLOSED and the client port set to
// the function call parameter client port.
//
// Returns:
// The TCB table entry index should be returned as the new socket ID to the
// client and be used to identify the connection on the client side. If no entry
// in the TC table is available the function returns -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// NOT THREAD SAFE

int srt_client_sock(unsigned int client_port) {
  for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
    if (tcb_list[i] == NULL) {
      client_tcb_t *tcb = (client_tcb_t *)malloc(sizeof(client_tcb_t));
      memset(tcb, 0, sizeof(client_tcb_t));
      pthread_mutex_init(&(tcb->mu), NULL);

      tcb->client_portNum = client_port;
      tcb->svr_portNum = -1;
      tcb->state = CLOSED;
      init_event_with_mu(&(tcb->ctrl_ev), &(tcb->mu));
      tcb->sockfd = i;

      tcb->next_seqNum = 1;
      tcb->bufMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
      pthread_mutex_init(tcb->bufMutex, NULL);
      // tcb->sendBufDummyHead = (segBuf_t *)malloc(sizeof(segBuf_t));
      tcb->sendBufHead = NULL;
      tcb->sendBufUnsent = NULL;
      tcb->sendBufTail = NULL;
      tcb->unAck_segNum = 0;
      init_event_with_mu(&(tcb->sendBuf_ev), tcb->bufMutex);

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

// Connect to a srt server
//
// This function is used to connect to the server. It takes the socket ID and
// the server’s port number as input parameters. The socket ID is used to find
// the TCB entry. This function sets up the TCB’s server port number and a SYN
// segment to send to the server using snp_sendseg(). After the SYN segment is
// sent, a timer is started. If no SYNACK is received after SYNSEG_TIMEOUT
// timeout, then the SYN is retransmitted. If SYNACK is received, return 1.
// Otherwise, if the number of SYNs sent > SYN_MAX_RETRY,  transition to CLOSED
// state and return -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

static void populate_syn_seg(const client_tcb_t *tcb, seg_t *seg) {
  memset(seg, 0, sizeof(seg_t));
  srt_hdr_t *hdr = &(seg->header);
  hdr->src_port = tcb->client_portNum;
  hdr->dest_port = tcb->svr_portNum;
  hdr->type = SYN;
  hdr->seq_num = tcb->next_seqNum;
}

int srt_client_connect(int sockfd, unsigned int server_port) {
  client_tcb_t *tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  pthread_mutex_t *mu = &(tcb->mu);
  pthread_mutex_lock(mu);
  // Wrong state
  if (tcb->state != CLOSED) UNLOCK_RETURN(mu, -2);
  // This socket is already occupied by other server port before
  if (tcb->svr_portNum != -1) UNLOCK_RETURN(mu, -2);

  tcb->svr_portNum = server_port;
  tcb->state = SYNSENT;
  pthread_mutex_unlock(mu);

  event_t *ctrl_ev = &(tcb->ctrl_ev);
  seg_t syn_seg;
  populate_syn_seg(tcb, &syn_seg);
  int send_count = 0;
  int result = 0;
  while (result == 0) {
    snp_sendseg(overlay_conn, &syn_seg);
    ++send_count;
    DPRINTF("client sockfd=%d, svr_port=%d, sending SYN, iter=%d\n", sockfd,
           server_port, send_count);

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

// Check if the sender buffer of |tcb| is empty.
// precondition: |tcb->bufMutex| is locked.
inline static int is_send_buf_empty(const client_tcb_t *tcb) {
  return (tcb->sendBufHead == NULL);
}

// buffer |data| into |tcb|'s send buffer, possibly disassembly into multiple
// segments due to size limitation.
// precondition: |tcb->bufMutex| is locked.
static void buf_new_data_segs(client_tcb_t *tcb, void *data, unsigned length) {
  DPRINTF("src_client=%d: buf data of size=%d, seq_num=%d\n", tcb->sockfd, length, tcb->next_seqNum);
  while (length > 0) {
    segBuf_t *new_seg_buf = (segBuf_t *)malloc(sizeof(segBuf_t));
    memset(new_seg_buf, 0, sizeof(segBuf_t));
    new_seg_buf->next = NULL;  // unnecessary
    seg_t *seg = &(new_seg_buf->seg);
    // set srt header
    srt_hdr_t *hdr = &(seg->header);
    hdr->src_port = tcb->client_portNum;
    hdr->dest_port = tcb->svr_portNum;
    hdr->type = DATA;
    hdr->seq_num = tcb->next_seqNum;
    // copy data
    // memset(seg->data, 0, MAX_SEG_LEN);
    int seg_data_sz = imin(length, MAX_SEG_LEN);
    hdr->length = seg_data_sz;
    memcpy((void *)(seg->data), data, seg_data_sz);
    length -= seg_data_sz;
    data += seg_data_sz;
    // append to the send buffer of |tcb|
    if (is_send_buf_empty(tcb)) {
      tcb->sendBufHead = new_seg_buf;
      tcb->sendBufTail = new_seg_buf;
    } else {
      tcb->sendBufTail->next = new_seg_buf;
      tcb->sendBufTail = new_seg_buf;
    }
    if (tcb->sendBufUnsent == NULL) {
      tcb->sendBufUnsent = new_seg_buf;
    }
    DPRINTF("  src_client=%d: new_seg_buf len=%d, seq_num=%d\n", tcb->sockfd, seg_data_sz, tcb->next_seqNum);
    tcb->next_seqNum += seg_data_sz;
  }
  DPRINTF("src_client=%d: after buf the data, seq_num=%d\n", tcb->sockfd, tcb->next_seqNum);
}

int srt_client_send(int sockfd, void *data, unsigned int length) {
  client_tcb_t *tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  pthread_mutex_t *tcb_mu = &(tcb->mu);
  pthread_mutex_lock(tcb_mu);
  // Wrong state
  if (tcb->state != CONNECTED) UNLOCK_RETURN(tcb_mu, -1);

  // make sure this is done in CONNECTED state
  pthread_mutex_t *buf_mu = tcb->bufMutex;
  pthread_mutex_lock(buf_mu);

  int was_empty_previously = is_send_buf_empty(tcb);
  buf_new_data_segs(tcb, data, length);
  if (was_empty_previously) {
    // start send buffer timer
    // DPRINTF("src_client: start send timer thread\n");
    pthread_t send_timer_thr;
    pthread_create(&send_timer_thr, NULL, sendBuf_timer, (void *)tcb);
  }

  pthread_mutex_unlock(buf_mu);
  pthread_mutex_unlock(tcb_mu);
  return 1;
}

// Disconnect from a srt server
//
// This function is used to disconnect from the server. It takes the socket ID
// as an input parameter. The socket ID is used to find the TCB entry in the TCB
// table. This function sends a FIN segment to the server. After the FIN segment
// is sent the state should transition to FINWAIT and a timer started. If the
// state == CLOSED after the timeout the FINACK was successfully received. Else,
// if after a number of retries FIN_MAX_RETRY the state is still FINWAIT then
// the state transitions to CLOSED and -1 is returned.

static void populate_fin_seg(const client_tcb_t *tcb, seg_t *seg) {
  memset(seg, 0, sizeof(seg_t));
  srt_hdr_t *hdr = &(seg->header);
  hdr->src_port = tcb->client_portNum;
  hdr->dest_port = tcb->svr_portNum;
  hdr->type = FIN;
}

static void clear_srt_send_buf(client_tcb_t *tcb) { 
  pthread_mutex_lock(tcb->bufMutex);
  // don't clear |tcb->next_seqNum|!
  while (tcb->sendBufHead) {
    segBuf_t* holder = tcb->sendBufHead;
    tcb->sendBufHead = holder->next;
    free(holder);
  }
  tcb->unAck_segNum = 0;
  pthread_mutex_unlock(tcb->bufMutex);
}

int srt_client_disconnect(int sockfd) {
  client_tcb_t *tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  pthread_mutex_t *mu = &(tcb->mu);
  pthread_mutex_lock(mu);
  // Wrong state
  if (tcb->state != CONNECTED) UNLOCK_RETURN(mu, -2);

  tcb->state = FINWAIT;
  pthread_mutex_unlock(mu);

  event_t *ctrl_ev = &(tcb->ctrl_ev);
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
  clear_srt_send_buf(tcb);

  return result;
}

// Close srt client

// This function calls free() to free the TCB entry. It marks that entry in TCB
// as NULL and returns 1 if succeeded (i.e., was in the right state to complete
// a close) and -1 if fails (i.e., in the wrong state).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_close(int sockfd) {
  client_tcb_t *tcb = tcb_list[sockfd];
  // This socket has not been allocated yet
  if (tcb == NULL) return -1;

  pthread_mutex_t *mu = &(tcb->mu);
  // Check state
  pthread_mutex_lock(mu);
  if (tcb->state != CLOSED) UNLOCK_RETURN(mu, -1);
  pthread_mutex_unlock(mu);
  // Release state related memory
  destroy_event(&(tcb->ctrl_ev));
  pthread_mutex_destroy(mu);
  // Release send buffer related memory
  destroy_event(&(tcb->sendBuf_ev));
  pthread_mutex_destroy(tcb->bufMutex);
  free(tcb->bufMutex);
  // Release the tcb entry
  free(tcb);
  tcb_list[sockfd] = NULL;

  return 1;
}

// The thread handles incoming segments
//
// This is a thread  started by srt_client_init(). It handles all the incoming
// segments from the server. The design of seghandler is an infinite loop that
// calls snp_recvseg(). If snp_recvseg() fails then the overlay connection is
// closed and the thread is terminated. Depending on the state of the connection
// when a segment is received  (based on the incoming segment) various actions
// are taken. See the client FSM for more details.
//

int find_sockfd_by_response(const srt_hdr_t *response_hdr) {
  for (int i = 0; i < MAX_TRANSPORT_CONNECTIONS; ++i) {
    client_tcb_t *tcb = tcb_list[i];
    if (tcb == NULL) continue;
    // note that since this is the header of a response message, the
    // src/dest port are flipped.
    if ((tcb->client_portNum == response_hdr->dest_port) &&
        (tcb->svr_portNum == response_hdr->src_port))
      return i;
  }
  return -1;
}

// precondition:
// - |tcb->mu| is locked
// - |tcb->bufMutex| is unlocked
static void handle_dataack(client_tcb_t *tcb, srt_hdr_t *hdr) {
  event_t *buf_ev = &(tcb->sendBuf_ev);
  lock_event(buf_ev);
  assert(!is_send_buf_empty(tcb));
  if (tcb->sendBufUnsent) {
    assert(hdr->ack_num <= tcb->sendBufUnsent->seg.header.seq_num);
  }

  segBuf_t *head = tcb->sendBufHead;
  while (head && (head->seg.header.seq_num < hdr->ack_num)) {
    tcb->sendBufHead = head->next;
    free(head);
    head = tcb->sendBufHead;
    assert(tcb->unAck_segNum > 0);
    --(tcb->unAck_segNum);
  }
  DPRINTF("src_client=%d: received DATAACK! ack_num=%d, unAck_segNum=%d\n", tcb->sockfd, hdr->ack_num, tcb->unAck_segNum);
  if (!(tcb->sendBufHead)) {
    tcb->sendBufHead = NULL;
    tcb->sendBufTail = NULL;
    assert(tcb->sendBufUnsent == NULL);
  }

  signal_event(buf_ev);
  unlock_event(buf_ev);
}

void *seghandler(void *arg) {
  seg_t seg;
  int rc = 0;
  while (1) {
    rc = snp_recvseg(overlay_conn, &seg);
    if (rc == -1) break;

    srt_hdr_t *hdr = &(seg.header);
    int sockfd = find_sockfd_by_response(hdr);
    if (sockfd == -1) {
      DPRINTF("Received a segment that nobody would handle: src=%d, dst=%d.\n",
             hdr->src_port, hdr->dest_port);
      continue;
    }

    client_tcb_t *tcb = tcb_list[sockfd];
    event_t *ctrl_ev = &(tcb->ctrl_ev);
    lock_event(ctrl_ev);
    switch (tcb->state) {
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
        if (hdr->type == DATAACK) {
          handle_dataack(tcb, hdr);
        }
        break;
      }
      case CLOSED: {
        // ignored
        break;
      }
      default: {
        DPRINTF("Unkonwn client state: %d\n", tcb->state);
        break;
      }
    }
    unlock_event(ctrl_ev);
  }
  pthread_exit(NULL);
}

// precondition: |tcb->bufMutex| is locked
static void send_buf_invariants(const client_tcb_t *tcb) {
  assert(tcb != NULL);
  if (tcb->sendBufHead) {
    assert(tcb->sendBufTail != NULL);
    // |tcb->sendBufUnsent| could be NULL;
  } else {
    assert(tcb->sendBufTail == NULL);
    assert(tcb->sendBufUnsent == NULL);
  }
  assert(tcb->unAck_segNum <= GBN_WINDOW);
}

// precondition: |tcb->bufMutex| is locked
// This function could modify |tcb->sendBufUnsent|.
static void send_from_first_unsent(client_tcb_t *tcb, unsigned long now_ns) {
  // DPRINTF("client_tcb=%d, sending from first unsent\n", tcb->sockfd);
  while ((tcb->unAck_segNum < GBN_WINDOW) && tcb->sendBufUnsent) {
    segBuf_t *cur = tcb->sendBufUnsent;
    snp_sendseg(overlay_conn, &(cur->seg));
    cur->sentTime = now_ns;

    tcb->sendBufUnsent = cur->next;
    ++(tcb->unAck_segNum);
  }
}

// precondition: |tcb->bufMutex| is locked
static void send_from_head(client_tcb_t *tcb) {
  DPRINTF("client_tcb=%d, sending from head, unAck_segNum=%d\n", tcb->sockfd, tcb->unAck_segNum);
  send_buf_invariants(tcb);
  long now_ns = get_now_nanos();
  segBuf_t *cur = tcb->sendBufHead;
  // Why don't we check |cur| != NULL here?
  // 1. If |tcb->sendBufUnsent| == NULL, then |tcb->sendBufHead| could be in
  //    any state, this is equivalent to |cur| != NULL
  // 2. Otherwise when |tcb->sendBufUnsent| != NULL, by the invariants,
  //    |tcb->sendBufHead| != NULL, therefore |cur| starts non-NULL. Since
  //    |tcb->sendBufUnsent| must be in the linked list whose head is
  //    |tcb->sendBufHead|, this stops when |cur| hits |tcb->sendBufUnsent|.
  while (cur != tcb->sendBufUnsent) {
    snp_sendseg(overlay_conn, &(cur->seg));
    cur->sentTime = now_ns;
    cur = cur->next;
  }
  send_from_first_unsent(tcb, now_ns);
  DPRINTF("client_tcb=%d, after sending from head, unAck_segNum=%d\n", tcb->sockfd, tcb->unAck_segNum);
}

void *sendBuf_timer(void *clienttcb) {
  client_tcb_t *tcb = (client_tcb_t *)clienttcb;

  pthread_mutex_t *buf_mu = tcb->bufMutex;
  pthread_mutex_lock(buf_mu);

  int has_data = !is_send_buf_empty(tcb);
  assert(has_data);
  send_from_head(tcb);
  pthread_mutex_unlock(buf_mu);

  event_t *buf_ev = &(tcb->sendBuf_ev);
  long wait_timeout_ns = SENDBUF_POLLING_INTERVAL;
  int resend_all = 0;
  while (has_data) {
    if (resend_all) {
      DPRINTF("client_tcb=%d, resending all data\n", tcb->sockfd);
      pthread_mutex_lock(buf_mu);
      send_from_head(tcb);
      pthread_mutex_unlock(buf_mu);
      resend_all = 0;
    }

    long time_start_waiting_ns = get_now_nanos();
    int wait_rc = wait_timeout_event(buf_ev, 0, wait_timeout_ns);
    has_data = !is_send_buf_empty(tcb);
    // |tcb->bufMutex| is held at this point.
    if (has_data) {
      if (wait_rc == 1) {
        // one or more empty slots are available
        DPRINTF("slot available for send\n");
        send_from_first_unsent(tcb, get_now_nanos());
        wait_timeout_ns -= (get_now_nanos() - time_start_waiting_ns);
        wait_timeout_ns = lmax(wait_timeout_ns, 0);
      } else if (wait_rc == -1) {
        // timeout
        DPRINTF("timeout when waiting for send\n");
        wait_timeout_ns = SENDBUF_POLLING_INTERVAL;
        resend_all =
            (DATA_TIMEOUT <= (get_now_nanos() - tcb->sendBufHead->sentTime));
      }
    }
    unlock_event(buf_ev);
  }
  pthread_exit(NULL);
}