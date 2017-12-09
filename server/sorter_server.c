#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "sorter_server.h"
#include "xmlparse.h"
#include "sortFile.h"

Node *collections = NULL;

typedef
    struct ClientArgs {
        int socket;
    }  
ClientArgs;

int port = 25565;

int getEmptyCollection(Node **dest){
    Node *ptr = collections;
    while (ptr != NULL){
        if (pthread_mutex_trylock(&(ptr->recsLock)) == 0){ // able to acquire data lock
            if (ptr->numOfRecs == -1){ // empty collection
                *dest = ptr;
                return 0;
            } else { // not empty
                pthread_mutex_unlock(&(ptr->recsLock));
            }
        }

        pthread_mutex_t *nextLock = &(ptr->nextLock);
        pthread_mutex_lock(nextLock);
        if (ptr->next == NULL){ // if at last element in collections
            // create new node
            Node *collection = (Node*)malloc(sizeof(Node));
            collection->id = (ptr->id)+1;
            collection->next = NULL;
            collection->numOfRecs = -1;
            collection->recs = NULL;
            pthread_mutex_init(&(collection->nextLock), NULL);
            pthread_mutex_init(&(collection->recsLock), NULL);
            pthread_mutex_lock(&(collection->recsLock));
            *dest = collection;
            ptr->next = collection;
            pthread_mutex_unlock(nextLock);
            return 1;
        } else { // not last element
            ptr = ptr->next;
            pthread_mutex_unlock(nextLock);
        }
    }
}

int findCollection(int id, Node **dest){
    if (id < 0){
        return -1;
    }

    Node *ptr = collections;
    while (ptr != NULL){
        if (ptr->id == id){ // found our collection
            *dest = ptr;
            return 0;
        }
        pthread_mutex_t *nextLock = &(ptr->nextLock);
        pthread_mutex_lock(nextLock);
        if (ptr->next == NULL){ // if at last element in collections
            pthread_mutex_unlock(nextLock);
            return -1;
        } else {
            ptr = ptr->next;
            pthread_mutex_unlock(nextLock);
        }
    }
}

int readSocket(int socket, char **dataPtr){
    char *delimTerm = "\r\n";

    char *dataIn = (char*)malloc(1);
    dataIn[0] = 0;
    char buff[1024];
    memset(&buff, 0, 1024);

    while (1){
        short bytes = recv(socket, buff, 1023, 0);
        if (bytes < 0){
            perror("Failed to read from client");
            free(dataIn);
            return -1;
        } else if (bytes == 0){ //EOF (end of stream)
            *dataPtr = dataIn;
            return strlen(dataIn);
        } else {
            int dataInLen = bytes + strlen(dataIn) + 1;
            dataIn = (char*) realloc(dataIn, dataInLen);
            strcat(dataIn, buff);
            char *delimPtr = dataIn + dataInLen - strlen(delimTerm) - 1;
            if ( strcmp(delimPtr, delimTerm) == 0 ){
                
                *dataPtr = dataIn;
                dataPtr[bytes - strlen(delimTerm)] = 0;
                return strlen(dataIn) - strlen(delimTerm);
            }
            memset(&buff, 0, 1024);
        }
    }
}

