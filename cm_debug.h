#ifndef CM_DEBUG_H
#define CM_DEBUG_H

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
 #define DEBUG_MSG(x) ; /* suppress the debug msg */
#endif

#endif 
