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

#define RD_BUFF_MAX 1024
#define CLIENT_MAX  10

int myhttpd_version;
int myhttpd_port;
int myhttpd_timeout;

struct myhttp_header {
	char method[5];
	char filename[200];
	char protocol[10];
	char type[100];
};

void error_handle(char *);
int read_from_client(int);
int write_to_client(int);
int parse_http_header(char *, struct myhttp_header *);
int check_http_header(struct myhttp_header *);
void send_header(int, char *, char *, int);
void send_response(int, struct myhttp_header *);


void error_handle(char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}

int parse_http_header(char *buff, struct myhttp_header *header)
{
	char *line, *section, *tmp;
	int i;

	// fetch the first line
	// line = strtok(buff, "\n");
	// fprintf (stdout, "%s\n", line);

	// parse line
	section = strtok(buff, " ");
	strcpy(header->method, section);
	section = strtok(NULL, " ");
	strcpy(header->filename, section);
	section = strtok(NULL, " ");
	strcpy(header->protocol, section);

	// find request file type
	section = strtok(NULL, "\n");
	while (strstr(section, "Accept:") == NULL) {
		section = strtok(NULL, "\n");
	}
	tmp = strtok(section, " ");
	tmp = strtok(NULL, ",");
	strcpy(header->type, tmp);
	return 0;
}

int check_http_header(struct myhttp_header *header)
{
	return 0;
}

void send_header(int sockfd, char *code, char *type, int size)
{
	char buf[100];
	char ver[4];

	if (myhttpd_version == 0) {
		strcpy(ver, "1.0");
	} else {
		strcpy(ver, "1.1");
	}

	if (strcmp(code, "200") == 0) {
		sprintf(buf, "HTTP/%s 200 OK\r\n", ver);
		send(sockfd, buf, strlen(buf), 0);
		if ((strstr(type, "image")) != NULL) {
			sprintf(buf, "Accept-Ranges: bytes\r\n");
			send(sockfd, buf, strlen(buf), 0);
			sprintf(buf, "Content-Length: %d\r\n", size);
			send(sockfd, buf, strlen(buf), 0);
		} else {
			sprintf(buf, "Content-Type: %s\r\n", type);
			send(sockfd, buf, strlen(buf), 0);
		}
	} else if (strcmp(code, "400") == 0) {
		sprintf(buf, "HTTP/%s 404 Bad Request\r\n", ver);
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "403") == 0) {
		sprintf(buf, "HTTP/%s 403 Forbidden\r\n", ver);
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	} else if (strcmp(code, "404") == 0) {
		sprintf(buf, "HTTP/%s 404 Not Found\r\n", ver);
		send(sockfd, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: %s\r\n", type);
		send(sockfd, buf, strlen(buf), 0);
	}
	sprintf(buf, "\r\n");
	send(sockfd, buf, strlen(buf), 0);
} /* send_header */

void send_response(int sockfd, struct myhttp_header *header)
{
	FILE *file;
	char filename[200];
	char filedata[8092];
	char code[4];
	off_t len;
	int nbytes, i, size;
	char *tmp;
	char new[200];

	if (strcmp(header->filename, "/") == 0) {
		strcpy(filename, "index.html");
	} else {
		strcpy(filename, ".");
		strcat(filename, header->filename);
	}
	// replace the %20
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

	file = fopen(filename, "rb");
	if (file == NULL) {
		if (errno == ENOENT) {
			// not found
			strcpy(code, "404");
		} else if (errno == EACCES) {
			// permission denied
			strcpy(code, "402");
		} else {
			// undefined
			strcpy(code, "404");
		}
		send_header(sockfd, code, header->type, 0);
	} else {
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		strcpy(code, "200");
		send_header(sockfd, code, header->type, size);
		while ((nbytes = fread(filedata, 1, 8092, file)) > 0) {
			send(sockfd, filedata, nbytes, 0);
			bzero(filedata, sizeof(filedata));
		}

		fclose(file);
	}
} /* send_response */

int read_from_client(int sockfd)
{
	char buffer[RD_BUFF_MAX];
	int nbytes;
	struct myhttp_header header;

	nbytes = read(sockfd, buffer, RD_BUFF_MAX);
	if (nbytes < 0) {
		error_handle("socket read failed");
		return -1;
	} else if (nbytes == 0) {
		return -1;
	} else {
		// parse_http_header
		parse_http_header(buffer, &header);
		if (check_http_header(&header) < 0) {
			// bad request
			send_header(sockfd, "400", header.type, 0);
			return 0;
		}

		send_response(sockfd, &header);
		return 0;
	}
}

