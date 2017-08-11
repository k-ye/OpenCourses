#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../common/seg.h"
#include "srt_client.h"

int main() {
    //random seed for loss rate
    srand(time(NULL));
    //start overlay 
    int overlay_conn = overlay_client_start();
    if(overlay_conn < 0) {
        printf("fail to start overlay\n");
        exit(1);
    }

    //initialize srt client 
    srt_client_init(overlay_conn);

    //create a srt client sock on port 87 and connect to srt server at port 88
    int sockfd = srt_client_sock(87);
    if(sockfd<0) {
        printf("fail to create srt client sock");
        exit(1);
    }
    if(srt_client_connect(sockfd,88)<0) {
        printf("fail to connect to srt server\n");
        exit(1);
    }
    printf("client connect to server, client port:87, server port 88\n");
    
    //create a srt client sock on port 89 and connect to srt server at port 90
    int sockfd2 = srt_client_sock(89);
    if(sockfd2<0) {
        printf("fail to create srt client sock");
        exit(1);
    }
    if(srt_client_connect(sockfd2,90)<0) {
        printf("fail to connect to srt server\n");
        exit(1);
    }
    printf("client connect to server, client port:89, server port 90\n");

    //wait for a while and close the connection to srt server
    sleep(5);
    
    if(srt_client_disconnect(sockfd) < 0) {
        printf("fail to disconnect from srt server\n");
        exit(1);
    }
    if(srt_client_close(sockfd) < 0) {
        printf("failt to close srt client\n");
        exit(1);
    }
    
    if(srt_client_disconnect(sockfd2) < 0) {
        printf("fail to disconnect from srt server\n");
        exit(1);
    }
    if(srt_client_close(sockfd2) < 0) {
        printf("failt to close srt client\n");
        exit(1);
    }
    //close overlay tcp connection
    overlay_stop(overlay_conn);
}
