#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#ifdef SGLTHING_COMPILE
#ifndef HEADLESS
#include <GLFW/glfw3.h>
#endif
#include "io.h"
#endif
#include "sglthing.h"
#include "keyboard.h"

#ifdef __WIN32
#define MSG_DONTWAIT 0 // not supported on Winsock
#define MSG_NOSIGNAL 0 // not supported on Winsock
#define SOCK_NONBLOCK 0 // not supported on Winsock
#endif

struct unacknowledged_packet
{
    struct network_packet_header header;

    struct network_packet* packet;
    struct network_client* client;
    
    double next_time;
    int tries;
};

int last_connection_id = 0;
#define ZERO(x) memset(&x,0,sizeof(x))

static int __db_fill_player(void* data, int argc, char** argv, char** col_name)
{
    struct db_player* player = (struct db_player*)data;

    for(int i = 0; i < argc; i++)
    {
        if(strcmp(col_name[i],"id")==0)
        {
            player->id = atoi(argv[i]);
        }
        else if(strcmp(col_name[i],"user_id")==0)
        {
            player->web_id = atoi(argv[i]);
        }
        else if(strcmp(col_name[i],"first_logon_time")==0)
        {
            player->login_time = atoi(argv[i]);
        }
    }

    if(argc == 0)
        player->found = false;
    else
        player->found = true;

    return 0;
}

struct db_player db_player_info(struct network* network, int web_player_id)
{
    struct db_player db_info;
    db_info.found = false;

    char sql[128];
    snprintf(sql,128,"SELECT * FROM USERS WHERE user_id=%i",web_player_id);
    printf("sglthing: executing sql stmt `%s`\n", sql);
    char* err;
    int rc = sqlite3_exec(network->database, sql, __db_fill_player, &db_info, &err);
    if(rc)
    {
        printf("sglthing: sql error `%s`\n", err);
    }

    if(!db_info.found)
        printf("sglthing: could not find db info for id %i\n", web_player_id);

    return db_info;
}

void network_init(struct network* network, struct script_system* script)
{
    network->mode = NETWORKMODE_UNKNOWN;
    network->client.socket = -1;
    network->client.connected = false;
    network->script = script;
    network->receive_packet_callback = NULL;
    network->del_player_callback = NULL;
    network->new_player_callback = NULL;
    network->old_player_add_callback = NULL;
    network->shutdown_ready = false;
    network->shutdown_empty = false;
    network->players = g_hash_table_new(g_int_hash, g_int_equal);
    network->packet_unacknowledged = g_array_new(true, true, sizeof(struct unacknowledged_packet));
    network->packet_id = 0;
    network->server_open = true;
    network->client_capabilities = CAPABILITY_OPUS_VOICE_CHAT;
}

void network_start_download(struct network_downloader* network, char* ip, int port, char* rqname, char* pass)
{
    network->http_client.server = false;
    http_create(&network->http_client, SGLAPI_BASE);
    int flags = 0;

#ifdef NETWORK_TCP
    flags |= SOCK_STREAM;
#else
    flags |= SOCK_DGRAM;
#endif    
    network->socket = socket(AF_INET, flags, 0);  
    if(network->socket == -1)
        printf("sglthing: downloader socket == -1, errno: %i\n", errno);
    network->server_info = false;
    network->data_offset = 0;
    network->data_packet_id = 0;
    ASSERT(network->socket != -1);
    memset(&network->dest, 0, sizeof(struct sockaddr_in));
    network->dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &network->dest.sin_addr);   
    network->dest.sin_port = htons(port);
    printf("sglthing: connecting to %s:%i...\n", ip, port);    
    #ifdef NETWORK_TCP
    if(connect(network->socket, (struct sockaddr*)&network->dest, sizeof(struct sockaddr)) == -1)
    {    
        printf("sglthing: connect() returned -1, failed %i (%s)\n", errno, strerror(errno));
    }
    else
    {
        printf("sglthing: downloader connection established\n");
    #endif
        struct network_packet client_info;
        ZERO(client_info);
        client_info.meta.packet_version = CR_PACKET_VERSION;
        client_info.meta.magic_number = MAGIC_NUMBER;
        client_info.meta.packet_type = PACKETTYPE_CLIENTINFO;
        client_info.packet.clientinfo.observer = true;
        client_info.packet.clientinfo.sglthing_revision = GIT_COMMIT_COUNT;
        client_info.packet.clientinfo.capabilities = 0;

        strncpy(client_info.packet.clientinfo.client_name, "DownloaderClient", 64);
        strncpy(client_info.packet.clientinfo.session_key, network->http_client.sessionkey, 64);
        if(pass)
            strncpy(client_info.packet.clientinfo.server_pass, pass, 64);
        strncpy(network->request_name, rqname, 64);    
#ifdef NETWORK_TCP    
        send(network->socket, &client_info, sizeof(client_info), 0);
    }
#else
    sendto(network->socket, &client_info, sizeof(client_info), MSG_NOSIGNAL, &network->dest, sizeof(network->dest));
#endif
}

