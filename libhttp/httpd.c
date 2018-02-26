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

#define RD_BUFF_MAX 1024
#define CLIENT_MAX  10

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
	DEBUG_MSG("line = %s\n", line);

	/* parse line */
	token = strtok_r(line, " ", &line);
	strcpy(header->method, token);
	token = strtok_r(line, " ", &line);
	strcpy(header->filename, token);
	token = strtok_r(line, " ", &line);
	strcpy(header->protocol, token);

	/* find request file type */
	token = strtok_r(buff, "\n", &buff);
	if ((token = strstr(buff, "Accept:")) != NULL) {
		token = strtok_r(token, "\n", &token);
	}

	/* pay attention. No error but unwanted coding. */
	token = strtok_r(token, " ", &token); /* 'Accept:' */
	token = token + strlen(token) + 1;
	token = strtok_r(token, ",", &token);
	DEBUG_MSG("token = %s\n", token);
	strcpy(header->type, token);
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
	struct coin_status *t;
	char tempstr[200];

	strcpy(code, "200");
	send_header(sockfd, code, "application/json");
	/* SQL output as json */
	sb = fetch_duration(db, "min_0", "min_10");

	t = sb->first;

	write(sockfd, "[\n", strlen("[\n"));
	while (t != NULL) {
		sprintf(tempstr, "\t{\n"
			   "\t\t\"coin_id\": \"%s\",\n"
			   "\t\t\"%s_rank\": %d,\n"
			   "\t\t\"%s_rank\": %d\n"
			   "\t}",
			   t->coin_id, t->col1, t->col1_rank, t->col2, t->col2_rank);
		write(sockfd, tempstr, strlen(tempstr));
		t = t->next;

		if(t != NULL)
			write(sockfd, ",\n", strlen(",\n"));
	}
	write(sockfd, "]\n", strlen("]\n"));

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
				send_header(sockfd, "400", header.type); /* bad request */
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

	pthread_t httpd_thread; /**! store thread ids */

	pthread_detach(pthread_self());

	/* port */
	httpd_port = 1040;
	if (httpd_port <= 1024 || httpd_port >= 65536) {
		CM_ERROR("port number has to between 1024 and 65536 (not included)\n");
		exit(1);
	}

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

		if (pthread_create(&httpd_thread, NULL, (void *)__cb_read_from_client,
			(void *)cb_arg) != 0) {
			CM_ERROR("pthread create error\n");
			pthread_exit(NULL);
		} else {
			DEBUG_MSG("New thread created (%d)\n", (int)httpd_thread);
		}
	}

	close(httpd_sockfd);

	pthread_exit(NULL);
} /* main */
