/**
 * @file snapshot.c
 * @brief Binary snapshot persistence for RuralKV.
 */
#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

static unsigned long snapshot_hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static HashEntry* snapshot_find_entry(HashTable* ht, const char* key) {
    if (!ht || !key) return NULL;

    unsigned long idx = snapshot_hash_djb2(key) % ht->size;
    HashEntry* entry = ht->buckets[idx];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

static int snapshot_is_expired(time_t expire_at) {
    return expire_at != 0 && time(NULL) >= expire_at;
}

int snapshot_save(HashTable* ht, const char* filename) {
    if (!ht || !filename) return -1;

    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;

    uint32_t total_entries = 0;

    for (size_t bucket = 0; bucket < ht->size; bucket++) {
        HashEntry* entry = ht->buckets[bucket];
        while (entry != NULL) {
            if (!snapshot_is_expired(entry->expire_at)) {
                total_entries++;
            }
            entry = entry->next;
        }
    }

    if (fwrite(&total_entries, sizeof(total_entries), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    for (size_t bucket = 0; bucket < ht->size; bucket++) {
        HashEntry* entry = ht->buckets[bucket];
        while (entry != NULL) {
            if (!snapshot_is_expired(entry->expire_at)) {
                uint32_t key_length = (uint32_t)strlen(entry->key);
                uint32_t value_length = (uint32_t)strlen(entry->value);
                int64_t expire_at = (int64_t)entry->expire_at;

                if (fwrite(&key_length, sizeof(key_length), 1, fp) != 1 ||
                    fwrite(entry->key, 1, key_length, fp) != key_length ||
                    fwrite(&value_length, sizeof(value_length), 1, fp) != 1 ||
                    fwrite(entry->value, 1, value_length, fp) != value_length ||
                    fwrite(&expire_at, sizeof(expire_at), 1, fp) != 1) {
                    fclose(fp);
                    return -1;
                }
            }
            entry = entry->next;
        }
    }

    fflush(fp);
    fclose(fp);
    printf("[SNAPSHOT] Saved %u entries.\n", total_entries);
    return (int)total_entries;
}

int snapshot_load(HashTable* ht, const char* filename) {
    if (!ht || !filename) return -1;

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("[SNAPSHOT] Loaded 0 entries.\n");
        return 0;
    }

    uint32_t total_entries = 0;
    if (fread(&total_entries, sizeof(total_entries), 1, fp) != 1) {
        fclose(fp);
        printf("[SNAPSHOT] Loaded 0 entries.\n");
        return 0;
    }

    int loaded_entries = 0;

    for (uint32_t i = 0; i < total_entries; i++) {
        uint32_t key_length = 0;
        uint32_t value_length = 0;
        int64_t expire_at_raw = 0;

        if (fread(&key_length, sizeof(key_length), 1, fp) != 1) break;

        char* key = (char*)malloc((size_t)key_length + 1);
        if (!key) break;
        if (fread(key, 1, key_length, fp) != key_length) {
            free(key);
            break;
        }
        key[key_length] = '\0';

        if (fread(&value_length, sizeof(value_length), 1, fp) != 1) {
            free(key);
            break;
        }

        char* value = (char*)malloc((size_t)value_length + 1);
        if (!value) {
            free(key);
            break;
        }
        if (fread(value, 1, value_length, fp) != value_length) {
            free(key);
            free(value);
            break;
        }
        value[value_length] = '\0';

        if (fread(&expire_at_raw, sizeof(expire_at_raw), 1, fp) != 1) {
            free(key);
            free(value);
            break;
        }

        time_t expire_at = (time_t)expire_at_raw;
        if (!snapshot_is_expired(expire_at)) {
            if (hash_put(ht, key, value)) {
                HashEntry* entry = snapshot_find_entry(ht, key);
                if (entry) {
                    entry->expire_at = expire_at;
                }
                loaded_entries++;
            }
        }

        free(key);
        free(value);
    }

    fclose(fp);
    printf("[SNAPSHOT] Loaded %d entries.\n", loaded_entries);
    return loaded_entries;
}
