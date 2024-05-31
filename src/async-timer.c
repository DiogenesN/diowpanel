/* Run a function asynchronously after n seconds delay
 * example:
 * declare a function_test();
 * run function_test(); after 5 seconds delay:
 * async_timer(function_test, 5);
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void async_timer(void (*timer_func)(int), size_t seconds) {
	struct sigevent sev;
	struct itimerspec timer_spec;
	timer_t timer_id;

	/// Set up the signal event to use the SIGALRM signal
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = &timer_id; // Optional, can be NULL

	/// Create the timer
	if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1) {
		perror("timer_create");
		return;
	}

	/// Set up the timer expiration and interval
	timer_spec.it_value.tv_sec = seconds; // Initial expiration time (5 seconds)
	timer_spec.it_value.tv_nsec = 0;
	timer_spec.it_interval.tv_sec = 0; // No repeat interval
	timer_spec.it_interval.tv_nsec = 0;

	/// Start the timer
	if (timer_settime(timer_id, 0, &timer_spec, NULL) == -1) {
		perror("timer_settime");
		return;
	}

	/// Set up the signal handler for SIGALRM
	signal(SIGALRM, timer_func);
}
