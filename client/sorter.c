#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include "sortFile.h"
#include "sorter.h"


	
char *outputFile;
pthread_mutex_t rec_lock;
pthread_mutex_t tid_lock;
pthread_mutex_t action_lock;

Record** gRecs;
int gNumOfRecs = 0;
pthread_t* gTid;
int gNumOfTids = 0;

char* hostName = NULL;

int main(int argc, char **argv){
	// --- READ CMD ARGS INTO BUFFER --- //
	
	char* inputDir = NULL;
	char* outputDir = NULL;
	char* tarColName = NULL;


	if(argc < 3){ // not enough args
		printf("Not enough arguments please re-enter.\n");
		return -1;
	}
	int i = 1; 
	while(i < argc){
		if(strcmp(argv[i], "-c") == 0){
			tarColName = argv[i+1];
			i++;
		}else if(strcmp(argv[i], "-d") == 0){
			inputDir = argv[i+1];
			i++;
		}else if(strcmp(argv[i], "-o") == 0){
			outputDir = argv[i+1];
			i++;
		}else if(strcmp(argv[i], "-h") == 0){		//added
			hostName = argv[i+1];
		}
		i++;
	}

	printf("host name: %s\n",hostName);

	
	
	
	if(inputDir == NULL){
		inputDir = ".";
	} else {
		DIR* iDir = (DIR*)opendir(inputDir);
		if (iDir == NULL){
			printf("The input directory specified does not exist or you do not have access to it.\n");
			return -1;
		} else {
			closedir(iDir);
		}
	}
	if(outputDir == NULL){
		outputDir = ".";
	} else {
		DIR* oDir = (DIR*)opendir(outputDir);
		if (oDir == NULL){
			printf("The output directory specified does not exist or you do not have access to it.\n");
			return -1;
		} else {
			closedir(oDir);
		}
	}
	//printf("%s\n",inputDir);

	//int initialPid = getpid();
	//printf("Initial PID: %d\n", initialPid);
	//printf("PIDS of all child processes: ");
	//fflush(stdout);



	//int numpid = recurseDir(tarColName, inputDir, outputDir);
/*
	if (getpid() == initialPid){ // initial process
		printf("\nTotal number of processes: %d\n", (numpid + 1));
		fflush(stdout);
		return 0;
	} else { // not initial process
		return numpid;
	}
	*/

	printf("Initial PID: %d\n", getpid());

	// malloc input dir
	char *inputDirTemp = (char*)malloc(strlen(inputDir) + 1);
	sprintf(inputDirTemp, "%s", inputDir);
	inputDir = inputDirTemp;

	outputFile = (char*)malloc(strlen(outputDir) + strlen("/AllFiles-sorted-") + strlen(tarColName) + strlen(".csv") + 1);
	sprintf(outputFile, "%s%s%s%s", outputDir, "/AllFiles-sorted-", tarColName, ".csv");

	recurseDirArgs *args = (recurseDirArgs*)malloc(sizeof(recurseDirArgs));
	args->tarColName = tarColName;
	args->inputDir = inputDir;
	args->outputDir = outputDir;

	gRecs = (Record**)malloc(1 * sizeof(Record*));
	gTid = (pthread_t*)malloc(1 * sizeof(pthread_t));
	pthread_mutex_init(&rec_lock, NULL);
	pthread_mutex_init(&tid_lock, NULL);
	recurseDir(args);
	pthread_mutex_destroy(&rec_lock);
	pthread_mutex_destroy(&tid_lock);

	if (gNumOfRecs > 0){
		FILE* fptr = fopen(outputFile, "w+");
		fprintf(fptr, "color,director_name,num_critic_for_reviews,duration,director_facebook_likes,actor_3_facebook_likes,actor_2_name,actor_1_facebook_likes,gross,genres,actor_1_name,movie_title,num_voted_users,cast_total_facebook_likes,actor_3_name,facenumber_in_poster,plot_keywords,movie_imdb_link,num_user_for_reviews,language,country,content_rating,budget,title_year,actor_2_facebook_likes,imdb_score,aspect_ratio,movie_facebook_likes\n");
		printArray(gRecs, gNumOfRecs, fptr);
		fclose(fptr);
	}

	printf("TIDS of all child threads: ");
	int j = 0;
	while (j < gNumOfTids){
		printf("%lu", gTid[j]);
		if (j != (gNumOfTids - 1)){
			printf(",");
		}
		j++;
	}
	printf("\nTotal number of threads: %d\n", gNumOfTids);

	//free(args);
	free(outputFile);

	return 0;
}

