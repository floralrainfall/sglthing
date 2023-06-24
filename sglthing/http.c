#include "http.h"
#include "sglthing.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "model.h"
#include <json.h>

static bool curl_initialized = false;

typedef struct __http_rq_internal
{
    char *response;
    int size;
} __http_rq;

static bool p_init = false;

// taken from libcurl doc
static size_t http_write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
    __http_rq* rq = (__http_rq*)userp;
    char *ptr = realloc(rq->response, rq->size + real_size + 1);
    if(ptr == NULL)
        return 0;
    rq->response = ptr;
    memcpy(rq->response, buffer, real_size);
    rq->size += real_size;
    rq->response[rq->size] = 0;
    return real_size;
}

char* http_get(struct http_client* client, char* url)
{
    char _url[512];
    snprintf(_url,512,"%s/%s",client->httpbase,url);
    curl_easy_setopt(client->easy, CURLOPT_URL, _url);
    curl_easy_setopt(client->easy, CURLOPT_WRITEFUNCTION, http_write_data);
    __http_rq rq = { .size = 0 };
    curl_easy_setopt(client->easy, CURLOPT_WRITEDATA, &rq);
    int success = curl_easy_perform(client->easy);
    int response_code;
    curl_easy_getinfo(client->easy, CURLINFO_RESPONSE_CODE, &response_code);
    printf("sglthing: HTTP GET %s/%s %i (%i)\n", client->httpbase, url, response_code, success);
    return rq.response;
}

char* http_post(struct http_client* client, char* url, char* postdata)
{
    char _url[512];
    snprintf(_url,512,"%s/%s",client->httpbase,url);
    curl_easy_setopt(client->easy, CURLOPT_URL, _url);
    curl_easy_setopt(client->easy, CURLOPT_WRITEFUNCTION, http_write_data);
    curl_easy_setopt(client->easy, CURLOPT_POSTFIELDS, postdata);
    __http_rq rq = { .size = 0 };
    curl_easy_setopt(client->easy, CURLOPT_WRITEDATA, &rq);
    int success = curl_easy_perform(client->easy);
    int response_code;
    curl_easy_getinfo(client->easy, CURLINFO_RESPONSE_CODE, &response_code);
    printf("sglthing: HTTP POST %s/%s %i (%i)\n", client->httpbase, url, response_code, success);
    return rq.response;
}

void http_create(struct http_client* client, char* http_base)
{
    if(!curl_initialized)
        curl_global_init(CURL_GLOBAL_ALL);

    config_load(&client->web_config, "config_private.ini");

    client->easy = curl_easy_init();
    strncpy(client->httpbase, http_base, 64);

    GError* error;
    if(g_key_file_has_key(client->web_config.key_file, "sglapi", "httpbase", error))
    {
        strncpy(client->httpbase, g_key_file_get_string(client->web_config.key_file, "sglapi", "httpbase", error), 64);
        printf("sglthing: using specified auth server: %s\n", client->httpbase);
    }

    http_update_motd(client);

    printf("sglthing: logging into sglnet as %s\n", config_string_get(&client->web_config, "user_username"));
    char postdata[1024];
    snprintf(postdata, 1024, "user_username=%s&user_password=%s&server=%s", 
        config_string_get(&client->web_config, "user_username"),
        config_string_get(&client->web_config, "user_password"),
        client->server ? "true" : "false");
    char* sessionkey = http_post(client, "auth/authorize", postdata);
    client->login = false;
    if(sessionkey)
    {
        client->login = true;
        
        struct http_user user_data = http_get_userdata(client, sessionkey);
        if(user_data.found)
        {
            strncpy(client->sessionkey, sessionkey, 256);
            printf("sglthing: logged in (sesk: %s)\n", sessionkey);
        }
        else
        {
            printf("sglthing: key received but couldnt find user data\n");
            client->login = false;
        }            
        free(sessionkey);
    }
    else
    {
        printf("sglthing: did not receive key\n");
    }

}

struct http_user http_get_userdata(struct http_client* client, char* key)
{
    struct http_user user;
    if(!client->login)
    {
        user.found = false;
        return user;
    }
    char url[256];
    snprintf(url, 256, "auth/user_data?sessionkey=%s", key);
    char* result = http_get(client, url);
    if(!result)
    {
        user.found = false;
        return user;
    }

    user.found = true;

    struct json_object* juser = json_tokener_parse(result);
    strncpy(user.user_username, json_object_get_string(
        json_object_object_get(juser, "user_username")
    ), 64);
    user.money = json_object_get_int(
        json_object_object_get(juser, "user_coins")
    );
    user.user_id = json_object_get_int(
        json_object_object_get(juser, "id")
    );

    free(result);
    
    return user;
}

void http_get_servers(struct http_client* client, char* game_name, GArray* servers_out)
{
    char url[256];
    snprintf(url, 256, "netplay/serverlist?game=%s", game_name);
    char* _server_list = http_get(client, url);
    if(!_server_list)
    {
        printf("sglthing: couldnt get server list data\n");
        return;
    }
    struct json_object* jobj = json_tokener_parse(_server_list);
    int obj_len = json_object_array_length(jobj);
    g_array_remove_range(servers_out, 0, servers_out->len);
    for(int i = 0; i < obj_len; i++)
    {
        struct http_server server;
        struct json_object* jserver = json_object_array_get_idx(jobj, i);

        strncpy(server.ip, json_object_get_string(
            json_object_object_get(jserver, "server_ip")
        ), 64);
        strncpy(server.name, json_object_get_string(
            json_object_object_get(jserver, "server_name")
        ), 64);
        strncpy(server.desc, json_object_get_string(
            json_object_object_get(jserver, "server_desc")
        ), 128);
        server.port = json_object_get_int(
            json_object_object_get(jserver, "server_port")
        );

        g_array_append_val(servers_out, server);
    }
    json_tokener_free(jobj);
    free(_server_list);
}

void http_post_server(struct http_client* client, char* game_name, char* server_name, char* server_desc, char* server_ip, int server_port)
{
    char postdata[256];
    snprintf(postdata, 256, "session_key=%s&game=%s&server_name=%s&server_desc=%s&server_ip=%s&server_port=%i", 
        client->sessionkey,
        game_name,
        server_name,
        server_desc,
        server_ip,
        server_port);
    char* _heartbeat = http_post(client, "netplay/punch", postdata);
    printf("sglthing: punched master server (%s)\n", _heartbeat);
    free(_heartbeat);
}

void http_update_motd(struct http_client* client)
{
    char* web_motd = http_get(client, "motd");
    if(web_motd)
    {
        printf("sglthing: motd '%s'\n", web_motd);
        strncpy(client->motd, web_motd, 64);
        free(web_motd);
    }
    else
        strncpy(client->motd, "Receive motd failed", 64);
}

void http_delete(struct http_client* client)
{
    curl_easy_cleanup(client->easy);
    client->login = false;
    client->server = false;
    g_key_file_free(client->web_config.key_file);
}