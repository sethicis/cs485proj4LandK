#ifdef __cplusplus
extern "C" {
#endif

#include "csapp.h"

#ifdef __cplusplus
}
#endif
#include "global.h"
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

/* First step in getting a working command identifier.
 This test function grabs the string immediately after the
 'GET' keyword and prepares it for analysis.
 Ideally this should be a call to the proxy asking for a webpage.
 After the URI is parsed it can be passed to a hash map to see if it
 has been cached before.
 */
void parseInput(stringstream &ss){
    string s;
    ss >> s;
    if (s.compare("GET") == 0){
        s.clear();
        ss >> s;
        cout << "I saw a: ";
        cout << s << endl;
    }
}

void echo(int connfd){
	size_t n;
	char buf[MAXLINE];
	rio_t rio;
    stringstream ss;
	Rio_readinitb(&rio, connfd);
	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		printf("Server received %d bytes\n", n);
        //printf("Received: %s",buf);
        ss << buf;
        parseInput(ss);
        size_t t = write(connfd,ss.str().c_str(),ss.str().length());
        ss.str("");
        //Rio_writen(connfd,buf,n);

	}
}