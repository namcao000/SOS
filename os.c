#include "os.h"
#include "armv7-m.h"
#include "thread_list.h"
#include <stdlib.h>

#define THREAD_STACK_SIZE 1024u

static uint32_t counter_ms;

void terminal(void);

static struct thread *last_active_thread;

void start_os(void)
{
	/* insert empty thread presenting this thread */
	struct thread thr;
	thr.state = ACTIVE;
	insert_thread(thr);
	last_active_thread = head_thread();

	start_thread(terminal);

	enter_os();
}

static struct thread *get_next_idle_thread(struct thread *t) {
	while (1) {
		t = next_thread(t);

		if (t == NULL)
			t = head_thread();

		if (t->state != SLEEP)
			break;
	}

	return t;
}

static struct thread *scheduler(void) {
	struct thread *n;

	if (last_active_thread == NULL)
		n = head_thread();
	else
		n = get_next_idle_thread(last_active_thread);
	
	return n;
}

struct OS_Handler_return {
	void **sp_save_addr;
	void *new_sp;
} OS_Handler(void)
{
	struct OS_Handler_return ret;

	counter_ms++;

	for (struct thread *p = head_thread(); p != NULL; p = next_thread(p)) {
		if (p->state == SLEEP &&
		    (counter_ms - p->sleep_start_time) > p->sleep_duration)
			p->state = IDLE;
	}

	struct thread *new_thread = scheduler();

	if (new_thread != last_active_thread) {
		if (last_active_thread != NULL) {
			if (last_active_thread->state != SLEEP)
				last_active_thread->state = IDLE;
			ret.sp_save_addr = &last_active_thread->stackptr;
		} else {
			ret.sp_save_addr = NULL;
		}

		ret.new_sp = new_thread->stackptr;
		new_thread->state = ACTIVE;

		last_active_thread = new_thread;
	}
	else {
		ret.new_sp = NULL;
		ret.sp_save_addr = NULL;
	}

	return ret;
}

void sleep(int duration)
{
	_sleep((uint32_t)duration);
}

void svc_sleep(uint32_t duration)
{
	for (struct thread *p = head_thread(); p != NULL; p = next_thread(p)) {
		if (p->state == ACTIVE) {
			p->state = SLEEP;
			p->sleep_start_time = counter_ms;
			p->sleep_duration = (int)duration;
			break;
		}
	}
}

uint32_t get_ms_counter(void)
{
	return counter_ms;
}

void start_thread(void (*addr)(void))
{
	_start_thread((uint32_t)addr);
}

static void *align8(void *ptr)
{
	uintptr_t p = (uintptr_t)ptr;
	p = p & ~(uintptr_t)8;
	return (void *)p;
}

void svc_start_thread(void (*addr)(void))
{
	struct thread thr;
	void *stack = malloc(THREAD_STACK_SIZE);
	thr.stack_start = stack;

	stack = align8(stack + THREAD_STACK_SIZE);

	stack = init_thread_stack(stack, addr);

	thr.stackptr = stack;
	thr.state = IDLE;

	insert_thread(thr);
}

void svc_end_thread(void) {
	free(last_active_thread->stack_start);
	remove_thread(last_active_thread);
	last_active_thread = NULL;
}
