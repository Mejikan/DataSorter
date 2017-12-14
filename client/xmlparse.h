typedef
struct _XMLDoc {
    char *name;
    char *text; // an XMLDoc can only contain text or children but not both!
    struct _XMLDoc **children;
    int numOfChildren;
}
XMLDoc;

/**
 * Returns a string with replaced sections. src is the source string to replace
 * pattern strings, search is the string pattern to replace, and replace is the
 * string to replace the found pattern with.
 */
char *replaceStr(char *src, const char *search, const char *replace);
/**
 * Escapes a string using ASCII decimal codes. Handles characters: <, >, /
 */
char *toEscStr(char *src);
/**
 * Converts an escaped string to a non-escaped one. Handles characters: <, >, /
 */
char *fromEscStr(char *src);
/**
 * Will deserialize a string into a C-object representation of an XML formatted document.
 * The returned object will always be wrapped in an object with the name "root". You should
 * not name any of your element "root" for this reason.
 */
XMLDoc* fromXmlStrRec(char *src);
/**
 * Wrapper function around fromXmlStrRec. Use this function.
 */
XMLDoc *fromXmlStr(char *src);
/**
 * Converts a XML document C-object into its string representation.
 */
char *toXMLStr(XMLDoc *doc);
void freeXMLDoc(XMLDoc *doc);