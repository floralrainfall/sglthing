#ifndef WEAPONS_H
#define WEAPONS_H

enum weapon_type
{
    WEAPON_HOLSTER,
    WEAPON_CROWBAR,
    WEAPON_PISTOL,
    WEAPON_SHOTGUN,
    WEAPON_AK47,
};

void weapon_fire_primary();
void weapon_fire_secondary();

#endif