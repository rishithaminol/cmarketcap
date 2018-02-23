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

#define RD_BUFF_MAX 1024 
#define CLIENT_MAX 10

struct myhttp_header {
    char method[5];
    char filename[200];
    char protocol[10];
    char type[100];
};

void error_handle(char *);
int read_from_client(int );
int write_to_client(int );
int parse_http_header(char *, struct myhttp_header *);
int check_http_header(struct myhttp_header *);
void send_header(int, char *, char * );
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
    //fprintf (stdout, "%s\n", line);

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

void send_header(int sockfd, char * code, char * type)
{
    char buf[100];

    if (strcmp(code, "200") == 0) {
        strcpy(buf, "HTTP/1.0 200 OK\r\n");
        send(sockfd, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: %s\r\n", type);
        send(sockfd, buf, strlen(buf), 0);
    }
    else if (strcmp(code, "400") == 0) {
        strcpy(buf, "HTTP/1.0 404 Bad Request\r\n");
        send(sockfd, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: %s\r\n", type);
        send(sockfd, buf, strlen(buf), 0);
    }
    else if (strcmp(code, "403") == 0) {
        strcpy(buf, "HTTP/1.0 403 Forbidden\r\n");
        send(sockfd, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: %s\r\n", type);
        send(sockfd, buf, strlen(buf), 0);
    }
    else if (strcmp(code, "404") == 0) {
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
    off_t len;
    int nbytes, i;
    char *tmp;
    char new[200];

    if (strcmp(header->filename, "/") == 0) {
        strcpy(filename, "index.html");
    }
    else {
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
        new[strlen(new)-1] = 0;
        strcpy(filename, new);
    }

    file =fopen(filename, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            // not found
            strcpy(code, "404");
        }
        else if (errno == EACCES) {
            // permission denied
            strcpy(code, "402");
        }
        else {
            // undefined
            strcpy(code, "404");
        }
        send_header(sockfd, code, header->type);
    }
    else {
        strcpy(code, "200");
        send_header(sockfd, code, header->type);
        while ((nbytes = fread(filedata, sizeof(char), 8092, file)) > 0) {
            write(sockfd, filedata, nbytes);
        }

        fclose(file);
    }
}

int read_from_client (int sockfd)
{
    char buffer[RD_BUFF_MAX];
    int nbytes;
    struct myhttp_header header;

    nbytes = read (sockfd, buffer, RD_BUFF_MAX);
    if (nbytes < 0)
    {
        error_handle("socket read failed");
        return -1;
    }
    else if (nbytes == 0) {
        return -1;
    }
    else {
        // parse_http_header
        parse_http_header(buffer, &header);
        if (check_http_header(&header) < 0) {
            // bad request
            send_header(sockfd, "400", header.type);
            return 0;
        }

        send_response(sockfd, &header);
        close(sockfd);
        return 0;
    }
}

int write_to_client(int sockfd) 
{
    return 0;
}

// Main function
int main(int argc, char *argv[]) {
    int myhttpd_sockfd, client_sockfd;
    int myhttpd_version;
    int myhttpd_port;
    int myhttpd_timeout;

    int i;

    struct sockaddr_in myhttpd_sockaddr, client_sockaddr;
    socklen_t size;

    pthread_t newthread;

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
    }
    else if (strcmp(argv[1], "1.1") == 0) {
        // http 1.1
        myhttpd_version = 1;
        if (atoi(argv[3]) <= 0) {
            error_handle("timeout for http 1.1 should be greater than zero");
        }
    }
    else {
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
    myhttpd_sockaddr.sin_family = AF_INET;
    myhttpd_sockaddr.sin_port = htons(myhttpd_port);
    myhttpd_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(myhttpd_sockfd, (struct sockaddr *)&myhttpd_sockaddr, sizeof(myhttpd_sockaddr)) == -1) {
        close(myhttpd_sockfd);
        error_handle("bind failed");
    }

    if (listen(myhttpd_sockfd, CLIENT_MAX) == -1) {
        close(myhttpd_sockfd);
        error_handle("listen failed");
    }


    while (1) {
        size = sizeof(client_sockaddr);
        client_sockfd = accept(myhttpd_sockfd, (struct sockaddr *)&client_sockaddr, &size);

        if (client_sockfd == -1) {
            error_handle("accept error");
        }
        if (pthread_create(&newthread , NULL, (void *)read_from_client, (void *)client_sockfd) != 0) {
            perror("pthread create error");
        }
    }

    close(myhttpd_sockfd);
}
