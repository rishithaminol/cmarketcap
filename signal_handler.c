#include <stdio.h>
#include <stdlib.h>
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h> // alarm()

#include "signal_handler.h"
#include "cmarketcap.h"

void handle_signal(int signal)
{
	const char *signal_name;
	sigset_t pending;

	// Find out which signal we're handling
	switch (signal) {
		case SIGHUP:
			signal_name = "SIGHUP";
			break;
		case SIGUSR1:
			signal_name = "SIGUSR1";
			break;
		case SIGINT:
			printf("Caught SIGINT, exiting now\n");
			close(global_data_handle.httpd_sockfd);
			exit(0);
		default:
			fprintf(stderr, "Caught wrong signal: %d\n", signal);
			return;
	}

	/*
	 * Please note that printf et al. are NOT safe to use in signal handlers.
	 * Look for async safe functions.
	 */
	printf("Caught %s, sleeping for ~3 seconds\n"
	  "Try sending another SIGHUP / SIGINT / SIGALRM "
	  "(or more) meanwhile\n", signal_name);

	/*
	 * Indeed, all signals are blocked during this handler
	 * But, at least on OSX, if you send 2 other SIGHUP,
	 * only one will be delivered: signals are not queued
	 * However, if you send HUP, INT, HUP,
	 * you'll see that both INT and HUP are queued
	 * Even more, on my system, HUP has priority over INT
	 */
	do_sleep(3);
	printf("Done sleeping for %s\n", signal_name);

	// So what did you send me while I was asleep?
	sigpending(&pending);
	if (sigismember(&pending, SIGHUP)) {
		printf("A SIGHUP is waiting\n");
	}
	if (sigismember(&pending, SIGUSR1)) {
		printf("A SIGUSR1 is waiting\n");
	}

	printf("Done handling %s\n\n", signal_name);
} /* handle_signal */

void handle_sigalrm(int signal)
{
	if (signal != SIGALRM) {
		fprintf(stderr, "Caught wrong signal: %d\n", signal);
	}

	printf("Got sigalrm, do_sleep() will end\n");
}

void do_sleep(int seconds)
{
	struct sigaction sa;
	sigset_t mask;

	sa.sa_handler = &handle_sigalrm; // Intercept and ignore SIGALRM
	sa.sa_flags   = SA_RESETHAND;    // Remove the handler after first signal
	sigfillset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	// Get the current signal mask
	sigprocmask(0, NULL, &mask);

	// Unblock SIGALRM
	sigdelset(&mask, SIGALRM);

	// Wait with this mask
	alarm(seconds);
	sigsuspend(&mask);

	printf("sigsuspend() returned\n");
}

