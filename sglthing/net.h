#ifndef NET_H
#define NET_H
#ifdef SGLTHING_COMPILE
#include "ui.h"
#include "script.h"
#endif 
#include <glib.h>
#include "sockets.h"
#include "http.h"
#include <sqlite3.h> 

#define CR_PACKET_VERSION 200
#define NO_DATAPACKETS_TICK 512

#define MAGIC_NUMBER 0x7930793179327934
#define DATA_PACKET_SIZE 256

typedef uint64_t checksum_t;

struct db_player
{
    bool found;
    
    int id;
    int web_id;
};

struct network_client {
    int socket;
    char client_name[64];
    int player_id;
    int web_player_id;
    int connection_id;
    int client_version;
    double last_ping_time;
    double next_ping_time;
    double ping_time_interval;
    double lag;
    bool authenticated;
    bool connected;
    bool debugger;
    bool observer;
    bool verified;
    float player_color_r;
    float player_color_g;
    float player_color_b;
    struct sockaddr_in sockaddr;
    char* dl_data;
    int dl_packet_id;
    int dl_final_packet_id;
    int dl_packet_size;    
    int dl_retries;
    int dl_successes;
    struct network* owner;
    void* user_data;

    struct http_user user;
    struct db_player db;
};

struct network_downloader {
    int socket;
    int data_size;
    int data_downloaded;
    int data_packet_id;

    char* data_offset;

    bool data_started;
    bool data_done;

    char request_name[64];
    char server_motd[128];
    char server_name[64];
    bool server_info;

    struct http_client http_client;    
    
    struct sockaddr_in dest;
};

struct player_auth_list_entry
{
    char client_name[64];
    bool admin;
    int player_id;
    
    float player_color_r;
    float player_color_g;
    float player_color_b;
};

struct network_packet {
    struct {
        enum { 
            PACKETTYPE_CLIENTINFO,       // C-->S
            PACKETTYPE_SERVERINFO,       // C<--S
            PACKETTYPE_PING,             // C<->S
            PACKETTYPE_ACKNOWLEDGE,      // C<->S
            PACKETTYPE_DISCONNECT,       // C<->S

            PACKETTYPE_PLAYER_ADD,       // C<--S
            PACKETTYPE_PLAYER_REMOVE,    // C<--S
            PACKETTYPE_CHAT_MESSAGE,     // C<->S
            
            PACKETTYPE_DATA,             // C<--S
            PACKETTYPE_DATA_REQUEST,     // C<->S
            PACKETTYPE_LUA_DATA,         // C<->S

            PACKETTYPE_DEBUGGER_QUIT,    // C-->S
            PACKETTYPE_DEBUGGER_KICK,    // C-->S

            PACKETTYPE_LAST_ID,
        } packet_type;
        int packet_version;
        int packet_id;
        int packet_checksum;
        bool acknowledge;
        int acknowledge_tries;
        float distributed_time;
        uint64_t magic_number;
        int network_frames;
    } meta;
    union {
        struct {
            int packet_id;
        } acknowledge;
        struct {
            char client_name[64];
            char session_key[256];
            char server_pass[64];
            char debugger_pass[64];
            int sglthing_revision;

            float color_r;
            float color_g;
            float color_b;

            bool observer;
        } clientinfo;
        struct {
            char disconnect_reason[32];
        } disconnect;
        struct {
            double distributed_time;
            double player_lag;
        } ping;
        struct {
            char server_name[64];
            char session_key[256];
            char server_motd[128];
            int sglthing_revision;
            int player_id;
            bool verified;

            float player_color_r;
            float player_color_g;
            float player_color_b;

            int player_count;
            struct player_auth_list_entry players[];
        } serverinfo;
        struct {
            char client_name[64];
            bool observer;
            bool admin;
            bool verified;
            int player_id;

            float player_color_r;
            float player_color_g;
            float player_color_b;

            struct http_user user_data;
        } player_add;
        struct {
            int player_id;
        } player_remove;
        struct {
            int type_id;
            char lua_data[512];
        } lua_data;
        struct {
            int player_id;
            char client_name[64];
            char message[128];
            bool verified;
        } chat_message;
        struct {
#define DATA_SIZE_MAX 3145728
            int data_request_id;
            int data_packet_id;
            int data_final_packet_id;
            int data_size;
            char data[DATA_PACKET_SIZE];
        } data;
        struct {
            char data_name[64];
            int data_crc;
            int data_request_id;
            int data_size;
            bool data_same;
        } data_request;
        struct {
            int player_id;
            char reason[32];
        } dbg_kick;
    } packet;
};

#define NETWORK_HISTORY_FRAMES 16

struct network {
    enum { NETWORKMODE_SERVER, NETWORKMODE_CLIENT, NETWORKMODE_UNKNOWN } mode;
    enum { NETWORKSTATUS_DISCONNECTED, NETWORKSTATUS_CONNECTED } status;
    char disconnect_reason[32];
    char server_motd[128];
    char server_name[64];
    char server_pass[64];
    char debugger_pass[64];
    struct network_client client;
    struct http_client http_client;
#ifdef SGLTHING_COMPILE
    struct script_system* script;
#endif
    double distributed_time;
    double time;
    int network_frames;

    GArray* server_clients;
    double next_tick;
    double client_default_tick;
    GArray* packet_unacknowledged;
    int packet_id;

    bool server_open;
    bool security;

    bool shutdown_empty;
    bool shutdown_ready;

    bool (*receive_packet_callback)(struct network* network, struct network_client* client, struct network_packet* packet);
    void (*new_player_callback)(struct network* network, struct network_client* client);
    void (*del_player_callback)(struct network* network, struct network_client* client);

    GHashTable* players;

    int packet_tx_numbers[NETWORK_HISTORY_FRAMES];
    int packet_rx_numbers[NETWORK_HISTORY_FRAMES];
    int packet_time;

    struct world* world;

    sqlite3 *database;    
};

void network_init(struct network* network, struct script_system* script);
void network_connect(struct network* network, char* ip, int port);
void network_open(struct network* network, char* ip, int port);
void network_manage_socket(struct network* network, struct network_client* client);
int network_transmit_packet(struct network* network, struct network_client* client, struct network_packet packet);
void network_transmit_packet_all(struct network* network, struct network_packet packet);
void network_transmit_data(struct network* network, struct network_client* client, char* data, int data_length);
void network_disconnect_player(struct network* network, bool transmit_disconnect, char* reason, struct network_client* client);
void network_frame(struct network* network, float delta_time, double time);
#ifdef SGLTHING_COMPILE
void network_dbg_ui(struct network* network, struct ui_data* ui);
#endif
struct network_client* network_get_player(struct network* network, int* player_id);
void network_close(struct network* network);

void network_start_download(struct network_downloader* network, char* ip, int port, char* rqname, char* pass);
void network_tick_download(struct network_downloader* network);
void network_stop_download(struct network_downloader* network);

#endif