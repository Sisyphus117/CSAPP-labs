#include <stdlib.h>
#include "cache.h"

Cache *init()
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->size = 0;
    cache->head = NULL;
    cache->tail = NULL;
    Sem_init(&cache->mutex, 0, 1);
    return cache;
}

void remove_entry(Cache *cache, CacheEntry *entry)
{
    if (entry == NULL)
    {
        return;
    }
    CacheEntry *prev = entry->prev;
    CacheEntry *next = entry->next;
    if (prev != NULL)
    {
        prev->next = next;
    }
    else
    {
        cache->head = next;
    }
    if (next != NULL)
    {
        next->prev = prev;
    }
    else
    {
        cache->tail = prev;
    }
}

void delete_entry(Cache *cache, CacheEntry *entry)
{
    remove_entry(cache, entry);
    cache->size -= entry->size;
    free(entry->key);
    free(entry->value);
    free(entry);
}
void promote_to_head(Cache *cache, CacheEntry *entry)
{
    if (entry == NULL)
    {
        return;
    }

    remove_entry(cache, entry);
    entry->prev = NULL;
    entry->next = cache->head;
    if (cache->head != NULL)
    {
        cache->head->prev = entry;
    }
    else
    {
        cache->tail = entry;
    }
    cache->head = entry;
}
void add_entry(Cache *cache, CacheEntry *entry)
{
    if (entry == NULL)
    {
        return;
    }
    P(&cache->mutex);
    entry->prev = NULL;
    entry->next = cache->head;
    if (cache->head != NULL)
    {
        cache->head->prev = entry;
    }
    else
    {
        cache->tail = entry;
    }
    cache->head = entry;

    cache->size += entry->size;
    evict_til_fit(cache);
    V(&cache->mutex);
}
void evict_til_fit(Cache *cache)
{
    while (cache->size > MAX_CACHE_SIZE)
    {
        evict_lru(cache);
    }
}
void evict_lru(Cache *cache)
{
    if (cache->tail != NULL)
    {
        delete_entry(cache, cache->tail);
    }
}

CacheEntry *new_cache_entry(char *key, void *value, size_t size)
{
    CacheEntry *entry = (CacheEntry *)malloc(sizeof(CacheEntry));
    entry->prev = NULL;
    entry->next = NULL;
    entry->key = strdup(key);
    if (entry->key == NULL)
    {
        free(entry);
        return NULL;
    }

    entry->value = malloc(size);
    if (entry->value == NULL)
    {
        free(entry);
        return NULL;
    }
    memcpy(entry->value, value, size);
    entry->size = size;
    return entry;
}

size_t find_entry(Cache *cache, char *key, void *buf)
{
    P(&cache->mutex);
    CacheEntry *cur = cache->head;
    while (cur != NULL)
    {
        if (strcmp(cur->key, key) == 0)
        {
            promote_to_head(cache, cur);
            memcpy(buf, cur->value, cur->size);
            size_t size = cur->size;
            V(&cache->mutex);
            return size;
        }
        cur = cur->next;
    }
    V(&cache->mutex);
    return 0;
}
