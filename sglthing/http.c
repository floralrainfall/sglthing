#include "http.h"
#include "sglthing.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>

static bool curl_initialized = false;

typedef struct __http_rq_internal
{
    char *response;
    int size;
} __http_rq;

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
    char _url[256];
    snprintf(_url,256,"%s/%s",client->httpbase,url);
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

    char* web_motd = http_get(client, "motd");
    if(web_motd)
    {
        printf("sglthing: motd '%s'\n", web_motd);
        strncpy(client->motd, web_motd, 64);
        free(web_motd);
    }
    else
        strncpy(client->motd, " ", 64);

    printf("sglthing: logging into sglnet as %s\n", config_string_get(&client->web_config, "user_username"));
    char postdata[256];
    snprintf(postdata, 256, "user_username=%s&user_password=%s&server=%s", 
        config_string_get(&client->web_config, "user_username"),
        config_string_get(&client->web_config, "user_password"),
        client->server ? "true" : "false");
    char* sessionkey = http_post(client, "auth/authorize", postdata);
    client->login = false;
    if(sessionkey)
    {
        client->login = true;
        if(http_check_sessionkey(client, sessionkey))
        {
            strncpy(client->sessionkey, sessionkey, 256);
            printf("sglthing: logged in\n");
            free(sessionkey);
        }
        else
        {
            printf("sglthing: key received but check failed\n");
            client->login = false;
        }
    }
    else
    {
        printf("sglthing: did not receive key\n");
    }
}

bool http_check_sessionkey(struct http_client* client, char* key)
{
    if(!client->login)
        return false;
    char url[256];
    snprintf(url, 256, "auth/check?sessionkey=%s", key);
    char* result = http_get(client, url);
    if(!result)
        return false;
    printf("sglthing: check = %s\n", result);
    return (strcmp(result,"1")==0);
}

int http_get_userid(struct http_client* client, char* key)
{
    if(!client->login)
        return false;
    char url[256];
    snprintf(url, 256, "auth/check?sessionkey=%s", key);
    char* result = http_get(client, url);
    if(!result)
        return false;
    return atoi(result);
}