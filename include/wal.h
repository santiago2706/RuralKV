#ifndef RURAL_WAL_H
#define RURAL_WAL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "hash.h"

typedef struct {
    FILE* fp;
    char filename[256];
} WAL;

typedef WAL Wal;

WAL* wal_init(const char* filepath);
bool wal_append_put(WAL* wal, const char* key, const char* value);
bool wal_append_del(WAL* wal, const char* key);
bool wal_append_expire(WAL* wal, const char* key, int seconds);
void wal_replay(WAL* wal, HashTable* ht);
void wal_close(WAL* wal);

#endif
