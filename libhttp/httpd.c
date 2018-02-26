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

#include "../cm_debug.h"

#define RD_BUFF_MAX 1024
#define CLIENT_MAX  10

const char dumy_json[] = "{"
						 "	\"coin_id\":\"bitcoin\","
						 "	\"min_0_rank\":1,"
						 "	\"min_5_rank\":2 "
						 "}";

struct myhttp_header {
	char method[5];
	char filename[200];
	char protocol[10];
	char type[100];
};

void error_handle(char *err);
void *__cb_read_from_client(void *sock);
int write_to_client(int sockfd);
void parse_http_header(char *buff, struct myhttp_header *header);
int check_http_header(struct myhttp_header *header);
void send_header(int, char *, char *);
void send_response(int buff, struct myhttp_header *header);

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
	fprintf (stdout, "line = %s\n", line);

	DEBUG_MSG("buff = %s\n", buff);
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
	printf("token = %s\n", token);
	strcpy(header->type, token);
}

int check_http_header(struct myhttp_header *header)
{
	return 0;
}

/* @brief Send http headers to the client
 *
 * called by send_response
 */
void send_header(int sockfd, char *code, char *type)
{
	char buf[100];

	if (strcmp(code, "200") == 0) {
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "400") == 0)   {
		strcpy(buf, "HTTP/1.0 404 Bad Request\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "403") == 0)   {
		strcpy(buf, "HTTP/1.0 403 Forbidden\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "404") == 0)   {
		strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	}
	sprintf(buf, "\r\n");
	send(sockfd, buf, strlen(buf), 0);
}

void send_response(int sockfd, struct myhttp_header *header)
{
	FILE *file;
	char filename[200];
	char filedata[8092];
	char code[4];
	//off_t len;
	int nbytes;
	char *tmp;
	char new[200];

	if (strcmp(header->filename, "/") == 0) {
		strcpy(filename, "index.html");
	} else {
		strcpy(filename, ".");
		strcat(filename, header->filename);
	}

	/* replace the %20 */
	if (strstr(filename, "%20") != NULL) {
		tmp = strtok(filename, "%20");
		strcpy(new, tmp);
		strcat(new, " ");
		while ((tmp = strtok(NULL, "%20")) != NULL) {
			strcat(new, tmp);
			strcat(new, " ");
		}
		new[strlen(new) - 1] = 0;
		strcpy(filename, new);
	}

	file = fopen(filename, "r");
	if (file == NULL) {
		if (errno == ENOENT)
			strcpy(code, "404"); /**! not found */
		else if (errno == EACCES)
			strcpy(code, "402"); /**! permission denied */
		else
			strcpy(code, "404"); /**! undefined */

		send_header(sockfd, code, header->type);
	} else {
		strcpy(code, "200");
		send_header(sockfd, code, header->type);

		while ((nbytes = fread(filedata, sizeof(char), 8092, file)) > 0)
			write(sockfd, filedata, nbytes);

		fclose(file);
	}
} /* send_response */

/* @brief Callback function for 'pthread_create' */
void *__cb_read_from_client(void *sock)
{
	char buffer[RD_BUFF_MAX];
	int nbytes;
	struct myhttp_header header; /* HEADER */
	int sockfd;
	struct sockaddr_in client_sockaddr;
	socklen_t clilen;

	int httpd_sockfd = *((int *)sock);

	/* accept function should be pthread mutexed */
	while ((sockfd = accept(httpd_sockfd, (struct sockaddr *)&client_sockaddr, &clilen))) {
		DEBUG_MSG("client accepted connection\n");

		DEBUG_MSG("waiting for client to write...\n");
		while ((nbytes = read(sockfd, buffer, RD_BUFF_MAX))) {
			DEBUG_MSG("client wrote %d bytes\n", nbytes);
			if (nbytes < 0) {
				CM_ERROR("socket read failed\n");
				break;
			} else if (nbytes == 0) {
				break;
			} else {
				if (check_http_header(&header) < 0) {
					send_header(sockfd, "400", header.type); /* bad request */
					break;
				}
				parse_http_header(buffer, &header); /**! parse_http_header */

				send_response(sockfd, &header); /* buffer and header should be filled with zeros */

				close(sockfd);
				DEBUG_MSG("Connection closed\n");
				break;
			}
		}

	}

	if (sockfd == -1)
		CM_ERROR("accept error\n");

	return NULL;
}

int write_to_client(int sockfd)
{
	return 0;
}

#define MAX_THREADS 3
char *prog_name = NULL;

int main(int argc, char *argv[])
{
	int httpd_sockfd;
	int httpd_port;

	prog_name = *argv;

	struct sockaddr_in httpd_sockaddr;

	pthread_t httpd_threads[MAX_THREADS]; /**! store thread ids */

	/* @brief verify number of arguments first */
	if (argc != 2)
		CM_ERROR("incorrect number of args");

	/* port */
	httpd_port = atoi(argv[1]);
	if (httpd_port <= 1024 || httpd_port >= 65536)
		CM_ERROR("port number has to between 1024 and 65536 (not included)");

	/* init server socket (ipv4, tcp) */
	httpd_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (httpd_sockfd < 0)
		CM_ERROR("httpd socket failed to create");

	memset(&httpd_sockaddr, 0, sizeof(httpd_sockaddr)); /* Fill zeros */
	httpd_sockaddr.sin_family      = AF_INET;
	httpd_sockaddr.sin_port        = htons(httpd_port);
	httpd_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd_sockfd, (struct sockaddr *)&httpd_sockaddr,
		sizeof(httpd_sockaddr)) == -1) {
		close(httpd_sockfd);
		CM_ERROR("bind failed");
	}

	if (listen(httpd_sockfd, CLIENT_MAX) == -1) {
		close(httpd_sockfd);
		CM_ERROR("listen failed");
	}

	int i;
	for(i = 0; i < MAX_THREADS; i++) {
		if (pthread_create(&httpd_threads[i], NULL, (void *)__cb_read_from_client,
			(void *)&httpd_sockfd) != 0)
			CM_ERROR("pthread create error");
		else
			DEBUG_MSG("New thread created (%d)\n", (int)httpd_threads[i]);
	}

	int j;
	for (j = 0; j < i; j++) {
		DEBUG_MSG("waiting for thread %d to exit\n", (int)httpd_threads[j]);
		pthread_join(httpd_threads[j], NULL);
	}

	close(httpd_sockfd);

	return 0;
} /* main */
