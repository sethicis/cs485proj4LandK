/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS: (put your names here)
 *     Student Kyle Blagg, kyle.blagg@uky.edu 
 *     Student Name2, student2@cs.uky.edu 
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 This very basic proxy server captures the client GET requests and
 serves downloads the webpage on behave of the client.  Once a webpage
 is visited it is saved into a cache, so that if a client requests that
 webpage again the proxy won't have to go get the webpage. Instead it will
 simply return the cached page.
 As stated before this is a very basic proxy cache server, so it does not
 cache things like external data or images.  Instead the proxy simple caches
 the header and html body.
 Furthermore, when a user sends a request to the proxy server the request is
 recorded in a local disk log file.
 */ 

#ifdef __cplusplus
extern "C" {
#endif
    
#include "csapp.h"
    
#ifdef __cplusplus
}
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <curl/curl.h>

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
int determine_Request(char *uri, std::string &filename, char *cgiargs); //From tiny server
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void proxyIt(int fd);
void read_requesthdrs(rio_t *rp);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
				 char *shortmsg, char *longmsg);
void cachePage(char*);
std::string getFormattedName(char* host, char* path);
std::map<char*, std::string> cache_map;
std::map<char*, hostent*> name_map;
std::map<char*, hostent*> addr_map;



/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	int listenfd, connfd, port;
    struct sockaddr_in clientaddr;
	socklen_t clientlen;
	port = 15213; //Hardcoded port value.
	
    listenfd = Open_listenfd(port);
    while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		proxyIt(connfd);
		Close(connfd);
    }
}


/* 
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }
       
    /* Extract the host name */

    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */

    *port = 80; /* default */

    if (*hostend == ':')   
        *port = atoi(hostend + 1);
    
    /* Extract the path */

    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/* This function takes the uri, builds the cache file name,
 performs a cURL call on it, then saves the result to a file,
 and assigns the filename of the cached webpage to the uri in the hashmap*/
void cachePage(char* uri){
	
    char host[MAXLINE];
    char path[MAXLINE];
    int portno;
    int retstatus;

    retstatus = parse_uri(uri, host, path, &portno);

    if (retstatus != 0) {
      std::cerr << "No bueno" << std::endl;
    }
    else {
		std::string filename = getFormattedName(host, path);
		cache_map[uri] = filename+".html"; //Assign value
		struct hostent* hName = Gethostbyname(host);
		if (hName == NULL) {
			std::cerr << "There was an issue in ghbn" << std::endl;
		}
		else {
			name_map[host] = hName;
			char** addrList;
			addrList = hName->h_addr_list;
			struct hostent* hAddr = Gethostbyaddr(addrList[0], sizeof(addrList[0]), AF_INET);

			addr_map[addrList[0]] = hAddr;
		}
		CURL* curlhandle;
		std::stringstream ss;
		curlhandle = curl_easy_init();
		if (curlhandle) {
			curl_easy_setopt(curlhandle,CURLOPT_URL,uri);
			curl_easy_setopt(curlhandle,CURLOPT_FOLLOWLOCATION,3L);
			FILE* oFileBody = Fopen((filename+".html").c_str(),"wb");
			if (oFileBody == NULL) {
				curl_easy_cleanup(curlhandle);
				std::cerr << "Shit went wrong: Couldn't open file\n";
				exit(-1);
			}
			FILE* oFileHeader = Fopen((filename+".head").c_str(),"wb");
			if (oFileHeader == NULL) {
				curl_easy_cleanup(curlhandle);
				std::cerr << "Poooooop, something broke: Couldn't open file\n";
				exit(-1);
			}
			curl_easy_setopt(curlhandle,CURLOPT_WRITEHEADER,oFileHeader);
			curl_easy_setopt(curlhandle,CURLOPT_WRITEDATA,oFileBody);
			if (curl_easy_perform(curlhandle)) {
				std::cerr << "FUCK, something went wrong!" << std::endl;
				exit(-1);
			}
			Fclose(oFileBody);
			Fclose(oFileHeader);
			curl_easy_cleanup(curlhandle);
			std::cout << "Completed Successfully\n";
		}
	}
}
/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int determine_Request(char *uri, std::string &filename, char *cgiargs)
{
    char *ptr;
	std::stringstream ss;
	
    if (!strstr(uri, "cgi-bin")) {
		strcpy(cgiargs, "");
		if (cache_map[uri].compare("") == 0){
			std::cout << "INFO: uri: " << uri << " not found in map\n";
			// webpage not cached
			
			cachePage(uri);
			
		}
		std::cout << "For URI: " << uri << std::endl;
		std::cout << "File is: " << cache_map[uri] << std::endl;
		filename = cache_map[uri];
		return 1;
    }
    else {
		std::cerr << "Error: We're handling a dynamic\n";
		ptr = index(uri, '?');
		if (ptr) {
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}
		else
			strcpy(cgiargs, "");
		char* tmp;
		tmp = uri + 7;
		filename += uri;
		return 0;
    }
}
/* $end parse_uri */

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}
/*
 * ProxyIt Is a modification of the CS485 textbook code for the tiny
 web server.  Much of the framework is the same, but some added functionality
 has been added and the code has been ported to a C++ standard.
 */
/* $begin proxyIt */
void proxyIt(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char cgiargs[MAXLINE];
	std::string filename;
    rio_t rio;
	
    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not Implemented",
					"Basic-Proxy does not implement this method");
        return;
    }
    read_requesthdrs(&rio);
	
    /* Parse URI from GET request */
    is_static = determine_Request(uri, filename, cgiargs);
    if (stat(filename.c_str(), &sbuf) < 0) {
		clienterror(fd, strdup(filename.c_str()), "404", "Not found",
					"Basic-Proxy couldn't find this file");
		return;
    }
	
    if (is_static) { /* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 			clienterror(fd, strdup(filename.c_str()), "403", "Forbidden",
						"Basic-Proxy couldn't read the file");
			return;
		}
		serve_static(fd, strdup(filename.c_str()), sbuf.st_size);
    }
    else { /* Serve dynamic content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(fd, strdup(filename.c_str()), "403", "Forbidden",
						"Basic-Proxy couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, strdup(filename.c_str()), cgiargs);
    }
}
/* $end proxyIt */

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
	
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
    }
    return;
}

/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Basic Proxy Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
	
    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = (char*)Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
    else
		strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };
	
    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Basic Proxy Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
	
    if (Fork() == 0) { /* child */
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1);
		Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */		Execve(filename, emptylist, environ); /* Run CGI program */
    }
    Wait(NULL); /* Parent waits for and reaps child */
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
				 char *shortmsg, char *longmsg){
    char buf[MAXLINE], body[MAXBUF];
	
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
	
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

std::string getFormattedName(char* host, char* path) {
  std::string tempht(host);
  std::string tempp(path);
  
  std::string file = tempht + tempp;

  
    for (int i = 0; i < file.length(); i++) {
      if ((file[i] < 48) || (file[i] > 57 && file[i] < 65) ||
        (file[i] > 90 && file[i] < 97) || (file[i] > 122)) {
        file[i] = '_';
      }
    }
	std::cout << "File name generated is: " << file << std::endl;
    return file;
}


