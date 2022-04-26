#ifndef STRING_MAP_H
#define STRING_MAP_H

#define MAX_VALUE 256
#include <stdlib.h>

class StringMap
{
public:
    struct str_list{
        struct str_list *pNext;
        char name[32];
        char subname[32];
        char value[MAX_VALUE];
	char text[128];
    };

    StringMap(const char * xml);
    ~StringMap();

    bool getValue(const char* name, const char* subname, char *result);        //returns value
    bool getText(const char*name, const char * subname, char *result);
    int size();
private:
    unsigned int m_size;
    struct str_list *mCurIndex;
    struct str_list *mListHead;
    bool insert(const char* name, const char* subname, const char * text, const char* value);
    void readFromXml(const char* xml);
};

#endif //STRING_MAP_H
