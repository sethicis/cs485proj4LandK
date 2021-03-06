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

//So the c++ code will play nice with others
#ifdef __cplusplus
extern "C" {
#endif
    
#include "csapp.h"
    
#ifdef __cplusplus
}
#endif
#include <iostream>	//For standard I/O
#include <fstream>	//For file I/O
#include <sstream>	//For easy management of c_string data and other things
#include <streambuf>//For reading file contents into a string
#include <string>	//For strings
#include <vector>	//For vectors
#include <locale>

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
int Open_byHname_clientfd(char *hostname, int port, struct sockaddr_in* serv); /* Wrapper */
int open_byHname_clientfd(char *hostname, int port, struct sockaddr_in* serv);
int Open_byAddr_clientfd(struct sockaddr_in* serv); /* Wrapper */
int open_byAddr_clientfd(struct sockaddr_in* serv);
void write_Result(std::string*,int);
void proxyIt(int fd);
void send_request(char* method,char* version, char* host,char* path,int* port,std::string* retBmsg);
void clienterror(int fd, char *cause, char *errnum,
				 char *shortmsg, char *longmsg);
void logActivity(char* uri, int filesize,struct sockaddr_in*, bool pageE, bool pageC, bool nameC);
int searchCache(char* host,char* path,std::string* retBmsg);
void getCache(std::string headerFile,std::string bodyFile,std::string* retBmsg);
void cacheNewHost(char* host,char* path,std::string* retBmsg,struct sockaddr_in* serv);
void cacheNewPath(struct hostCache& hc,char* path,std::string* retBmsg);
std::string writeCacheFile(std::string* buf);
void writeResponse(int connfd,int filesize);
void readResponse(int clientfd,char* method,char* host,char*path,char*version,std::string* retBmsg);
void loadPathContent(struct hostCache& hc,char* path,std::string* retBmsg);
/* End of function prototypes */

/* Structure definitions */
/* paths of a host name */
struct logInfo{
	bool hcache;
	bool pcache;
	bool ecache;
	std::string url;
	int bSize;
};
struct hostPaths {
	std::string pathname;
	std::string F_bodyName;
};
/* Each unique host has an entry */
struct hostCache {
	std::string hostname;
	std::vector<struct hostPaths*> pathnames;
	struct sockaddr_in* SAddr_in; /* DNS cache */
};
/* End of Structure definitions */

/* Global variables */
int filenameCounter; //Used to determine what to name the next file
std::vector<struct hostCache> proxyCache; //Global collection of hostCaches
struct logInfo logEvent;
/* End of global variables */

/*
 * Beginning of program functions
 */

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN); //Don't get stopped by stupid SIGPIPE handle the errors
	int listenfd, connfd, port;
    struct sockaddr_in clientaddr;
	socklen_t clientlen;
	filenameCounter = 0; //Initialize the filename counter
	port = 15213; //Hardcoded listen port value
	
    listenfd = Open_listenfd(port);
    while (1) {
		//initalize the log structure
		logEvent.hcache = false;
		logEvent.pcache = false;
		logEvent.ecache = false;
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		proxyIt(connfd);
		logActivity(strdup(logEvent.url.c_str()),logEvent.bSize,&clientaddr,logEvent.ecache,logEvent.pcache,logEvent.hcache);
		Close(connfd);
    }
}
/*
 * ProxyIt Is a modification of the CS485 textbook code for the tiny
 web server.  Much of the framework is the same, but some added functionality
 has been added and the code has been ported to a C++ standard.
 */
/* $begin proxyIt */
void proxyIt(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char hostname[MAXLINE], pathname[MAXLINE];
	int port;
	std::string* retMsg;
	std::string* retBody;
	rio_t rio;
	
	retBody = new std::string;
    
    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
	/* gets the request header and identifies the type of request */
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not Implemented",
					"Basic-Proxy does not implement this method");
        return;
    }
	//Get the information that matters
	parse_uri(uri,hostname,pathname,&port);
	
	send_request(method,version,hostname,pathname,&port,retBody);
	//std::cout << "Response:\n" << *retMsg << "Body:\n" << *retBody;
	//Send the result back to the client
	if (retBody != NULL) {
		write_Result(retBody,fd);
	}
	logEvent.url = uri;
}
/* $end proxyIt */

/*
 This function takes the users request and creates a connection with the
 requested server, performs a GET call to said server and retreivees the
 resulting response header and message body.
 */
void send_request(char* method,char* version, char* host,char* path,int* port,std::string* retBmsg)
{
	 if (int ret = searchCache(host,path,retBmsg)) {
		 if (ret == 1) {
			 //Handle request from cache
			 return;
		 }else if (ret == 0){
			 //Let things proceed;
		 }
		 //error case!
		return; //We don't need to connect
	}
	struct sockaddr_in* serv = (sockaddr_in*)Malloc(sizeof(sockaddr_in));
	int clientfd = Open_byHname_clientfd(host, *port,serv);
	readResponse(clientfd,method,host,path,version, retBmsg); //Opens the read and write channels for the client/server connection
	cacheNewHost(host,path,retBmsg,serv);
	
}