int mergeDataLinear(Record** recs, int numOfRecs, int columnName){
	// Merge recs with gRecs 
	pthread_mutex_lock(&rec_lock);

	Record** newGRecs = (Record**)malloc( (gNumOfRecs + numOfRecs) * sizeof(Record*) );
	mergeInt(gRecs, recs, newGRecs, gNumOfRecs, numOfRecs, columnName);
	free(gRecs);
	gRecs = newGRecs;
	gNumOfRecs = (gNumOfRecs + numOfRecs);

	pthread_mutex_unlock(&rec_lock);
	return 3;
}

int recurseDir(recurseDirArgs *dirArgs){
	


	char *inputDir = dirArgs->inputDir;
	char *outputDir = dirArgs->outputDir;
	char *tarColName = dirArgs->tarColName;
	free(dirArgs);

	DIR* dptr = (DIR*)opendir(inputDir);
	//DIR* origDptr = dptr;
	struct dirent* entry;
	
	int size = 10;
	pthread_t* tid = (pthread_t*)malloc(size * sizeof(pthread_t));
	int numTid = 0;
	
	if(dptr == NULL){
		printf("error\n");
		return 0;
	}
	
	 
	int sd = socket(AF_INET, SOCK_STREAM, 0);

	
	if(sd < 0){
		printf("Error, could not create socket\n");
		exit(0);
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(25566);
	
	//getaddrinfo

	struct hostent *hp;
	char buffer[1024];

	hp = gethostbyname(hostName);

	if(hp == 0){
		perror("gethostbyname failed \n");
		exit(1);
	}


	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);




	/*struct addrInfo* res;
	int pass = getaddrinfo("www.google.com", NULL, NULL, &res);
	printf("pass: %d\n", pass);

	if(pass){
		printf("ERROR, could not getaddrinfo\n");
		exit(0);
	}*/



	int connectStatus = connect(sd, (struct sockaddr *)&server, sizeof(server));
	
	//printf("connectStatus: %d\n", connectStatus);
		

	if(connectStatus < 0){
		printf("ERROR, connect faield\n");
		exit(0);
	}
	//printf("CONNECTED \n");
	
	
	
	while((entry = readdir(dptr)) != NULL){
		if(entry->d_type == DT_REG){ // checks if entry found is a regular file
			//check ext
			
			int len = strlen(entry->d_name);
			if('.' == entry->d_name[len - 4] && ('c' == entry->d_name[len - 3] || 'C' == entry->d_name[len - 3]) &&
				('s' == entry->d_name[len - 2] || 'S' == entry->d_name[len - 2]) &&
				('v' == entry->d_name[len - 1] || 'V' == entry->d_name[len - 1])){ // check if file found is .csv
					
				len = strlen(inputDir) + 1 + strlen(entry->d_name) + 1;
				//char inFileName[strlen(inputDir) + 1 + strlen(entry->d_name) + 1]; // in file path
				//sprintf(inFileName, "%s/%s", inputDir, entry->d_name);
				char *inFileName = (char*)malloc(len);
				sprintf(inFileName, "%s/%s", inputDir, entry->d_name);

				conServArgs* args = (conServArgs*) malloc(sizeof(conServArgs));
				args->dataToSort = inFileName;
				args->colName = tarColName;
				args->action = "sort";
				args->socketDesc = sd;
				
				
				if ( (numTid+1) > size ){
					size = (size + 10);
					tid = (pthread_t*)realloc(tid, size * sizeof(pthread_t));
				}

				pthread_create(&tid[numTid], NULL, (void*)clientToServer, args);
				numTid++;
				
				/*else{
					printf("%d,", pid);
					fflush(stdout);
					numPid++;
				}*/
			}
		}
		else if(entry->d_type == DT_DIR){ // checks if entry found is a directory
			if( !(strcmp(entry->d_name, ".") == 0) && !(strcmp(entry->d_name, "..") == 0) ){	
				int len = strlen(inputDir) + 1 + strlen(entry->d_name) + 1;
				//char dirPath[strlen(inputDir) + 1 + strlen(entry->d_name) + 1]; // entry's directory path
				//sprintf(dirPath, "%s/%s", inputDir, entry->d_name);
				char *dirPath = (char*)malloc(len);
				sprintf(dirPath, "%s/%s", inputDir, entry->d_name);

				recurseDirArgs *args = (recurseDirArgs*)malloc(sizeof(recurseDirArgs));
				args->tarColName = tarColName;
				args->inputDir = dirPath;
				args->outputDir = outputDir;

				if ( (numTid+1) > size ){
					size = (size + 10);
					tid = (pthread_t*)realloc(tid, size * sizeof(pthread_t));
				}

				pthread_create(&tid[numTid], NULL, (void*)recurseDir, args);
				numTid++;

				/*
				int pid = fork();
				if(pid == 0){
					numPid = recurseDir(tarColName, dirPath, outputDir);
					//free(origDptr);
					closedir(dptr);
					return numPid;
				}
				else{
					printf("%d,", pid);
					fflush(stdout);
					numPid++;
				}*/
			} 
		}
	}
	
	int i = 0;
	while(i < numTid){
		/*int retVal = 0;
		wait(&retVal);
		if (WIFEXITED(retVal)){
			totalNumPid += WEXITSTATUS(retVal);
			i++;
		}*/
		//int retVal = 0;
		//int *retValPtr = &retVal;
		pthread_join(tid[i], NULL);
		//printf("return value: %d", retVal);
		i++;
	}
	addTid(tid, numTid);

	//printf("[%s] All threads joined!\n", inputDir);
	free(tid);
	free(inputDir);
	//free(origDptr);
	closedir(dptr);
	return 1;
	
}

