const char * usage =
		"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/time.h>
#include <sys/wait.h> 
#include <sys/mman.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdio.h> 
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <vector> 
#include <algorithm>
#include <dlfcn.h>
#include <unordered_map>

int QueueLength = 5;

// Processes time request
void processRequest(int socket, struct sockaddr_in client);
void logRequest(struct sockaddr_in client, char * request);
struct qparams
{
	int n;
	char ** params;
};
void serveDirectory(FILE * socket, char * path, struct qparams q);
void statsPage(FILE * socket, char * path);
void cgi_bin(FILE * socket, int fd, char * path, char * query, char * method);

extern "C" void killZombie(int n){
	wait3(0,0,NULL);
	while(waitpid(-1,NULL, WNOHANG)>0){

	}
	printf("Fork Killed\n");
}

char * SECRET = "1597538524560";
int PORT =1738;


char * NAME = "Ammar Husain";
unsigned long * numRequests;
time_t start_time;
double * min_time;
double * max_time;

std::unordered_map<std::string, void*> openlibs;

void slaveWrapper(int fd){
	while(1){
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof(clientIPAddress);
		int slaveSocket = accept(fd,
				(struct sockaddr * ) & clientIPAddress,
				(socklen_t * ) & alen);

		if (slaveSocket < 0) {
				perror("accept");
				exit(-1);
		}

		// Process request.
		processRequest(slaveSocket, clientIPAddress);

		// Close socket
		close(slaveSocket);
	}
}

struct pRequestParams{
	int fd;
	struct sockaddr_in clientIPAddress;
};
void processRequestThread(int socket){
	struct sockaddr_in clientIPAddress;
	processRequest(socket, clientIPAddress);
	void * ret;
	pthread_exit(ret);
}

