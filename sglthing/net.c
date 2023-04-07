#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <zlib.h>
#ifdef SGLTHING_COMPILE
#include <GLFW/glfw3.h>
#include "s7/script_networking.h"
#include "io.h"
#endif
#include "sglthing.h"

#define ZERO(x) memset(&x,0,sizeof(x))

static s7_scheme* network_get_s7_pointer(struct network* network)
{
    return (s7_scheme*)script_s7(network->script);
}

void network_init(struct network* network, struct script_system* script)
{
    network->mode = NETWORKMODE_UNKNOWN;
    network->client.socket = -1;
    network->client.connected = false;
    network->script = script;
    network->receive_packet_callback = NULL;
    network->shutdown_ready = false;
    network->shutdown_empty = false;
    network->players = g_hash_table_new(g_int_hash, g_int_equal);
}

void network_start_download(struct network_downloader* network, char* ip, int port, char* rqname, char* pass)
{
    network->http_client.server = false;
    http_create(&network->http_client, SGLAPI_BASE);
    struct sockaddr_in dest;
    network->socket = socket(AF_INET, SOCK_STREAM, 0);  
    if(network->socket == -1)
        printf("sglthing: downloader socket == -1, errno: %i\n", errno);
    network->server_info = false;
    network->data_offset = 0;
    network->data_packet_id = 0;
    ASSERT(network->socket != -1);
    memset(&dest, 0, sizeof(struct sockaddr_in));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &dest.sin_addr);   
    dest.sin_port = htons(port);
    printf("sglthing: connecting to %s:%i...\n", ip, port);    
    if(connect(network->socket, (struct sockaddr*)&dest, sizeof(struct sockaddr)) == -1)
    {    
        printf("sglthing: connect() returned -1, failed %i (%s)\n", errno, strerror(errno));
    }
    else
    {
        printf("sglthing: downloader connection established\n");
        struct network_packet client_info;
        ZERO(client_info);
        client_info.meta.packet_version = CR_PACKET_VERSION;
        client_info.meta.magic_number = MAGIC_NUMBER;
        client_info.meta.packet_type = PACKETTYPE_CLIENTINFO;
        client_info.packet.clientinfo.observer = true;
        client_info.packet.clientinfo.sglthing_revision = GIT_COMMIT_COUNT;

        strncpy(client_info.packet.clientinfo.client_name, "DownloaderClient", 64);
        strncpy(client_info.packet.clientinfo.session_key, network->http_client.sessionkey, 64);
        if(pass)
            strncpy(client_info.packet.clientinfo.server_pass, pass, 64);
        strncpy(network->request_name, rqname, 64);        
        send(network->socket, &client_info, sizeof(client_info), 0);
    }
}