void addTid(pthread_t* tids, int numOfTids){
	pthread_mutex_lock(&tid_lock);
	
	gTid = (pthread_t*)realloc( gTid, (gNumOfTids + numOfTids) * sizeof(pthread_t) );
	int i = 0;
	while (i < numOfTids){
		gTid[gNumOfTids + i] = tids[i];
		i++;
	}
	gNumOfTids = (gNumOfTids + numOfTids);

	pthread_mutex_unlock(&tid_lock);
}

char *readCmdArgs(int argc, char** argv){
	short bufferSize = 1;
	char *buffer;
	buffer = (char*) malloc(bufferSize * sizeof(char)); // reserve 16 chars of initial space
	if (buffer == NULL){
		return NULL; // failed to allocate memory
	}
	buffer[0] = '\0';

	short i0 = 1;
	if (i0 < argc){
		while (i0 < argc){
			short nameLenDiff = bufferSize - (bufferSize + strlen(argv[i0]));
			if (i0 > 1){
				nameLenDiff-=1;
			}
			if (nameLenDiff < 0){ // allocate more memory
				nameLenDiff *= -1;
				//printf("new size: %d \n", bufferSize + nameLenDiff);
				buffer = realloc(buffer, bufferSize + nameLenDiff);
				bufferSize += nameLenDiff;
				if (buffer == NULL){
					return NULL; // failed to allocate memory
				}
			}

			if (i0 > 1){
				strcat(buffer, " ");
			}
			strcat(buffer, argv[i0]);
			//printf("%d %lu %s\n", bufferSize, strlen(buffer), buffer);
			i0++;
		}
	} else {
		buffer[0] = '\0';
	}

	return buffer;
}

