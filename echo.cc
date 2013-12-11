#ifdef __cplusplus
extern "C" {
#endif

#include "csapp.h"

#ifdef __cplusplus
}
#endif
#include "global.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

void testCurl(string s,int connfd){
	CURL* curlhandle;
	string line;
	ifstream iFile;
	ifstream iFileHead;
	stringstream ss;
	curlhandle = curl_easy_init();
	if (curlhandle) {
		curl_easy_setopt(curlhandle,CURLOPT_URL,s.c_str());
		curl_easy_setopt(curlhandle,CURLOPT_FOLLOWLOCATION,3L);
		FILE* oFileBody = Fopen("testOut.html","wb");
		if (oFileBody == NULL) {
			curl_easy_cleanup(curlhandle);
			cerr << "Shit went wrong: Couldn't open file\n";
			exit(-1);
		}
		FILE* oFileHeader = Fopen("testOut.head","wb");
		if (oFileHeader == NULL) {
			curl_easy_cleanup(curlhandle);
			cerr << "Poooooop, something broke: Couldn't open file\n";
			exit(-1);
		}
		curl_easy_setopt(curlhandle,CURLOPT_WRITEHEADER,oFileHeader);
		curl_easy_setopt(curlhandle,CURLOPT_WRITEDATA,oFileBody);
		if (curl_easy_perform(curlhandle)) {
			cerr << "FUCK, something went wrong!" << endl;
		}
		Fclose(oFileBody);
		Fclose(oFileHeader);
		curl_easy_cleanup(curlhandle);
		cerr << "Completed Successfully\n";
	}
	/*FILE *f = Fopen("testOut.html", "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char *str = (char*)malloc(fsize + 1);
	Fread(str, fsize, 1, f);
	Fclose(f);*/
	
	/*if (Dup2(connfd,STDOUT_FILENO) < 0){
		cerr << "NOOOOOO, dup broke\n";
	}*/
	string aStr;
	iFile.open("testOut.html");
	if (iFile.is_open()) {
		
		iFileHead.open("testOut.head");
		if (!iFileHead.is_open()){
			cerr << "Could not open testOut.head\n";
			exit(-1);
		}
		/*while (getline(iFileHead,aStr)) {
			ss << aStr;
		}*/
		ss << iFileHead.rdbuf();
		//ss << endl << endl;
		/*while (getline(iFile,aStr)) {
			ss << aStr;
		}*/
		//ss << endl;
		ss << iFile.rdbuf();
		cout << ss.str();
		cout << "Size sent: " << ss.str().length() << endl;
		char* buf;
		int count;
		count = ss.str().length();
		buf = (char*)Malloc(count+1);
		buf = strdup(ss.str().c_str());
		buf[ss.str().length()] = 0;
		//Write(connfd,ss.str().c_str(),ss.str().length());
		Rio_writen(connfd,buf,count);
		//*buf += count/2;
		//Rio_writen(connfd,buf,count/2);
	}
	//fprintf((FILE*)&connfd,"%s",str);
	//str[fsize] = 0;
	//printf("%s",str);
	//str << endl;
	//Fputs(str,stdout);
	//cout << "Size sent: " << fsize << endl;
	//cout << str << endl;
	//Rio_writen(connfd,str,fsize);
}

/* First step in getting a working command identifier.
 This test function grabs the string immediately after the
 'GET' keyword and prepares it for analysis.
 Ideally this should be a call to the proxy asking for a webpage.
 After the URI is parsed it can be passed to a hash map to see if it
 has been cached before.
 */
void parseInput(stringstream &ss,int connfd){
    string s;
    ss >> s;
    if (s.compare("GET") == 0){
        s.clear();
        ss >> s;
        cout << "I saw a: ";
        cout << s << endl;
		testCurl(s,connfd);
    }
}

void echo(int connfd){
	size_t n;
	char buf[MAXLINE];
	rio_t rio;
    stringstream ss;
	Rio_readinitb(&rio, connfd);
	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 1) {
		//printf("Server received %d bytes\n", n);
        //printf("Received: %s",buf);
        ss << buf;
		//int size = ss.str().length();
		cout << ss.str() << endl;
        parseInput(ss,connfd);
        //size_t t = write(connfd,ss.str().c_str(),ss.str().length());
		//cout << "Size of stringstream buf " << size << " number of characters written: " << t << endl;
        ss.str("");
        //Rio_writen(connfd,buf,n);

	}
	ss << "Null string received: Terminating Connection...\n";
	cerr << ss.str();
	//write(connfd,ss.str().c_str(),ss.str().length());
}