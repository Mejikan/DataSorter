#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sorter_server.h"
#include "xmlparse.h"
#include "sortFile.h"

Node *collections = NULL;

typedef
    struct ClientArgs {
        int socket;
    }  
ClientArgs;

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

char *nowTime(){
    time_t utc = time(NULL);
    struct tm *ltime = localtime(&utc);

    char *res = (char*)malloc(32);
    memset(res, 0, 32);
    //sprintf(res, "%d %d %d:%d:%d", ltime->tm_mon, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    strftime(res, 32-1, "%b \%d %H:%M:\%S", ltime);
    return res;
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

void runClient(ClientArgs *args){
    int socket = args->socket;

    while (1){
        char *dataIn = NULL;
        int readRes = readSocket(socket, &dataIn);
        //printf("READ from client %d: %s\n", readRes, dataIn);
        if (readRes == 0){
            break;
        } else if (readRes > 0){

            ClientAction action = NONE;
            int collectionId = -1;
            char *data = NULL;
            char *tarColName = NULL;

            XMLDoc *doc = fromXmlStr(dataIn);
            free(dataIn);
            if (doc != NULL && doc->numOfChildren > 0){
                //puts("converted to xml");
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
            } else {
                freeXMLDoc(doc);
                char errMsg[] = "<response><error>Empty request!</error></response>\r\n";

                if (send(socket, errMsg, strlen(errMsg), 0) < 0){
                    perror("Failed to send.");
                } else {
                    //puts("Message sent");
                }

                // handle error case
                //puts("failed to be converted to xml");
            }

            if (action == DISCONNECT){
                freeXMLDoc(doc);
                break;
            } else if (action == SORT && data != NULL){ // ADD data
                //puts("sorting...");
                data = fromEscStr(data);
                
                if (data != NULL){
                    
                    Record **parsedrecs = NULL;
                    int numOfRecs = 0;

                    if (parseDataIntoRecs(data, &parsedrecs, &numOfRecs) > -1 && numOfRecs > 0 && parsedrecs != NULL){
                        
                        Node *collection = NULL;

                        if (collectionId == -1){ // create new collection
                            getEmptyCollection(&collection);
                            collection->numOfRecs = numOfRecs;
                            collection->recs = parsedrecs;
                            
                            pthread_mutex_unlock(&(collection->recsLock));

                            // WRITE COLLECTION ID TO CLIENT
                            char payloadStr[strlen("<response><collectionId></collectionId></response>\r\n") + 24 + 1];
                            sprintf(payloadStr, "<response><collectionId>%d</collectionId></response>\r\n", collection->id);
                            if (send(socket, payloadStr, strlen(payloadStr), 0) < 0){
                                perror("Failed to send.");
                            } else {
                                //printf("Message sent. Pass id #%d\n", collection->id);
                            }

                        } else { // find existing collection
                            //puts("finding existing collection");
                            if (findCollection(collectionId, &collection) < 0){
                                //puts("Cannot find requested collection.");

                                // WRITE TO CLIENT
                                char payloadStr[strlen("<response><collectionId></collectionId><error>Collection not found!</error></response>\r\n") + 24 + 1];
                                sprintf(payloadStr, "<response><collectionId>%d</collectionId><error>Collection not found!</error></response>\r\n", collection->id);
                                if (send(socket, payloadStr, strlen(payloadStr), 0) < 0){
                                    perror("Failed to send.");
                                } else {
                                    //puts("Message sent");
                                }

                            } else {
                                
                                // merge collection
                                pthread_mutex_lock(&(collection->recsLock));
                                
                                Record **colRecs = collection->recs;
                                int colNumOfRecs = collection->numOfRecs;
                                //colRecs = (Record**) realloc(colRecs, colNumOfRecs + numOfRecs);
                                collection->recs = (Record**) malloc(colNumOfRecs + numOfRecs);
                                int i = 0;
                                //colNumOfRecs = colNumOfRecs + numOfRecs;
                                while (i < colNumOfRecs){
                                    collection->recs[i] = colRecs[i];
                                    i++;
                                }
                                int j = 0;
                                while (j < numOfRecs){
                                    collection->recs[i] = parsedrecs[j];
                                    i++;
                                    j++;
                                }
                                colNumOfRecs = colNumOfRecs + numOfRecs;
                                collection->numOfRecs = colNumOfRecs;

                                free(colRecs);
                                pthread_mutex_unlock(&(collection->recsLock));
                                
                                // WRITE TO CLIENT
                                char payloadStr[strlen("<response><collectionId></collectionId><collectionSize></collectionSize></response>\r\n") + 64 + 1];
                                sprintf(payloadStr, "<response><collectionId>%d</collectionId><collectionSize>%d</collectionSize></response>\r\n", collection->id, i);
                                if (send(socket, payloadStr, strlen(payloadStr), 0) < 0){
                                    perror("Failed to send.");
                                } else {
                                    //puts("Message sent");
                                }
                            }
                        }
                    }

                    free(data);
                    freeXMLDoc(doc);
                }
            } else if (action == DUMP && tarColName != NULL && collectionId > -1){ // SORT and DUMP
                Node *collection = NULL;
                
                int findStat = findCollection(collectionId, &collection);
                
                int tarColIdx = findColumnIndex(tarColName);
                if (tarColIdx >= 0 && findStat >= 0){
                    
                    pthread_mutex_lock(&(collection->recsLock));
                    mergeSortInt(collection->recs, collection->numOfRecs, tarColIdx);
                    
                    char *csvStr = NULL;
                    printArray(collection->recs, collection->numOfRecs, &csvStr);
                    
                    if (csvStr == NULL){
                        csvStr = (char*)malloc(1);
                        csvStr[0] = 0;
                    }

                    char *csvHeaderStr = "color,director_name,num_critic_for_reviews,duration,director_facebook_likes,actor_3_facebook_likes,actor_2_name,actor_1_facebook_likes,gross,genres,actor_1_name,movie_title,num_voted_users,cast_total_facebook_likes,actor_3_name,facenumber_in_poster,plot_keywords,movie_imdb_link,num_user_for_reviews,language,country,content_rating,budget,title_year,actor_2_facebook_likes,imdb_score,aspect_ratio,movie_facebook_likes\n";
                    char *fullCsvStr = (char*)malloc(strlen(csvHeaderStr) + strlen(csvStr) + 1);
                    sprintf(fullCsvStr, "%s%s", csvHeaderStr, csvStr);
                    free(csvStr);
                    char *escapedPayloadData = toEscStr(fullCsvStr);
                    free(fullCsvStr);

                    char *payloadStrMetaFormat = "<response><data></data></response>\r\n";
                    int payloadStrSize = strlen(escapedPayloadData) + strlen(payloadStrMetaFormat) + 1;

                    char *payloadStr = (char*)malloc(payloadStrSize);                    
                    sprintf(payloadStr, "<response><data>%s</data></response>\r\n", escapedPayloadData);
                    free(escapedPayloadData);
                    
                    send(socket, payloadStr, payloadStrSize-1, 0);
                    
                    free(payloadStr);
                    pthread_mutex_unlock(&(collection->recsLock));
                }
            }
        }
    }

    //printf("Closed socket connection %d\n", socket);
    close(socket);
}

int main(int argc, char **argv){
    short cmdArgsValid = 0;
    if (argc >= 3){
        if (strcmp("-p", argv[1]) == 0){
            cmdArgsValid = 1;
        }
    }

    if (cmdArgsValid == 0){
        puts("Invalid command line arguments.");
        return -1;
    }

    int port = atoi(argv[2]);

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
        //printf("Server listening on port %d\n", port);
        printf("Received connections from: ");
    }
    fflush(stdout);

    Node *collection = (Node*)malloc(sizeof(Node));
    collection->id = 0;
    collection->next = NULL;
    collection->numOfRecs = -1;
    collection->recs = NULL;
	pthread_mutex_init(&(collection->nextLock), NULL);
    pthread_mutex_init(&(collection->recsLock), NULL);
    collections = collection;

    int size = 10;
	pthread_t* tid = (pthread_t*)malloc(size * sizeof(pthread_t));
	int numTid = 0;

    int clientSocket = -1;
    
    while (1){
        clientSocket = accept(serverSocket, (struct sockaddr *)&addr, &addrlen);
        if (clientSocket < 0){
            printf("Failed to accept socket.\n");
            return -1;
        } else {
            //printf("Client connected! %d\n", numTid);
            char ipstr[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &(addr.sin_addr), ipstr, INET_ADDRSTRLEN );
            //puts("huy");
            char *timestamp = nowTime();
            printf("%s [%s], ", ipstr, timestamp);
            fflush(stdout);
            free(timestamp);
            //puts("iop");

            ClientArgs *args = (ClientArgs*) malloc(sizeof(ClientArgs));
            args->socket = clientSocket;

            if ( (numTid+1) > size ){
                size = (size + 10);
                tid = (pthread_t*)realloc(tid, size * sizeof(pthread_t));
            }

            pthread_create(&tid[numTid], NULL, (void*)runClient, args);
            numTid++;
        }
    }

    return 0;
}


