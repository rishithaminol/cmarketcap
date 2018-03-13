#include <sys/socket.h>
#include <sys/types.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "cm_debug.h"
#include "sql_api.h"
#include "httpd.h"
#include "cmarketcap.h"

#define RD_BUFF_MAX 1024
#define CLIENT_MAX  10
#define MAX_URI_SIZE 2048

/**! this locks others while shift_columns in action. */
#define LOCK_SHIFT_COLUMN_LOCKER pthread_mutex_lock(&shift_column_locker)
#define UNLOCK_SHIFT_COLUMN_LOCKER pthread_mutex_unlock(&shift_column_locker)

void error_handle(char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}

/** @brief Parse http headers into a data structure.
 *
 * This function is called by multiple threads. All modifications should be
 * thread safe
 *
 */
void parse_http_header(char *buff, struct myhttp_header *header)
{
	char *token;
	char *line = NULL;

	/* fetch the first line. like 'GET /path HTTP/1.1' */
	line = strtok_r(line, "\n", &buff);
	DEBUG_MSG("request line = \"%s\"\n", line);

	/* parse line */
	token = strtok_r(line, " ", &line);
	strcpy(header->method, token);
	token = strtok_r(line, " ", &line);
	strcpy(header->url, token);
	token = strtok_r(line, " ", &line);
	strcpy(header->protocol, token);
}

int check_http_header(struct myhttp_header *header)
{
	return 0;
}

/* @brief Send http headers to the client
 *
 * called by send_json_response
 */
