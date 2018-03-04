#ifndef SIGNAL_HANDLER_H_
#define SIGNAL_HANDLER_H_

extern void handle_signal(int signal);
extern void handle_sigalrm(int signal);
extern void do_sleep(int seconds);

#endif

