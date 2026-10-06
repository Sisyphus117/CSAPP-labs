#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAXLINE 8192

Cache *cache;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void parse_uri(char *uri, char *host, char *path, char *port);

void *handle_client(void *arg);

int main(int argc, char **argv)
{
    int listen_fd = Open_listenfd(argv[1]);

    cache = init();

    while (1)
    {
        int *connfdp = malloc(sizeof(int));

        *connfdp = Accept(listen_fd, NULL, NULL);

        pthread_t pid;
        Pthread_create(&pid, NULL, handle_client, connfdp);
        Pthread_detach(pid);
    }

    return 0;
}

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    rio_t client_rio;
    Rio_readinitb(&client_rio, client_fd);

    char host[MAXLINE];
    char path[MAXLINE];
    char port[MAXLINE];

    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];

    char buf[MAXLINE];
    Rio_readlineb(&client_rio, buf, MAXLINE);

    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, host, path, port);

    char key[MAXLINE];
    snprintf(key, MAXLINE, "%s:%s%s", host, port, path);
    char object[MAX_OBJECT_SIZE];
    size_t object_size = 0;

    if ((object_size = find_entry(cache, key, object)))
    {
        Rio_writen(client_fd, object, object_size);
        Close(client_fd);
        return NULL;
    }

    char request[MAXLINE];
    snprintf(request, MAXLINE, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\nAccept: */*\r\n\r\n", path, host);

    int server_fd = Open_clientfd(host, port); // 80 for http

    Rio_writen(server_fd, request, strlen(request));

    rio_t server_rio;
    Rio_readinitb(&server_rio, server_fd);
    ssize_t read_size;

    while ((read_size = Rio_readnb(&server_rio, buf, MAXLINE)) > 0)
    {
        Rio_writen(client_fd, buf, read_size);

        if (object_size + read_size <= MAX_OBJECT_SIZE)
        {
            memcpy(object + object_size, buf, read_size);
        }
        object_size += read_size;
    }

    if (object_size <= MAX_OBJECT_SIZE)
    {
        CacheEntry *entry = new_cache_entry(key, object, object_size);
        add_entry(cache, entry);
    }

    Close(client_fd);
    Close(server_fd);
}
void parse_uri(char *uri, char *host, char *path, char *port)
{
    char *p = strstr(uri, "://");
    p += 3;
    char *colon = strchr(p, ':');
    char *slash;
    if (colon == NULL)
    {
        strcpy(port, "80");
        slash = strchr(p, '/');
        strncpy(host, p, slash - p);
        host[slash - p] = '\0';
    }
    else
    {
        strncpy(host, p, colon - p);
        host[colon - p] = '\0';
        p = colon;
        slash = strchr(p, '/');
        strncpy(port, p + 1, slash - p - 1); // :9000 or 9000
        port[slash - p - 1] = '\0';
    }

    strcpy(path, slash);
}
