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

/*! url key=value pair */
struct url_entry {
	size_t index;
	char *key;
	char *value;

	struct url_entry *next;
};

struct url_base {
	size_t entry_count;
	struct url_entry *first;
	struct url_entry *last;
};
/*! @} */ /* url_tokenization */

extern int __cb_main_thread(MYSQL *db, int port_num);

/*! \addtogroup url_tokenization
 *  url tokenization mechanism
 *  @{
 */
extern struct url_base *tokenize_url(const char *url);
extern void print_url_base(struct url_base *ub);
extern void free_url_base(struct url_base *ub);
/*! @} */ /* url_tokenization */

extern size_t num_of_clients();
extern void init_httpd_mutexes();

#endif