void network_tick_download(struct network_downloader* network)
{
    if(network->socket == -1)
        return;

    struct network_packet in_packet;
    while(recv(network->socket, &in_packet, sizeof(struct network_packet), MSG_DONTWAIT) != -1)
    {
        bool cancel_tick = false;
        switch(in_packet.meta.packet_type)
        {
            case PACKETTYPE_PING:
                {
                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_version = CR_PACKET_VERSION;
                    response.meta.magic_number = MAGIC_NUMBER;
                    response.meta.packet_type = PACKETTYPE_PING;
                    response.packet.ping.distributed_time = glfwGetTime();
                    send(network->socket, &response, sizeof(response), 0);
                }
                break;
            case PACKETTYPE_SERVERINFO:
                strncpy(network->server_motd, in_packet.packet.serverinfo.server_motd, 128);
                strncpy(network->server_name, in_packet.packet.serverinfo.server_name, 64);
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
                for(int i = 0; i < in_packet.packet.data.data_size; i++)
                {
                    char byte = in_packet.packet.data.data[i];
                    uint64_t addr = in_packet.packet.data.data_size * in_packet.packet.data.data_packet_id + i;
                    if(addr > network->data_size + 512)
                    {
                        printf("sglthing: BAD INTENTS AHEAD\nsglthing: server attempted to send PACKETTYPE_DATA write to address out of data size (packet id: %i, offset: %08x)\n", in_packet.packet.data.data_packet_id, addr);
                        network_stop_download(network);
                        return;
                    }
                    network->data_offset[addr] = byte;
                    if(in_packet.packet.data.data_packet_id < network->data_packet_id)
                    {
                        printf("sglthing: packet %i resent\n", in_packet.packet.data.data_packet_id);
                    }
                    else
                    {
                        network->data_downloaded ++;
                        network->data_packet_id = in_packet.packet.data.data_packet_id;
                    }

                    if(in_packet.packet.data.data_packet_id == in_packet.packet.data.data_final_packet_id)
                    {
                        printf("sglthing: last packet downloaded, datapacket id: %i\n", in_packet.packet.data.data_packet_id);
                        network->data_done = true;
                        network_stop_download(network);
                        return;
                    }
                }
                cancel_tick = true;
                break;
            case PACKETTYPE_DATA_REQUEST:
                network->data_size = in_packet.packet.data_request.data_size;
                network->data_offset = malloc(network->data_size + 512);
                printf("sglthing: dl: packet request, size: %i bytes\n", network->data_size);
                break;
            default:
                //printf("sglthing: dl: unknown packet id %i\n", in_packet.meta.packet_type);
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

    network->mode = NETWORKMODE_CLIENT; 
    struct sockaddr_in dest;
    network->client.socket = socket(AF_INET, SOCK_STREAM, 0);  
    ASSERT(network->client.socket != -1);
    
    memset(&dest, 0, sizeof(struct sockaddr_in));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &dest.sin_addr);   
    dest.sin_port = htons(port);
    network->network_frames = 0;
    strncpy(network->debugger_pass, "debugger", 64);

    printf("sglthing: connecting to %s:%i...\n", ip, port);
    
    if(connect(network->client.socket, (struct sockaddr*)&dest, sizeof(struct sockaddr)) == -1)
    {    
        printf("sglthing: connect() returned -1, failed %i (%s)\n", errno, strerror(errno));
        network->status = NETWORKSTATUS_DISCONNECTED;
    }
    else
    {
        printf("sglthing: connection established\n");
        struct network_packet client_info;
        ZERO(client_info);
        network->status = NETWORKSTATUS_CONNECTED;
        network->client.connected = true;
        client_info.meta.packet_type = PACKETTYPE_CLIENTINFO;
        client_info.packet.clientinfo.sglthing_revision = GIT_COMMIT_COUNT;
        strncpy(client_info.packet.clientinfo.client_name, config_string_get(&network->http_client.web_config, "user_username"),64);
        strncpy(client_info.packet.clientinfo.session_key, network->http_client.sessionkey, 256);
        strncpy(client_info.packet.clientinfo.server_pass, network->server_pass, 64);
        strncpy(client_info.packet.clientinfo.debugger_pass, network->debugger_pass, 64);
        client_info.packet.clientinfo.color_r = network->client.player_color_r;
        client_info.packet.clientinfo.color_g = network->client.player_color_g;
        client_info.packet.clientinfo.color_b = network->client.player_color_b;
        network_transmit_packet(network, &network->client, client_info);

        if(network->script)
            nets7_init_network(network_get_s7_pointer(network), network);

        network->next_tick = glfwGetTime() + 0.01;
    }
}

void network_open(struct network* network, char* ip, int port)
{
    network->http_client.server = true;
    http_create(&network->http_client, SGLAPI_BASE);

    struct sockaddr_in serv;
    network->client.socket = socket(AF_INET, SOCK_NONBLOCK | SOCK_STREAM, 0);
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

    if(listen(network->client.socket, 5) == -1)
    {
        printf("sglthing: listen() failed, %i\n", errno);
        return;
    }

    network->server_clients = g_array_new(false, true, sizeof(struct network_client));
    network->status = NETWORKSTATUS_CONNECTED;
    network->next_tick = glfwGetTime() + 0.01;
    network->mode = NETWORKMODE_SERVER;
    network->network_frames = 0;
    network->shutdown_ready = false;
    network->client_default_tick = 0.01;
    strncpy(network->server_name, "SGLThing game server", 64);
    strncpy(network->server_motd, "Server has not set a server_motd", 128);
    strncpy(network->debugger_pass, "debugger", 64);

    if(network->script)
        nets7_init_network(network_get_s7_pointer(network), network);

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

int network_transmit_packet(struct network* network, struct network_client* client, struct network_packet packet)
{
    packet.meta.distributed_time = network->distributed_time;
    packet.meta.packet_version = CR_PACKET_VERSION;
    packet.meta.magic_number = MAGIC_NUMBER;
    packet.meta.network_frames = network->network_frames;
    if(!client->connected)
        return -1;
    if(g_random_double_range(-1.0, 1.0) > 0.9)
        return -1;
    return send(client->socket, &packet, sizeof(struct network_packet), MSG_DONTWAIT | MSG_NOSIGNAL);
}

void network_transmit_packet_all(struct network* network, struct network_packet packet)
{
    for(int i = 0; i < network->server_clients->len; i++)
    {
        struct network_client *cli = &g_array_index(network->server_clients, struct network_client, i);
        network_transmit_packet(network, cli, packet);
    }
}

int last_given_player_id = 0;

void network_manage_socket(struct network* network, struct network_client* client)
{
    struct network_packet in_packet;
    if(network->script)
        nets7_tick_network(network_get_s7_pointer(network), network, client);
    while(recv(client->socket, &in_packet, sizeof(struct network_packet), MSG_DONTWAIT) != -1)
    {
        if(in_packet.meta.magic_number != MAGIC_NUMBER)
            return;
        else if(in_packet.meta.packet_version != CR_PACKET_VERSION)
            return;
        bool passthru = true;
        if(network->receive_packet_callback)
            passthru = network->receive_packet_callback(network, client, &in_packet);
        if(!passthru)
            continue;
        switch(in_packet.meta.packet_type)
        {
            case PACKETTYPE_DISCONNECT:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    network_disconnect_player(network, false, in_packet.packet.disconnect.disconnect_reason, client);
                }
                else
                {
                    printf("sglthing: client '%s' disconnected '%s'\n", client->client_name, in_packet.packet.disconnect.disconnect_reason);
                    network_disconnect_player(network, true, in_packet.packet.disconnect.disconnect_reason, client);
                }
                break;
            case PACKETTYPE_PING:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_type = PACKETTYPE_PING;
                    response.packet.ping.distributed_time = network->distributed_time;
                    network_transmit_packet(network, client, response);

                    network->distributed_time = in_packet.packet.ping.distributed_time;
                }
                else
                {
                    client->last_ping_time = network->distributed_time;
                    client->lag = network->distributed_time - in_packet.packet.ping.distributed_time;
                }
                break;
            case PACKETTYPE_CLIENTINFO:
                if(network->mode == NETWORKMODE_SERVER && !client->authenticated)
                {
                    if(strlen(network->server_pass) == 0 || strncmp(in_packet.packet.clientinfo.server_pass, network->server_pass, 64) == 0)
                    {
                        client->verified = true;
                        if(!http_check_sessionkey(&network->http_client,in_packet.packet.clientinfo.session_key))
                        {
                            printf("sglthing: client '%s' failed key\n", in_packet.packet.clientinfo.client_name);
                            if(network->security)
                                network_disconnect_player(network, true, "Bad key", client);
                            client->verified = false;

                            client->player_color_r = 0.0;
                            client->player_color_g = 1.0;
                            client->player_color_b = 1.0;
                        }
                        else
                        {
                            char url[256];
                            snprintf(url,256,"auth/key_owner?sessionkey=%s",in_packet.packet.clientinfo.session_key);
                            char* username = http_get(&network->http_client,url);
                            if(username)
                            {
                                strncpy(in_packet.packet.clientinfo.client_name, username, 64);
                                free(username);
                            }

                            client->player_color_r = in_packet.packet.clientinfo.color_r;
                            client->player_color_g = in_packet.packet.clientinfo.color_g;
                            client->player_color_b = in_packet.packet.clientinfo.color_b;
                        }

                        if(!in_packet.packet.clientinfo.observer && strlen(network->debugger_pass) && strncmp(in_packet.packet.clientinfo.debugger_pass, network->debugger_pass, 64) == 0)
                        {
                            printf("sglthing: client '%s' used debugger password\n", in_packet.packet.clientinfo.client_name);
                            client->debugger = true;
                        }

                        strncpy(client->client_name, in_packet.packet.clientinfo.client_name, 64);
                        client->authenticated = true;
                        client->player_id = last_given_player_id++;
                        client->client_version = in_packet.packet.clientinfo.sglthing_revision;
                        client->owner = network;
                        if(client->client_version != GIT_COMMIT_COUNT)
                            printf("sglthing: WARN: new client '%s' is on sglthing r%i, while server is on sglthing r%i\n", in_packet.packet.clientinfo.client_name, client->client_version, GIT_COMMIT_COUNT);

                        printf("sglthing: client '%s' authenticated\n", in_packet.packet.clientinfo.client_name);

                        struct network_packet response;
                        ZERO(response);

                        if(!in_packet.packet.clientinfo.observer)
                        {
                            g_hash_table_insert(network->players, &client->player_id, client);

                            response.meta.packet_type = PACKETTYPE_PLAYER_ADD;
                            response.packet.player_add.player_id = client->player_id;
                            response.packet.player_add.admin = client->debugger;
                            response.packet.player_add.player_color_r = client->player_color_r;
                            response.packet.player_add.player_color_g = client->player_color_g;
                            response.packet.player_add.player_color_b = client->player_color_b;
                            strncpy(response.packet.player_add.client_name, in_packet.packet.clientinfo.client_name, 64);

                            network_transmit_packet_all(network, response);
                            ZERO(response);
                        }

                        response.meta.packet_type = PACKETTYPE_SERVERINFO;
                        response.packet.serverinfo.player_id = response.packet.player_add.player_id;
                        response.packet.serverinfo.sglthing_revision = GIT_COMMIT_COUNT;
                        response.packet.serverinfo.player_color_r = client->player_color_r;
                        response.packet.serverinfo.player_color_g = client->player_color_g;
                        response.packet.serverinfo.player_color_b = client->player_color_b;
                        memcpy(response.packet.serverinfo.session_key, network->http_client.sessionkey, 256);
                        strncpy(response.packet.serverinfo.server_motd,"I am a vey glad to meta you",128);
                        strncpy(response.packet.serverinfo.server_name,"SGLThing Server",64);
                        network_transmit_packet(network, client, response);                            

                        client->observer = in_packet.packet.clientinfo.observer;            
                        client->ping_time_interval = network->client_default_tick;

                        if(network->new_player_callback)
                            network->new_player_callback(network, client);
                    }
                    else
                    {
                        printf("sglthing: client '%s' failed password (%s)\n", in_packet.packet.clientinfo.client_name, in_packet.packet.clientinfo.server_pass);
                        network_disconnect_player(network, true, "Bad password", client);
                    }
                }
                break;
            case PACKETTYPE_SERVERINFO:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    printf("sglthing: authenticated to server %s, motd: %s\n", in_packet.packet.serverinfo.server_name, in_packet.packet.serverinfo.server_motd);
                    network->client.player_id = in_packet.packet.serverinfo.player_id;
                    strncpy(network->server_name, in_packet.packet.serverinfo.server_name, 64);
                    strncpy(network->server_motd, in_packet.packet.serverinfo.server_name, 128);
                    network->client.client_version = in_packet.packet.serverinfo.sglthing_revision;
                    network->client.authenticated = true;
                }
                break;
            case PACKETTYPE_PLAYER_ADD:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    struct network_client* new_client = (struct network_client*)malloc(sizeof(struct network_client));
                    printf("sglthing: client new player %s %i\n", in_packet.packet.player_add.client_name, in_packet.packet.player_add.player_id);
                    // copy

                    new_client->authenticated = true;
                    new_client->connected = true;
                    new_client->player_color_r = in_packet.packet.player_add.player_color_r;
                    new_client->player_color_g = in_packet.packet.player_add.player_color_g;
                    new_client->player_color_b = in_packet.packet.player_add.player_color_b;
                    new_client->player_id = in_packet.packet.player_add.player_id;
                    new_client->owner = network;
                    strncpy(new_client->client_name, in_packet.packet.player_add.client_name, 64);

                    g_hash_table_insert(network->players, &in_packet.packet.player_add.player_id, new_client);        

                    if(network->new_player_callback)
                        network->new_player_callback(network, new_client);        
                }
                break;
            case PACKETTYPE_PLAYER_REMOVE:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    printf("sglthing: client del player %i\n", in_packet.packet.player_remove.player_id);
                    struct network_client* old_client = g_hash_table_lookup(network->players, &in_packet.packet.player_remove.player_id);
                    if(network->del_player_callback)
                        network->del_player_callback(network, client);
                    g_hash_table_remove(network->players, &in_packet.packet.player_remove.player_id);
                    free(old_client);
                    if(in_packet.packet.player_remove.player_id == client->player_id)
                    {
                        client->connected = false;
                        network->status = NETWORKSTATUS_DISCONNECTED;
                        printf("sglthing: we ought to be out of here!\n");
                        network_disconnect_player(network, false, "disconnected", client);
                        return;
                    }
                }
                break;
            case PACKETTYPE_SCM_EVENT:
                // printf("sglthing: %s received event %i, data: '%s'\n", (network->mode == NETWORKMODE_SERVER) ? "server" : "client", in_packet.packet.scm_event.event_id, in_packet.packet.scm_event.event_data);
                if(network->script)
                    nets7_receive_event(network_get_s7_pointer(network), network, client, &in_packet);
                break;
            case PACKETTYPE_CHAT_MESSAGE:
                if(network->mode == NETWORKMODE_SERVER)
                {
                    in_packet.packet.chat_message.verified = client->verified;
                    strncpy(in_packet.packet.chat_message.client_name,client->client_name,64);
                    network_transmit_packet_all(network, in_packet);
                }
                else
                {
                    printf("sglthing '%s: %s'", in_packet.packet.chat_message.client_name, in_packet.packet.chat_message.message);
                }
                break;   
            case PACKETTYPE_DATA_REQUEST:
                if(network->mode == NETWORKMODE_SERVER)
                {
                    if(!client->observer)
                        return;
                    printf("sglthing: client requested %s\n", in_packet.packet.data_request.data_name);

                    if(strncmp(in_packet.packet.data_request.data_name,"game/data.tar",64)==0)
                    {
                        #define TMP_DATA_SIZE 26666666
                        char* mdata = malloc(TMP_DATA_SIZE);
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
                        network_transmit_packet(network, client, response);
                        printf("sglthing: transmitting data\n");
                        network_transmit_data(network, client, mdata, TMP_DATA_SIZE);
                    }
                }
                break;      
            default:            
                printf("sglthing: %s unknown packet %i\n", network->mode?"true":"false", (int)in_packet.meta.packet_type);
                break;
        }
    }
    //if(errno)
    //    printf("sglthing: send() %s\n", strerror(errno));
}