void readResponse(int clientfd,char* method,char* host,char*path,char*version,std::string* retBmsg){
	
	char response[MAXLINE];
	rio_t rio;
	Rio_readinitb(&rio, clientfd);
	char requestMsg[MAXLINE];
	std::stringstream ss;
	printf("Host is: %s\n",host);
	sprintf(requestMsg,"%s %s %s\nHost: %s\n\n",method,path,version,host);
	std::cout << "Request looks like:\n" << requestMsg;
	Rio_writen(clientfd, requestMsg, strlen(requestMsg));
	bool found = false;
	ss.str("");
	//This code block is to insure that we don't grab too much data at once.
	//Because we are using readn instead of read lines we have to keep
	//track of where the header ends and the body begins or we risk losing
	//data.
	while((Rio_readn(clientfd,response,MAXLINE))){
		ss << response;
		bzero(response,MAXLINE);
	}
	*retBmsg = ss.str();
	logEvent.bSize = retBmsg->length(); //Total number of bytes received
	
	//std::cout << "The rHead is: " << rHeadEnd << "The rEnd: " << rEnd << std::endl;
	std::cout << *retBmsg;
	Close(clientfd);
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
        //pathbegin++;
        strcpy(pathname, pathbegin);
    }
	
    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring,struct sockaddr_in* clientaddr, char *uri, int size)
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
    host = ntohl(clientaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;
	
	
    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %i", time_str, a, b, c, d, uri, size);
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
				 char *shortmsg, char *longmsg){
    char buf[MAXLINE], body[MAXBUF];
	
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Basic Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Basic Proxy server</em>\r\n", body);
	
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

/*logActivity generates the logfile strings and writes them to the proxy.log file
 after doing some checks: is the page previously cached, and was anything actually
 returned from the server. */
void logActivity(char* uri, int filesize,struct sockaddr_in* sock, bool pageE, bool pageC, bool nameC) {
    
    FILE * logFile;
    char logLine[MAXLINE];
    //struct stat filebuf;
    //int filestate;
    //int filesize;
    char page[20] = "(PAGE CACHED)\0";
    char host[25] = "(HOSTNAME CACHED)\0";
    char nothing[15] = "(NOTFOUND)\0";
	
    
    //char * fcstr = new char[filename.length() + 1];
    //std::strcpy(fcstr, filename.c_str());
    
    //filestate = stat(fcstr, &filebuf);
    //filesize = (intmax_t)filebuf.st_size;
    format_log_entry(logLine,sock, uri,filesize);
    
    
    logFile = Fopen("proxy.log", "a");
    Fputs(logLine, logFile);
    if (pageE) {
        Fputs(nothing, logFile);
    }
    else {
        if(pageC == true) {
            Fputs(page, logFile);
        }
        if (nameC == true) {
            Fputs(host, logFile);
        }
    }
    
    Fputs( "\n", logFile);
    
    Fclose(logFile);
}

/* This function handles returning the retrieved information to the user */
void write_Result(std::string* retBmsg,int connfd){
	/* Lets not keep our fans waiting.  Let them know how things went */
	//Rio_writen(connfd,strdup(retHmsg->c_str()),retHmsg->length());
	writeResponse(connfd,retBmsg->length());
	/* Time to send the body too! */
	Rio_writen(connfd,strdup(retBmsg->c_str()),retBmsg->length());
	/* K thx bye */
}

void writeResponse(int connfd,int filesize){
	/* Build the HTTP response body */
	std::stringstream ss;
	ss << filesize;
	std::string retStr;
	retStr = "HTTP/1.1 200 OK\n";
	retStr += "MIME-Version: 1.0\n";
	retStr += "Date: Mon, 8 Jan 2010 4:59:42 GMT\n";
	retStr += "Server: Basic Proxy/1.1\n";
	retStr += "Content-Type: text/html\n";
	retStr = retStr + "Content-Length: " + ss.str() + "\n\n";
	//std::cout << retStr;
	Rio_writen(connfd,strdup(retStr.c_str()),retStr.length());
	
}

int searchCache(char* host,char* path,std::string* retBmsg){
	struct hostCache h;
	bool matched = false;
	std::string h_name = host;
	std::string p_name = path;
	for (int i = 0; i < proxyCache.size(); i++) {
		if (!(proxyCache[i].hostname.compare(h_name))){
			h = proxyCache[i];
			matched = true;
			break;
		}
	}
	if (!matched) {
		logEvent.hcache = true;
		logEvent.pcache = true;
		return 0; //host is not in cache
	}
	for (int i = 0; i < h.pathnames.size(); i++) {
		if (!h.pathnames[i]->pathname.compare(p_name)){
			p_name = h.pathnames[i]->F_bodyName;
			getCache(h_name,p_name,retBmsg);
			//Host and page are in cache already
			return 1; //Skip DNS lookup return response
		}
	}
	loadPathContent(h,path,retBmsg); //Note, header files are depricated.
											//We generate new a new header instead of
											//Recycling the same header the host sent.
	if (retBmsg == NULL) {
		//Error occurred
		logEvent.ecache = true;
		return -1;
	}
	cacheNewPath(h,path,retBmsg);
	logEvent.pcache = true;
	return 1; //Skip DNS lookup return response
}

void loadPathContent(struct hostCache& hc,char* path,std::string* retBmsg){
	int connfd = Open_byAddr_clientfd(hc.SAddr_in);
	readResponse(connfd,"GET",strdup(hc.hostname.c_str()),path,"HTTP/1.1",retBmsg);
	if (retBmsg == NULL) {
		//An error occurred
		return;
	}
}

void getCache(std::string headerFile,std::string bodyFile,std::string* retBmsg){
	std::ifstream h_tmp(bodyFile.c_str());
	std::string str((std::istreambuf_iterator<char>(h_tmp)),
					std::istreambuf_iterator<char>());
	std::cout << "Contents of response:\n" << str;
	std::cout << "Debugging early terminate\n";
	//exit(0); //For debugging
}

void cacheNewHost(char* host,char* path,std::string* retBmsg,struct sockaddr_in* serv){
	printf("In create new host\n");
	struct hostCache newHost;
	newHost.hostname = host;
	newHost.SAddr_in = serv;
	cacheNewPath(newHost,path,retBmsg);
	proxyCache.push_back(newHost);
	printf("Leaving create new host\n");
}

void cacheNewPath(struct hostCache& hc,char* path,std::string* retBmsg){
	printf("In create new path\n");
	struct hostPaths* newPath;
	newPath = new hostPaths;
	newPath->pathname = path;
	printf("Between\n");
	//newPath->F_bodyName = new std::string; //Create header cache
	newPath->F_bodyName = writeCacheFile(retBmsg); //Create body cache
	hc.pathnames.push_back(newPath);
	printf("Leaving new path\n");
}
/* The writes returned data to a file */
std::string writeCacheFile(std::string* buf){
	std::stringstream ss;
	printf("In Write cache\n");
	//std::stringstream ss;  //Convert filename to string
	ss << filenameCounter; //Get the next filename and increment
	std::ofstream outF(ss.str().c_str());
	outF << (*buf);	//Put the data into the file
	outF.close();	//Lets be tidy
	printf("End of Write cache\n");
	//tmp;
	return ss.str(); //Return the name of the file we just made
}

/* Wrapper for openning a connection to a server by hostname. */
int Open_byHname_clientfd(char *hostname, int port, struct sockaddr_in* serv)
{
	int rc;
	if ((rc = open_byHname_clientfd(hostname, port,serv)) < 0) {
		if (rc == -1)
			unix_error("Open_clientfd Unix error");
		else
			dns_error("Open_clientfd DNS error");
	}
	return rc;
}

/* Wrapper for: Open connection to a server without DNS lookup */
int Open_byAddr_clientfd(struct sockaddr_in* serv)
{
	int rc;
	if ((rc = open_byAddr_clientfd(serv)) < 0) {
		if (rc == -1)
			unix_error("Open_clientfd Unix error");
		else
			dns_error("Open_clientfd DNS error");
	}
	return rc;
}

/* Open connection to a server */
int open_byHname_clientfd(char *hostname, int port,struct sockaddr_in* serveraddr)
{
	int clientfd;
	struct hostent *hp;
	//struct sockaddr_in serveraddr;
	
	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1; /* check errno for cause of error */
	
	/* Fill in the server's IP address and port */
	if ((hp = gethostbyname(hostname)) == NULL)
		return -2; /* check h_errno for cause of error */
	bzero((char *) serveraddr, sizeof(serveraddr));
	serveraddr->sin_family = AF_INET;
	bcopy((char *)hp->h_addr_list[0],
		  (char *)&(serveraddr->sin_addr.s_addr), hp->h_length);
	serveraddr->sin_port = htons(port);
	
	/* Establish a connection with the server */
	if (connect(clientfd, (SA *)&(*serveraddr), sizeof(*serveraddr)) < 0){
		return -1;
	}
	return clientfd;
}

/* Open connection to a server without need of a DNS lookup */
int open_byAddr_clientfd(struct sockaddr_in* serveraddr){
	int clientfd;
	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (connect(clientfd, (SA*)&(*serveraddr), sizeof(*serveraddr)) < 0) {
		return -1;
	}
	return clientfd;
}