void network_tick_download(struct network_downloader* network)
{
    if(network->socket == -1)
        return;

    // TODO: make work with packet_size
    struct network_packet* in_packet;
    while(recv(network->socket, in_packet, NETWORK_PACKET_SIZE, MSG_DONTWAIT) != -1)
    {
        bool cancel_tick = false;
        switch(in_packet->meta.packet_type)
        {
            case PACKETTYPE_PING:
                {
                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_size = sizeof(response.packet.ping);
                    response.meta.packet_version = CR_PACKET_VERSION;
                    response.meta.magic_number = MAGIC_NUMBER;
                    response.meta.packet_type = PACKETTYPE_PING;
                    response.packet.ping.distributed_time = 0.f;
                    send(network->socket, &response, sizeof(response), 0);
                }
                break;
            case PACKETTYPE_SERVERINFO:
                strncpy(network->server_motd, in_packet->packet.serverinfo.server_motd, 128);
                strncpy(network->server_name, in_packet->packet.serverinfo.server_name, 64);
                {
                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_version = CR_PACKET_VERSION;
                    response.meta.magic_number = MAGIC_NUMBER;
                    response.meta.packet_type = PACKETTYPE_DATA_REQUEST;
                    strncpy(response.packet.data_request.data_name, network->request_name, 64);
                    send(network->socket, &response, sizeof(response), 0);
                }
                break;
            case PACKETTYPE_DATA:
                if(!network->data_offset)
                {
                    printf("sglthing: data packet received but data_offset still equals 0\n");
                    network_stop_download(network);
                    break;
                }
                for(int i = 0; i < in_packet->packet.data.data_size; i++)
                {
                    char byte = in_packet->packet.data.data[i];
                    uint64_t addr = in_packet->packet.data.data_size * in_packet->packet.data.data_packet_id + i;
                    if(addr > network->data_size + 512)
                    {
                        printf("sglthing: BAD INTENTS AHEAD\nsglthing: server attempted to send PACKETTYPE_DATA write to address out of data size (packet id: %i, offset: %08x)\n", in_packet->packet.data.data_packet_id, addr);
                        network_stop_download(network);
                        return;
                    }
                    network->data_offset[addr] = byte;
                    if(in_packet->packet.data.data_packet_id < network->data_packet_id)
                    {
                        printf("sglthing: packet %i resent\n", in_packet->packet.data.data_packet_id);
                    }
                    else
                    {
                        network->data_downloaded ++;
                        network->data_packet_id = in_packet->packet.data.data_packet_id;
                    }

                    if(in_packet->packet.data.data_packet_id == in_packet->packet.data.data_final_packet_id)
                    {
                        printf("sglthing: last packet downloaded, datapacket id: %i\n", in_packet->packet.data.data_packet_id);
                        network->data_done = true;
                        network_stop_download(network);
                        return;
                    }
                }
                cancel_tick = true;
                break;
            case PACKETTYPE_DATA_REQUEST:
                network->data_size = in_packet->packet.data_request.data_size;
                network->data_offset = malloc2(network->data_size + 512);
                printf("sglthing: dl: packet request, size: %i bytes\n", network->data_size);
                break;
            default:
                //printf("sglthing: dl: unknown packet id %i\n", in_packet->meta.packet_type);
                break;
        }
    }

    if(network->data_size && network->data_downloaded == network->data_size)
        network_stop_download(network);
}

void network_stop_download(struct network_downloader* network)
{
    if(network->socket == -1)
        return;
    struct network_packet client_info;
    ZERO(client_info);
    client_info.meta.packet_type = PACKETTYPE_DISCONNECT;
    client_info.meta.packet_version = CR_PACKET_VERSION;
    client_info.meta.magic_number = MAGIC_NUMBER;
    strncpy(client_info.packet.disconnect.disconnect_reason, "Download stopped", 32);        
    send(network->socket, &client_info, sizeof(client_info), 0);
    close(network->socket);
    printf("sglthing: download stopped\n");
}

void network_connect(struct network* network, char* ip, int port)
{
    network->http_client.server = false;
    http_create(&network->http_client, SGLAPI_BASE);

    int flags = 0;

#ifdef NETWORK_TCP
    flags |= SOCK_STREAM;
#else
    flags |= SOCK_DGRAM;
#endif

    network->mode = NETWORKMODE_CLIENT; 
    network->client.socket = socket(AF_INET, flags, 0);  
    ASSERT(network->client.socket != -1);
    
    memset(&network->client.sockaddr, 0, sizeof(struct sockaddr_in));
    network->client.sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &network->client.sockaddr.sin_addr);   
    network->client.sockaddr.sin_port = htons(port);
    network->network_frames = 0;
    strncpy(network->debugger_pass, "debugger", 64);

    printf("sglthing: connecting to %s:%i...\n", ip, port);
    
#ifdef NETWORK_TCP
    if(connect(network->client.socket, (struct sockaddr*)&dest, sizeof(struct sockaddr)) == -1)
    {    
        printf("sglthing: connect() returned -1, failed %i (%s)\n", errno, strerror(errno));
        network->status = NETWORKSTATUS_DISCONNECTED;
    }
    else
    {
        printf("sglthing: connection established\n");
#endif
        struct network_packet client_info;
        ZERO(client_info);
        network->status = NETWORKSTATUS_CONNECTED;
        network->client.connected = true;
        client_info.meta.packet_size = sizeof(client_info.packet);
        client_info.meta.acknowledge = true;
        client_info.meta.packet_type = PACKETTYPE_CLIENTINFO;
        client_info.packet.clientinfo.sglthing_revision = GIT_COMMIT_COUNT;
        strncpy(client_info.packet.clientinfo.client_name, config_string_get(&network->http_client.web_config, "user_username"),64);
        strncpy(client_info.packet.clientinfo.session_key, network->http_client.sessionkey, 256);
        strncpy(client_info.packet.clientinfo.server_pass, network->server_pass, 64);
        strncpy(client_info.packet.clientinfo.debugger_pass, network->debugger_pass, 64);
        client_info.packet.clientinfo.color_r = network->client.player_color_r;
        client_info.packet.clientinfo.color_g = network->client.player_color_g;
        client_info.packet.clientinfo.color_b = network->client.player_color_b;
        client_info.packet.clientinfo.capabilities = network->client_capabilities;
        network_transmit_packet(network, &network->client, &client_info);

#ifdef OPUS_ENABLED
        int channels = OPUS_VOICE_CHANNELS;
        int buf_size = OPUS_VOICE_BUFSIZE;
        int sample_rate = OPUS_VOICE_SAMPLERATE;


        if(Pa_OpenDefaultStream(&network->voice_chat_stream, channels, channels, paInt16, sample_rate, buf_size, NULL, NULL) != paNoError)
        {
            printf("sglthing: unable to open voice chat stream\n");   
        }
        else
        {
            Pa_StartStream(network->voice_chat_stream);         
            network->voice_chat_captured_buf = malloc2(buf_size * channels * sizeof(uint16_t));
            network->voice_chat_decoded_buf = malloc2(buf_size * channels * sizeof(uint16_t));
            network->voice_chat_encoded_buf = malloc2(buf_size * channels * sizeof(uint16_t));

            network->voice_chat_encoder = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_AUDIO, NULL);
            network->voice_chat_decoder = opus_decoder_create(sample_rate, channels, NULL);
            printf("sglthing: using libopus voice chat\n");
        }
#endif

        network->next_tick = network->distributed_time + 0.01;
#ifdef NETWORK_TCP
    }
#endif
}

