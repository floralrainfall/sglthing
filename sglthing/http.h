#ifndef HTTP_H
#define HTTP_H

#include <curl/curl.h>
#include <stdbool.h>
#include <glib.h>
#include "config.h"

#ifndef SGLAPI_BASE
#define SGLAPI_BASE "http://localhost:9911/game"
#endif SGLAPI_BASE

struct http_server
{
    char ip[64];
    int port;
    char name[64];
    char desc[128];
};

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

struct http_user
{
    bool found;

    int item_id;
    int user_id; 
};

void http_create(struct http_client* client, char* http_base);
char* http_get(struct http_client* client, char* url);
char* http_post(struct http_client* client, char* url, char* postdata);

bool http_check_sessionkey(struct http_client* client, char* key);
struct http_user http_get_userdata(struct http_client* client, char* key);
void http_get_servers(struct http_client* client, char* game_name, GArray* servers_out);
void http_post_server(struct http_client* client, char* game_name, char* server_name, char* server_desc, char* server_ip, int server_port);

#endif