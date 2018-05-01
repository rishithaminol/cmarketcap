/**
 * @file signal_handler.h
 *
 * @brief Add signal handling functionalities to the program.
 *
 * @detail In unexpected exits the program have to close
 *		   all openned listenning socket file descriptors.
 *		   If we couldn't close thos file descriptors next
 *		   start may have errors for some time.
 *
 */

#ifndef SIGNAL_HANDLER_H_
#define SIGNAL_HANDLER_H_

extern void init_sighandlers();

#endif

