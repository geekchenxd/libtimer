#include <stdio.h>
#include "timer.h"


void timer1_cb(void *arg)
{
	struct timeval now;
	struct timeval diff;
	struct timer *t = (struct timer*)arg;
	if (!t) return;

	gettimeofday(&now, NULL);
	_timersub(&now, &t->last_time, &diff);
	time_t timeout = tv_to_msec(&diff);
	printf("call time is %u\n", timeout);

	t->ev_timeout.tv_sec = 1;
	add_timer(t);
}

int main(int argc, char *argv[])
{
	struct timer_base *base;

	base = timer_base_create();
	if (!base) {
		printf("Crete timer base failed!\n");
		return -1;
	}
	timer_base_init(base);

	struct callback cb[4]; /* = {
		.list.next = NULL,
		.list.prev = NULL,
		.cb = timer1_cb,
		.data = NULL,
	};*/

	struct timer timer[] = {
		{
			.cb = &cb[0],
			.flags = 0,
			.min_heap_idx = -1,
			.tm_base = base,
			.ev_timeout.tv_sec = 2,
			.ev_timeout.tv_usec = 0,
		}, {
			.cb = &cb[1],
			.flags = 0,
			.min_heap_idx = -1,
			.tm_base = base,
			.ev_timeout.tv_sec = 1,
			.ev_timeout.tv_usec = 0,
		},
		{
			.cb = &cb[2],
			.flags = 0,
			.min_heap_idx = -1,
			.tm_base = base,
			.ev_timeout.tv_sec = 3,
			.ev_timeout.tv_usec = 0,
		},
		{
			.cb = &cb[3],
			.flags = 0,
			.min_heap_idx = -1,
			.tm_base = base,
			.ev_timeout.tv_sec = 0,
			.ev_timeout.tv_usec = 1000,
		},
	};

	int i = 0;
	for (i = 0; i < 4; i++) {
		timer[i].cb->cb = timer1_cb;
		timer[i].cb->data = &timer[i];
		if (add_timer(&timer[i])) {
			printf("add timer failed!\n");
			return -1;
		}
	}

	timer_base_dispatch(base);

	return 0;
}
