

typedef struct recurseDirArgs{
	char* tarColName;
	char* inputDir;
	char* outputDir;
}recurseDirArgs;

typedef struct conServArgs{
	char* dataToSort;
	char* colName;
	char* action;
	int socketDesc;
	int collecId;

}conServArgs;

/*struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    socklen_t        ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;
    struct addrinfo *ai_next;
};*/


int readSocket(int socket, char **dataPtr);
char *readCmdArgs(int,  char**);
char *readSTDIN();
int recurseDir(recurseDirArgs *);
int mergeDataLinear(Record**, int, int);
void addTid(pthread_t*, int);
void clientToServer(conServArgs *);
int getClientId(int);