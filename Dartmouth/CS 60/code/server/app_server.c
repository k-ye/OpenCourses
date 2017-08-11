#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "../common/seg.h"
#include "srt_server.h"

int main() {
    //random seed for segment loss
    srand(time(NULL));

    //create a tcp connection to client
    int overlay_conn = overlay_server_start();
    if (overlay_conn < 0) {
        printf("can not start overlay\n");
    }

    //initialize srt server
    srt_server_init(overlay_conn);

    /*one server*/  
    //create a srt server sock at port 88 
    int sockfd = srt_server_sock(88);
    if (sockfd < 0) {
        printf("can't create srt server\n");
        exit(1);
    }
    //listen and accept connection from a srt client 
    srt_server_accept(sockfd);

    /*another server*/
    //create a srt server sock at port 90 
    int sockfd2 = srt_server_sock(90);
    if(sockfd2 < 0) {
        printf("can't create srt server\n");
        exit(1);
    }
    //listen and accept connection from a srt client 
    srt_server_accept(sockfd2);


    sleep(10);

    //close srt client 
    if(srt_server_close(sockfd)<0) {
        printf("can't destroy srt server\n");
        exit(1);
    }               
    if(srt_server_close(sockfd2)<0) {
        printf("can't destroy srt server\n");
        exit(1);
    }               

    //close tcp connection to the client
    overlay_stop(overlay_conn);
}
