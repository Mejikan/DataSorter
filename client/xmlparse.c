#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "xmlparse.h"

char *replaceStr(char *src, const char *search, const char *replace){
    if (src == NULL){
        return NULL;
    }

    char *newStr = (char*)malloc(1);
    newStr[0] = '\0';

    char *prev = src;
    char *next = NULL;
    while ( (next = strstr(prev, search)) != NULL ){
        int substrLen = next - prev;
        newStr = (char*) realloc(newStr, substrLen + strlen(newStr) + strlen(replace) + 1);
        strncat(newStr, prev, substrLen);
        strcat(newStr, replace);

        prev = next + strlen(search);
    }

    newStr = (char*) realloc(newStr, strlen(newStr) + strlen(prev) + 1);
    strcat(newStr, prev);

    return newStr;
}

char *toEscStr(char *src){
    if (src == NULL){
        return NULL;
    }

    char *escaped = src;
    char *old = escaped;
    escaped = replaceStr(old, "&", "&#10");
    old = escaped;
    escaped = replaceStr(old, "<", "&#60");
    free(old); old = escaped;
    escaped = replaceStr(old, ">", "&#62");
    free(old); old = escaped;
    escaped = replaceStr(old, "\n", "&#10");
    free(old); old = escaped;
    escaped = replaceStr(old, "\r", "&#13");
    free(old); old = escaped;
    escaped = replaceStr(old, "\t", "&#9");
    free(old);
    return escaped;
}

char *fromEscStr(char *src){
    if (src == NULL){
        return NULL;
    }

    char *escaped = src;
    char *old = escaped;
    escaped = replaceStr(old, "&#9", "\t");
    old = escaped;
    escaped = replaceStr(old, "&#13", "\r");
    free(old); old = escaped;
    escaped = replaceStr(old, "&#10", "\n");
    free(old); old = escaped;
    escaped = replaceStr(old, "&#62", ">");
    free(old); old = escaped;
    escaped = replaceStr(old, "&#60", "<");
    free(old); old = escaped;
    escaped = replaceStr(old, "&#10", "&");
    free(old);
    return escaped;
}

/**
 * Will deserialize a string into a C-object representation of an XML formatted document.
 * The returned object will always be wrapped in an object with the name "root". You should
 * not name any of your element "root" for this reason.
 */
XMLDoc* fromXmlStrRec(char *src){

    //XMLDoc **docs = (XMLDoc**)malloc(1 * sizeof(XMLDoc*));
    XMLDoc **docs = NULL;
    int numOfDocs = 0;

    char *search = src;
    char *lt1 = NULL;
    while ( (lt1 = strstr(search, "<")) != NULL ){
        char *gt1 = strstr(lt1 + 1, ">");
        if (gt1 == NULL){
            break;
        } else {
            char *fieldName = (char*) malloc(gt1 - lt1 - 1 + 1);
            //char fieldName[gt1 - lt1 - 1 + 1];
            fieldName[0] = '\0';
            strncat(fieldName, lt1 + 1, gt1 - lt1 - 1);

            char *lt2 = gt1 + 1;
            char *gt2 = NULL;
            char *fieldName2 = NULL;
            while ( (lt2 = strstr(lt2, "</")) != NULL ){
                gt2 = strstr(lt2 + 2, ">");
                if (gt2 == NULL){
                    free(fieldName);
                    break;
                }

                fieldName2 = (char*) malloc(gt2 - lt2 - 2 + 1);
                //char fieldName2[gt2 - lt2 - 2 + 1];
                fieldName2[0] = '\0';
                strncat(fieldName2, lt2 + 2, gt2 - lt2 - 2);

                // check if field names are equal
                if (strcmp(fieldName, fieldName2) == 0){
                    free(fieldName2);
                    break;
                } else {
                    free(fieldName2);
                    lt2 = gt2 + 1;
                    gt2 = NULL;
                    fieldName2 = NULL;
                }
            }

            if (lt2 == NULL && gt2 == NULL){
                free(fieldName);
                break;
            } else {
                // get content
                char *content = (char*) malloc( lt2 - (gt1 + 1) + 1);
                //char content[lt2 - (gt1 + 1) + 1];
                content[0] = '\0';
                strncat(content, gt1 + 1, lt2 - (gt1 + 1));

                XMLDoc *doc;
                XMLDoc *sub = fromXmlStrRec(content);
                if (sub == NULL){ // must be text content
                    doc = (XMLDoc*)malloc(sizeof(XMLDoc));
                    doc->text = fromEscStr(content);
                    //doc->text = content;
                    doc->children = NULL;
                    doc->numOfChildren = 0;

                } else {
                    doc = sub;
                }
                free(content);
                doc->name = fieldName;
                
                if (numOfDocs > 0){
                    docs = (XMLDoc**)realloc(docs, (numOfDocs + 1) * sizeof(XMLDoc*));
                } else if (numOfDocs == 0){
                    docs = (XMLDoc**)malloc(1 * sizeof(XMLDoc*));
                }
                docs[numOfDocs] = doc;
                numOfDocs = numOfDocs + 1;

                search = gt2 + 1;
            }
        }
    }


    if (docs == NULL){
        free(docs);
        return NULL;
    } else {
        XMLDoc *root = (XMLDoc*) malloc(sizeof(XMLDoc));
        root->name = "root";
        root->text = NULL;
        root->children = docs;
        root->numOfChildren = numOfDocs;
        return root;
    }
}

XMLDoc *fromXmlStr(char *src){
    XMLDoc *recurDoc = fromXmlStrRec(src);

    if (recurDoc == NULL){
        return NULL;
    }
    XMLDoc *doc = recurDoc->children[0];
    printf("num of children in xml: %d, %s, %s\n", recurDoc->numOfChildren, recurDoc->name, recurDoc->text);
    free(recurDoc->children);
    free(recurDoc);
    return doc;
}

char *toXMLStr(XMLDoc *doc){
    if ((doc->text) != NULL){
        char *serial = (char*)malloc(strlen("<></>") + (2 * strlen(doc->name)) + strlen(doc->text) + 1);
        sprintf(serial, "<%s>%s</%s>", doc->name, toEscStr(doc->text), doc->name);
        //puts(serial);
        return serial;
    } else if ((doc->children) != NULL){
        char *serial = (char*)malloc(strlen("<>") + strlen(doc->name) + 1);
        sprintf(serial, "<%s>", doc->name);
        int i = 0;
        while (i < doc->numOfChildren){
            char *docSerial = toXMLStr(doc->children[i]);
            
            if (docSerial != NULL){
                size_t newLen = strlen(serial);
                newLen += strlen(docSerial);
                newLen += 1;
                
                char *newSerial = (char*)malloc(newLen);
                sprintf(newSerial, "%s%s", serial, docSerial);
                free(serial);
                serial = newSerial;
            }
            i++;
        }

        serial = (char*)realloc(serial, strlen(serial) + strlen(doc->name) + strlen("</>") + 1);
        strcat(serial, "</");
        strcat(serial, doc->name);
        strcat(serial, ">");
        //puts(serial);
        return serial;
    } else {
        return NULL;
    }
}

void freeXMLDoc(XMLDoc *doc){
	if (doc == NULL){
		return;
	} else if ((doc->children) != NULL){
		int i = 0;
		while (i < (doc->numOfChildren)){
			freeXMLDoc(doc->children[i]);
			i++;
		}
        free(doc->children);
	}

	if ((doc->text) != NULL){
		free((doc->text));
	}

	free((doc->name));
	free(doc);
}