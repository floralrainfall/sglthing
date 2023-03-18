#ifndef NET_H
#define NET_H
#ifdef SGLTHING_COMPILE
#include "ui.h"
#include "script.h"
#endif 
#include <glib.h>
#include <netinet/in.h>
#ifdef SGLTHING_COMPILE
#include "s7/netbundle.h"
#include "script.h"
#endif 
#include "http.h"

#define CR_PACKET_VERSION 102
#define NO_DATAPACKETS_TICK 1

#define MAGIC_NUMBER 0x7930793179327934
#define DATA_PACKET_SIZE 1024

struct network_client {
    int socket;
    char client_name[64];
    int player_id;
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
};

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
    int network_frames;

    GArray* server_clients;
    double next_tick;
    double client_default_tick;

    bool security;

    bool shutdown_empty;
    bool shutdown_ready;
};

struct network_packet {
    struct {
        enum { 
            PACKETTYPE_CLIENTINFO,       // C-->S
            PACKETTYPE_DISCONNECT,       // C<->S
            PACKETTYPE_PING,             // C<->S
            PACKETTYPE_SERVERINFO,       // C<--S

            PACKETTYPE_PLAYER_ADD,       // C<--S
            PACKETTYPE_PLAYER_REMOVE,    // C<--S

            PACKETTYPE_SCM_EVENT,        // C<->S
            PACKETTYPE_CHAT_MESSAGE,     // C<->S

            PACKETTYPE_DATA,             // C<--S
            PACKETTYPE_DATA_REQUEST,     // C<->S

            PACKETTYPE_DEBUGGER_QUIT,    // C-->S
            PACKETTYPE_DEBUGGER_KICK,    // C-->S
        } packet_type;
        int packet_version;
        float distributed_time;
        uint64_t magic_number;
        int network_frames;
    } meta;
    union {
        struct {
            char client_name[64];
            char session_key[256];
            char server_pass[64];
            char debugger_pass[64];
            int sglthing_revision;

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

            float player_color_r;
            float player_color_g;
            float player_color_b;
        } serverinfo;
        struct {
            char client_name[64];
            bool admin;
            int player_id;

            float player_color_r;
            float player_color_g;
            float player_color_b;
        } player_add;
        struct {
            int player_id;
        } player_remove;
        struct {
            int event_id;
#ifdef SGLTHING_COMPILE
            struct net_bundle bundle;
#endif
        } scm_event;
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

void network_init(struct network* network, struct script_system* script);
void network_connect(struct network* network, char* ip, int port);
void network_open(struct network* network, char* ip, int port);
void network_manage_socket(struct network* network, struct network_client* client);
void network_transmit_packet(struct network* network, struct network_client* client, struct network_packet packet);
void network_transmit_packet_all(struct network* network, struct network_packet packet);
void network_transmit_data(struct network* network, struct network_client* client, char* data, int data_length);
void network_disconnect_player(struct network* network, bool transmit_disconnect, char* reason, struct network_client* client);
void network_frame(struct network* network, float delta_time);
#ifdef SGLTHING_COMPILE
void network_dbg_ui(struct network* network, struct ui_data* ui);
#endif
void network_close(struct network* network);

void network_start_download(struct network_downloader* network, char* ip, int port, char* rqname, char* pass);
void network_tick_download(struct network_downloader* network);
void network_stop_download(struct network_downloader* network);

#endif