void send_header(int sockfd, char *code, char *type)
{
	char buf[100];

	if (strcmp(code, "200") == 0) {
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "400") == 0) {
		strcpy(buf, "HTTP/1.0 404 Bad Request\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "403") == 0) {
		strcpy(buf, "HTTP/1.0 403 Forbidden\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "404") == 0) {
		strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	}
	sprintf(buf, "\r\n");
	send(sockfd, buf, strlen(buf), 0);
}

/*! \addtogroup url_tokenization
 *  url tokenization mechanism
 *  @{
 */
static struct uri_base *init_uri_base()
{
	struct uri_base *x = (struct uri_base *)malloc(sizeof(struct uri_base));
	if (!x) {
		CM_ERROR("memory allocation\n");
		return NULL;
	}

	x->entry_count = 0;
	x->first = NULL;
	x->last = NULL;

	return x;
}

static struct uri_entry *init_uri_entry()
{
	struct uri_entry *x = (struct uri_entry *)malloc(sizeof(struct uri_entry));
	if (!x) {
		CM_ERROR("memory allocation\n");
		return NULL;
	}

	x->next = NULL;

	return x;
}

/**
 * @brief expects 'hello=world' like string. expects '\0' terminated string
 *
 * @return On error returns NULL indicating invalid 'key=value' pair
 */
static struct uri_entry *mk_uri_entry(char *uri_e)
{
	struct uri_entry *x;
	char *value;
	char *key;

	value = strchr(uri_e, '=') + 1;
	if (!value) {
		CM_ERROR("Invalid uri_entry\n");
		return NULL;
	}

	*(value - 1) = '\0';
	key = uri_e;

	x = init_uri_entry();
	x->key = strdup(key);
	x->value = strdup(value);

	return x;
}

/**
 * @brief append the given uri entry at the end of the linked list 
 *
 *	@param[in] eb uri_base
 *	@param[in] cd uri_entry
 */
static void append_uri_entry(struct uri_base *ub, struct  uri_entry *ue)
{
	if (ub->first == NULL) {/* first and last = NULL */
		ub->first = ue;
		ub->last = ub->first; /* initially first == last */
	} else {
		ub->last->next = ue;
		ub->last = ub->last->next;
	}

	ub->last->index = (size_t)ub->entry_count++;
}

/* @brief delimeter charachter is '&' 
 *
 * '/any/path?hello=world&rishitha=minol' breaks this string using '&'
 * and pass 'key=value' like strings to 'mk_uri_entry()'
 *
 * @param uri This string gets modified.
 */
struct uri_base *tokenize_uri(const char *uri)
{
	char *x = NULL;
	char *y;
	char uri_[MAX_URI_SIZE];
	struct uri_entry *t;

	struct uri_base *uri_base = init_uri_base();

	strncpy(uri_, uri, MAX_URI_SIZE);

	x = strchr(uri_, '?') + 1; /**! start of the url options */
							  /* 'hello=world&rishitha=minol' */
	if (!x) {
		CM_ERROR("path string\n");
		free(uri_base);
		return NULL;
	}

	while (x != NULL) {
		y = x;
		x = strchr(x, '&');
		if (x) {
			*x = '\0';
			x++;
		}

		t = mk_uri_entry(y); /* hello=world */
		if (t != NULL) {
			append_uri_entry(uri_base, t);
		}
	}

	return uri_base;
}

/* @brief freeup 'struct uri_base *' */
void free_uri_base(struct uri_base *ub)
{
	struct uri_entry *entry, *t;

	entry = ub->first;
	free(ub);

	while (entry != NULL) {
		free(entry->key);
		free(entry->value);

		t = entry->next;
		free(entry);

		entry = t;
	}
}

void print_uri_base(struct uri_base *ub)
{
	struct uri_entry *t;

	t = ub->first;
	while (t != NULL) {
		printf("index: %lu, \"%s\":\"%s\"\n", t->index, t->key, t->value);
		t = t->next;
	}
}
/*! @} */ /* uri_tokenization */

/* @brief Send response to the user.
 *
 * @param[in] header Used for reading path name (URL parameters).
 *			  header has user reqested data.
 * @param[in] sockfd File descriptor to write (Openned connection fd)
 *
 * We have to write a functio to tokenize url data.
 */
void send_json_response(int sockfd, struct myhttp_header *header, sqlite3 *db)
{
	char code[4];
	struct coin_status_base *sb;
	struct coin_status *t = NULL;
	char tempstr[200];
	int full_rank = 0;

	strcpy(code, "200");
	send_header(sockfd, code, "application/json");

	/* example url sets 
	 * 
	 * /home/path?rank=full
	 * /home/path?coinid=bitcoin&start=min_5&limit=min_30
	 */
	struct uri_base *tokens;
	struct uri_entry *te_entry;
	tokens = tokenize_uri(header->url);
	if (tokens == NULL) {
		CM_ERROR("tokenization malfunction\n");
		return;
	}

	te_entry = tokens->first;
	if (strcmp(te_entry->key, "rank") == 0) {
		LOCK_SHIFT_COLUMN_LOCKER;
		if (strcmp(te_entry->value, "full") == 0) { /* /home/path/?rank=full */
			sb = fetch_entire_rank(db);
			full_rank = 1;
		} else { /* /home/path/?rank=min_5 ... etc.. */
			sb = fetch_duration(db, "min_0", te_entry->value);
		}
		UNLOCK_SHIFT_COLUMN_LOCKER;
		t = sb->first;
	} else if (strcmp(te_entry->key, "coinid") == 0) { /* /home/path/?coinid=bitcoin&range=1 or 2 */
		char *coin_id = te_entry->value;
		te_entry = te_entry->next;

		if (strcmp(te_entry->key, "range") == 0) {
			char *range = te_entry->value;

			if (strcmp(range, "1") == 0){
				LOCK_SHIFT_COLUMN_LOCKER;
				fetch_range_level1(coin_id, db, sockfd);
				UNLOCK_SHIFT_COLUMN_LOCKER;
				free_uri_base(tokens);

				return;
			}
		} else {
			CM_ERROR("invalid option for '%s' coinid\n", coin_id);
			return;
		}
	} else {
		CM_ERROR("tokenization malfunction\n");
		return;
	}
	free_uri_base(tokens);

	if (t == NULL) {
		write(sockfd, "{\"error\": \"error occured\"}\n", strlen("{\"error\": \"error occured\"}\n"));
		return;
	}
	while (t != NULL) {
		if (full_rank > 0)
			sprintf(tempstr, "#%d:%s\n", t->col1_rank, t->coin_id);
		else
			sprintf(tempstr, "#%s: %d (%d)\n", t->coin_id, t->col1_rank, t->col2_rank);
		write(sockfd, tempstr, strlen(tempstr));
		t = t->next;
	}

	free_coin_status_base(sb);
} /* send_json_response */

struct __cb_args {
	int socket;
	sqlite3 *db;
};

/* @brief Callback function for 'pthread_create' */
void *__cb_read_from_client(void *cb_arg)
{
	char buffer[RD_BUFF_MAX];
	int nbytes;
	struct myhttp_header header; /* HEADER */
	int sockfd;
	struct __cb_args *_cb_arg = (struct __cb_args *)cb_arg;

	sockfd = _cb_arg->socket;
	sqlite3 *db = _cb_arg->db;

	pthread_detach(pthread_self());

	DEBUG_MSG("waiting for client to write...\n");
	while ((nbytes = read(sockfd, buffer, RD_BUFF_MAX))) {
		DEBUG_MSG("client wrote %d bytes\n", nbytes);
		if (nbytes < 0) {
			CM_ERROR("socket read failed\n");
			break;
		} else if (nbytes == 0) {
			DEBUG_MSG("Client wrote 0 bytes\n");
			break;
		} else {
			if (check_http_header(&header) < 0) {
				send_header(sockfd, "400", "text/html"); /* bad request */
				break;
			}
			parse_http_header(buffer, &header); /**! parse_http_header */

			send_json_response(sockfd, &header, db); /* buffer and header should be filled with zeros */

			break;
		}
	}

	close(sockfd);
	free(_cb_arg);
	DEBUG_MSG("Connection closed\n");

	pthread_exit(NULL);
}

int write_to_client(int sockfd)
{
	return 0;
}

/* @brief needs openned database */
int __cb_main_thread(sqlite3 *db)
{
	int httpd_sockfd;
	int httpd_port;
	int sockfd;

	struct sockaddr_in httpd_sockaddr;
	struct sockaddr_in client_sockaddr;
	socklen_t clilen = sizeof(struct sockaddr_in);;

	pthread_t httpd_thread; /**! store thread id */

	//pthread_detach(pthread_self());

	/* port number has to between 1024 and 65536 (not included) */
	httpd_port = 1040;

	/* init server socket (ipv4, tcp) */
	httpd_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (httpd_sockfd < 0) {
		CM_ERROR("httpd socket failed to create\n");
		exit(1);
	}

	memset(&httpd_sockaddr, 0, sizeof(httpd_sockaddr)); /* Fill zeros */
	httpd_sockaddr.sin_family      = AF_INET;
	httpd_sockaddr.sin_port        = htons(httpd_port);
	httpd_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd_sockfd, (struct sockaddr *)&httpd_sockaddr,
		sizeof(httpd_sockaddr)) == -1) {
		close(httpd_sockfd);
		CM_ERROR("bind failed\n");
		exit(1);
	}

	if (listen(httpd_sockfd, CLIENT_MAX) == -1) {
		close(httpd_sockfd);
		CM_ERROR("listen failed\n");
		exit(1);
	}

	/* at this stage 'httpd_sockfd', 'httpd_sockaddr', 'httpd_port', 'clilen'
	 * settled up */
	while (1) {
		struct __cb_args *cb_arg = NULL;
		sockfd = accept(httpd_sockfd, (struct sockaddr *)&client_sockaddr, &clilen);
		if (sockfd == -1) {
			CM_ERROR("Connection accept error\n");
			continue;
		} else {
			DEBUG_MSG("Client accepted connection\n");
			cb_arg = (struct __cb_args *)malloc(sizeof(struct __cb_args));
			cb_arg->socket = sockfd;
			cb_arg->db = db;
		}

		/* @todo additional thread destruction should be started from here */
		if (pthread_create(&httpd_thread, NULL, (void *)__cb_read_from_client,
			(void *)cb_arg) != 0) {
			CM_ERROR("pthread create error\n");
			pthread_exit(NULL);
		} else {
			DEBUG_MSG("New thread created (%d)\n", (int)httpd_thread);
		}
	}

	close(httpd_sockfd);

	//pthread_exit(NULL);
	return 0;
} /* main */
