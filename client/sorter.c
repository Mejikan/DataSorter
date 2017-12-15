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
#include "xmlparse.h"


FILE *outFilePtr;
pthread_mutex_t action_lock;
pthread_mutex_t id_lock;

int clientId = -1;

char* hostName = NULL;
int portNumber = -1;
char* tarColName = NULL;

int main(int argc, char **argv){
	char* inputDir = NULL;
	char* outputDir = NULL;


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
		}else if(strcmp(argv[i], "-p") == 0){
			portNumber = atoi(argv[i+1]);
		}
		i++;
	}

	if(hostName == NULL || portNumber == -1 || tarColName == NULL){
		puts("Missing command line arguments.");
		return -1;
	}
	
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

	printf("Initial PID: %d\n", getpid());

	// malloc input dir
	char *inputDirTemp = (char*)malloc(strlen(inputDir) + 1);
	sprintf(inputDirTemp, "%s", inputDir);
	inputDir = inputDirTemp;

	char *outputFile = (char*)malloc(strlen(outputDir) + strlen("/AllFiles-sorted-") + strlen(tarColName) + strlen(".csv") + 1);
	sprintf(outputFile, "%s%s%s%s", outputDir, "/AllFiles-sorted-", tarColName, ".csv");
	outFilePtr = fopen(outputFile, "w+");

	pthread_mutex_init(&id_lock, NULL);

	recurseDirArgs *args = (recurseDirArgs*)malloc(sizeof(recurseDirArgs));
	args->inputDir = inputDir;

	recurseDir(args);

	fclose(outFilePtr);
	free(outputFile);

	pthread_mutex_destroy(&id_lock);
	return 0;
}

int writeDataToFile(char *sortedCsvData){
	fprintf(outFilePtr, "%s", sortedCsvData);
}