void network_open(struct network* network, char* ip, int port)
{
    network->http_client.server = true;
    http_create(&network->http_client, SGLAPI_BASE);

    struct sockaddr_in serv;

    int flags = SOCK_NONBLOCK;

#ifdef NETWORK_TCP
    flags |= SOCK_STREAM;
#else
    flags |= SOCK_DGRAM;
#endif

    network->client.socket = socket(AF_INET, flags, 0);
    ASSERT(network->client.socket != -1);

    memset(&serv, 0, sizeof(struct sockaddr_in));
    serv.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serv.sin_addr);   
    serv.sin_port = htons(port);

    network->status = NETWORKSTATUS_DISCONNECTED;
    if(bind(network->client.socket,(struct sockaddr*)&serv,sizeof(struct sockaddr)) == -1)
    {
        printf("sglthing: bind() failed, %i\n", errno);
        return;
    }

#ifdef NETWORK_TCP
    if(listen(network->client.socket, 5) == -1)
    {
        printf("sglthing: listen() failed, %i\n", errno);
        return;
    }
#endif

    network->server_clients = g_array_new(false, true, sizeof(struct network_client));
    network->status = NETWORKSTATUS_CONNECTED;
    network->next_tick = network->distributed_time + 0.01;
    network->mode = NETWORKMODE_SERVER;
    network->network_frames = 0;
    network->shutdown_ready = false;
    network->client_default_tick = 0.01;

    int rc = sqlite3_open("sglthing.db",&network->database);
    if(rc){
        printf("sglthing: cant open sqlite db (%s)\n", sqlite3_errmsg(network->database));
    }

    printf("sglthing: server open on ip %s and port %i\n", ip, port);
}

void network_transmit_data(struct network* network, struct network_client* client, char* data, int data_length)
{
    int data_packet_size = DATA_PACKET_SIZE;
    int data_packets = data_length / data_packet_size;
    printf("sglthing: transmitting %i datapackets (packet size: %i)\n", data_packets, data_packet_size);
    client->dl_data = data;
    client->dl_final_packet_id = data_packets; 
    client->dl_packet_id = 0;
    client->dl_packet_size = DATA_PACKET_SIZE;
    client->dl_retries = 0;
}

int network_transmit_packet(struct network* network, struct network_client* client, struct network_packet* packet)
{
    packet->meta.distributed_time = network->distributed_time;
    packet->meta.packet_version = CR_PACKET_VERSION;
    packet->meta.magic_number = MAGIC_NUMBER;
    packet->meta.network_frames = network->network_frames;
    if(packet->meta.packet_type == PACKETTYPE_CLIENTINFO)
        printf("sglthing: sending clientinfo packet (attempt %i)\n",packet->meta.acknowledge_tries);
    if(!client->connected)
        return -1;
    if(packet->meta.acknowledge && packet->meta.acknowledge_tries == 0)
    {
        packet->meta.packet_id = network->packet_id++;
        struct network_packet* tmp = (struct network_packet*)malloc2(sizeof(struct network_packet_header) + packet->meta.packet_size);
        memcpy(tmp, packet, sizeof(struct network_packet_header) + packet->meta.packet_size);
        struct unacknowledged_packet tmp2;
        tmp2.packet = tmp;
        tmp2.tries = 1;
        tmp2.client = client;
        tmp2.next_time = network->distributed_time + 1.f;
        memcpy(&tmp2.header, &packet->meta, sizeof(struct network_packet_header));
        g_array_append_val(network->packet_unacknowledged, tmp2);
    }
    #ifdef NETWORK_TCP
        return send(client->socket, packet, packet->meta.packet_size, MSG_DONTWAIT | MSG_NOSIGNAL);
    #else
        int s_cmd = sendto(network->client.socket, packet, sizeof(struct network_packet_header) + packet->meta.packet_size, MSG_DONTWAIT | MSG_NOSIGNAL, &client->sockaddr, sizeof(client->sockaddr));
        network->packet_tx_numbers[network->packet_time]++;
        if(network->packet_tx_numbers[network->packet_time] > 500)
            printf("sglthing: (m:%i) over 500 packets sent in network frame??? (trying to send a %i)\n", network->mode, packet->meta.packet_type);
        return s_cmd;
    #endif
}

void network_transmit_packet_all(struct network* network, struct network_packet* packet)
{
    for(int i = 0; i < network->server_clients->len; i++)
    {
        struct network_client *cli = &g_array_index(network->server_clients, struct network_client, i);
        network_transmit_packet(network, cli, packet);
    }
}

struct network_client* network_get_player(struct network* network, int player_id)
{
    if(network->mode == NETWORKMODE_SERVER)
    {
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* client = &g_array_index(network->server_clients, struct network_client, i);
            if(player_id == client->connection_id)
                return client;
        }
        return NULL;
    }
    else
        return (struct network_client*)g_hash_table_lookup(network->players, &player_id);
}

int last_given_player_id = 0;