int last_connection_id = 0;

void network_frame(struct network* network, float delta_time)
{
    if(!network)
        return;
    if(network->status != NETWORKSTATUS_CONNECTED)
        return;
    //if(glfwGetTime() > network->next_tick)
    //    return;

    network->network_frames++;

    if(network->mode == NETWORKMODE_SERVER)
    {
        network->distributed_time = glfwGetTime();
        if(network->server_clients->len == 0 && network->shutdown_ready && network->shutdown_empty)
        {
            printf("sglthing: server shutting down because it is empty\n");
            network_close(network);
            return;
        }
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
            network_manage_socket(network, cli);

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
                    response.packet.ping.distributed_time = network->distributed_time;
                    response.packet.ping.player_lag = cli->lag;
                    network_transmit_packet(network, cli, response);
                    cli->next_ping_time = glfwGetTime() + cli->ping_time_interval;
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
                                int ret = network_transmit_packet(network, cli, data_packet);  
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
                                free(cli->dl_data);
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
            g_array_append_val(network->server_clients, new_client);

            network->shutdown_ready = true;
        }
        else
        {
            if(errno != EAGAIN)
                printf("sglthing: accept() errno == %i %s\n", errno, strerror(errno));
        }
    }
    else
    {
        network_manage_socket(network, &network->client);
    }
}

