#include "chan.h"

#include <assert.h>
#include <errno.h>
#include <string.h>   // memset, memcpy
#include <time.h>

void init_chan(chan_t* chan) {
    pthread_mutex_init(&(chan->cv_mutex), NULL);
    pthread_cond_init(&(chan->cv), NULL);
    memset(chan->buffer, 0, sizeof(chan->buffer));
    chan->ri = 0;
    chan->wi = 0;
    chan->initalized = 1;
}

void destroy_chan(chan_t* chan) {
    pthread_cond_destroy(&(chan->cv));
    pthread_mutex_destroy(&(chan->cv_mutex));
    memset(chan->buffer, 0, sizeof(chan->buffer));
    chan->ri = 0;
    chan->wi = 0;
    chan->initalized = 0;
}

int is_chan_empty(const chan_t* chan) {
    // precondition: |chan->cv_mutex| is locked
    int diff = chan->wi - chan->ri;
    assert(0 <= diff && diff <= CHAN_BUF_SIZE);
    return (diff == 0);
}

int is_chan_full(const chan_t* chan) {
    // precondition: |chan->cv_mutex| is locked
    int diff = chan->wi - chan->ri;
    assert(0 <= diff && diff <= CHAN_BUF_SIZE);
    return (diff == CHAN_BUF_SIZE);
}

const seg_t* read_next(chan_t* chan) {
    assert(!is_chan_empty(chan));
    int ri = ((chan->ri)++) % CHAN_BUF_SIZE;
    return &(chan->buffer[ri]);
}

void write_next(const seg_t* seg, chan_t* chan) {
    assert(!is_chan_full(chan));
    int wi = ((chan->wi)++) % CHAN_BUF_SIZE;
    memcpy(&(chan->buffer[wi]), seg, sizeof(seg_t));
}

int wait_for_data(chan_t* chan, int sec, int nano) {
    pthread_mutex_lock(&(chan->cv_mutex));
    
    struct timespec abs_to;
    abs_to.tv_sec = time(NULL) + sec;
    abs_to.tv_nsec = nano;
    int result = 0;
    while (result == 0) {
        int rc = pthread_cond_timedwait(&(chan->cv), 
                                        &(chan->cv_mutex), &abs_to);
        if (rc == ETIMEDOUT) {
            result = -1;
        } else if (!is_chan_empty(chan)) {
            result = 1;
        }
    }
    return result;
}