int write_to_client(int sockfd)
{
	return 0;
}

// Main function
int main(int argc, char *argv[])
{
	int myhttpd_sockfd, client_sockfd;

	int i;

	struct sockaddr_in myhttpd_sockaddr, client_sockaddr;
	socklen_t size;

	fd_set active_fdset, read_fdset;
	struct timeval tv;

	// verify arg first
	if (argc != 4) {
		error_handle("incorrect number of args");
	}
	if (strcmp(argv[1], "1") == 0) {
		// http 1.0
		myhttpd_version = 0;
		if (atoi(argv[3]) != 0) {
			error_handle("timeout for http 1.0 should be zero");
		}
	} else if (strcmp(argv[1], "1.1") == 0) {
		// http 1.1
		myhttpd_version = 1;
		if (atoi(argv[3]) <= 0) {
			error_handle("timeout for http 1.1 should be greater than zero");
		}
	} else {
		perror("incorrect http version");
		exit(1);
	}

	// port
	myhttpd_port = atoi(argv[2]);
	if (myhttpd_port <= 1024 || myhttpd_port >= 65536) {
		error_handle("port number has to between 1024 and 65536 (not included)");
	}

	// timeout
	myhttpd_timeout = atoi(argv[3]);


	// init server socket (ipv4, tcp)
	myhttpd_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (myhttpd_sockfd < 0) {
		error_handle("myhttpd socket failed to create");
	}
	memset(&myhttpd_sockaddr, 0, sizeof(myhttpd_sockaddr));
	myhttpd_sockaddr.sin_family      = AF_INET;
	myhttpd_sockaddr.sin_port        = htons(myhttpd_port);
	myhttpd_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(myhttpd_sockfd, (struct sockaddr *)&myhttpd_sockaddr, sizeof(myhttpd_sockaddr)) == -1) {
		close(myhttpd_sockfd);
		error_handle("bind failed");
	}

	if (listen(myhttpd_sockfd, CLIENT_MAX) == -1) {
		close(myhttpd_sockfd);
		error_handle("listen failed");
	}

	// init fd set
	FD_ZERO(&active_fdset);
	FD_SET(myhttpd_sockfd, &active_fdset);

	tv.tv_sec  = myhttpd_timeout;
	tv.tv_usec = 0;

	while (1) {
		read_fdset = active_fdset;
		int ret;
		if (myhttpd_version == 0) {
			// HTTP 1.0
			ret = select(FD_SETSIZE, &read_fdset, NULL, NULL, NULL);
			if (ret < 0) {
				error_handle("select failed");
			}

			for (i = 0; i < FD_SETSIZE; i++) {
				if (FD_ISSET(i, &read_fdset)) {
					if (i == myhttpd_sockfd) {
						// server socket
						// establish connection with new client
						size = sizeof(client_sockfd);
						client_sockfd = accept(myhttpd_sockfd, (struct sockaddr *)&client_sockaddr, &size);
						
						if (client_sockfd < 0) {
							error_handle("client socket failed to create");
						}
						FD_SET(client_sockfd, &active_fdset);
					} else {
						// data arrived on connected socket from client
						if (read_from_client(i) < 0) {
							error_handle("read err");
						}
						// HTTP 1.0
						FD_CLR(i, &active_fdset);
						close(i);
					}
				}
			}
		} else if (myhttpd_version == 1) {
			ret = select(FD_SETSIZE, &read_fdset, NULL, NULL, &tv);
			if (ret < 0) {
				printf("wtf\n");
				error_handle("select failed");
			} else if (ret == 0) {
				// time out
				for (i = 0; i < FD_SETSIZE; i++) {
					if (FD_ISSET(i, &active_fdset) && i != myhttpd_sockfd) {
						close(i);
					}
				}
				FD_ZERO(&active_fdset);
				FD_SET(myhttpd_sockfd, &active_fdset);
				continue;
			}

			for (i = 0; i < FD_SETSIZE; i++) {
				if (FD_ISSET(i, &read_fdset)) {
					if (i == myhttpd_sockfd) {
						// server socket
						// establish connection with new client
						size = sizeof(client_sockfd);
						client_sockfd = accept(myhttpd_sockfd, (struct sockaddr *)&client_sockaddr, &size);
						printf("client accepted connection\n");
						if (client_sockfd < 0) {
							error_handle("client socket failed to create");
						}
						FD_SET(client_sockfd, &active_fdset);
					} else {
						// data arrived on connected socket from client
						if (read_from_client(i) < 0) {
							error_handle("read err");
						}
					}
				}
			}
		}
	}
} /* main */
