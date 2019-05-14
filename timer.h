#ifndef __TIMER__H__
#define __TIMER__H__

#include <pthread.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include "list.h"

#define TIMER_INSERTED 0x1
#define TIMER_ACTIVE 0x2

#define _timeradd(tvp, uvp, vvp)	\
	do {	\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;	\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {	\
			(vvp)->tv_sec++;	\
			(vvp)->tv_usec -= 1000000;	\
		}	\
	} while(0)

#define	_timersub(tvp, uvp, vvp)					\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define	_timercmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :				\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))
	 
#define min_heap_elem_greater(a, b) \
	(timercmp(&(a)->ev_timeout, &(b)->ev_timeout, >))

#define fimerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0

#define FDSIZE 1000
/* On Linux kernels at least up to 2.6.24.4, epoll can't handle timeout
 * values bigger than (LONG_MAX - 999ULL)/HZ.  HZ in the wild can be
 * as big as 1000, and LONG_MAX can be as small as (1<<31)-1, so the
 * largest number of msec we can support here is 2147482.  Let's
 * round that down by 47 seconds.
 */
#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

struct min_heap_t {
	struct timer **p;
	unsigned n, a;
};

struct timer_base {
	struct min_heap_t timer_heap;
	/** Stored timeval: used to avoid calling gettimeofday/clock_gettime
	 * too often. */
	struct timeval tv_cache;
	pthread_mutex_t lock;

	int timer_count;
	int epfd;
	struct epoll_event events[FDSIZE];

	void *current_event;
	unsigned int current_event_waiters;
	unsigned int active_count;
	struct list_head work_queue;
};

struct callback {
	struct list_head list;
	void (*cb)(void *arg);
	void *data;
};

struct timer {
	struct callback *cb;
	int flags;
	int min_heap_idx;
	struct timer_base *tm_base;
	struct timeval ev_timeout;
	struct timeval last_time;
};


int add_timer(struct timer *t);
struct timer_base *timer_base_create(void);
int timer_base_init(struct timer_base *base);
void timer_base_destroy(struct timer_base *base);
int timer_base_dispatch(struct timer_base *base);

#endif
