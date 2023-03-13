#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#ifdef SGLTHING_COMPILE
#include <GLFW/glfw3.h>
#endif
#include "sglthing.h"

#define ZERO(x) memset(&x,0,sizeof(x))

void network_connect(struct network* network, char* ip, int port)
{
    http_create(&network->http_client, "https://sgl.endoh.ca/game");

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
        strncpy(client_info.packet.clientinfo.client_name, config_string_get(&network->http_client.web_config, "user_username"),64);
        strncpy(client_info.packet.clientinfo.session_key, network->http_client.sessionkey, 256);
        network_transmit_packet(network, &network->client, client_info);

        network->next_tick = glfwGetTime() + 0.01;
    }
}

void network_open(struct network* network, char* ip, int port)
{
    http_create(&network->http_client, "https://sgl.endoh.ca/game");

    struct sockaddr_in serv;
    network->client.socket = socket(AF_INET, SOCK_NONBLOCK | SOCK_STREAM, 0);
    ASSERT(network->client.socket != -1);

    memset(&serv, 0, sizeof(struct sockaddr_in));
    serv.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serv.sin_addr);   
    serv.sin_port = htons(port);

    if(bind(network->client.socket,(struct sockaddr*)&serv,sizeof(struct sockaddr)) == -1)
    {
        printf("sglthing: bind() failed, %i\n", errno);
    }

    network->server_clients = g_array_new(false, true, sizeof(struct network_client));
    network->status = NETWORKSTATUS_CONNECTED;
    network->next_tick = glfwGetTime() + 0.01;
    network->mode = NETWORKMODE_SERVER;
    network->network_frames = 0;
    strncpy(network->server_name, "SGLThing game server", 64);
    strncpy(network->server_motd, "Server has not set a server_motd", 128);
    strncpy(network->debugger_pass, "debugger", 64);

    if(listen(network->client.socket, 5) == -1)
    {
        printf("sglthing: listen() failed, %i\n", errno);
    }
    printf("sglthing: server open on ip %s and port %i\n", ip, port);
}

