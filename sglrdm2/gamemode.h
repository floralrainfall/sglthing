#ifndef GAMEMODE_H
#define GAMEMODE_H

#include <stdbool.h>
#include <sglthing/world.h>

enum rdm_gamemode
{
    GAMEMODE_FREEFORALL,
    GAMEMODE_SECRET,
    GAMEMODE_EXTENDED,

    __MAX_GAMEMODE,
};

enum rdm_team
{
    TEAM_NEUTRAL,
    TEAM_RED,
    TEAM_BLUE,
    TEAM_NA,
    TEAM_SPECTATOR,

    __TEAM_MAX,
};

struct gamemode_data
{
    enum rdm_gamemode gamemode;
    double gamemode_end;
    double gamemode_nextannouncement;
    bool secret;
    bool started;
    bool server;
    float network_time;

    enum rdm_team last_team;
};

void gamemode_init(struct gamemode_data* data, bool server);
void gamemode_frame(struct gamemode_data* data, struct world* world);
char* gamemode_name(enum rdm_gamemode gamemode);
// i hate c includes
void gamemode_player_add(struct gamemode_data* data, void* player);

#endif