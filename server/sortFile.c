#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sortFile.h"
#include "mergesort.c"

int parseDataIntoRecs(char *csvData, Record ***dest, long *totalRecs){
	if (dest == NULL || totalRecs == NULL || csvData == NULL || strlen(csvData) < 1){
		return -1;
	}

	char* inCsvStr = csvData;
	
	// --- PARSE CSV STRING INTO RECORDS --- //
	char *colNames;
	char *recTkn;
	char* reqCol[28] = {"color", "director_name","num_critic_for_reviews","duration","director_facebook_likes","actor_3_facebook_likes","actor_2_name","actor_1_facebook_likes","gross","genres","actor_1_name","movie_title","num_voted_users","cast_total_facebook_likes","actor_3_name","facenumber_in_poster","plot_keywords","movie_imdb_link","num_user_for_reviews","language","country","content_rating","budget","title_year","actor_2_facebook_likes","imdb_score","aspect_ratio","movie_facebook_likes"};

	recTkn = strtok_r(inCsvStr, "\n", &inCsvStr);
	colNames = recTkn;

	recTkn = strtok_r(NULL,"\n", &inCsvStr);
	if (recTkn == NULL){
		// file input has no records
		//printf("\n[Error - \"%s\"]: There are no records in file.\n", inFileName);
		return -1;					
	}
	//free(inFileName);
	
	Record **recs = (Record**)malloc(1 * sizeof(Record*));		//SHOULD THERE BE A '1' IN THIS MALLOC
	if (recs == NULL){ // failed to alloc memory
		printf("Failed to allocate memory.\n");
		return -1;
	}
	
	long recIdx = 0;
	while ( recTkn != NULL ){
		// convert tkn to record
		recs = (Record**)realloc(recs, (recIdx +1 )* sizeof(Record*));
		Record *rec = csvToRec(recTkn);
		recs[recIdx] = rec;
		recTkn = strtok_r(NULL, "\n", &inCsvStr);
		recIdx++;
	}
	*totalRecs = recIdx;
	*dest = recs;

	return 0;
}

int sortCSV(regFileArgs *args){
	
	char* inFileName = args->entryId;
	//char* fcontents = readFile(inFileName);
	char* fcontents = args->csvStr;
	char* inCsvStr = fcontents;
	char* tarColName = args->targetColName;
	
	// --- PARSE CSV STRING INTO RECORDS --- //
	char *colNames;
	char *recTkn;
	char* reqCol[28] = {"color", "director_name","num_critic_for_reviews","duration","director_facebook_likes","actor_3_facebook_likes","actor_2_name","actor_1_facebook_likes","gross","genres","actor_1_name","movie_title","num_voted_users","cast_total_facebook_likes","actor_3_name","facenumber_in_poster","plot_keywords","movie_imdb_link","num_user_for_reviews","language","country","content_rating","budget","title_year","actor_2_facebook_likes","imdb_score","aspect_ratio","movie_facebook_likes"};
	
	recTkn = strtok_r(inCsvStr, "\n", &inCsvStr);
	colNames = recTkn;
	
	// parse column names
	/*
	char headerStr[strlen(colNames)+1];
	snprintf(headerStr, strlen(colNames)+1, "%s", colNames);
	Record *header = csvToRec(headerStr);
	int i = 0;
	int j = 0;
	int successes = 0; // match column names
	int len = sizeof(reqCol)/sizeof(char*);

	if(header->numOfCols != len){
		printf("\n[Error - \"%s\"]: Number of columns not valid.\n", inFileName);
		freeRec(header);
		return -1;
	}
	
	while(i < len){
		while(j<len){
			//printf("stuff: %s %s\n", header->fields[j].data, reqCol[i]);
			if(strcmp(header->fields[j].data, reqCol[i]) == 0){		//free memory
				successes++;
				break;
			}
			j++;
		}
		i++;
	}
	

	if(successes != len){
		printf("\n[Error - \"%s\"]: Column names not valid.\n", inFileName);
		freeRec(header);
		return -1;
	}
	*/
	// find column index correlated to column name
	int tarColIdx = -1;
	int i = 0;
	while (i < 28){
		if (strcmp(tarColName, reqCol[i]) == 0){
			tarColIdx = i;
			break;
		}
		i++;
	}
	
	//freeRec(header);

	if (tarColIdx == -1){
		// column name that the user requested doesn't exist
		//printf("\n[Error - \"%s\"]: Column name that is requested does not exist.\n", inFileName);
		//free(inFileName);
		free(fcontents);
		free(args);
		return -1;
	}
	

	recTkn = strtok_r(NULL,"\n", &inCsvStr);
	if (recTkn == NULL){
		// file input has no records
		//printf("\n[Error - \"%s\"]: There are no records in file.\n", inFileName);
		return -1;					
	}
	//free(inFileName);
	

	Record **recs = (Record**)malloc(1 * sizeof(Record*));		//SHOULD THERE BE A '1' IN THIS MALLOC
	if (recs == NULL){ // failed to alloc memory
		printf("Failed to allocate memory.\n");
		return -1;
	}
	
	int recIdx = 0;
	while ( recTkn != NULL ){
		// convert tkn to record
		recs = (Record**)realloc(recs, (recIdx +1 )* sizeof(Record*));
		Record *rec = csvToRec(recTkn);
		recs[recIdx] = rec;
		recTkn = strtok_r(NULL, "\n", &inCsvStr);
		recIdx++;
	}
	int totalRecs = recIdx;
	mergeSortInt(recs, totalRecs, tarColIdx);
		
	args->mergeData(recs, totalRecs, tarColIdx, args->structureId);
		
	// --- CLEANUP --- //
	/*
	recIdx = 0;
	while (recIdx < totalRecs){
		Record *rec = recs[recIdx];
		//freeRec(rec);
		recIdx++;
	}
	*/
	free(recs);
	free(fcontents);
	free(args);

	return 0;
	
}

