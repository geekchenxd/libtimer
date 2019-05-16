#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "timer.h"

#define EVENT 1000

static int called = 0;
static unsigned seed = 0;

void timeout_cb(void *arg)
{
	struct timeval now;
	struct timeval diff;
	struct timer *t = (struct timer*)arg;
	if (!t) return;

	called++;

#if 1
	gettimeofday(&now, NULL);
	_timersub(&now, &t->last_time, &diff);
	time_t timeout = tv_to_msec(&diff);
	printf("call time is %lu\n", timeout);

	//t->ev_timeout.tv_sec = 1;
	//add_timer(t);
#endif
}

unsigned int seed_get()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	seed = (unsigned int)tv.tv_sec + (unsigned int)tv.tv_usec;
	seed += (unsigned int)getpid();

	return seed;
}

int rand_int(unsigned int seed, int n)
{
	int seed_ = 0;
	seed_ = (seed * 1103515245 + 123456) & 0x7fffffff;
	return seed_ % n;
}

int main(int argc, char *argv[])
{
	struct timer_base *base;
	struct callback *cb = NULL;
	struct timer *timer = NULL;
	int i;

	base = timer_base_create();
	if (!base) {
		printf("Crete timer base failed!\n");
		return -1;
	}
	timer_base_init(base);

	seed = seed_get();

	//cb = (struct callback *)malloc(sizeof *cb);
	//memset(cb, 0x0, sizeof *cb);
	//cb->cb = timeout_cb;

	for (i = 0; i < EVENT; i++) {
		timer = (struct timer *)malloc(sizeof *timer);
		if (!timer) {
			printf("Create timer failed!\n");
			break;
		}
		timer->flags = 0;
		timer->min_heap_idx = -1;
		timer->tm_base = base;
		timer->cb = (struct callback *)malloc(sizeof *cb);
		memset(timer->cb, 0x0, sizeof *cb);
		timer->cb->cb = timeout_cb;
		timer->cb->data = timer;
		timer->ev_timeout.tv_sec = 0;
		timer->ev_timeout.tv_usec = 500 * 1000;//rand_int(seed, 50000);

		if (add_timer(timer)) {
			printf("add timer failed!\n");
			break;
		}
	}

	timer_base_dispatch(base);
	
	printf("%d, %d\n", called, EVENT);

	return 0;
}
