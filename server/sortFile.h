enum DataType {
	TYPE_STRING,
	TYPE_NUMERIC,
	TYPE_NULL
};

typedef
struct _Field{
    enum DataType type;
    char *data;
}
Field;

typedef
struct _Record{
    Field *fields;
    int numOfCols;
}
Record;	

typedef 
	struct Node {
		int id;
		struct Node *next;
		pthread_mutex_t nextLock; // gain if modifying next
		int numOfRecs; // if -1, then it is unused
		Record **recs;
		pthread_mutex_t recsLock; // gain if modifying data(recs)
	}
Node;

typedef struct _regFileArgs{
	int structureId;
	char* csvStr;
	char* targetColName;
	int (*mergeData)(Record**, int, int, int);
}regFileArgs;

int findColumnIndex(char *tarColName);
int parseDataIntoRecs(char *csvData, Record ***dest, int *totalRecs);
short determineTypeofData(char *);
void trim(char*);
void mergeSortInt(Record**, int, int);			
void mergeInt(Record**, Record**, Record**, int, int, int);
void printArray(Record**, int, char**);
Record *csvToRec(char *);
int sortCSV(regFileArgs *);
void freeRec(Record*);
char* readFile(char *);