// FILE: client/app_stress_client.c
//
// Description: this is the stress test client application code. The client
// first starts the overlay by creating a direct TCP link between the client and
// the server. Then it initializes the SRT client by calling srt_client_init().
// It creates a socket and connects to the server  by calling srt_client_sock()
// and srt_client_connect(). Then it reads text data from file
// send_this_text.txt, and sends the length of the file and file data to the
// server. After some time, the client disconnects from the server by calling
// srt_client_disconnect(). Finally the client closes the socket by calling
// srt_client_close(). Overlay is stopped by calling overlay_end().

// Date: April 26,2008

// Input: None

// Output: SRT client states

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/seg.h"
#include "srt_client.h"

// One connection is created using client port CLIENTPORT1 and server port
// SVRPORT1.
#define CLIENTPORT1 87
#define SVRPORT1 88
// after the file is sent, the client waits for WAITTIME seconds, and then
// closes the connection
#define WAITTIME 5

int main() {
  // random seed for loss rate
  srand(time(NULL));

  // start overlay and get the overlay TCP socket descriptor
  int overlay_conn = overlay_client_start();
  if (overlay_conn < 0) {
    printf("fail to start overlay\n");
    exit(1);
  }

  // initialize srt client
  srt_client_init(overlay_conn);

  // create a srt client sock on port CLIENTPORT1 and connect to srt server port
  // SVRPORT1
  int sockfd = srt_client_sock(CLIENTPORT1);
  if (sockfd < 0) {
    printf("fail to create srt client sock");
    exit(1);
  }
  if (srt_client_connect(sockfd, SVRPORT1) < 0) {
    printf("fail to connect to srt server\n");
    exit(1);
  }
  printf("client connected to server, client port:%d, server port %d\n",
         CLIENTPORT1, SVRPORT1);

  // get sampletext.txt file length,
  // create a buffer and read the file data in
  FILE *f;
  f = fopen("send_this_text.txt", "r");
  assert(f != NULL);
  fseek(f, 0, SEEK_END);
  int fileLen = ftell(f);
  fseek(f, 0, SEEK_SET);
  printf("stress client, fileLen=%d\n", fileLen);
  char *buffer = (char *)malloc(fileLen);
  fread(buffer, fileLen, 1, f);
  fclose(f);

  // send file length first, then send the whole file
  srt_client_send(sockfd, &fileLen, sizeof(int));
  srt_client_send(sockfd, buffer, fileLen);
  free(buffer);

  // wait for a while and close the connections
  sleep(WAITTIME);

  if (srt_client_disconnect(sockfd) < 0) {
    printf("fail to disconnect from srt server\n");
    exit(1);
  }
  if (srt_client_close(sockfd) < 0) {
    printf("fail to close srt client\n");
    exit(1);
  }

  // stop the overlay
  overlay_stop(overlay_conn);
}
