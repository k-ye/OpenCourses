#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "seg.h"

#define CHAN_BUF_SIZE 3
#define CHAN_R_INDEX(chan) ((int)(chan->ri % CHAN_BUF_SIZE))
#define CHAN_W_INDEX(chan) ((int)(chan->ri % CHAN_BUF_SIZE))

typedef struct chan {
    pthread_mutex_t cv_mutex;
    pthread_cond_t cv;

    seg_t buffer[CHAN_BUF_SIZE];
    int ri;
    int wi;

    int initalized;
} chan_t;

void init_chan(chan_t* chan);

void destroy_chan(chan_t* chan);

// Check if chan's buffer is empty.
// precondition: |chan->cv_mutex| is locked
int is_chan_empty(const chan_t* chan);

// Check if chan's buffer is full.
// precondition: |chan->cv_mutex| is locked
int is_chan_full(const chan_t* chan);

// precondition: 
//  - |chan->cv_mutex| is locked
//  - is_chan_empty(chan) == 0 (false)
const seg_t* read_next(chan_t* chan);

// precondition: 
//  - |chan->cv_mutex| is locked
//  - is_chan_full(chan) == 0 (false)
void write_next(const seg_t* seg, chan_t* chan);


// When for data coming to |chan| for a maximum of |sec| + |nano| period.
// Returns:
//   1: on success
//  -1: on timeout
//
// precondition: |chan->cv_mutex| is unlocked.
// postcondition: |chan->cv_mutex| is *locked*.
int wait_for_data(chan_t* chan, int sec, int nano);