static void network_manage_packet(struct network* network, struct network_client* client, struct network_packet* in_packet)
{
    if(in_packet->meta.magic_number != MAGIC_NUMBER)
        return;
    else if(in_packet->meta.packet_version != CR_PACKET_VERSION)
        return;
    profiler_event("network_manage_packet");
    bool passthru = true;
    if(in_packet->meta.acknowledge)
    {
        struct network_packet ack;
        ack.meta.packet_type = PACKETTYPE_ACKNOWLEDGE;
        ack.meta.packet_size = sizeof(ack.packet.acknowledge);
        ack.meta.acknowledge = false;
        ack.packet.acknowledge.packet_id = in_packet->meta.packet_id;
        network_transmit_packet(network, client, &ack);
    }
    if(network->receive_packet_callback)
    {
        profiler_event("receive_packet_callback");
        passthru = network->receive_packet_callback(network, client, in_packet);
        profiler_end();
    }
    if(!passthru)
    {
        profiler_end();
        return;
    }
    switch(in_packet->meta.packet_type)
    {
        case PACKETTYPE_ACKNOWLEDGE:
            for(int i = 0; i < network->packet_unacknowledged->len; i++)
            {
                struct unacknowledged_packet pkt = g_array_index(network->packet_unacknowledged, struct unacknowledged_packet, i);
                if(pkt.packet->meta.packet_id == in_packet->packet.acknowledge.packet_id)
                {
                    g_array_remove_index(network->packet_unacknowledged, i);
                    free2(pkt.packet);
                    break;
                }
            }
            break;
        case PACKETTYPE_DISCONNECT:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                network_disconnect_player(network, false, in_packet->packet.disconnect.disconnect_reason, client);
            }
            else
            {
                printf("sglthing: client '%s' disconnected '%s'\n", client->client_name, in_packet->packet.disconnect.disconnect_reason);
                network_disconnect_player(network, true, in_packet->packet.disconnect.disconnect_reason, client);
            }
            break;
        case PACKETTYPE_PING:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                network->distributed_time = in_packet->packet.ping.distributed_time;
                network->pung = true;

                struct network_packet response;
                ZERO(response);
                response.meta.packet_type = PACKETTYPE_PING;
                response.meta.packet_size = sizeof(response.packet.ping);
                response.packet.ping.ping_id = client->last_ping_id++;
                response.packet.ping.distributed_time = network->distributed_time;
                network_transmit_packet(network, client, &response);
            }
            else
            {
                if(in_packet->packet.ping.ping_id > client->last_ping_id)
                {
                    client->last_ping_time = network->distributed_time;
                    client->lag = network->distributed_time - in_packet->packet.ping.distributed_time;
                    client->last_ping_id = in_packet->packet.ping.ping_id;
                }
            }
            break;
        case PACKETTYPE_CLIENTINFO:
            if(network->mode == NETWORKMODE_SERVER && !client->authenticated)
            {
                if(strlen(network->server_pass) == 0 || strncmp(in_packet->packet.clientinfo.server_pass, network->server_pass, 64) == 0)
                {
                    client->verified = true;
                    if(!http_check_sessionkey(&network->http_client, in_packet->packet.clientinfo.session_key))
                    {
                        printf("sglthing: client '%s' failed key\n", in_packet->packet.clientinfo.client_name);
                        if(network->security)
                            network_disconnect_player(network, true, "Bad key", client);
                        client->verified = false;

                        client->player_color_r = 0.0;
                        client->player_color_g = 1.0;
                        client->player_color_b = 1.0;

                        //client->web_player_id = http_get_userid(&network->http_client, in_packet->packet.clientinfo.session_key);
                    }
                    else
                    {
                        client->user = http_get_userdata(&network->http_client, in_packet->packet.clientinfo.session_key);
                        if(client->user.found)
                        {
                            printf("sglthing: was able to get web userdata, id = %i\n", client->user.user_id);
                        }

                        client->db = db_player_info(network, client->user.user_id);
                        if(!client->db.found)
                        {
                            char sql[128];
                            snprintf(sql,128,"INSERT INTO USERS (user_id, first_logon_time) VALUES (%i, %lu)", client->user.user_id, (unsigned long)time(NULL));
                            printf("sglthing: executing sql stmt `%s`\n", sql);
                            char* err;
                            int rc = sqlite3_exec(network->database, sql, NULL, NULL, &err);
                            if(rc)
                                printf("sglthing: sql error `%s`\n", err);
                            else
                                client->db = db_player_info(network, client->user.user_id); // try again
                        }

                        char url[256];
                        snprintf(url,256,"auth/key_owner?sessionkey=%s",in_packet->packet.clientinfo.session_key);
                        char* username = http_get(&network->http_client,url);
                        if(username)
                        {
                            strncpy(in_packet->packet.clientinfo.client_name, username, 64);
                            free2(username);
                        }

                        client->player_color_r = in_packet->packet.clientinfo.color_r;
                        client->player_color_g = in_packet->packet.clientinfo.color_g;
                        client->player_color_b = in_packet->packet.clientinfo.color_b;
                        client->verified = true;
                    }

                    if(!in_packet->packet.clientinfo.observer && strlen(network->debugger_pass) && strncmp(in_packet->packet.clientinfo.debugger_pass, network->debugger_pass, 64) == 0)
                    {
                        printf("sglthing: client '%s' used debugger password\n", in_packet->packet.clientinfo.client_name);
                        client->debugger = true;
                    }

                    strncpy(client->client_name, in_packet->packet.clientinfo.client_name, 64);
                    client->authenticated = true;
                    client->player_id = last_given_player_id++;
                    client->client_version = in_packet->packet.clientinfo.sglthing_revision;
                    client->owner = network;
                    client->last_ping_id = 0;
                    if(client->client_version != GIT_COMMIT_COUNT)
                        printf("sglthing: WARN: new client '%s' is on sglthing r%i, while server is on sglthing r%i\n", in_packet->packet.clientinfo.client_name, client->client_version, GIT_COMMIT_COUNT);

                    printf("sglthing: client '%s' authenticated (%i)\n", in_packet->packet.clientinfo.client_name, client->player_id);

                    struct network_packet response;
                    ZERO(response);

                    response.meta.packet_type = PACKETTYPE_SERVERINFO;  
                    response.meta.packet_size = sizeof(response.packet);
                    response.meta.acknowledge = true;
                    response.packet.serverinfo.player_id = client->player_id;
                    response.packet.serverinfo.sglthing_revision = GIT_COMMIT_COUNT;
                    response.packet.serverinfo.player_color_r = client->player_color_r;
                    response.packet.serverinfo.player_color_g = client->player_color_g;
                    response.packet.serverinfo.player_color_b = client->player_color_b;
                    response.packet.serverinfo.player_count = 0;
                    response.packet.serverinfo.verified = client->verified;
                    memcpy(response.packet.serverinfo.session_key, network->http_client.sessionkey, 256);
                    strncpy(response.packet.serverinfo.server_motd, network->server_motd,128);
                    strncpy(response.packet.serverinfo.server_name, network->server_name,64);
                    network_transmit_packet(network, client, &response);                            

                    response.meta.packet_type = PACKETTYPE_PLAYER_ADD;        
                    response.packet.player_add.player_id = client->player_id;
                    response.packet.player_add.admin = client->debugger;
                    response.packet.player_add.player_color_r = client->player_color_r;
                    response.packet.player_add.player_color_g = client->player_color_g;
                    response.packet.player_add.player_color_b = client->player_color_b;
                    response.packet.player_add.user_data = client->user;
                    response.packet.player_add.verified = client->verified;
                    strncpy(response.packet.player_add.client_name, in_packet->packet.clientinfo.client_name, 64);
                    network_transmit_packet_all(network, &response);

                    g_hash_table_insert(network->players, &client->player_id, (gpointer)client->connection_id);
                    for(int i = 0; i < network->server_clients->len; i++)
                    {
                        struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
                        if(cli->player_id != client->player_id)
                        {
                            struct network_packet player_packet;
                            player_packet.meta.packet_type = PACKETTYPE_PLAYER_ADD;        
                            player_packet.meta.packet_size = sizeof(player_packet.packet.player_add);
                            player_packet.packet.player_add.admin = cli->debugger;
                            player_packet.packet.player_add.player_color_r = cli->player_color_r;
                            player_packet.packet.player_add.player_color_g = cli->player_color_g;
                            player_packet.packet.player_add.player_color_b = cli->player_color_b;
                            player_packet.packet.player_add.observer = cli->observer;
                            player_packet.packet.player_add.player_id = cli->player_id;
                            strncpy(player_packet.packet.player_add.client_name, cli->client_name, 64);
                            printf("sglthing: player list: %s (%i, c: %p)\n", cli->client_name, cli->player_id, cli);
                            network_transmit_packet(network, client, &player_packet);

                            if(network->old_player_add_callback)
                                network->old_player_add_callback(network, client, cli);
                        }
                    }

                    client->observer = in_packet->packet.clientinfo.observer;            
                    client->ping_time_interval = network->client_default_tick;

                    if(network->new_player_callback)
                        network->new_player_callback(network, client);
                }
                else
                {
                    printf("sglthing: client '%s' failed connect password (%s)\n", in_packet->packet.clientinfo.client_name, in_packet->packet.clientinfo.server_pass);
                    network_disconnect_player(network, true, "Bad password", client);
                }
            }
            break;
        case PACKETTYPE_SERVERINFO:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                printf("sglthing: authenticated to server %s, motd: %s\n", in_packet->packet.serverinfo.server_name, in_packet->packet.serverinfo.server_motd);
                network->client.player_id = in_packet->packet.serverinfo.player_id;
                strncpy(network->server_name, in_packet->packet.serverinfo.server_name, 64);
                strncpy(network->server_motd, in_packet->packet.serverinfo.server_name, 128);
                network->client.client_version = in_packet->packet.serverinfo.sglthing_revision;
                network->client.authenticated = true;
                network->client.verified = in_packet->packet.serverinfo.verified;

                /*for(int i = 0; i < in_packet->packet.serverinfo.player_count; i++)
                {
                    struct network_client* new_client = (struct network_client*)malloc2(sizeof(struct network_client));
                    struct player_auth_list_entry* player = &in_packet->packet.serverinfo.players[i];

                    if(player->player_id == network->client.player_id)
                        continue;

                    strncpy(new_client->client_name, player->client_name, 64);
                    new_client->debugger = player->admin;
                    new_client->player_color_r = player->player_color_r;
                    new_client->player_color_b = player->player_color_g;
                    new_client->player_color_g = player->player_color_b;
                    new_client->player_id = player->player_id;
                    new_client->owner = network;                    
                    new_client->user_data = 0;
                    
                    printf("sglthing: client new player (PACKETTYPE_SERVERINFO users) %s %i\n", new_client->client_name, new_client->player_id);

                    g_hash_table_insert(network->players, &new_client->player_id, new_client);  

                    if(network->new_player_callback)
                        network->new_player_callback(network, new_client);    
                }*/
            }
            break;
        case PACKETTYPE_PLAYER_ADD:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                struct network_client* new_client = (struct network_client*)malloc2(sizeof(struct network_client));
                printf("sglthing: client new player %s %i\n", in_packet->packet.player_add.client_name, in_packet->packet.player_add.player_id);
                // copy

                new_client->authenticated = true;
                new_client->connected = true;
                new_client->player_color_r = in_packet->packet.player_add.player_color_r;
                new_client->player_color_g = in_packet->packet.player_add.player_color_g;
                new_client->player_color_b = in_packet->packet.player_add.player_color_b;
                new_client->player_id = in_packet->packet.player_add.player_id;
                new_client->user = in_packet->packet.player_add.user_data;
                new_client->verified = in_packet->packet.player_add.verified;
                new_client->owner = network;                    
                new_client->user_data = 0;

                strncpy(new_client->client_name, in_packet->packet.player_add.client_name, 64);

                g_hash_table_insert(network->players, &new_client->player_id, new_client);        

                if(network->new_player_callback)
                    network->new_player_callback(network, new_client);        
            }
            break;
        case PACKETTYPE_PLAYER_REMOVE:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                printf("sglthing: client del player %i\n", in_packet->packet.player_remove.player_id);
                struct network_client* old_client = g_hash_table_lookup(network->players, &in_packet->packet.player_remove.player_id);
                if(network->del_player_callback)
                    network->del_player_callback(network, client);
                g_hash_table_remove(network->players, &in_packet->packet.player_remove.player_id);
                if(in_packet->packet.player_remove.player_id != client->player_id)
                {
                    free2(old_client);
                    //client->connected = false;
                    //network->status = NETWORKSTATUS_DISCONNECTED;
                    //printf("sglthing: we ought to be out of here!\n");
                    //network_disconnect_player(network, false, "disconnected", client);
                    return;
                }
            }
            break;
        case PACKETTYPE_LUA_DATA:
            // printf("sglthing: %s received event %i, data: '%s'\n", (network->mode == NETWORKMODE_SERVER) ? "server" : "client", in_packet->packet.scm_event.event_id, in_packet->packet.scm_event.event_data);

            break;
        case PACKETTYPE_CHAT_MESSAGE:
            if(network->mode == NETWORKMODE_SERVER)
            {
                printf("sglthing '%s: %s'\n", client->client_name, in_packet->packet.chat_message.message);
                in_packet->packet.chat_message.verified = client->verified;
                in_packet->packet.chat_message.player_id = client->player_id;
                strncpy(in_packet->packet.chat_message.client_name,client->client_name,64);
                network_transmit_packet_all(network, &in_packet);
            }
            else
            {
                printf("sglthing '%s: %s'\n", in_packet->packet.chat_message.client_name, in_packet->packet.chat_message.message);
            }
            break;   
        case PACKETTYPE_DATA_REQUEST:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(!client->observer)
                    return;
                printf("sglthing: client requested %s\n", in_packet->packet.data_request.data_name);

                if(strncmp(in_packet->packet.data_request.data_name,"game/data.tar",64)==0)
                {
                    #define TMP_DATA_SIZE 26666666
                    char* mdata = malloc2(TMP_DATA_SIZE);
                    z_stream strm;
                    strm.zalloc = Z_NULL;
                    strm.zfree = Z_NULL;
                    strm.opaque = Z_NULL;
                    int ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
                    int len = strlen("Hello World")+1;
                    strm.next_in = (z_const unsigned char*)"Hello World";
                    strm.next_out = mdata;
                    while (strm.total_in != len && strm.total_out < TMP_DATA_SIZE) {
                        strm.avail_in = strm.avail_out = 1; /* force small buffers */
                        ret = deflate(&strm, Z_NO_FLUSH);
                    }
                    for (;;) {
                        strm.avail_out = 1;
                        ret = deflate(&strm, Z_FINISH);
                        if(ret == Z_STREAM_END) break;
                    }
                    ret = deflateEnd(&strm);
                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_type = PACKETTYPE_DATA_REQUEST;
                    response.packet.data_request.data_size = TMP_DATA_SIZE;
                    network_transmit_packet(network, client, &response);
                    printf("sglthing: transmitting data\n");
                    network_transmit_data(network, client, mdata, TMP_DATA_SIZE);
                }
            }
            break;      
        case PACKETTYPE_OPUS_PACKET:
            if(network->mode == NETWORKMODE_SERVER)
            {
                bool can_hear = true; // TODO: callback function for this
                if(can_hear)
                {
                    in_packet->packet.opus_packet.player_id = client->player_id;
                    network_transmit_packet_all(network, in_packet);
                }
            }
