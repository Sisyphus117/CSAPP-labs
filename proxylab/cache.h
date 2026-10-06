#ifndef __PROXY_CACHE_H__
#define __PROXY_CACHE_H__

#include <stdio.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct CacheEntry
{
    char *key;
    void *value;
    size_t size;

    struct CacheEntry *prev;
    struct CacheEntry *next;
} CacheEntry;

typedef struct Cache
{
    CacheEntry *head;
    CacheEntry *tail;
    size_t size;

    sem_t mutex;
} Cache;

Cache *init();
void promote_to_head(Cache *cache, CacheEntry *entry);
void remove_entry(Cache *cache, CacheEntry *entry); // remove from list without free
void delete_entry(Cache *cache, CacheEntry *entry); // remove and free
void add_entry(Cache *cache, CacheEntry *entry);
void evict_til_fit(Cache *cache);
void evict_lru(Cache *cache);

CacheEntry *new_cache_entry(char *key, void *value, size_t size);
size_t find_entry(Cache *cache, char *key, void *buf);

#endif
