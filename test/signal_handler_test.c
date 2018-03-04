#include <stdio.h>
#include <stdlib.h>
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h> // alarm()

#include <signal_handler.h>

/* Usage example
 *
 * First, compile and run this program:
 *     $ gcc signal.c
 *     $ ./a.out
 *
 * It will print out its pid. Use it from another terminal to send signals
 *     $ kill -HUP <pid>
 *     $ kill -USR1 <pid>
 *     $ kill -ALRM <pid>
 *
 * Exit the process with ^C ( = SIGINT) or SIGKILL, SIGTERM
 */
int main()
{
	struct sigaction sa;

	// Print pid, so that we can send signals from other shells
	printf("My pid is: %d\n", getpid());

	// Setup the sighub handler
	sa.sa_handler = &handle_signal;

	// Restart the system call, if at all possible
	sa.sa_flags = SA_RESTART;

	// Block every signal during the handler
	sigfillset(&sa.sa_mask);

	// Intercept SIGHUP and SIGINT
	if (sigaction(SIGHUP, &sa, NULL) == -1) {
		perror("Error: cannot handle SIGHUP"); // Should not happen
	}

	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("Error: cannot handle SIGUSR1"); // Should not happen
	}

	// Will always fail, SIGKILL is intended to force kill your process
	if (sigaction(SIGKILL, &sa, NULL) == -1) {
		perror("Cannot handle SIGKILL"); // Will always happen
		printf("You can never handle SIGKILL anyway...\n");
	}

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("Error: cannot handle SIGINT"); // Should not happen
	}

	for (;;) {
		printf("\nSleeping for ~3 seconds\n");
		do_sleep(3); // Later to be replaced with a SIGALRM
	}
} /* main */