#ifdef OPUS_ENABLED
            else
            {
                struct network_client* client = g_hash_table_lookup(network->players, &in_packet->packet.opus_packet.player_id);
                client->last_voice_packet = network->time;
                int dec_bytes = opus_decode(network->voice_chat_decoder, in_packet->packet.opus_packet.buf_data, in_packet->packet.opus_packet.buf_size, network->voice_chat_decoded_buf, OPUS_VOICE_BUFSIZE, 0);
                client->last_voice_packet_avg = 0.f;
                for(int i = 0; i < dec_bytes; i++)
                    client->last_voice_packet_avg += network->voice_chat_decoded_buf[i];
                client->last_voice_packet_avg /= dec_bytes;
                client->last_voice_packet_avg -= 32767;
                client->last_voice_packet_avg /= 32767;
                client->last_voice_packet_avg *= 10.f;
                client->last_voice_packet_avg = fabsf(client->last_voice_packet_avg);
#ifdef OPUS_VOICE_DISALLOW_SELF
                if(in_packet->packet.opus_packet.player_id != client->player_id)
#endif
                    if(dec_bytes > 0)
                        Pa_WriteStream(network->voice_chat_stream, network->voice_chat_decoded_buf, dec_bytes);
                    else
                        printf("sglthing: unable to decode voice packet (%i, b: %i)\n", dec_bytes, in_packet->packet.opus_packet.buf_size);
            }
            break;
