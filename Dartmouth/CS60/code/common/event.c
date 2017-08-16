#include "event.h"

#include <assert.h>
#include <errno.h>
#include <string.h>  // memset, memcpy
#include <sys/time.h>
#include <time.h>

void init_event(event_t* e) { init_event_with_mu(e, NULL); }

void init_event_with_mu(event_t* e, pthread_mutex_t* mu) {
  if (mu == NULL) {
    e->cv_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(e->cv_mutex, NULL);
    e->own_mutex = 1;
  } else {
    e->cv_mutex = mu;
    e->own_mutex = 0;
  }
  pthread_cond_init(&(e->cv), NULL);
  e->cond = 0;
}

void destroy_event(event_t* e) {
  pthread_cond_destroy(&(e->cv));
  if (e->own_mutex) {
    pthread_mutex_destroy(e->cv_mutex);
    free(e->cv_mutex);
  }
  e->cv_mutex = NULL;
  e->own_mutex = -1;
  e->cond = 0;
}

void lock_event(event_t* e) { pthread_mutex_lock(e->cv_mutex); }

void unlock_event(event_t* e) { pthread_mutex_unlock(e->cv_mutex); }

void reset_event(event_t* e) { e->cond = 0; }

void signal_event(event_t* e) {
  // assert(e->cond == 0);
  e->cond = 1;
  pthread_cond_signal(&(e->cv));
}

void wait_event(event_t* e) {
  lock_event(e);
  while (e->cond == 0) {
    pthread_cond_wait(&(e->cv), e->cv_mutex);
  }
  reset_event(e);
}

#define NANO_PER_SEC_ 1000000000

static void get_abs_ts_for_cond_wait(int sec, int nano, struct timespec* ts) {
  struct timeval now;
  gettimeofday(&now, NULL);

  long secl = now.tv_sec + sec;
  long nanol = now.tv_usec * 1000 + nano;

  ts->tv_sec = secl + (nanol / NANO_PER_SEC_);
  ts->tv_nsec = (nanol % NANO_PER_SEC_);
}

#undef NANO_PER_SEC_

int wait_timeout_event(event_t* e, int sec, int nano) {
  lock_event(e);
  if (e->cond == 1) return 1;

  // struct timeval now_tv;
  // gettimeofday(&now_tv, NULL);

  struct timespec abs_ts;
  get_abs_ts_for_cond_wait(sec, nano, &abs_ts);

  // printf("now=%ld.%d\n", now_tv.tv_sec, now_tv.tv_usec);
  int result = 0;
  while (result == 0) {
    int rc = pthread_cond_timedwait(&(e->cv), e->cv_mutex, &abs_ts);
    if (e->cond == 1) {
      result = 1;
      reset_event(e);
    } else if (rc == ETIMEDOUT) {
      // gettimeofday(&now_tv, NULL);
      // printf("event timeout! now=%ld.%d\n", now_tv.tv_sec, now_tv.tv_usec);
      result = -1;
    }
  }
  return result;
}