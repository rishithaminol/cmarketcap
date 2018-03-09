#ifndef HTTPD_H_
#define HTTPD_H_

#include <sqlite3.h>

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

extern int __cb_main_thread(sqlite3 *db);
extern void error_handle(char *err);
extern void *__cb_read_from_client(void *sock);
extern int write_to_client(int sockfd);
extern void parse_http_header(char *buff, struct myhttp_header *header);
extern int check_http_header(struct myhttp_header *header);
extern void send_header(int, char *, char *);
extern void send_json_response(int sockfd, struct myhttp_header *header, sqlite3 *db);
extern struct uri_base *tokenize_uri(const char *uri);
extern void print_uri_base(struct uri_base *ub);
extern void free_uri_base(struct uri_base *ub);

#endif