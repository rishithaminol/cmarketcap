#ifndef HTTPD_H_
#define HTTPD_H_

struct myhttp_header {
	char method[5];
	char filename[200];
	char protocol[10];
	char type[100];
};

extern int __cb_main_thread(sqlite3 *db);
extern void error_handle(char *err);
extern void *__cb_read_from_client(void *sock);
extern int write_to_client(int sockfd);
extern void parse_http_header(char *buff, struct myhttp_header *header);
extern int check_http_header(struct myhttp_header *header);
extern void send_header(int, char *, char *);
extern void send_json_response(int sockfd, struct myhttp_header *header, sqlite3 *db);

#endif