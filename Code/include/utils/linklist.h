#ifndef __LINKLIST_H__
#define __LINKLIST_H__

#include "../type.h"

typedef struct intLinkNode* intLinkList;
typedef struct strLinkNode* strLinkList;

struct intLinkNode {
    int64_t value;
    intLinkList prev, next;
};

intLinkList intNewLinkList();
uint8_t intLinklistInsert(intLinkList, int64_t);
uint8_t intLinklistDelete(intLinkList, int64_t);
uint8_t intLinklistClear(intLinkList);

struct strLinkNode {
    char *value;
    strLinkList prev, next;
};

strLinkList strNewLinkList();
uint8_t strLinklistInsert(strLinkList, char*);
uint8_t strLinklistDelete(strLinkList, char*);
uint8_t strLinklistClear(strLinkList);

#endif