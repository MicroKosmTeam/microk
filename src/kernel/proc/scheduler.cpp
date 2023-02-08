#include <proc/scheduler.h>
#include <stdio.h>
#include <dev/tty/tty.h>
#include <dev/timer/pit/pit.h>

#define TASK_NUMBER 4

static bool scheduler_running = false;

static void (*tasks[TASK_NUMBER]) (void *p);

static void start_function(void (*funcptr)()) {
	funcptr();
	return;
}

static void TTY_GetChar() {
	GlobalTTY->GetChar();
	return;
}

static void GOP_Refresh() {
	return;
}

static void USB_Poll() {
	return;
}

void init_scheduler() {
	tasks[0] = USB_Poll;
	tasks[1] = TTY_GetChar;
	tasks[2] = GOP_Refresh;
	tasks[3] = NULL;
	return;
}

static void run_scheduler() {
	static int task_index = 0;
	if (tasks[task_index] == NULL && task_index != 0) task_index = 0;
	if (tasks[task_index] == NULL && task_index == 0) scheduler_running = false;

	start_function(tasks[task_index]);
	PIT::Sleep(10);

	task_index++;
	return;
}

static void check_state() {
	// Do things here
	return;
}

void start_scheduler() {
	scheduler_running = true;

	while(scheduler_running) {
		run_scheduler();
		check_state();
	}
}

void scheduler_stop() {
	scheduler_running = false;
}

void scheduler_list() {
	printf("Total ktasks: %d\n"
	       "Listing addresses: \n", TASK_NUMBER);

	for (int i = 0; i < TASK_NUMBER; i++) {
		if (tasks[i] == NULL) printf(" -> %d INACTIVE\n", i);
		else printf(" -> %d ACTIVE: 0x%d\n", i, (uint64_t)tasks[i]);
	}
}
