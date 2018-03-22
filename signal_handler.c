#include <stdio.h>
#include <stdlib.h>
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h> // alarm()

#include "signal_handler.h"
#include "cm_debug.h"
#include "cmarketcap.h"

void handle_SIGINT(int signal)
{
	printf("Caught SIGINT, exiting now\n");
	close(global_data_handle.httpd_sockfd);
}

void init_sighandlers()
{
	if (signal(SIGINT, handle_SIGINT) == SIG_ERR)
		CM_ERROR("can't catch SIGINT\n");
}