int recurseDir(recurseDirArgs *dirArgs){

	char *inputDir = dirArgs->inputDir;
	free(dirArgs);

	DIR* dptr = (DIR*)opendir(inputDir);
	struct dirent* entry;
	
	int size = 10;
	pthread_t* tid = (pthread_t*)malloc(size * sizeof(pthread_t));
	int numTid = 0;
	
	if(dptr == NULL){
		printf("error\n");
		return 0;
	}

	while((entry = readdir(dptr)) != NULL){
		if(entry->d_type == DT_REG){ // checks if entry found is a regular file
			
			int len = strlen(entry->d_name);
			if('.' == entry->d_name[len - 4] && ('c' == entry->d_name[len - 3] || 'C' == entry->d_name[len - 3]) &&
				('s' == entry->d_name[len - 2] || 'S' == entry->d_name[len - 2]) &&
				('v' == entry->d_name[len - 1] || 'V' == entry->d_name[len - 1])){ // check if file found is .csv
					
				int sd = socket(AF_INET, SOCK_STREAM, 0);
	
				if(sd < 0){
					printf("Error, could not create socket\n");
					exit(0);
				}
				
				struct sockaddr_in server;
				server.sin_family = AF_INET;
				server.sin_port = htons(portNumber);
				
				//getaddrinfo
				struct hostent *hp;

				hp = gethostbyname(hostName);

				if(hp == 0){
					perror("Gethostbyname failed \n");
					exit(1);
				}

				memcpy(&server.sin_addr, hp->h_addr, hp->h_length);

				int connectStatus = connect(sd, (struct sockaddr *)&server, sizeof(server));
				
				if(connectStatus < 0){
					printf("Error, connect failed.\n");
					exit(0);
				}
	
				len = strlen(inputDir) + 1 + strlen(entry->d_name) + 1;
				char *inFileName = (char*)malloc(len);
				sprintf(inFileName, "%s/%s", inputDir, entry->d_name);

				conServArgs* args = (conServArgs*) malloc(sizeof(conServArgs));
				args->dataToSort = inFileName;
				args->action = "sort";
				args->socketDesc = sd;

				pthread_mutex_lock(&id_lock);
				args->collecId = clientId;
				if(clientId != -1){
					pthread_mutex_unlock(&id_lock);
				}
				
				
				if ( (numTid+1) > size ){
					size = (size + 10);
					tid = (pthread_t*)realloc(tid, size * sizeof(pthread_t));
				}

				pthread_create(&tid[numTid], NULL, (void*)clientToServer, args);
				numTid++;
			}
		}
		else if(entry->d_type == DT_DIR){ // checks if entry found is a directory
			if( !(strcmp(entry->d_name, ".") == 0) && !(strcmp(entry->d_name, "..") == 0) ){	
				int len = strlen(inputDir) + 1 + strlen(entry->d_name) + 1;

				char *dirPath = (char*)malloc(len);
				sprintf(dirPath, "%s/%s", inputDir, entry->d_name);

				recurseDirArgs *args = (recurseDirArgs*)malloc(sizeof(recurseDirArgs));
				args->inputDir = dirPath;

				if ( (numTid+1) > size ){
					size = (size + 10);
					tid = (pthread_t*)realloc(tid, size * sizeof(pthread_t));
				}

				pthread_create(&tid[numTid], NULL, (void*)recurseDir, args);
				numTid++;
			} 
		}
	}
	
	int i = 0;
	while(i < numTid){
		puts("Thread joined!");
		pthread_join(tid[i], NULL);
		i++;
	}

	int sdesc = socket(AF_INET, SOCK_STREAM, 0);
	if(sdesc < 0){
		printf("Error, could not create socket\n");
		exit(1);
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(portNumber);
	
	//getaddrinfo

	struct hostent *hp;

	hp = gethostbyname(hostName);

	if(hp == 0){
		perror("gethostbyname failed \n");
		exit(1);
	}

	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
	
	int connectStatus = connect(sdesc, (struct sockaddr *)&server, sizeof(server));
				
	if(connectStatus < 0){
		perror("Failed to connect to server");
		exit(0);
	}
	
	/*created single socket*/
	/*colName , collecId, action*/
	
	char* message = (char*)malloc(strlen(tarColName)+strlen("dump")+ 24 +strlen("<doc><colName></colName><action></action></doc><collectionId></collectionId>\r\n")+1);
	sprintf(message, "<doc><colName>%s</colName><action>%s</action><collectionId>%d</collectionId></doc>\r\n", tarColName, "dump", clientId);
			
	printf("Requesting dump from server for collection id %d.\n", clientId);
	send(sdesc, message, strlen(message), 0);
	free(message);
	char* recMsg = NULL;

	int rec;
	rec = readSocket(sdesc, &recMsg);
	if(rec == -1){
		perror("Failed to read socket");
	}
	close(sdesc);
	
	char* msgId = NULL;
	XMLDoc *doc = fromXmlStr(recMsg);
	free(recMsg);
	XMLDoc *child = doc->children[0];
	
	writeDataToFile(child->text);
	
	freeXMLDoc(doc);
	
	//printf("[%s] All threads joined!\n", inputDir);
	free(tid);
	free(inputDir);
	
	//free(origDptr);
	closedir(dptr);
	return 1;
	
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
	buff[0] = 0;

    while (1){
        short bytes = recv(socket, buff, 1023, 0);
        if (bytes < 0){
            perror("Failed to read socket");
            free(dataIn);
            return -1;
        } else if (bytes == 0){ //EOF (end of stream)
            *dataPtr = dataIn;
            return strlen(dataIn);
        } else {
            int dataInLen = bytes + strlen(dataIn) + 1;
            dataIn = (char*) realloc(dataIn, dataInLen);
            strncat(dataIn, buff, bytes);			
            char *delimPtr = dataIn + dataInLen - strlen(delimTerm) - 1;
            if ( (dataInLen - 1) >= strlen(delimTerm) && strcmp(delimPtr, delimTerm) == 0 ){				
                dataIn[dataInLen - strlen(delimTerm) -1] = 0;
                *(dataPtr) = dataIn;
                return strlen(dataIn);
            }
            memset(&buff, 0, 1024);
			buff[0] = 0;
        }
    }
}


void clientToServer(conServArgs* args){
	//func that change struct into toString
	char* data = readFile(args->dataToSort);
	char* action = args->action;
	int sd = args->socketDesc;
	int collecId = args->collecId;
	//printf("sd in func: %d\n", sd);
	
	/*change param to long formatted string*/
	char* escapedData = toEscStr(data);
	free(data);
	
	char* message = (char*)malloc(strlen(escapedData)+strlen(tarColName)+strlen(action)+
		24+strlen("<doc><data></data><colName></colName><action></action></doc><collectionId></collectionId>\r\n")+1);
	sprintf(message, "<doc><data>%s</data><colName>%s</colName><action>%s</action><collectionId>%d</collectionId></doc>\r\n",
		escapedData, tarColName, action, collecId);
	free(escapedData);

	int bytes = send(sd, message, strlen(message), 0);
	free(message);

	printf("Attempting to send csv data to be sorted for file: %s.\n", args->dataToSort);

	char* recMsg = NULL;
	
	int rec;
	
	rec = readSocket(sd, &recMsg);
	if(rec == -1){
		perror("Failed to read response from server");
	} else {
		printf("Read from server: %s\n", recMsg);
	}
	close(sd);
	
	int i = 0;
	char* msgId = NULL;
	XMLDoc *doc = fromXmlStr(recMsg);
	free(recMsg);
	while(i < doc->numOfChildren){
		XMLDoc *child = doc->children[i];
		if(strcasecmp(child->name, "collectionId") == 0){
			msgId = child->text;
		} else {
			printf("%s\n", child->name);
		}
		i++;
	}
	
	printf("Data sent successfully to server for file: %s. Stored in remote collection with id %s.\n", args->dataToSort, msgId);
	if (clientId == -1){
		clientId = atoi(msgId);
		pthread_mutex_unlock(&id_lock);
	}

	freeXMLDoc(doc);

	free(args->dataToSort);
	free(args);

}