void runClient(ClientArgs *args){
    int socket = args->socket;

    while (1){
        char *dataIn = NULL;
        int readRes = readSocket(socket, &dataIn);
        printf("READ %d: %s\n", readRes, dataIn);
        if (readRes == 0){
            break;
        } else if (readRes > 0){
            puts(dataIn);

            ClientAction action = NONE;
            int collectionId = -1;
            char *data = NULL;
            char *tarColName = NULL;


            XMLDoc *doc = fromXmlStr(dataIn);
            if (doc != NULL && doc->numOfChildren > 0){
                int i = 0;
                while (i < doc->numOfChildren){
                    XMLDoc *child = doc->children[i];
                    if (strcasecmp(child->name, "action") == 0){
                        if (strcasecmp(child->text, "sort") == 0){
                            action = SORT;
                        } else if (strcasecmp(child->text, "dump") == 0){
                            action = DUMP;
                        } else if (strcasecmp(child->text, "disconnect") == 0){
                            action = DISCONNECT;
                        }
                    } else if (strcasecmp(child->name, "data") == 0){
                        data = child->text;
                    } else if (strcasecmp(child->name, "colName") == 0){
                        tarColName = child->text;
                    } else if (strcasecmp(child->name, "collectionId") == 0){
                        collectionId = atoi(child->text);
                    }
                    i++;
                }
            }

            if (action == DISCONNECT){
                break;
            } else if (action == SORT && data != NULL){ // ADD data
                data = fromEscStr(data);
                if (data != NULL){
                    Record **parsedrecs = NULL;
                    long numOfRecs = 0;

                    if (parseDataIntoRecs(data, &parsedrecs, &numOfRecs) > -1 && numOfRecs > 0 && parsedrecs != NULL){
                        Node *collection = NULL;

                        if (collectionId == -1){ // create new collection
                            getEmptyCollection(&collection);
                            
                            collection->numOfRecs = numOfRecs;
                            collection->recs = parsedrecs;
                            pthread_mutex_unlock(&(collection->recsLock));
                            // ~~~~WRITE COLLECTION ID TO CLIENT
                            char payloadStr[strlen("<response><id></id></response>\r\n") + 24 + 1];
                            sprintf(payloadStr, "<response><id>%d</id></response>\r\n", collection->id);
                            if (send(socket, payloadStr, strlen(payloadStr), 0) < 0){
                                perror("Failed to send.");
                            } else {
                                puts("Message sent");
                            }

                        } else { // find existing collection
                            if (findCollection(collectionId, &collection) < 0){
                                puts("Cannot find requested collection.");
                            } else {
                                // merge collection
                                pthread_mutex_lock(&(collection->recsLock));
                                collection->recs = (Record**) realloc(collection->recs, (collection->numOfRecs) + numOfRecs);
                                int j = 0;
                                int i = collection->numOfRecs;
                                collection->numOfRecs = (collection->numOfRecs) + numOfRecs;
                                while (i < (collection->numOfRecs)){
                                    collection->recs[i] = parsedrecs[j];
                                    j++;
                                    i++;
                                }
                                pthread_mutex_unlock(&(collection->recsLock));
                            }
                        }
                    }
                }
            } else if (action == DUMP && tarColName != NULL && collectionId > -1){ // SORT and DUMP
                Node *collection = NULL;
                if (findColumnIndex(tarColName) >= 0 && findCollection(collectionId, &collection) >= 0){
                    pthread_mutex_lock(&(collection->recsLock));

                    mergeSortInt(collection->recs, collection->numOfRecs, collectionId);
                    char *csvStr = NULL;
                    printArray(collection->recs, collection->numOfRecs, &csvStr);
                    if (csvStr != NULL){
                        char *payloadStrMetaFormat = "<response><data></data><error></error></response>";
                        int payloadStrSize = strlen(csvStr) + strlen(payloadStrMetaFormat) + 1;
                        char *payloadStr = (char*)malloc(payloadStrSize);
                        sprintf(payloadStr, "<response><data>%s</data><error></error></response>", csvStr);

                        send(socket, payloadStr, payloadStrSize, 0);

                        free(payloadStr);
                        free(csvStr);
                    } else {
                        puts("Data string is null!");
                    }

                    pthread_mutex_unlock(&(collection->recsLock));
                }
            }
        }
    }

    printf("Closed socket connection %d\n", socket);
    close(socket);
}

int main(int argc, char **argv){
    int serverSocket;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( port );

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0){
        puts("Failed to create socket.");
        return -1;
    }
    if ( bind(serverSocket, (struct sockaddr *)&addr, addrlen) < 0 ){
        printf("Failed to bind on port %d\n", port);
        return -1;
    }

    if (listen(serverSocket, 10) < 0){
        printf("Unable to listen on port %d\n", port);
        return -1;
    } else {
        printf("Server listening on port %d\n", port);
    }
    fflush(stdout);

    Node *collection = (Node*)malloc(sizeof(Node));
    collection->id = 0;
    collection->next = NULL;
    collection->numOfRecs = -1;
    collection->recs = NULL;
	pthread_mutex_init(&(collection->nextLock), NULL);
    pthread_mutex_init(&(collection->recsLock), NULL);

    int size = 10;
	pthread_t* tid = (pthread_t*)malloc(size * sizeof(pthread_t));
	int numTid = 0;

    int clientSocket = -1;
    puts("hello1");
    while (1){
        clientSocket = accept(serverSocket, (struct sockaddr *)&addr, &addrlen);
        if (clientSocket < 0){
            printf("Failed to accept socket.\n");
            return -1;
        } else {
            printf("Client connected! %d\n", numTid);
            ClientArgs *args = (ClientArgs*) malloc(sizeof(ClientArgs));
            args->socket = clientSocket;

            if ( (numTid+1) > size ){
                size = (size + 10);
                tid = (pthread_t*)realloc(tid, size * sizeof(pthread_t));
            }
            puts("hello2");

            pthread_create(&tid[numTid], NULL, (void*)runClient, args);
            puts("hello3");
            numTid++;
        }
    }

    return 0;
}


