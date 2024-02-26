#pragma once

#include <stdbool.h>
#include <time.h>
struct coro;
typedef int (*coro_f)(void *);

/** Make current context scheduler. */
void
coro_sched_init(void);

struct timespec get_start_time();
double get_latency();

void set_start_time(struct timespec t);

void set_full_time(double t);
double get_full_time(struct coro* c);
int get_id(struct coro* c);

/**
 * Block until any coroutine has finished. It is returned. NULl,
 * if no coroutines.
 */
struct coro *
coro_sched_wait(void);

/** Currently working coroutine. */
struct coro *
coro_this(void);

/**
 * Create a new coroutine. It is not started, just added to the
 * scheduler.
 */
struct coro *
coro_new(coro_f func, void *func_arg, double latency, bool reuse, struct coro *source, int id);

/** Return status of the coroutine. */
int
coro_status(const struct coro *c);

void set_func_arg(void *func_arg, struct coro *c);

long long
coro_switch_count(const struct coro *c);

/** Check if the coroutine has finished. */
bool
coro_is_finished(const struct coro *c);

/** Free coroutine stack and it itself. */
void
coro_delete(struct coro *c);

/** Switch to another not finished coroutine. */
void
coro_yield(void);

//void coro_set_quantum(unsigned int latency);
