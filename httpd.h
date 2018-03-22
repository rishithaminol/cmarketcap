#ifndef HTTPD_H_
#define HTTPD_H_

#include "mysql_api.h"

struct myhttp_header {
	char method[5];
	char url[200];
	char protocol[10];
};

/*! \addtogroup url_tokenization
 *  url tokenization mechanism
 *  @{
 */

/*! uri key=value pair */
struct uri_entry {
	size_t index;
	char *key;
	char *value;

	struct uri_entry *next;
};

struct uri_base {
	size_t entry_count;
	struct uri_entry *first;
	struct uri_entry *last;
};
/*! @} */ /* uri_tokenization */

extern int __cb_main_thread(MYSQL *db, int port_num);

/*! \addtogroup url_tokenization
 *  url tokenization mechanism
 *  @{
 */
extern struct uri_base *tokenize_uri(const char *uri);
extern void print_uri_base(struct uri_base *ub);
extern void free_uri_base(struct uri_base *ub);
/*! @} */ /* uri_tokenization */

extern size_t num_of_clients();
extern void init_httpd_mutexes();

#endif