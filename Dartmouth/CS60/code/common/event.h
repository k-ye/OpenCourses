#ifndef EVENT_H
#define EVENT_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// wrapper struct of a condition variable and mutex
typedef struct event {
  pthread_mutex_t* cv_mutex;
  pthread_cond_t cv;
  int own_mutex;  // indicates whether |cv_mutex| is owned
  int cond;
} event_t;

// If |mu| is not NULL, then this event shares a mutex owned by another object.
void init_event(event_t* e);
void init_event_with_mu(event_t* e, pthread_mutex_t* mu);

void destroy_event(event_t* e);

void lock_event(event_t* e);

void unlock_event(event_t* e);

// reset |e->cond|
void reset_event(event_t* e);

// Set |e->cond|.
// precondition: |e->cv_mutex| is locked.
// postcondition: |e->cv_mutex| is still locked after this function call.
void signal_event(event_t* e);

// Block waiting for |e->cond| to be 1.
//
// precondition: |e->cv_mutex| is unlocked.
// postcondition: |e->cv_mutex| is *locked*. Caller MUST call
// unlock_event() later on.
void wait_event(event_t* e);

// Block waiting for |e->cond| to be 1, for a maximum of |sec| + |nano| period.
// Returns:
//   1: on success
//  -1: on timeout
//
// precondition: |e->cv_mutex| is unlocked.
// postcondition: |e->cv_mutex| is *locked*. Caller MUST call
// unlock_event() later on.
int wait_timeout_event(event_t* e, int sec, int nano);

#endif  // EVENT_H
