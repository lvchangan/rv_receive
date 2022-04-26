#include "string_map.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#define LOG_TAG "string_map"
#include "cutils/log.h"

#define DBG 0

StringMap::StringMap(const char * file):
m_size(0)
{
	mListHead = NULL;
	mCurIndex = NULL;
	readFromXml(file);
}

StringMap::~StringMap() {

}

void StringMap::readFromXml(const char* file)
{
	char line[512];
	char text[256];
	char name[128];
	char subname[128];
	char value[256];
	char * temp;

	FILE *fp = fopen(file, "r");
	if (NULL == fp) return ;
    
	while (! feof(fp)) {
		memset(line, 0, 512);
		if (fgets(line, sizeof line, fp) == NULL) continue;
		memset(text, 0, 256);
		memset(name, 0, 128);
		memset(subname, 0, 128);
		memset(value, 0, 256);
	
		sscanf(line,"%*[^<]<string%*[^\"]\"\%[^\"]", text);       
		if(DBG) ALOGD("text=%s", text);

		temp = strstr(line, "name=");
		if(DBG) ALOGD("temp=%s", temp);
		if(temp != NULL) {
			sscanf(temp, "name=%[^\" \"]", name);
		} else {
			if (strlen(text)) {
				ALOGE("no string name found");
			}
			continue;
		}

		temp = strstr(line, "subname=");
		if(temp != NULL) sscanf(temp, "subname=%[^\" \"]", subname);

		sscanf(line,"%*[^<]<string%*[^>]>\%[^<]",value);

		if(DBG) ALOGD("StringMap::readFromXml name=%s ",name);
		if(DBG) ALOGD("StringMap::readFromXml subname=%s ",subname);
		if(DBG) ALOGD("StringMap::readFromXml value=%s",value);
		if(DBG) ALOGD("StringMap::readFromXml text=%s",text);

		if(strlen(name)>0) {
			insert(name, subname, text, value);
		}
	}
	fclose(fp);
}

bool StringMap::insert( const char*name, const char * subname, const char * text, const char*value)
{
	if(DBG) ALOGD("####### insert(%d): name=%s; subname=%s", m_size, name, subname);
	if(DBG) ALOGD("####### insert value = %s ",value);

	if(NULL==mCurIndex){
		mListHead = new struct str_list;
		mCurIndex = mListHead;
		mCurIndex->pNext = NULL;
		m_size = 1;
	}else{
		mCurIndex->pNext = new struct str_list;
		if(NULL==mCurIndex->pNext){
			return false;
		}
		mCurIndex = mCurIndex->pNext;		
		mCurIndex->pNext = NULL;
		m_size++;
	}

	sprintf(mCurIndex->name,"%s",name);
	if((subname!=NULL) && (strlen(subname)>0)) {
		strcpy(mCurIndex->subname, subname);
	}
	sprintf(mCurIndex->value,"%s",value);
	sprintf(mCurIndex->text,"%s",text);
	return true;
}


//bool StringMap::remove( const char*name )
//{
//  m_Map.erase( index );
//  return true;
//}


//bool StringMap::isEmpty()
//{
//    return m_Map.empty();
//}


//bool StringMap::clear()
//{
//    m_Map.clear();
//    return true;
//}

bool StringMap::getValue(const char* name, const char*subname, char *result)
{
	if(DBG) ALOGD("getValue: name = %s, subname = %s",name, subname);
	struct str_list* p = mListHead;
	while(NULL!=p){
		if(0==strcmp(p->name,name)){
			if(DBG) ALOGD("p->name = %s, p->subname = %s", p->name, p->subname);
			if (subname == NULL) {
				sprintf(result,"%s",p->value);			
				if(DBG) ALOGD("StringMap::getValue  p=%s", p->value);
            			return true;
			} else if(0 == strcmp(p->subname, subname)) {
				sprintf(result,"%s",p->value);			
				if(DBG) ALOGD("StringMap::getValue  p=%s", p->value);
            			return true;
			}
        	}
		p = p->pNext;
	}
	return false;
}

bool StringMap::getText(const char* name, const char * subname, char *result)
{
	if(DBG) ALOGD("getText: name = %s, subname = %s",name, subname);
	struct str_list* p = mListHead;
	while(NULL!=p){
		if(DBG) ALOGD("p->name = %s, p->subname = %s", p->name, p->subname);
        	if(0==strcmp(p->name,name)) {
			if(subname == NULL) {
				sprintf(result,"%s",p->text);			
            			return true;

			} else if(0==strcmp(p->subname, subname)) {
				sprintf(result,"%s",p->text);			
            			return true;
			}
        	}
		p = p->pNext;
    	}

	return false;
}

int StringMap::size()
{
    return m_size;
}