int main(int argc, char ** argv) {
	// Print usage if not enough arguments
	char flag = 0;
	if (argc < 2) {
		fprintf(stderr, "%s", usage);
		exit(-1);
	}
	for(int i= 0 ; i < argc; i++){
		if(argv[i][0] == '-'){
			flag = argv[i][1];
		}else{
			PORT = atoi(argv[i]);
		}
	}
	printf("Running on port: %d\n", PORT);
	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress;
	memset( & serverIPAddress, 0, sizeof(serverIPAddress));
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) PORT);
	protoent * p = getprotobyname("tcp");
	// Allocate a socket
	int masterSocket = socket(PF_INET, SOCK_STREAM, p->p_proto);
	if (masterSocket < 0) {
		perror("socket");
		exit(-1);
	}


	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1;
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR,
			(char * ) & optval, sizeof(int));
	signal(SIGPIPE, SIG_IGN);

	// Bind the socket to the IP address and port
	int error = bind(masterSocket,
			(struct sockaddr * ) & serverIPAddress,
			sizeof(serverIPAddress));
	if (error) {
		perror("bind");
		exit(-1);
	}

	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen(masterSocket, QueueLength);
	if (error) {
		perror("listen");
		exit(-1);
	}
	numRequests = (unsigned long *)mmap(NULL, sizeof(unsigned long), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	min_time = (double *)mmap(NULL, sizeof(double), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	max_time = (double *)mmap(NULL, sizeof(double), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	*numRequests = 0;
	*min_time = 1000000000;
	*max_time = 0;
	time(&start_time);

	if(flag == 'f'){
		struct sigaction sa;
		sa.sa_handler = killZombie;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		sigaction(SIGCHLD, &sa, NULL);
		while (1) {

			// Accept incoming connections
			struct sockaddr_in clientIPAddress;
			int alen = sizeof(clientIPAddress);
			int slaveSocket = accept(masterSocket,
					(struct sockaddr * ) & clientIPAddress,
					(socklen_t * ) & alen);

			if (slaveSocket < 0) {
					perror("accept");
					exit(-1);
			}

			pid_t slave = fork();
			if(slave == 0){
				// Process request.
				processRequest(slaveSocket,clientIPAddress);
				close(slaveSocket);
				exit(EXIT_SUCCESS);
			}
			

			// Close socket
			close(slaveSocket);
		}
	}else if(flag == 't'){
		while (1) {

			// Accept incoming connections
			struct sockaddr_in clientIPAddress;
			int alen = sizeof(clientIPAddress);
			int slaveSocket = accept(masterSocket,
					(struct sockaddr * ) & clientIPAddress,
					(socklen_t * ) & alen);

			if (slaveSocket < 0) {
					perror("accept");
					exit(-1);
			}

			pthread_t t;
			pthread_attr_t a;
			pthread_attr_init(&a);
			pthread_attr_setscope(&a, PTHREAD_SCOPE_SYSTEM);
			struct pRequestParams params;
			params.fd = slaveSocket;
			params.clientIPAddress = clientIPAddress;
			pthread_create(&t, &a, (void*(*)(void*))processRequestThread, (void*) (intptr_t)slaveSocket);
		}
	}else if(flag == 'p'){
		pthread_t tid[5];
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setscope(&a, PTHREAD_SCOPE_SYSTEM);
		for(int i=0; i< 5;i++){
			pthread_create(&tid[i], &a, (void*(*)(void*))slaveWrapper, (void*) (intptr_t)masterSocket);
		} 
		pthread_join(tid[0], NULL); 
	}else{
		while (1) {

			// Accept incoming connections
			struct sockaddr_in clientIPAddress;
			int alen = sizeof(clientIPAddress);
			int slaveSocket = accept(masterSocket,
					(struct sockaddr * ) & clientIPAddress,
					(socklen_t * ) & alen);

			if (slaveSocket < 0) {
					perror("accept");
					exit(-1);
			}

			// Process request.
			processRequest(slaveSocket,clientIPAddress);

			// Close socket
			close(slaveSocket);
		}
	}
}

void writeResponseFile(FILE * f, int code, char * status, char * ctype){
	fprintf(f, "HTTP/1.1 %d %s\r\nServer: ammu/1.0\r\nContent-type: %s\r\n\r\n", code, status, ctype);
	fflush(f);
}

void fileNotFound(FILE * socket){
	writeResponseFile(socket, 404, "Not Found", "text/html");
	fprintf(socket, "<h1>404 Not Found</h1>");
}

void dumpFileWithFile(FILE * socket, FILE * f){
	//printf("Dumping file...\n");
	int c;
	while((c = fgetc(f)) != EOF){
		fputc(c, socket);
	}
	//printf("File Dumped\n");
}



struct qparams parseQuery(char * qstring){
	struct qparams params;
	params.n = 0;
	params.params = (char **) malloc(sizeof(char *) * 20);
	if(qstring == NULL){
		return params;
	}
	char * str = strdup(qstring);
	char * tok1;
	tok1 = strtok (str,"&");
	while (tok1 != NULL)
	{
		params.params[params.n++] = strdup(tok1);
		tok1 = strtok (NULL, "&");
	}
	free(str);
	return params;
}

void processRequest(int fd, struct sockaddr_in client) {
	clock_t t;
	t = clock();

	FILE * f = fdopen(fd, "a+");

	// Buffer used to store the name received from the client
	const int MAX_BUFF = 4096;
	char buf[MAX_BUFF + 1];
	char * ptr = buf;
	//
	// The client should send <name><cr><lf>
	// Read the name of the client character by character until a
	// <CR><LF> is found.
	//
	//printf("Reading request\n");
	int newChar;
	int lastChar = 0;
	while (ptr < buf + MAX_BUFF &&
		(newChar = fgetc(f)) != EOF) {
		*ptr = newChar;
		ptr++;
		if (lastChar == '\r' && newChar == '\n') {

				break;
		} else {
				lastChar = newChar;
		}
	}

	*ptr = 0;
	

	char method[8];
	char path[128];

	int i = 0;
	ptr = buf;
	while ( * ptr != ' ' && * ptr != '\0') {
		method[i] = * ptr;
		ptr++;
		i++;
	}
	method[i] = 0;
	i = 0;
	ptr++;
	while(*ptr != '\0' && *ptr != ' '){
		path[i] = *ptr;
		i++;
		ptr++;
	}
	path[i] = 0;

	i = 1;
	(*numRequests)++;
	if(strncmp(SECRET, path+1, strlen(SECRET)) != 0){
		writeResponseFile(f, 401, "Unauthorized", "text/html");
		fprintf(f, "<h1>401 Unauthorized</h1>\n<p>Your access key was incorrect.\n</p>\n");
		fflush(f);
		fclose(f);
		close(fd);
		return;
	}
	
	char * rpath = &path[0] + 14;
	char * query = strrchr(rpath, '?');
	if(query != NULL){
		*query = 0;
		query++;
	}

	struct qparams params  = parseQuery(query);

	for(int i = 0; i < params.n; i++)
	{
		printf("Param: %s\n", params.params[i]);
	}


	char fpath[1024];
	getcwd(fpath, 1024);	
	strcat(fpath, "/http-root-dir");

	if(strlen(rpath) > 1 && !strncmp("/stats", rpath, strlen("/stats"))){
		statsPage(f, rpath);
	}else if(strlen(rpath) > 1 && !strncmp("/cgi-bin", rpath, strlen("/cgi-bin"))){
		cgi_bin(f, fd,rpath, query, method);
	}else{
		if(strcmp(rpath, "/") == 0){
			strcat(fpath, "/htdocs/index.html");
		}else if(strlen(rpath) > 1 && !strncmp("/logs", rpath, strlen("/logs"))){
			strcpy(fpath, "./http-log.txt");
		}else if(strlen(rpath) > 1 && strncmp("/icons", rpath, strlen("/icons")) != 0 && strncmp("/htdocs", rpath, strlen("/htdocs")) != 0){
			strcat(fpath, "/htdocs");
			strcat(fpath, rpath);
		}else{
			strcat(fpath, rpath);
		}
		char * content;
		if(strstr(fpath, ".html")){
			content = "text/html";
		}else if(strstr(fpath, ".gif")){
			content = "image/gif";
		}else if(strstr(fpath, ".svg")){
			content = "image/svg+xml";
		}else if(strstr(fpath, ".xbm")){
			content = "image/xbm";
		}else{
			content = "text/plain";
		}

		struct stat st;
	    stat(fpath, &st);
	    if(S_ISDIR(st.st_mode)){
			serveDirectory(f,fpath, params);
		}else{
			FILE * ftowrite = fopen(fpath, "r");
			if(ftowrite == NULL){
				fileNotFound(f);
			}else{
				writeResponseFile(f, 200, "Document follows", content);
				dumpFileWithFile(f, ftowrite);
				fclose(ftowrite);
			}
	    }
	}
	t = clock() - t;
	double ptime = t*1.0/CLOCKS_PER_SEC;
	if(ptime < *min_time){
		*min_time = ptime;
	}
	if(ptime > *max_time){
		*max_time = ptime;
	}
	//unsigned char nline = '\n';
	//write(fd, &nline, 1);
	fprintf(f, "\n");
	fflush(f);
	fclose(f);
	close(fd);
	logRequest(client, strdup(buf));
	if(query != NULL){
		printf("RealPath: %s\n", rpath);
		printf("Query: %s\n", query);
	}
	// Free query params
	for(int i = 0; i < params.n; i++)
	{
		free(params.params[i]);
	}
	free(params.params);
}


struct dirEntry{
	char * name;
	int size;
	char * mod;
	char * ahref;
	char * iconUrl;
};

bool dirCompName(struct dirEntry a, struct dirEntry b){
	return (strcmp(a.name, b.name) < 0);
}
bool dirCompSize(struct dirEntry a, struct dirEntry b){
	return (a.size < b.size);
}
bool dirCompMod(struct dirEntry a, struct dirEntry b){
	return (strcmp(a.mod, b.mod) < 0);
}

bool dirCompNameD(struct dirEntry a, struct dirEntry b){
	return (strcmp(a.name, b.name) > 0);
}
bool dirCompSizeD(struct dirEntry a, struct dirEntry b){
	return (a.size > b.size);
}
bool dirCompModD(struct dirEntry a, struct dirEntry b){
	return (strcmp(a.mod, b.mod) > 0);
}

void serveDirectory(FILE * socket, char * path, struct qparams q){
	writeResponseFile(socket, 200, "Document follows", "text/html");
	char * dirTemp = 
	"<html>"
	"<head>"
	"    <title>Index of %s</title>"
	"</head>"
	"<body>"
	"    <h1>Index of %s</h1>"
	"    <table>"
	"        <tr>"
	"            <th valign=\"top\"><img src=\"/%s/icons/menu.gif\" alt=\"[PARENTDIR]\"></th>"
	"            <th><a href=\"?C=N&O=D\">Name</a></th>"
	"            <th><a href=\"?C=M&O=A\">Last modified</a></th>"
	"            <th><a href=\"?C=S&O=A\">Size</a></th>"
	"        </tr>"
	"        <tr>"
	"            <th colspan=\"5\">"
	"                <hr>"
	"            </th>"
	"        </tr>"
	"      %s "
	"        <tr>"
	"            <th colspan=\"5\">"
	"                <hr>"
	"            </th>"
	"        </tr>"
	"    </table>"
	"    <address>Ammu Server at www.data.cs.purdue.edu Port: %d</address>"
	"</body>"
	"</html>";
	
	char * rowTemp = 
	"        <tr>"
	"            <td valign=\"top\"><img src=\"%s\" alt=\"[PARENTDIR]\"></td>"
	"            <td><a href=\"%s\">%s</a></td>"
	"            <td>%s</td>"
	"            <td align=\"right\">%d</td>"
	"        </tr>";

	DIR * dirs = opendir(path);

	struct dirent * dir;

	char rows[10000];
	strcpy(rows, "");


	std::vector<dirEntry> dirEntries;

	while((dir = readdir(dirs)) != NULL){
		char fullPath[512];
		strcpy(fullPath, "");
		strcat(fullPath, path);
		if(fullPath[strlen(fullPath)-1] != '/'){
			strcat(fullPath, "/");
		}
		strcat(fullPath, dir->d_name);

		struct stat st;
    	stat(fullPath, &st);

    	time_t rawtime = st.st_mtime;
		struct tm * timeinfo;
		timeinfo = localtime ( &rawtime );
		char tbuffer[80];
		strftime (tbuffer,80,"%F %T",timeinfo);

		char iconUrl[80];
		strcpy(iconUrl, "/");
		strcat(iconUrl, SECRET);

		if(S_ISDIR(st.st_mode)){
			strcat(iconUrl, "/icons/menu.gif");
	    }else if(strstr(dir->d_name, ".svg") || strstr(dir->d_name, ".gif") || strstr(dir->d_name, ".jpg") || strstr(dir->d_name, ".png") || strstr(dir->d_name, ".xbm")){
	    	strcat(iconUrl, "/icons/image.gif");
	    }else if(strstr(dir->d_name, ".html") || strstr(dir->d_name, ".txt")){
			strcat(iconUrl, "/icons/text.gif");
	    }else{
	    	strcat(iconUrl, "/icons/binary.gif");
	    }

	    char ahref[50];
	    strcpy(ahref, "./");
	    if(!strcmp(dir->d_name, "..")){
	    	strcat(ahref, ".");
	    }else if(!strcmp(dir->d_name, ".")){
	    	strcpy(ahref, "#");
	    }else{
	    	strcat(ahref, dir->d_name);
	    }

	    struct dirEntry entry;
	    entry.iconUrl = strdup(iconUrl);
	    entry.ahref = strdup(ahref);
	    entry.name = strdup(dir->d_name);
	    entry.mod = strdup(tbuffer);
	    entry.size = (int)st.st_size;
	    dirEntries.push_back(entry);
	}

	int C = 0;
	int O = 0;
	
	for(int i = 0; i < q.n; i++)
	{
		if(q.params[i][0] == 'C'){
			char * val = strchr(q.params[i], '=') + 1;
			if(*val == 'N'){
				C = 0;
			}else if(*val == 'M'){
				C = 1;
			}else if(*val == 'S'){
				C = 2;
			}
		}else if(q.params[i][0] == 'O'){
			char * val = strchr(q.params[i], '=') + 1;
			if(*val == 'A'){
				O = 0;
			}else if(*val == 'D'){
				O = 1;
			}
		}
	}
	
	if(O == 0){
		if(C == 0){
			std::sort (dirEntries.begin(), dirEntries.end(), dirCompName);
		}else if(C == 1){
			std::sort (dirEntries.begin(), dirEntries.end(), dirCompMod);
		}else if(C == 2){
			std::sort (dirEntries.begin(), dirEntries.end(), dirCompSize);
		}
	}else{
		if(C == 0){
			std::sort (dirEntries.begin(), dirEntries.end(), dirCompNameD);
		}else if(C == 1){
			std::sort (dirEntries.begin(), dirEntries.end(), dirCompModD);
		}else if(C == 2){
			std::sort (dirEntries.begin(), dirEntries.end(), dirCompSizeD);
		}
	}

	for(int i = 0; i < dirEntries.size(); i++){
		struct dirEntry entry = dirEntries[i];
		char row[1000];
		strcpy(row,"");
		//printf("%s\t%s\t%s\t%s\t%d\n", entry.iconUrl, entry.ahref, entry.name, entry.mod, entry.size);
		sprintf(row, rowTemp, entry.iconUrl, entry.ahref, entry.name, entry.mod, entry.size);
		strcat(rows, row);
		free(entry.iconUrl);
		free(entry.ahref);
		free(entry.name);
		free(entry.mod);
	}
	
	fprintf(socket, dirTemp, path, path, SECRET, rows, PORT);
}

void cgi_bin(FILE * socket, int fd, char * path, char * query, char * method){
	if(strstr(path, ".so")){
		fprintf(socket, "HTTP/1.1 %d %s\r\nServer: ammu/1.0\r\n", 200, "Document follows");
		fflush(socket);
		if (openlibs["hello.so"] == NULL)
		{
			printf("Loading dl\n");
			void * dl = dlopen("./http-root-dir/cgi-bin/hello.so", RTLD_LAZY);
		  openlibs["hello.so"] = dl;
		  if(dl == NULL){
		  	printf("DLOPEN ERROR\n");
		  }
		}

		printf("dlopen done\n");
		void (*hrun)(int, char *);
		hrun = (void(*)(int, char*))dlsym(openlibs["hello.so"], "httprun");
		if(hrun == NULL){
			printf("DLOPEN SYM\n");
		}
		printf("Function loaded\n");
		(*hrun)(fd, query);
		printf("Function executedf\n");
		return;
	}else{
		printf("Forking process\n");
		pid_t slave = fork();
		if(slave == 0){
			char pbuf[200];
			strcpy(pbuf, "./http-root-dir");
			strcat(pbuf, path);
			fprintf(socket, "HTTP/1.1 %d %s\r\nServer: ammu/1.0\r\n", 200, "Document follows");
			fflush(socket);
			printf("Socket flushed, path: %s\n, query: %s\n", pbuf, query);
			setenv("REQUEST_METHOD", method, 1);
			if(query != NULL){
				setenv("QUERY_STRING", query, 1);
			}
			printf("Env set, duping stdout and calling execvp\n");
			fflush(stdout);
			dup2(fd, 1);
			execvp(pbuf, NULL);
			perror("execvp");
			fprintf(stdout, "<h1>CGI Error</h1><p>An error has occured with the script execution.</p>");
			fflush(stdout);
			_exit(1);
			return;
		}
		int stat;
		waitpid(slave, &stat, 0);
	}
}

void statsPage(FILE * socket, char * path){
	writeResponseFile(socket, 200, "Document follows", "text/html");
	char * res = 
	"<html>"
	"<head>"
	"    <title>Server Stast</title>"
	"</head>"
	"<body>"
	"    <h1>Server Stats for %s</h1>"
	" <ul><li>Uptime: %f seconds</li><li>Number of Requests: %d</li><li>Min Request Time: %f</li><li>Max Request Time: %f</li></ul>"
	"    <address>Ammu Server at www.data.cs.purdue.edu Port: %d</address>"
	"</body>"
	"</html>";
	time_t now;
	time(&now);  /* get current time; same as: now = time(NULL)  */
	fprintf(socket, res, NAME, difftime(now,start_time), *numRequests, *min_time, *max_time, PORT);
}

 
void logRequest(struct sockaddr_in client, char * request){
	FILE * log = fopen("./http-log.txt", "a");
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char buffer[80];
	request[strlen(request)-1] = 0;
	strftime (buffer,80,"%F %T",timeinfo);
	printf("[%s] %d.%d.%d.%d %s\n", buffer, (client.sin_addr.s_addr) & 0xFF,(client.sin_addr.s_addr >> 8) & 0xFF,(client.sin_addr.s_addr >> 16) & 0xFF,(client.sin_addr.s_addr>> 24) & 0xFF, request);
	fprintf(log, "[%s] %d.%d.%d.%d %s\n", buffer, (client.sin_addr.s_addr) & 0xFF,(client.sin_addr.s_addr >> 8) & 0xFF,(client.sin_addr.s_addr >> 16) & 0xFF,(client.sin_addr.s_addr>> 24) & 0xFF, request);
	free(request);
	fclose(log);
} 