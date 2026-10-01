#define _POSIX_C_SOURCE 200809L
#include "cachelab.h"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

typedef struct
{
    bool isValid;
    unsigned long tag;
    unsigned long lastUsed;
} CacheLine;

typedef struct
{
    CacheLine *lines;
} CacheSet;

typedef struct
{
    CacheSet *sets;
    int S;
    int E;
    int B;
    int s;
    int b;
} Cache;

unsigned long timestamp = 0;

Cache *cacheCreate(int s, int E, int b);
void accessCache(Cache *cache, unsigned long address, int *hit_count, int *miss_count, int *eviction_count);
void accessTrace(Cache *cache, unsigned long address, int size, int *hit_count, int *miss_count, int *eviction_count);
void parse_args(int argc, char *argv[], int *s, int *E, int *b, char **tracefile);

int main(int argc, char *argv[])
{
    int s, E, b;
    char *tracefile;
    int hit_count = 0, miss_count = 0, eviction_count = 0;

    parse_args(argc, argv, &s, &E, &b, &tracefile);

    Cache *cache = cacheCreate(s, E, b);

    FILE *fp = fopen(tracefile, "r");
    if (!fp)
    {
        perror("fopen");
        exit(1);
    }

    char operation;
    unsigned long address;
    int size;

    while (fscanf(fp, " %c %lx,%d", &operation, &address, &size) == 3)
    {
        if (operation == 'I')
        {
            continue;
        }

        accessTrace(cache, address, size, &hit_count, &miss_count, &eviction_count);

        if (operation == 'M')
        {
            accessTrace(cache, address, size, &hit_count, &miss_count, &eviction_count);
        }
    }

    fclose(fp);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}

Cache *cacheCreate(int s, int E, int b)
{
    int S = 1 << s;
    int B = 1 << b;
    Cache *cache = malloc(sizeof(Cache));
    cache->B = B;
    cache->E = E;
    cache->S = S;
    cache->s = s;
    cache->b = b;

    CacheSet *sets = malloc(sizeof(CacheSet) * S);
    for (int i = 0; i < S; i++)
    {
        CacheLine *lines = malloc(sizeof(CacheLine) * E);
        for (int j = 0; j < E; j++)
        {
            lines[j].isValid = false;
            lines[j].lastUsed = 0;
        }
        sets[i].lines = lines;
    }
    cache->sets = sets;

    return cache;
}

void accessCache(Cache *cache, unsigned long address, int *hit_count, int *miss_count, int *eviction_count)
{
    unsigned long tag = address >> (cache->s + cache->b);
    int set_index = (address >> cache->b) & (cache->S - 1);
    CacheSet *set = &cache->sets[set_index];

    unsigned long oldTime = timestamp;
    int oldId = -1;
    int invalidId = -1;
    int hitId = -1;
    for (int i = 0; i < cache->E; i++)
    {
        CacheLine line = set->lines[i];
        if (line.isValid)
        {
            if (line.tag == tag)
            {
                hitId = i;
                break;
            }
            else
            {
                if (line.lastUsed <= oldTime)
                {
                    oldTime = line.lastUsed;
                    oldId = i;
                }
            }
        }
        else
        {
            if (invalidId == -1)
            {
                invalidId = i;
            }
        }
    }

    if (hitId != -1)
    {
        (*hit_count)++;
        set->lines[hitId].lastUsed = timestamp;
    }
    else
    {
        (*miss_count)++;
        CacheLine *line;
        if (invalidId != -1)
        {
            line = &set->lines[invalidId];
            line->isValid = true;
        }
        else
        {
            line = &set->lines[oldId];
            (*eviction_count)++;
        }
        line->tag = tag;
        line->lastUsed = timestamp;
    }
}

void accessTrace(Cache *cache, unsigned long address, int size, int *hit_count, int *miss_count, int *eviction_count)
{
    timestamp++;
    unsigned long block_size = cache->B;
    unsigned long first_block = address / block_size;
    unsigned long last_block = (address + size - 1) / block_size;
    for (unsigned long cur = first_block; cur <= last_block; cur++)
    {
        accessCache(cache, cur * block_size, hit_count, miss_count, eviction_count);
    }
}

void parse_args(int argc, char *argv[], int *s, int *E, int *b, char **tracefile)
{
    int opt;

    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 's':
            *s = atoi(optarg);
            break;

        case 'E':
            *E = atoi(optarg);
            break;

        case 'b':
            *b = atoi(optarg);
            break;

        case 't':
            *tracefile = optarg;
            break;

        default:
            exit(1);
        }
    }
}
