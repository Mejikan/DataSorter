

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

}conServArgs;




char *readCmdArgs(int,  char**);
char *readSTDIN();
int recurseDir(recurseDirArgs *);
int mergeDataLinear(Record**, int, int);
void addTid(pthread_t*, int);
void clientToServer(conServArgs *);