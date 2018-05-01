/**
 * @file cm_debug.h
 *
 * @brief A common way of error reporting and debugging messages
 *
 * @detail This section handles all the error reporting
 *		   features of all sections. Using this section
 *		   the programmer can debug the code (finding the
 *		   line number, section, function name). To enable
 *		   this feature programmer need to enable 'CM_DEBUG_'
 *		   at compile time.
 *
 * @todo   Stack tracing mechanism should be implemented
 */

#ifndef CM_DEBUG_H
#define CM_DEBUG_H

/*! @brief Make text bold */
#define BOLD  "\033[1m"
/*! @brief Text color rest */
#define COLOR_RESET "\033[0m"
/*! @brief Make text color red */
#define COLOR_RED  "\033[31m"
/*! @brief Make text color green */
#define COLOR_GREEN  "\033[32m"
/*! @brief Make text color cyan */
#define COLOR_CYN  "\033[36m"

#ifdef CM_DEBUG_
extern void cm_error(const char *section, const char *func, int line_num,
	const char *err_str, ...);
extern void cm_warn(const char *section, const char *func, int line_num,
	const char *warn_str, ...);
extern void debug_msg(const char *section, const char *func, int line_num,
	const char *debug_msg, ...);
#else
extern void cm_error(const char *err_str, ...);
extern void cm_warn(const char *warn_str, ...);
#endif

#ifdef CM_DEBUG_
 #define CM_ERROR(ER_FRMT, ...) cm_error(__FILE__, __func__, __LINE__, ER_FRMT, ##__VA_ARGS__)
 #define CM_WARN(ER_FRMT, ...) cm_warn(__FILE__, __func__, __LINE__, ER_FRMT, ##__VA_ARGS__)
 #define DEBUG_MSG(DEBUG_FRMT, ...) debug_msg(__FILE__, __func__, __LINE__, DEBUG_FRMT, ##__VA_ARGS__)
#else
 #define CM_ERROR(ER_FRMT, ...) cm_error(ER_FRMT, ##__VA_ARGS__)
 #define CM_WARN(ER_FRMT, ...) cm_warn(ER_FRMT, ##__VA_ARGS__)
 #define DEBUG_MSG(x) ; /**< suppress the debug msg */
#endif

#endif 
