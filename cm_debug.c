/*! @file cm_debug.c
 *  @brief A common way of error reporting and debugging messages
 *  @detail This section handles all the error reporting
 *			features in all sections. Using this section
 *			the programmer can debug the code (finding the
 *			line number, section, function name). To enable
 *			this feature programmer need to enable CM_DEBUG_
 *			at compile time.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>

#include "cmarketcap.h"
#include "cm_debug.h"

#define BOLD  "\033[1m"
#define COLOR_RESET "\033[0m"
#define COLOR_RED  "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_CYN  "\033[36m"

#ifdef CM_DEBUG_
void cm_error(const char *section, const char *func, int line_num,
	const char *err_str, ...)
#else
void cm_error(const char *err_str, ...)
#endif
{
	va_list args;
	char debug_string[256] = "";
	char temp_string[4096];

#ifdef CM_DEBUG_
	sprintf(debug_string, "%s:%s%s()%s:%d", basename((char *)section), COLOR_RED, func, COLOR_RESET, line_num);
#endif

	va_start(args, err_str);
	vsprintf(temp_string, err_str, args);
	va_end(args);

	fprintf(stderr, "%s:%s error: %s", prog_name, debug_string, temp_string);
}

#ifdef CM_DEBUG_
void cm_warn(const char *section, const char *func,	int line_num,
	const char *warn_str, ...)
#else
void cm_warn(const char *warn_str, ...)
#endif
{
	va_list args;
	char debug_string[256] = "";
	char temp_string[4096];

#ifdef CM_DEBUG_
	sprintf(debug_string, "%s:%s:%d", basename((char *)section), func, line_num);
#endif

	va_start(args, warn_str);
	vsprintf(temp_string, warn_str, args);
	va_end(args);

	fprintf(stderr, "%s:%s warning: %s", prog_name, debug_string, temp_string);
}

#ifdef CM_DEBUG_
void debug_msg(const char *section, const char *func, int line_num,
	const char *debug_msg, ...)
{
	va_list args;
	char debug_string[256] = "";
	char temp_string[4096];

	sprintf(debug_string, "%s:%s%s()%s:%d", basename((char *)section), BOLD, func, COLOR_RESET, line_num);

	va_start(args, debug_msg);
	vsprintf(temp_string, debug_msg, args);
	va_end(args);

	fprintf(stdout, "%s:%s : %s", prog_name, debug_string, temp_string);
}
#endif
