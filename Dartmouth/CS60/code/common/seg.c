//
// FILE: seg.h

// Description: This file contains segment definitions and interfaces to send
// and receive segments. The prototypes support snp_sendseg() and snp_rcvseg()
// for sending to the network layer.
//
// Date: April 18, 2008
//       April 21, 2008 **Added more detailed description of prototypes fixed
//       ambiguities** ATC April 26, 2008 **Added checksum descriptions**
//

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "seg.h"
#include "util.h"

int overlay_client_start() {
  int out_conn;
  struct sockaddr_in servaddr;
  struct hostent* hostInfo;

  char hostname_buf[50];
  printf("Enter server name to connect:");
  scanf("%s", hostname_buf);

  hostInfo = gethostbyname(hostname_buf);
  if (!hostInfo) {
    printf("host name error!\n");
    return -1;
  }

  servaddr.sin_family = hostInfo->h_addrtype;
  memcpy((char*)&servaddr.sin_addr.s_addr, hostInfo->h_addr_list[0],
         hostInfo->h_length);
  servaddr.sin_port = htons(PORT);

  out_conn = socket(AF_INET, SOCK_STREAM, 0);
  if (out_conn < 0) {
    return -1;
  }
  if (connect(out_conn, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    return -1;
  return out_conn;
}

int overlay_server_start() {
  int tcpserv_sd;
  struct sockaddr_in tcpserv_addr;
  int connection;
  struct sockaddr_in tcpclient_addr;
  socklen_t tcpclient_addr_len;

  tcpserv_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (tcpserv_sd < 0) return -1;
  memset(&tcpserv_addr, 0, sizeof(tcpserv_addr));
  tcpserv_addr.sin_family = AF_INET;
  tcpserv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  tcpserv_addr.sin_port = htons(PORT);

  if (bind(tcpserv_sd, (struct sockaddr*)&tcpserv_addr, sizeof(tcpserv_addr)) <
      0)
    return -1;
  if (listen(tcpserv_sd, 1) < 0) return -1;
  printf("waiting for connection\n");
  connection = accept(tcpserv_sd, (struct sockaddr*)&tcpclient_addr,
                      &tcpclient_addr_len);
  return connection;
}

void overlay_stop(int overlay_conn) { close(overlay_conn); }

//
//
//  SNP API for the client and server sides
//  =======================================
//
//  In what follows, we provide the prototype definition for each call and
//  limited pseudo code representation of the function. This is not meant to be
//  comprehensive - more a guideline.
//
//  You are free to design the code as you wish.
//
//  NOTE: snp_sendseg() and snp_recvseg() are services provided by the
//  networking layer i.e., simple network protocol to the transport layer.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

// Send a SRT segment over the overlay network (this is simply a single TCP
// connection in the case of Lab4). TCP sends data as a byte stream. In order to
// send segments over the overlay TCP connection, delimiters for the start and
// end of the packet must be added to the transmission. That is, first send the
// characters ``!&'' to indicate the start of a  segment; then send the segment
// seg_t; and finally, send end of packet markers ``!#'' to indicate the end of
// a segment. Return 1 in case of success, and -1 in case of failure.
// snp_sendseg() uses send() to first send two chars, then send() again but for
// the seg_t, and, then send() two chars for the end of packet.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int snp_sendseg(int connection, seg_t* segPtr) {
  char bufstart[2];
  char bufend[2];
  bufstart[0] = '!';
  bufstart[1] = '&';
  bufend[0] = '!';
  bufend[1] = '#';
  if (send(connection, bufstart, 2, 0) < 0) {
    return -1;
  }
  if (send(connection, segPtr, sizeof(seg_t), 0) < 0) {
    return -1;
  }
  if (send(connection, bufend, 2, 0) < 0) {
    return -1;
  }
  return 1;
}

// Receive a segment over overlay network (this is a single TCP connection in
// the case of Lab4). We recommend that you receive one byte at a time using
// recv(). Here you are looking for
// ``!&'' characters then seg_t and then ``!#''. This is a FSM of sorts and you
// should code it that way. Make sure that you cover cases such as
// ``#&bbb!b!bn#bbb!#'' The assumption here (fairly limiting but simplistic) is
// that !& and !# will not be seen in the data in the segment. You should read
// in one byte as a char at a time and copy the data part into a buffer to be
// returned to the caller.
//
// IMPORTANT: once you have parsed a segment you should call seglost(). The code
// for seglost(seg_t* segment) is provided for you below snp_recvseg().
//
// a segment has PKT_LOST_RATE probability to be lost or invalid checksum
// with PKT_LOST_RATE/2 probability, the segment is lost, this function returns
// 1 if the segment is not lost, return 0 Even the segment is not lost, the
// packet has PKT_LOST_RATE/2 probability to have invalid checksum
//  We flip  a random bit in the segment to create invalid checksum

int seglost(seg_t* segPtr) {
  int random = rand() % 100;
  if (random < PKT_LOSS_RATE * 100) {
    printf("seg lost!!!\n");
    return 1;
  }
  return 0;
}

static int maybe_flip_bit(seg_t* segPtr) {
  int random = rand() % 100;
  if (random < PKT_FLIP_BIT_RATE * 100) {
    printf("error bit!!!\n");
    // get data length
    int len = sizeof(srt_hdr_t) + segPtr->header.length;
    // get a random bit that will be flipped
    int errorbit = rand() % (len * 8);
    // flip the bit
    char* temp = (char*)segPtr;
    temp = temp + errorbit / 8;
    *temp = *temp ^ (1 << (errorbit % 8));
    return 1;
  }
  return 0;
}

int snp_recvseg(int connection, seg_t* segPtr) {
  char buf[sizeof(seg_t) + 2];
  char c;
  int idx = 0;
  // state can be 0,1,2,3;
  // 0 starting point
  // 1 '!' received
  // 2 '&' received, start receiving segment
  // 3 '!' received,
  // 4 '#' received, finish receiving segment
  int state = 0;
  while (recv(connection, &c, 1, 0) > 0) {
    if (state == 0) {
      if (c == '!') state = 1;
    } else if (state == 1) {
      if (c == '&')
        state = 2;
      else
        state = 0;
    } else if (state == 2) {
      if (c == '!') {
        buf[idx] = c;
        idx++;
        state = 3;
      } else {
        buf[idx] = c;
        idx++;
      }
    } else if (state == 3) {
      if (c == '#') {
        buf[idx] = c;
        idx++;
        state = 0;
        idx = 0;
        if (seglost(segPtr) > 0) {
          continue;
        }
        memcpy(segPtr, buf, sizeof(seg_t));
        maybe_flip_bit(segPtr);
        return 1;
      } else if (c == '!') {
        buf[idx] = c;
        idx++;
      } else {
        buf[idx] = c;
        idx++;
        state = 2;
      }
    }
  }
  return -1;
}

static unsigned short ones_comp_add_ushort(unsigned short lhs,
                                           const void* rhs_ptr) {
  unsigned short rhs = *((unsigned short*)(rhs_ptr));
  unsigned result = lhs + rhs;
  const unsigned MASK = (1u << 16);
  if (result >= MASK) {
    result = (result + 1) % MASK;
  }
  assert(result < MASK);
  return (unsigned short)result;
}

static inline unsigned get_seg_sz_for_checksum(const seg_t* seg) {
  assert((sizeof(srt_hdr_t) & 1) == 0);
  unsigned padded_data_len = seg->header.length;
  padded_data_len += (padded_data_len & 1);
  assert(((padded_data_len & 1) == 0) && (padded_data_len <= MAX_SEG_LEN));
  return (sizeof(srt_hdr_t) + padded_data_len);
}
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// this function calculates checksum over the given segment
// the checksum is calculated over the segment header and segment data
// you should first clear the checksum field in segment header to be 0
// if the data has odd number of octets, add an 0 octets to calculate checksum
// use 1s complement for checksum calculation
unsigned short checksum(seg_t* segment) {
  segment->header.checksum = 0;
  const unsigned seg_sz = get_seg_sz_for_checksum(segment);
  char* base = (char*)segment;
  unsigned short result = 0;
  for (int i = 0; i < seg_sz; i += 2) {
    result = ones_comp_add_ushort(result, base + i);
  }
  segment->header.checksum = (~result);
  return result;
}

// check the checksum in the segment,
// return 1 if the checksum is valid,
// return -1 if the checksum is invalid
int checkchecksum(const seg_t* segment) {
  const unsigned seg_sz = get_seg_sz_for_checksum(segment);
  char* base = (char*)segment;
  unsigned short result = 0;
  for (int i = 0; i < seg_sz; i += 2) {
    result = ones_comp_add_ushort(result, base + i);
  }
  const unsigned short MASK = 0xffff;
  if (result != MASK) {
    DPRINTF("WRONG checkchecksum=0x%04x!\n", result);
  }
  return ((result == 0xffff) ? 1 : -1);
}