void network_disconnect_player(struct network* network, bool transmit_disconnect, char* reason, struct network_client* client)
{
    if(network->mode == NETWORKMODE_SERVER)
    {
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* v = &g_array_index(network->server_clients,struct network_client,i);
            if(client->connection_id == v->connection_id)
            {
                g_array_remove_index(network->server_clients, i);
                break;
            }
        }
        if(network->del_player_callback)
            network->del_player_callback(network, client);
        g_hash_table_remove(network->players, &client->player_id);
        if(client->authenticated && transmit_disconnect)
        {
            struct network_packet response;
            ZERO(response);
            response.meta.packet_type = PACKETTYPE_PLAYER_REMOVE;
            response.packet.player_remove.player_id = client->player_id;
            network_transmit_packet_all(network, response);
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
            network_transmit_packet(network, client, response);
        }
        printf("sglthing: network disconnected '%s'\n", reason);
        strncpy(network->disconnect_reason, reason, 32);
        network->status = NETWORKSTATUS_DISCONNECTED;
    }
    client->connected = false;
    close(client->socket);
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
    if(network->mode == NETWORKMODE_SERVER)
    {
        pos_base[0] = 0.f;
        pos_base[1] = 64.f;
        snprintf(tx,256,"SERVER, %i clients connected, frame %i", network->server_clients->len, network->network_frames);
        ui_draw_text(ui,pos_base[0],pos_base[1]-16.f,tx,1.f);
        snprintf(tx,256,"dT=%f", network->distributed_time);
        ui_draw_text(ui,pos_base[0],pos_base[1],tx,1.f);
        vec4 oldfg, oldbg;
        glm_vec4_copy(ui->foreground_color, oldfg);
        glm_vec4_copy(ui->background_color, oldbg);
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
            snprintf(tx,256,"Client '%s' %i %i %i, ping %fs, dtd %fs, %c%c%c%c%c", cli->client_name, i, cli->player_id, cli->connection_id, cli->lag, network->distributed_time - cli->last_ping_time,
                cli->authenticated?'A':'-',cli->connected?'C':'-',cli->debugger?'D':'-',cli->observer?'O':'-',cli->verified?'V':'-');
            ui_draw_text(ui,pos_base[0],pos_base[1]+((1+i)*16.f),tx,1.f);
        }
        glm_vec4_copy(oldfg, ui->foreground_color);
        glm_vec4_copy(oldbg, ui->background_color);
    }
    else
    {
        pos_base[0] = 512.f;
        pos_base[1] = 64.f;
        ui_draw_text(ui,pos_base[0],pos_base[1]-16.f,"CLIENT",1.f);
        snprintf(tx,256,"dT=%f", network->distributed_time);
        ui_draw_text(ui,pos_base[0],pos_base[1],tx,1.f);
        if(network->status == NETWORKSTATUS_DISCONNECTED)
            ui_draw_text(ui,pos_base[0],pos_base[1]+16.f,network->disconnect_reason,1.f);

        if(network->client.authenticated && GIT_COMMIT_COUNT != network->client.client_version && (int)glfwGetTime() % 2 == 0)
        {
            ui->foreground_color[2] = 0.f;
            snprintf(tx,256,"WARNING: Client (sglthing r%i) and server (sglthing r%i) versions dont match",GIT_COMMIT_COUNT,network->client.client_version);
            ui_draw_text(ui, 0, 0, tx, 1.f);
            ui->foreground_color[2] = 1.f;
        }
    }
}
#endif