void network_transmit_packet(struct network* network, struct network_client* client, struct network_packet packet)
{
    packet.meta.distributed_time = network->distributed_time;
    packet.meta.packet_version = CR_PACKET_VERSION;
    packet.meta.magic_number = MAGIC_NUMBER;
    packet.meta.network_frames = network->network_frames;
    if(!client->connected)
        return;
    send(client->socket, &packet, sizeof(struct network_packet), 0);
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
    while(recv(client->socket, &in_packet, sizeof(struct network_packet), MSG_DONTWAIT) != -1)
    {
        if(in_packet.meta.magic_number != MAGIC_NUMBER)
            return;
        else if(in_packet.meta.packet_version != CR_PACKET_VERSION)
            return;
        switch(in_packet.meta.packet_type)
        {
            case PACKETTYPE_DISCONNECT:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    network_disconnect_player(network, false, in_packet.packet.disconnect.disconnect_reason, client);
                }
                else
                {
                    printf("sglthing: client '%s' disconnected\n", client->client_name);
                    network_disconnect_player(network, true, in_packet.packet.disconnect.disconnect_reason, client);
                }
                break;
            case PACKETTYPE_PING:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    network->distributed_time = in_packet.packet.ping.distributed_time;

                    struct network_packet response;
                    ZERO(response);
                    response.meta.packet_type = PACKETTYPE_PING;
                    response.packet.ping.distributed_time = glfwGetTime();
                    network_transmit_packet(network, client, response);
                }
                else
                {
                    client->last_ping_time = in_packet.packet.ping.distributed_time;
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
                            break;
                        }
                        else
                        {
                            char url[256];
                            snprintf(url,256,"auth/key_owner.sgl?sessionkey=%s",in_packet.packet.clientinfo.session_key);
                            char* username = http_get(&network->http_client,url);
                            if(username)
                            {
                                strncpy(in_packet.packet.clientinfo.client_name, username, 64);
                                free(username);
                            }
                        }

                        if(strlen(network->debugger_pass) && strncmp(in_packet.packet.clientinfo.debugger_pass, network->debugger_pass, 64) == 0)
                        {
                            client->debugger = true;
                        }

                        strncpy(client->client_name, in_packet.packet.clientinfo.client_name, 64);
                        client->authenticated = true;
                        client->player_id = last_given_player_id++;

                        struct network_packet response;
                        ZERO(response);
                        response.meta.packet_type = PACKETTYPE_PLAYER_ADD;
                        response.packet.player_add.player_id = client->player_id;
                        response.packet.player_add.admin = client->debugger;
                        strncpy(response.packet.player_add.client_name, in_packet.packet.clientinfo.client_name, 64);
                        network_transmit_packet_all(network, response);

                        response.meta.packet_type = PACKETTYPE_SERVERINFO;
                        response.packet.serverinfo.player_id = response.packet.player_add.player_id;
                        memcpy(response.packet.serverinfo.session_key, network->http_client.sessionkey, 256);
                        strncpy(response.packet.serverinfo.server_motd,"I am a vey glad to meta you",128);
                        strncpy(response.packet.serverinfo.server_name,"SGLThing Server",64);
                        network_transmit_packet(network, client, response);                            
                        printf("sglthing: client '%s' authenticated\n", in_packet.packet.clientinfo.client_name);

                        client->observer = in_packet.packet.clientinfo.observer;            
                        client->ping_time_interval = 0.01;

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
                }
                break;
            case PACKETTYPE_PLAYER_ADD:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    printf("sglthing: client new player %s %i\n", in_packet.packet.player_add.client_name, in_packet.packet.player_add.player_id);
                }
                break;
            case PACKETTYPE_PLAYER_REMOVE:
                if(network->mode == NETWORKMODE_CLIENT)
                {
                    printf("sglthing: client del player %i\n", in_packet.packet.player_remove.player_id);
                }
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
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
            network_manage_socket(network, cli);

            if(cli->connected)
            {
                if(network->distributed_time - cli->last_ping_time > 4.0)
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
            new_client.connection_id = last_connection_id;
            new_client.connected = true;
            new_client.last_ping_time = glfwGetTime() + 5.f;
            new_client.ping_time_interval = 5.0;
            g_array_append_val(network->server_clients, new_client);
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
        if(client->authenticated && transmit_disconnect)
        {
            struct network_packet response;
            ZERO(response);
            response.meta.packet_type = PACKETTYPE_PLAYER_REMOVE;
            response.packet.player_remove.player_id = client->player_id;
            network_transmit_packet_all(network, response);
        }
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* v = &g_array_index(network->server_clients,struct network_client,i);
            if(client->connection_id == v->connection_id)
            {
                g_array_remove_index(network->server_clients, i);
                break;
            }
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
    if(network->status != NETWORKSTATUS_CONNECTED)
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
        ui_draw_text(ui,pos_base[0],pos_base[1]-16.f,"SERVER",1.f);
        snprintf(tx,256,"dT=%f", network->distributed_time);
        ui_draw_text(ui,pos_base[0],pos_base[1],tx,1.f);
        for(int i = 0; i < network->server_clients->len; i++)
        {
            struct network_client* cli = &g_array_index(network->server_clients, struct network_client, i);
            snprintf(tx,256,"Client '%s' %i %i %i, lag %f, %c%c%c%c%c", cli->client_name, i, cli->player_id, cli->connection_id, cli->lag,
                cli->authenticated?'A':'-',cli->connected?'C':'-',cli->debugger?'D':'-',cli->observer?'O':'-',cli->verified?'V':'-');
            ui_draw_text(ui,pos_base[0],pos_base[1]+((1+i)*16.f),tx,1.f);
        }
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
    }
}
#endif