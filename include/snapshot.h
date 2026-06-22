#ifndef RURAL_SNAPSHOT_H
#define RURAL_SNAPSHOT_H

#include "hash.h"

int snapshot_save(HashTable* ht, const char* filename);
int snapshot_load(HashTable* ht, const char* filename);

#endif