#endif
        default:            
            printf("sglthing: cli:%s unknown packet %i\n", network->mode?"true":"false", (int)in_packet->meta.packet_type);
            break;
    }
    profiler_end();
}

void network_manage_socket(struct network* network, struct network_client* client)
{
    profiler_event("network_manage_socket");
    char packet_datagram[65527];
    struct network_packet* in_packet = (struct network_packet*)packet_datagram;
    struct sockaddr_in sockaddr;
    int sockaddr_len = sizeof(sockaddr);
#ifdef NETWORK_TCP
    while(recv(client->socket, packet_datagram, sizeof(struct network_packet_header), MSG_DONTWAIT) != -1)
#else
    while(recvfrom(client->socket, packet_datagram, 65527, MSG_DONTWAIT, &sockaddr, &sockaddr_len) != -1)
#endif
    {
        network->packet_rx_numbers[network->packet_time]++;
        #ifndef NETWORK_TCP
        if(network->mode == NETWORKMODE_SERVER)
        {
            char ip_addr[64];
            inet_ntop(AF_INET, &(sockaddr.sin_addr), ip_addr, INET_ADDRSTRLEN);
            struct network_client* sender = NULL;
            for(int i = 0; i < network->server_clients->len; i++)
            {
                struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
                if(cli->sockaddr.sin_addr.s_addr == sockaddr.sin_addr.s_addr && cli->sockaddr.sin_port == sockaddr.sin_port)
                {
                    sender = cli;
                    break;
                }
            }

            if(sender == NULL)
            {
                if(network->server_open)
                {
                    printf("sglthing: server: new udp connection\n");
                    struct network_client new_client;
                    new_client.sockaddr = sockaddr;
                    new_client.authenticated = false;
                    new_client.player_id = -1;
                    new_client.connection_id = last_connection_id++;
                    new_client.connected = true;
                    new_client.last_ping_time = network->distributed_time;
                    new_client.ping_time_interval = 0.5f;
                    new_client.next_ping_time = network->distributed_time + new_client.ping_time_interval;
                    new_client.dl_data = NULL;
                    new_client.owner = network;                    
                    new_client.user_data = 0;
                    g_array_append_val(network->server_clients, new_client);
                    sender = &g_array_index(network->server_clients, struct network_client, new_client.connection_id);
                    char sock_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &sockaddr.sin_addr, sock_str, INET_ADDRSTRLEN);
                    printf("sglthing: server: udp connection sockaddr: %s:%i (%p)\n", sock_str, new_client.sockaddr.sin_port, sender);
                }
                else
                {
                    printf("sglthing: not accepting because server is closed");
                }
            }

            network_manage_packet(network, sender, in_packet);
        }
        else
        {
            network_manage_packet(network, &network->client, in_packet);
        }


        #endif
    }
    //if(errno)
    //    printf("sglthing: send() %s\n", strerror(errno));
    profiler_end();
}

