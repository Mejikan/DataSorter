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

typedef struct _regFileArgs{
	char* filePath;
	char* targetColName;
	int (*mergeData)(Record**, int, int);	
}regFileArgs;

short determineTypeofData(char *);
void trim(char*);
void mergeSortInt(Record**, int, int);			
void mergeInt(Record**, Record**, Record**, int, int, int);
void printArray(Record**, int, FILE*);
Record *csvToRec(char *);
int sortCSV(regFileArgs *);
void freeRec(Record*);
char* readFile(char *);