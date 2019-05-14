#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <limits.h>
#include "timer.h"

int min_heap_elt_is_top_(const struct timer *t);
int min_heap_reserve_(struct min_heap_t* s, unsigned n);
struct timer* min_heap_pop_(struct min_heap_t* s);
int min_heap_push_(struct min_heap_t* s, struct timer *t);
int min_heap_erase_(struct min_heap_t* s, struct timer *t);
int min_heap_adjust_(struct min_heap_t *s, struct timer *t);
void min_heap_shift_up_unconditional_(struct min_heap_t* s, unsigned hole_index, struct timer* t);

void min_heap_ctor_(struct min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor_(struct min_heap_t* s) { if (s->p) free(s->p); }
void min_heap_elem_init_(struct timer* t) { t->min_heap_idx = -1; }
int min_heap_empty_(struct min_heap_t* s) { return 0u == s->n; }
unsigned min_heap_size_(struct min_heap_t* s) { return s->n; }
struct timer* min_heap_top_(struct min_heap_t* s) { return s->n ? *s->p : 0; }

int min_heap_elt_is_top_(const struct timer *t)
{
	return t->min_heap_idx == 0;
}

int min_heap_reserve_(struct min_heap_t* s, unsigned n)
{
	if (s->a < n)
	{
		struct timer** p;
		unsigned a = s->a ? s->a * 2 : 8;
		if (a < n)
			a = n;
		if (!(p = (struct timer**)realloc(s->p, a * sizeof *p)))
			return -1;
		s->p = p;
		s->a = a;
	}
	return 0;
}

void min_heap_shift_down_(struct min_heap_t* s, unsigned hole_index, struct timer* t)
{
    unsigned min_child = 2 * (hole_index + 1);
    while (min_child <= s->n)
	{
		min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
		if (!(min_heap_elem_greater(t, s->p[min_child])))
			break;
		(s->p[hole_index] = s->p[min_child])->min_heap_idx = hole_index;
		hole_index = min_child;
		min_child = 2 * (hole_index + 1);
	}
    (s->p[hole_index] = t)->min_heap_idx = hole_index;
}

struct timer* min_heap_pop_(struct min_heap_t* s)
{
	if (s->n)
	{
		struct timer* t = *s->p;
		min_heap_shift_down_(s, 0u, s->p[--s->n]);
		t->min_heap_idx = -1;
		return t;
	}
	return 0;
}

int min_heap_erase_(struct min_heap_t* s, struct timer *t)
{
	if (-1 != t->min_heap_idx)
	{
		struct timer *last = s->p[--s->n];
		unsigned parent = (t->min_heap_idx - 1) / 2;
		/* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
		if (t->min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
			min_heap_shift_up_unconditional_(s, t->min_heap_idx, last);
		else
			min_heap_shift_down_(s, t->min_heap_idx, last);
		t->min_heap_idx = -1;
		return 0;
	}
	return -1;
}

int min_heap_adjust_(struct min_heap_t *s, struct timer *t)
{
	if (-1 == t->min_heap_idx) {
		return min_heap_push_(s, t);
	} else {
		unsigned parent = (t->min_heap_idx - 1) / 2;
		/* The position of e has changed; we shift it up or down
		 * as needed.  We can't need to do both. */
		if (t->min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], t))
			min_heap_shift_up_unconditional_(s, t->min_heap_idx, t);
		else
			min_heap_shift_down_(s, t->min_heap_idx, t);
		return 0;
	}
}

void min_heap_shift_up_unconditional_(struct min_heap_t* s, unsigned hole_index, struct timer* t)
{
    unsigned parent = (hole_index - 1) / 2;
    do
    {
		(s->p[hole_index] = s->p[parent])->min_heap_idx = hole_index;
		hole_index = parent;
		parent = (hole_index - 1) / 2;
    } while (hole_index && min_heap_elem_greater(s->p[parent], t));
    
	(s->p[hole_index] = t)->min_heap_idx = hole_index;
}

void min_heap_shift_up_(struct min_heap_t* s, unsigned hole_index, struct timer* t)
{
    unsigned parent = (hole_index - 1) / 2;
    while (hole_index && min_heap_elem_greater(s->p[parent], t))
    {
		(s->p[hole_index] = s->p[parent])->min_heap_idx = hole_index;
		hole_index = parent;
		parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = t)->min_heap_idx = hole_index;
}

int min_heap_push_(struct min_heap_t* s, struct timer *t)
{
	if (min_heap_reserve_(s, s->n + 1))
		return -1;
	min_heap_shift_up_(s, s->n++, t);
	return 0;
}

struct timer_base *timer_base_create(void)
{
	struct timer_base *base;

	base = (struct timer_base *)malloc(sizeof *base);
	if (!base)
		return NULL;

	memset(base, 0x0, sizeof *base);

	return base;
}

