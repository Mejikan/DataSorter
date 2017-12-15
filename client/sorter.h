

typedef struct recurseDirArgs{
	char* inputDir;
}recurseDirArgs;

typedef struct conServArgs{
	char* dataToSort;
	char* action;
	int socketDesc;
	int collecId;

}conServArgs;


int readSocket(int socket, char **dataPtr);
int recurseDir(recurseDirArgs *);
void clientToServer(conServArgs *);