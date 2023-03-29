#include "script_networking.h"

static void __net_send_event(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "net-send-event", 1, s7_car(args), "network"));
    struct network* network = (struct network*)s7_c_pointer(s7_car(args));

    if(!s7_is_c_pointer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "net-send-event", 2, s7_cadr(args), "client"));
    struct network_client* client = (struct network_client*)s7_c_pointer(s7_cadr(args));

    if(!s7_is_string(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "net-send-event", 3, s7_caddr(args), "data"));
    const char* data = s7_string(s7_caddr(args));

    if(!s7_is_integer(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "net-send-event", 4, s7_cadddr(args), "event id"));
    int eventid = s7_integer(s7_cadddr(args));

    struct network_packet packet;
    packet.meta.packet_type = PACKETTYPE_SCM_EVENT;
    packet.packet.scm_event.event_id = eventid;
    strncpy(packet.packet.scm_event.event_data, data, 512);

    network_transmit_packet(network, client, packet);
}

void sgls7_registernetworking(s7_scheme* sc)
{
    s7_define_function(sc, "net-send-event", __net_send_event, 4, 0, false, "(net-send-event n c e t)");
}

void nets7_init_network(s7_scheme* sc, struct network* net)
{
    char function_name[64];
    snprintf(function_name, 64, "%s-init", ((net->mode == NETWORKMODE_SERVER) ? "server" : "client"));
    s7_call(sc, s7_name_to_value(sc, function_name), s7_cons(sc, s7_make_c_pointer(sc, net), s7_nil(sc)));
}

void nets7_tick_network(s7_scheme* sc, struct network* net, struct network_client* client)
{
    char function_name[64];
    snprintf(function_name, 64, "%s-tick", ((net->mode == NETWORKMODE_SERVER) ? "server" : "client"));
    s7_call(sc, s7_name_to_value(sc, function_name), s7_cons(sc, s7_make_c_pointer(sc, net), s7_cons(sc, s7_make_c_pointer(sc, client), s7_nil(sc))));
}

void nets7_receive_event(s7_scheme* sc, struct network* net, struct network_client* client, struct network_packet* packet)
{
    char function_name[64];
    snprintf(function_name, 64, "%s-event-%i", ((net->mode == NETWORKMODE_SERVER) ? "server" : "client"), packet->packet.scm_event.event_id);
    s7_call(sc, s7_name_to_value(sc, function_name), s7_cons(sc, s7_make_c_pointer(sc, net), s7_cons(sc, s7_make_c_pointer(sc, client), s7_cons(sc, s7_make_string_with_length(sc, packet->packet.scm_event.event_data, 512), s7_nil(sc)))));
}