void network_frame(struct network* network, float delta_time, double time)
{
    if(!network)
        return;
    if(network->status != NETWORKSTATUS_CONNECTED)
        return;
    //if(glfwGetTime() > network->next_tick)
    //    return;

    network->packet_time = network->network_frames % NETWORK_HISTORY_FRAMES;
    network->packet_tx_numbers[network->packet_time] = 0;
    network->packet_rx_numbers[network->packet_time] = 0;

    network->network_frames++;
    network->time = time;

    profiler_event("network_frame");
    for(int i = 0; i < network->packet_unacknowledged->len; i++)
    {
        struct unacknowledged_packet* pkt = &g_array_index(network->packet_unacknowledged, struct unacknowledged_packet, i);
        if(network->distributed_time > pkt->next_time)
        {
            if(pkt->tries >= 10)
            {
                printf("sglthing: pkt too many tries (%i)\n", pkt->packet->meta.packet_type);
                g_array_remove_index(network->packet_unacknowledged, i);
                break;
            }
            pkt->next_time = network->distributed_time + 0.5f;
            pkt->tries++;
            pkt->packet->meta.acknowledge_tries = pkt->tries;
            network_transmit_packet(network, pkt->client, pkt->packet);
        }
    }

    if(network->mode == NETWORKMODE_SERVER)
    {
        network->distributed_time = time;
        if(network->server_clients->len == 0 && network->shutdown_ready && network->shutdown_empty)
        {
            printf("sglthing: server shutting down because it is empty\n");
            network_close(network);
            profiler_end();
            return;
        }
#ifndef NETWORK_TCP
        network_manage_socket(network, &network->client);
#endif
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
#ifdef NETWORK_TCP
            network_manage_socket(network, cli);
#endif

            if(cli->connected)
            {
                if(network->distributed_time - cli->last_ping_time > 10.0)
                {
                    printf("sglthing: disconnecting player %i after 10 seconds of inactivity\n", cli->player_id);
                    network_disconnect_player(network, true, "Timed out (10 seconds)", cli);
                    break;
                }
                if(network->distributed_time - cli->next_ping_time > cli->ping_time_interval)
                {
                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_type = PACKETTYPE_PING;
                    response.meta.acknowledge = false;
                    response.meta.packet_size = sizeof(response.packet.ping);
                    response.packet.ping.distributed_time = network->distributed_time;
                    response.packet.ping.player_lag = cli->lag;
                    response.packet.ping.ping_id = -1;
                    network_transmit_packet(network, cli, &response);
                    cli->next_ping_time = network->time + cli->ping_time_interval;
                    if(cli->dl_data)
                    {
                        struct network_packet data_packet;
                        int l_pctg = 0;

                        for(int i = 0; i < NO_DATAPACKETS_TICK; i++)
                        {
                            if(cli->dl_packet_id <= cli->dl_final_packet_id)
                            {
                                char* data2 = cli->dl_data + (i*cli->dl_packet_size);
                                memcpy(data_packet.packet.data.data, data2, cli->dl_packet_size);
                                data_packet.meta.packet_type = PACKETTYPE_DATA;
                                data_packet.packet.data.data_size = cli->dl_packet_size;
                                data_packet.packet.data.data_final_packet_id = cli->dl_final_packet_id;
                                data_packet.packet.data.data_packet_id = cli->dl_packet_id;  
                                int ret = network_transmit_packet(network, cli, &data_packet);  
                                if(ret == -1)
                                {
                                    cli->dl_retries++;
                                    i -= 1;
                                }
                                else
                                {
                                    cli->dl_successes++;
                                    cli->dl_packet_id++;
                                }
                            }
                            else
                            {
                                printf("sglthing: final datapacket sent\n");
                                printf("sglthing: transmission failure rate %f%%\n", ((float)cli->dl_retries/(cli->dl_retries+cli->dl_final_packet_id))*100.f);
                                printf("sglthing: transmission sent rate (should be 100%%) %f%%\n", ((float)cli->dl_successes/(cli->dl_final_packet_id))*100.f);
                                free2(cli->dl_data);
                                cli->dl_data = NULL;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            else
                break;
        }
        struct sockaddr_in connecting_client;
        socklen_t socksize = sizeof(struct sockaddr_in);

        #ifdef NETWORK_TCP
        int new_socket = accept(network->client.socket, (struct sockaddr*)&connecting_client, &socksize);
        if(new_socket != -1)
        {
            printf("sglthing: server: new connection\n");
            struct network_client new_client;
            new_client.socket = new_socket;
            new_client.sockaddr = connecting_client;
            new_client.authenticated = false;
            new_client.player_id = -1;
            new_client.connection_id = last_connection_id++;
            new_client.connected = true;
            new_client.last_ping_time = glfwGetTime();
            new_client.ping_time_interval = 5.0;
            new_client.dl_data = NULL;
            new_client.owner = network;
            new_client.user_data = 0;
            g_array_append_val(network->server_clients, new_client);

            network->shutdown_ready = true;
        }
        else
        {
            if(errno != EAGAIN)
                printf("sglthing: accept() errno == %i %s\n", errno, strerror(errno));
        }
        #endif
    }
    else
    {
#ifdef OPUS_ENABLED
        if(Pa_GetStreamReadAvailable(network->voice_chat_stream) > OPUS_VOICE_BUFSIZE && keys_down[GLFW_KEY_V])
        {
            struct network_packet response; // for sizeof, probably bad
            Pa_ReadStream(network->voice_chat_stream, network->voice_chat_captured_buf, OPUS_VOICE_BUFSIZE);
            int buf_size = (OPUS_VOICE_BUFSIZE * OPUS_VOICE_CHANNELS);
            struct network_packet* voice_packet = malloc2(buf_size + sizeof(response.packet.opus_packet) + sizeof(response.meta));
            voice_packet->meta.packet_type = PACKETTYPE_OPUS_PACKET;
            voice_packet->meta.packet_size = sizeof(response.packet.opus_packet) + buf_size;
            int enc_bytes = opus_encode(network->voice_chat_encoder, network->voice_chat_captured_buf, OPUS_VOICE_BUFSIZE, voice_packet->packet.opus_packet.buf_data, buf_size);
            voice_packet->packet.opus_packet.buf_size = buf_size;
            if(enc_bytes < 0)
            {
                printf("sglthing: unable to encode voice packet (%i)\n", enc_bytes);
            }
            else
                network_transmit_packet(network, &network->client, voice_packet);
            free2(voice_packet);
        }
#endif
        network_manage_socket(network, &network->client);
    }
    profiler_end();
}

void network_disconnect_player(struct network* network, bool transmit_disconnect, char* reason, struct network_client* client)
{
    if(network->mode == NETWORKMODE_SERVER)
    {            
        struct network_packet response;
        ZERO(response);
        response.meta.packet_type = PACKETTYPE_DISCONNECT;
        strncpy(response.packet.disconnect.disconnect_reason, reason, 32);
        network_transmit_packet(network, client, &response);
        if(network->del_player_callback)
            network->del_player_callback(network, client);
        g_hash_table_remove(network->players, &client->player_id);
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* v = &g_array_index(network->server_clients,struct network_client,i);
            if(client->connection_id == v->connection_id)
            {
                g_array_remove_index(network->server_clients, i);
                break;
            }
        }
        if(client->authenticated && transmit_disconnect)
        {
            response.meta.packet_type = PACKETTYPE_PLAYER_REMOVE;
            response.packet.player_remove.player_id = client->player_id;
            network_transmit_packet_all(network, &response);
        }
    }
    else
    {    
        if(transmit_disconnect)
        {
            struct network_packet response;
            ZERO(response);
            response.meta.packet_type = PACKETTYPE_DISCONNECT;
            strncpy(response.packet.disconnect.disconnect_reason, reason, 32);
            network_transmit_packet(network, client, &response);
        }
        printf("sglthing: network disconnected '%s'\n", reason);
        strncpy(network->disconnect_reason, reason, 32);
        network->status = NETWORKSTATUS_DISCONNECTED;
    }
    client->connected = false;
    if(client->socket != -1)
        close(client->socket);
    client->socket = -1;
}

void network_close(struct network* network)
{
    if(!network)
        return;
    if(network->mode == NETWORKMODE_SERVER)
    {
        while(network->server_clients->len > 0)
        {
            struct network_client* v = &g_array_index(network->server_clients,struct network_client,0);
            network_disconnect_player(network,true,"Server shutting down",v);
            g_array_remove_index(network->server_clients, 0);
        }
        sqlite3_close(network->database);
    }
    else
    {
        network_disconnect_player(network,true,"Disconnect by user",&network->client);
    }
    network->status = NETWORKSTATUS_DISCONNECTED;
}

#ifdef SGLTHING_COMPILE
void network_dbg_ui(struct network* network, struct ui_data* ui)
{
    if(!network)
        return;
    if(network->status != NETWORKSTATUS_CONNECTED)
        return;
    vec2 pos_base;
    char tx[256];
    vec4 oldfg, oldbg;
    glm_vec4_copy(ui->foreground_color, oldfg);
    glm_vec4_copy(ui->background_color, oldbg);
    if(network->mode == NETWORKMODE_SERVER)
    {
        pos_base[0] = 0.f;
        pos_base[1] = 64.f;
        snprintf(tx,256,"SERVER, %i clients connected, frame %i", network->server_clients->len, network->network_frames);
        ui_draw_text(ui,pos_base[0],pos_base[1]-16.f,tx,1.f);
        ui->background_color[3] = 0.f;
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
            ui->foreground_color[0] = cli->player_color_r;
            ui->foreground_color[1] = cli->player_color_g;
            ui->foreground_color[2] = cli->player_color_b;
            ui->background_color[0] = cli->player_color_b;
            ui->background_color[1] = cli->player_color_r;
            ui->background_color[2] = cli->player_color_g;
            snprintf(tx,256,"Client '%s' %i %i %i, ping %fmsec, dtd %fmsec, %c%c%c%c%c", cli->client_name, i, cli->player_id, cli->connection_id, cli->lag * 1000.0, (network->distributed_time - cli->last_ping_time) * 1000.0,
                cli->authenticated?'A':'-',cli->connected?'C':'-',cli->debugger?'D':'-',cli->observer?'O':'-',cli->verified?'V':'-');
            ui_draw_text(ui,pos_base[0],pos_base[1]+((1+i)*16.f),tx,1.f);
        }
        glm_vec4_copy(oldfg, ui->foreground_color);
        glm_vec4_copy(oldbg, ui->background_color);
        int frame_avg_rx = 0;
        int frame_avg_tx = 0;
        for(int i = 0; i < NETWORK_HISTORY_FRAMES; i++)
        {
            snprintf(tx,256,"%03i\n%03i",network->packet_rx_numbers[i],network->packet_tx_numbers[i]);
            ui_draw_text(ui,(16.f*(i*2)),199.f-16.f,tx,1.f);
            frame_avg_rx += network->packet_rx_numbers[i];
            frame_avg_tx += network->packet_tx_numbers[i];
        }

        snprintf(tx,256,"dT=%f, tx_a: %fpr, rx_a: %fpr", network->distributed_time, frame_avg_tx / 16.f, frame_avg_rx / 16.f);
        ui_draw_text(ui,pos_base[0],pos_base[1],tx,1.f);
    }
    else
    {
        pos_base[0] = 512.f;
        pos_base[1] = 64.f;
        ui_draw_text(ui,pos_base[0],pos_base[1]-16.f,"CLIENT",1.f);
        if(network->status == NETWORKSTATUS_DISCONNECTED)
            ui_draw_text(ui,pos_base[0],pos_base[1]+16.f,network->disconnect_reason,1.f);

        if(network->client.authenticated && GIT_COMMIT_COUNT != network->client.client_version && (int)network->time % 2 == 0)
        {
            snprintf(tx,256,"WARNING: Client (sglthing r%i) and server (sglthing r%i) versions dont match",GIT_COMMIT_COUNT,network->client.client_version);
            ui->foreground_color[2] = 0.f;    
            ui_draw_text(ui, 0, 0, tx, 1.f);
            ui->foreground_color[2] = 1.f;
        } else if(!network->client.authenticated)
        {
            snprintf(tx,256,"WARNING: Client has not been authenticated yet by server");
            ui->foreground_color[2] = 0.f;    
            ui_draw_text(ui, 0, 0, tx, 1.f);
            ui->foreground_color[2] = 1.f;
        }            
        int frame_avg_rx = 0;
        int frame_avg_tx = 0;
        for(int i = 0; i < NETWORK_HISTORY_FRAMES; i++)
        {
            snprintf(tx,256,"%03i\n%03i",network->packet_rx_numbers[i],network->packet_tx_numbers[i]);
            ui_draw_text(ui,(16.f*(i*2)),199.f+24.f,tx,1.f);
            frame_avg_rx += network->packet_rx_numbers[i];
            frame_avg_tx += network->packet_tx_numbers[i];
        }

        snprintf(tx,256,"dT=%f, tx_a: %fpr, rx_a: %fpr", network->distributed_time, frame_avg_tx / 16.f, frame_avg_rx / 16.f);
        ui_draw_text(ui,pos_base[0],pos_base[1],tx,1.f);
    }
    glm_vec4_copy(oldfg, ui->foreground_color);
    glm_vec4_copy(oldbg, ui->background_color);
}
#endif