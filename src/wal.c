/**
 * @file wal.c
 * @brief Write-Ahead Logging (WAL) implementation for atomicity and durability.
 */
#include "wal.h"
#include <stdlib.h>
#include <string.h>

Wal* wal_init(const char* filepath) {
    Wal* wal = (Wal*)malloc(sizeof(Wal));
    if (!wal) return NULL;
    
    /* "ab+" = Append Binary mode. Creates the file if it does not exist.
       Ensures disk writes always occur at the end of the file (O(1) sequential I/O). */
    wal->file = fopen(filepath, "ab+");
    if (!wal->file) {
        free(wal);
        return NULL;
    }
    
    return wal;
}

bool wal_append_put(Wal* wal, const char* key, const char* value) {
    if (!wal || !wal->file) return false;
    
    /* Plain-text log format for crash-resiliency. 
       Format: PUT <key> | <value> */
    fprintf(wal->file, "PUT %s | %s\n", key, value);
    
    /* Forces the operating system to flush user-space buffers directly 
       to persistent storage media. */
    fflush(wal->file);
    return true;
}

bool wal_append_del(Wal* wal, const char* key) {
    if (!wal || !wal->file) return false;
    fprintf(wal->file, "DEL %s\n", key);
    /* Log format for structural deletion.
       Format: DEL <key> */
    fflush(wal->file);
    return true;
} 

bool wal_append_expire(Wal* wal, const char* key, int seconds) {
    if (!wal || !wal->file) return false;
    fprintf(wal->file, "EXPIRE %s %d\n", key, seconds);
    /* Log format for Time-To-Live (TTL) eviction mapping.
       Format: EXPIRE <key> <seconds> */
    fflush(wal->file);
    return true;
}

void wal_close(Wal* wal) {
    if (wal) {
        if (wal->file) {
            fclose(wal->file);
        }
        free(wal);
    }
}
