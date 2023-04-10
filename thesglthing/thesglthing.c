#include <sglthing/world.h>
#include <stdio.h>

void test(struct world* a)
{

}

void sglthing_init_api(struct world* a)
{
    a->world_frame_user = test;

    // lol

    a->cam.position[1] = 2.f;
    a->cam.position[2] = -2.f;
}