#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_URL_LEN 1024
#define IO_BUF_SIZE 2048

typedef struct url_s {
	char path[MAX_URL_LEN];
	char host_addr[MAX_URL_LEN];
} url_t;


char * create_http_request(char *buf, char* verb, url_t* url) {
	sprintf(buf, "%s %s HTTP/1.1\r\nHost: %s\r\nAccept: text/plain, text/html; q=0.5\r\n\r\n",
	        verb, url->path, url->host_addr);
	return buf;
}

void print_help() {
	printf("This program should get one parameter with URL address of page which "
	       "should be downloaded.\n");
}

void logger(const char *s) {
	fprintf(stderr, "%s\n", s);
}

int parse_url(char *s, url_t *res) {
	unsigned int path_len, addr_len;
	char *addr_st = strchr(s, ':');
	if (addr_st != NULL) {
		if( addr_st[1] != '/' || addr_st[2] != '/') {
			logger("Error. Incorrect url addres.");
			return -1;
		}
		addr_st += 3;
		if(strncmp(s, "http://", 7) != 0) {
			printf("Unsopported protocol in URL. Please use HTTP.\n");
			logger("Error. Incorrect protocol.");
			return -1;
		}
	}
	else {
		addr_st = s;
	}
	if (strchr(addr_st, ':')) {
		logger("Error. Incorrect url addres.");
		return -1;
	}
	char *path_st = strchr(addr_st, '/');
	if (path_st == NULL) {
		path_len = 1;
		addr_len = strlen(addr_st);
		path_st = "/";
	}
	else {
		path_len = strlen(path_st);
		addr_len = path_st - addr_st;
	}
	if (path_len > (sizeof(res->path) + 1) || addr_len > (sizeof(res->host_addr) + 1)) { 
		logger("Error. Too big url address");
		return -1;
	}
	if (addr_len < 1) {
		logger("Error. Empty hostname.");
		return -1;
	}
	strncpy(res->path, path_st, path_len);
    res->path[path_len] = '\0';
	strncpy(res->host_addr, addr_st, addr_len);
	res->host_addr[addr_len] = '\0';
	return 0; 
}


int read_socket_to_file(int sock, char* filename) {	
	FILE *output = fopen(filename, "wt");
	char buf[IO_BUF_SIZE];
	if (output == NULL)
	{
		logger("Error while opening file");
		return -1;
	}
	int len;
	len = recv(sock, buf, sizeof(buf)-1, 0);
	while (len > 0) {
		buf[len] = '\0';
		fprintf(output, "%s", buf);
		len = recv(sock, buf, sizeof(buf)-1, 0);
	}
	fclose(output);
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		print_help();
		return -1;
	}
	char buf[IO_BUF_SIZE];
	url_t url;
	struct addrinfo addr_hints;
	struct addrinfo *addr_info;
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (parse_url(argv[1], &url) < 0)
	{
		logger("Error while parsing URL.");
		return -1;
	}
	memset(&addr_hints, 0, sizeof(addr_hints));
	addr_hints.ai_family = AF_UNSPEC;
	addr_hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(url.host_addr, "http", &addr_hints, &addr_info) != 0) {
		logger("Error while getting info about address.");
		return -1;
	}
	if(connect(sock, addr_info->ai_addr, sizeof(struct sockaddr)) < 0) {
		logger("Error while trying to connect address.");
		return -1;
	}
	freeaddrinfo(addr_info);

	create_http_request(buf, "GET", &url);
	send(sock, buf, sizeof(buf), 0);
	read_socket_to_file(sock, url.host_addr);
	printf("Internet page save into file \"%s\".\n", url.host_addr);
	shutdown(sock, 2);
    return 0;
}