void freeRec(Record *rec){
	Field *fields = rec->fields;
	int fIdx = 0;
	while (fIdx < rec->numOfCols){
		free(fields[fIdx].data);
		fIdx++;
	}
	free(fields);
	free(rec);
}

short determineTypeofData(char *data){
	int i = 0;
	int len = strlen(data);
	short neg = 0;
	short dec = 0;
	if (len > 0){
		const char *valid = "0123456789-.";
		while (i < len){
			if (!strchr(valid, data[i])){
				return TYPE_STRING;
			} else if (data[i] == '-'){
				// allow only a single '-' at beginning and length of the string must be larger than 1
				if (neg == 1 || i != 0 || len < 2){
					return TYPE_STRING;
				} else {
					neg = 1;
				}
			} else if (data[i] == '.'){
				// allow only a single '.' and length of the string must be larger than 1
				if (dec == 1 || len < 2){
					return TYPE_STRING;
				} else {
					dec = 1;
				}
			}
			i++;
		}
		return TYPE_NUMERIC;
	}
	return TYPE_NULL;
}

void trimChar(char* target, char c){
	int len = strlen(target);
	char tmp[len+1];
	tmp[0] = '\0';
	int i = len - 1;
	// eliminate whitespace at end of string
	while (i > 0 && target[i] == c){
		i--;
	}
	snprintf(tmp, i+2, "%s", target);
	// eliminate whitespace at front of string
	len = strlen(tmp);
	i = 0;
	while (i < len && tmp[i] == c){
		i++;
	}
	snprintf(target, len+1, "%s", tmp+i);
}

void trim(char* target){
	trimChar(target, ' ');
	trimChar(target, '\r');

}



Record *csvToRec(char *csv){
	const short wordMemAlloc = 4;
	char **csvTkns = (char**) malloc(wordMemAlloc * sizeof(char*));
	if (csvTkns == NULL){ // failed to allocate memoy
		return NULL;
	}
	int maxWords = wordMemAlloc;
	int wordi = 0; // word counter (number of words so far)
	int ci = 0; // char counter (number of chars in current word so far)
	int csvi = 0; // another char counter (this one keeps track of how many chars of the csv string have been read)
	int csvLen = strlen(csv);
	short quote = 0;
	while (ci <= csvLen){
		
		// search for next delimiter
		char c = csv[ci];
		if (c == '\0' || (c == ',' && quote == 0) || (c == '"' && quote == 1)){
			int wordSize = ci - csvi + 1;
			//printf("sr: %d %d\n", wordi, maxWords);
			if (wordi == maxWords){
				maxWords += wordMemAlloc;
				csvTkns = (char**) realloc(csvTkns, maxWords * sizeof(char*));
			}
			csvTkns[wordi] = (char*) malloc(wordSize * sizeof(char));
			snprintf(csvTkns[wordi], wordSize, "%s", csv+csvi);
			//printf("WORD: %s %d\n", csvTkns[wordi], wordSize);
			wordi++;
			csvi = ci+1;
			if (quote == 1){
				ci++;
				csvi++;
				quote = 0;
			}
		} else if (c == '"' && quote == 0){
			quote = 1;
			csvi++;
		}
		ci++;
	}
	
	Record *rec = (Record*) malloc(sizeof(Record));
	if (rec == NULL){ // failed to allocate memory
		return NULL;
	}
	//rec->sourceStr = csv;
	rec->fields = (Field*) malloc(wordi * sizeof(Field));

	int colIndex = 0;
	while (colIndex < wordi){
		char *csTkn = csvTkns[colIndex];
		trim(csTkn);
		rec->fields[colIndex].type = determineTypeofData(csTkn);
		rec->fields[colIndex].data = csTkn;
		colIndex++;
	}
	rec->numOfCols = wordi;

	free(csvTkns);
	return rec;
}


char *readFile(char *filename){
  FILE *file = NULL;
  file = fopen(filename, "r");

  if (file == NULL){
    //printf("error\n");
    return NULL; // failed to open file
  }

  int fileContSize = 1;
  char *fileContent = (char*) malloc(fileContSize * sizeof(char));
  if (fileContent == NULL){
    free(fileContent);
    fclose(file);
    return NULL; // failed to allocate memory
  }
  fileContent[0] = '\0';
  char buffer[1024];
  buffer[0] = '\0';

  while ( fgets(buffer, 1024, file) != NULL ){
    int contSizeDiff = fileContSize - (fileContSize + (strlen(buffer) + 1));
    if (contSizeDiff < 0){
      contSizeDiff *= -1;
      fileContent = realloc(fileContent, fileContSize + contSizeDiff);
      fileContSize += contSizeDiff;
      if (fileContent == NULL){
        free(fileContent);
        fclose(file);
        return NULL; // failed to allocate memory
      }
    }
    strncat(fileContent, buffer, 1023);
  }

  fclose(file);

  //printf("Read In: %s\n", fileContent);

  return fileContent;
}
