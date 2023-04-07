#ifndef HTTP_H
#define HTTP_H

#include <curl/curl.h>
#include <stdbool.h>
#include "config.h"

#ifndef SGLAPI_BASE
#define SGLAPI_BASE "http://localhost:9911/game"
#endif SGLAPI_BASE

struct http_client
{
    CURL* easy;
    char httpbase[64];
    char motd[64];
    char sessionkey[256];
    bool login;
    bool server;

    struct config_file web_config;
};

void http_create(struct http_client* client, char* http_base);
char* http_get(struct http_client* client, char* url);
char* http_post(struct http_client* client, char* url, char* postdata);

bool http_check_sessionkey(struct http_client* client, char* key);

#endif