/**
 * @file wal.c
 * @brief Write-Ahead Logging (WAL) implementation for atomicity and durability.
 */
#include "wal.h"
#include "hash.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_whitespace(char* text) {
    char* start = text;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    char* end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
}

WAL* wal_init(const char* filepath) {
    if (!filepath) return NULL;

    WAL* wal = (WAL*)malloc(sizeof(WAL));
    if (!wal) return NULL;

    memset(wal, 0, sizeof(WAL));
    strncpy(wal->filename, filepath, sizeof(wal->filename) - 1);

    /* "ab+" = Append Binary mode. Creates the file if it does not exist.
       Ensures disk writes always occur at the end of the file (O(1) sequential I/O). */
    wal->fp = fopen(filepath, "ab+");
    if (!wal->fp) {
        free(wal);
        return NULL;
    }

    return wal;
}

bool wal_append_put(WAL* wal, const char* key, const char* value) {
    if (!wal || !wal->fp) return false;

    /* Plain-text log format for crash-resiliency.
       Format: PUT <key> | <value> */
    fprintf(wal->fp, "PUT %s | %s\n", key, value);

    /* Forces the operating system to flush user-space buffers directly
       to persistent storage media. */
    fflush(wal->fp);
    return true;
}

bool wal_append_del(WAL* wal, const char* key) {
    if (!wal || !wal->fp) return false;
    fprintf(wal->fp, "DEL %s\n", key);
    /* Log format for structural deletion.
       Format: DEL <key> */
    fflush(wal->fp);
    return true;
}

bool wal_append_expire(WAL* wal, const char* key, int seconds) {
    if (!wal || !wal->fp) return false;
    fprintf(wal->fp, "EXPIRE %s %d\n", key, seconds);
    /* Log format for Time-To-Live (TTL) eviction mapping.
       Format: EXPIRE <key> <seconds> */
    fflush(wal->fp);
    return true;
}

void wal_replay(WAL* wal, HashTable* ht) {
    int recovered_ops = 0;

    if (!wal || !ht || wal->filename[0] == '\0') {
        printf("[WAL] Recovery completed.\n");
        printf("[WAL] %d operations replayed.\n", recovered_ops);
        return;
    }

    FILE* replay_fp = fopen(wal->filename, "r");
    if (!replay_fp) {
        printf("[WAL] Recovery completed.\n");
        printf("[WAL] %d operations replayed.\n", recovered_ops);
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), replay_fp)) {
        char* newline = strpbrk(line, "\r\n");
        if (newline) {
            *newline = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        if (strncmp(line, "PUT ", 4) == 0) {
            char key[256];
            char value[768];
            if (sscanf(line, "PUT %255[^|] | %767[^\n]", key, value) == 2) {
                trim_whitespace(key);
                trim_whitespace(value);
                if (hash_put(ht, key, value)) {
                    recovered_ops++;
                }
            }
        } else if (strncmp(line, "DEL ", 4) == 0) {
            char key[256];
            if (sscanf(line, "DEL %255[^\n]", key) == 1) {
                trim_whitespace(key);
                if (hash_delete(ht, key)) {
                    recovered_ops++;
                }
            }
        } else if (strncmp(line, "EXPIRE ", 7) == 0) {
            char key[256];
            int seconds;
            if (sscanf(line, "EXPIRE %255s %d", key, &seconds) == 2) {
                if (hash_set_expire(ht, key, seconds)) {
                    recovered_ops++;
                }
            }
        }
    }

    fclose(replay_fp);
    printf("[WAL] Recovery completed.\n");
    printf("[WAL] %d operations replayed.\n", recovered_ops);
}

void wal_close(WAL* wal) {
    if (wal) {
        if (wal->fp) {
            fclose(wal->fp);
        }
        free(wal);
    }
}
