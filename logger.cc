int logActivity(char* uri);
//the filename here should be the curl html file
int logActivity(char* uri, std::string filename) {

  FILE * logFile;
  char[MAXLINE] logLine;
  struct sockaddr_in *saddr;
  struct stat filebuf;
  int filestate;
  int filesize;
  bool pageCached;
  bool hostCached;
  char page[20] = "(PAGE CACHED)";
  char host[25] = "(HOSTNAME CACHED)";
  bool exists;
  char nothing[15] = "(NOTFOUND)";
  int pageCounts;
  int nameCounts;

  char * fcstr = new char[filename.length() + 1];
  std::strcpy(fcstr, filename.c_str());

  filestate = stat(fcstr, &filebuf);
  filesize = (intmax_t)filebuf.st_size;
  format_log_entry(logLine, saddr, uri, filesize);
  
  pageCounts = cache_map.count(uri);
  nameCounts = name_map.count(uri);

  logFile = Fopen("proxy.log", "a");

  //TODO: where does sock_addr in come from
  if (filesize < 1) {
    char times[MAXLINE];
    unsigned long hostaddr;

    now = time(NULL);
    strftime(times, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));
    
    hostaddr = ntohl(sockaddr->sin_addr.s_addr);
    a = hostaddr >> 24;
    b = (hostaddr >> 16) & 0xff;
    c = (hostaddr >> 8) & 0xff;
    d = hostaddr & 0xff;
    sprintf(logLine, "%s: %d.%d.%d.%d %s %s", times, a, b, c, d, uri, nothing); 
  }
  Fputs(logLine, logFile);
  if(pageCounts > 0) {
    Fputs(page, logFile);
  }
  if (nameCounts > 0) {
    Fputs(host, logFile);
  }
  
  Fputs( "\n", logFile);

  Fclose(logFile);
}