char *readSTDIN(){
	int contentSize = 1;
	char *content = (char*) malloc(contentSize * sizeof(char));
	if (content == NULL){
		return NULL;
	}
	char buffer[1024];
	buffer[0] = '\0';
	content[0] = '\0';
	
	while ( fgets(buffer, 1024, stdin) != NULL ){
		int contSizeDiff = contentSize - (contentSize + strlen(buffer) + 1);
		if (contSizeDiff < 0){ // allocate more memory
		  contSizeDiff *= -1;
		  content = realloc(content, contentSize + contSizeDiff);
		  contentSize += contSizeDiff;
		  if (content == NULL){
			return NULL; // failed to allocate memory
		  }
		}
		strcat(content, buffer);
	}

	return content;
}

void printArray(Record** records, int len, FILE* fptr){
    int recIdx = 0;
	while (recIdx < len){
		Record *record = records[recIdx];
		int fieldIdx = 0;
		while (fieldIdx < record->numOfCols){
			if (fieldIdx != 0){
				//printf(",");
				fprintf(fptr, ",");
			}
			if (!strchr(record->fields[fieldIdx].data, ',')){
				//printf("%s", record->fields[fieldIdx].data);
				fprintf(fptr, "%s", record->fields[fieldIdx].data);
			} else {
				//printf("\"%s\"", record->fields[fieldIdx].data);
				fprintf(fptr, "\"%s\"", record->fields[fieldIdx].data);
			}
			fieldIdx++;
		}
		//printf("\n");
		fprintf(fptr, "\n");
		recIdx++;
	}
}

int readSocket(int socket, char **dataPtr){
    char *delimTerm = "\r\n";

    char *dataIn = (char*)malloc(1);
    dataIn[0] = 0;
    char buff[1024];
    memset(&buff, 0, 1024);

    while (1){
        short bytes = recv(socket, buff, 1024, 0);
        if (bytes < 0){
            perror("Failed to read from client");
            free(dataIn);
            return -1;
        } else if (bytes == 0){ //EOF (end of stream)
            *dataPtr = dataIn;
            return strlen(dataIn);
        } else {
            dataIn = (char*) realloc(dataIn, bytes + strlen(dataIn) + 1);
            strcat(dataIn, buff);
            if (strstr(dataIn, delimTerm) == (dataIn + bytes - strlen(delimTerm))){
                *dataPtr = dataIn;
                dataPtr[bytes - strlen(delimTerm)] = 0;
                return strlen(dataIn) - strlen(delimTerm);
            }
            memset(&buff, 0, 1024);
        }
    }
}


void clientToServer(conServArgs* args){
	//func that change struct into toString
	char* data = readFile(args->dataToSort);
	char* colName = args->colName;
	char* action = args->action;
	int sd = args->socketDesc;
	//printf("sd in func: %d\n", sd);
	
	/*change param to long formatted string*/
	char* message = (char*)malloc(strlen(data)+strlen(colName)+strlen(action)+strlen("<doc><data></data><colName></colName><action></action></doc>\r\n")+1);
	sprintf(message, "<doc><data>%s</data><colName>%s</colName><action>%s</action></doc>\r\n", data, colName, action);

	
	/*mutex lock*/
	pthread_mutex_init(&action_lock, NULL);
	/*send message*/
	
	printf("********\n");
	int bytes = send(sd, message, strlen(message), 0);
	printf("++++++++\n");
	printf("%d\n", bytes);

	char* recMsg = NULL;
	/*wait for response*/
	int rec;
	//printf("right before while\n");
	while(1){
		//printf("right before readSocket in while\n");
		rec = readSocket(sd, &recMsg);
		//printf("right after readSocket in while\n");
		//printf("inside loop: %s\n", recMsg);
		if(rec == -1){
			printf("failed.\n");
		}else{
			break;
		}
	}
	printf("message: %s \n", recMsg);
	pthread_mutex_destroy(&action_lock);
	
}


