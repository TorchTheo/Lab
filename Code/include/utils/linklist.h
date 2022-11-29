#ifndef __LINKLIST_H__
#define __LINKLIST_H__

#include "../type.h"

typedef struct LinkNode* LinkList;

struct LinkNode {
    int64_t value;
    LinkList prev, next;
};

LinkList newLinkList();
uint8_t linklistInsert(LinkList, int64_t);
uint8_t linklistDelete(LinkList, int64_t);
uint8_t linklistClear(LinkList);

#endif