static void wq_init(struct list_head *workq)
{
	if (workq)
		INIT_LIST_HEAD(workq);
}

static void wq_enqueue(struct list_head *workq, struct callback *cb)
{
	if (workq && cb)
		list_add_tail(&cb->list, workq);
}

static inline void wq_remove_work(struct callback *cb)
{
	list_del(&cb->list);
}

static struct callback *wq_dequeue(struct list_head *workq)
{
	struct list_head *pos;
	struct callback *cb = NULL;

	if (workq) {
		pos = workq->next;
		if (pos != workq) {
			cb = list_entry(pos, struct callback, list);
			wq_remove_work(cb);
		}
	}

	return cb;
}

int add_timer(struct timer *t)
{
	if (!t && !t->cb)
		return -1;

	t->min_heap_idx = -1;
	struct timer_base *base = t->tm_base;
	gettimeofday(&t->last_time, NULL);

	memset(&t->cb->list, 0x0, sizeof(struct list_head));

	t->flags |= TIMER_INSERTED;

	base->timer_count++;
	min_heap_push_(&base->timer_heap, t);

	return 0;
}

int del_timer(struct timer *t)
{
	if (!t || !t->cb)
		return -1;

	if (t->min_heap_idx == -1)
		return 0;

	struct timer_base *base = t->tm_base;

	t->flags = 0;
	base->timer_count--;
	min_heap_erase_(&base->timer_heap, t);

	return 0;
}

int mod_timer();

static int epoll_add_event(int epfd, int fd)
{
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN;

	return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}

static int epoll_del_event(int epfd, int fd)
{
	return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

int timer_base_init(struct timer_base *base)
{
	if (!base)
		return -1;

	base->epfd = epoll_create(1000);
	if (base->epfd <= 0)
		return base->epfd;

	if (epoll_add_event(base->epfd, -1))
		//printf("add fd to epfd fialed!\n");
		//return -1;

	pthread_mutex_init(&base->lock, NULL);
	wq_init(&base->work_queue);

	return 0;
}

void timer_base_destroy(struct timer_base *base)
{
	if (base) {
		pthread_mutex_destroy(&base->lock);
		epoll_del_event(base->epfd, -1);

		free(base);
	}
}

long tv_to_msec(const struct timeval *tv)
{
	if (tv->tv_usec > 1000000 || tv->tv_sec > ((LONG_MAX) - 999) / 1000)
		return -1;

	return (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
}

static int
timeout_next(struct timer_base *base, struct timeval **ptv)
{
	struct timeval now;
	struct timeval diff;
	struct timer *t;
	struct timeval *tv = *ptv;
	int res = 0;

	t = min_heap_top_(&base->timer_heap);
	if (!t) {
		*ptv = NULL;
		goto out;
	}

	gettimeofday(&now, NULL);
	_timersub(&now, &t->last_time, &diff);

	if (_timercmp(&t->ev_timeout, &diff, <=)) {
		timerclear(tv);
		goto out;
	}

	_timersub(&t->ev_timeout, &diff, tv);

out:
	return res;
}

void timeout_process(struct timer_base *base)
{
	struct timeval now;
	struct timeval diff;
	struct timer *t;

	if (min_heap_empty_(&base->timer_heap))
		return;

	gettimeofday(&now, NULL);

	while ((t = min_heap_top_(&base->timer_heap))) {
		_timersub(&now, &t->last_time, &diff);
		if (_timercmp(&t->ev_timeout, &diff, >))
			break;
		
		if (base->current_event == t->cb) {
			wq_remove_work(t->cb);
			base->active_count--;
		}

		wq_enqueue(&base->work_queue, t->cb);
		base->active_count++;
		del_timer(t);
	}
}

int timer_base_dispatch(struct timer_base *base)
{
	int ret = -1;
	struct timeval tv;
	struct timeval *ptv;
	long timeout = -1;

	if (!base)
		return ret;

	for (;;) {
		ptv = &tv;
		
		/* if no active tivers, wait timer, if we have active timers,
		 * just poll new timers without waiting.
		 */
		if (base->active_count == 0)
			timeout_next(base, &ptv);
		else
			timerclear(&tv);

		/* If we have no events, we just exit */
		if (base->timer_count == 0 && base->active_count == 0)
			goto done;

		timeout = tv_to_msec(ptv);
		if (timeout < 0 || timeout > MAX_EPOLL_TIMEOUT_MSEC) {
			timeout = MAX_EPOLL_TIMEOUT_MSEC;
		}

		ret = epoll_wait(base->epfd, base->events, 1000, timeout);
		if (ret == -1)
			goto done;

		timeout_process(base);

		if (base->active_count) {
			struct callback *cb;
			while ((cb = wq_dequeue(&base->work_queue)) != NULL) {
				base->current_event = cb;
				cb->cb(cb->data);
				base->current_event = NULL;
				base->active_count--;
			}
		}
	}

done:
	return ret;
}



