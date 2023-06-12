#include "guy.h"

void guy_render(struct world* world, guy_t guy, struct model* guy_mdl, int guy_prg, struct civ_info* civs)
{
    mat4 guy_mtx;
    glm_mat4_identity(guy_mtx);
    vec3 guy_pos = {
        ((float)guy.position[0])/100.0,
        0.0,
        ((float)guy.position[1])/100.0,
    };
    glm_translate(guy_mtx,guy_pos);
    world_draw_model(world, guy_mdl, guy_prg, guy_mtx, true);
    if(world->gfx.shadow_pass == false)
    {
        char dbginfo[64];
        snprintf(dbginfo,64,"%i",guy.state);
        mat4 txt_mtx;
        glm_mat4_identity(txt_mtx);
        ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, guy_pos, world->cam.fov, txt_mtx, world->vp, dbginfo);
    }
}

void guy_ai_tick(guy_t* guy, struct civ_info* civs, GHashTable* building_table, float delta_time)
{
    struct civ_info* guy_civ = &civs[guy->civ_id];
    switch(guy->state)
    {
        case GUY_STATE_IDLE: // decide next state here
            if(guy_civ->current_building_id != -1)
            {
                printf("%i\n",guy_civ->current_building_id);
                struct building* building = g_hash_table_lookup(building_table, &guy_civ->current_building_id);
                if(building)
                {
                    if(glm_vec2_distance2(guy->position, building->position) < 1)
                    {
                        guy->wanted_position[0] = building->position[0];
                        guy->wanted_position[1] = building->position[1];
                        guy->state = GUY_STATE_MOVING;
                    }
                    else
                    {
                        guy->state = GUY_STATE_ASSIST_BUILD;                    
                    }
                }
            }
            else
            {
                guy->wanted_position[0] = guy->position[0] + g_random_int_range(-1000, 1000);
                guy->wanted_position[1] = guy->position[1] + g_random_int_range(-1000, 1000);
                guy->state = GUY_STATE_MOVING;  
            }
            break;
        case GUY_STATE_MOVING:
            guy->position[0] += roundf(guy->wanted_position[0] - guy->position[0] * delta_time);
            guy->position[1] += roundf(guy->wanted_position[1] - guy->position[1] * delta_time);
            if(glm_vec2_distance2(guy->position, guy->wanted_position) < 25)
            {
                guy->state = GUY_STATE_IDLE;
            }
            break;
        case GUY_STATE_ASSIST_BUILD:
            if(guy_civ->current_building_id != -1)
            {                
                struct building* building = g_hash_table_lookup(building_table, &guy_civ->current_building_id);
                if(building->building_inprogress)
                {
                    building->building_progress_points++;
                    if(building->building_progress_points >= building->building_progress_points_max)
                    {
                        building->building_inprogress = false;
                        guy_civ->current_building_id = -1;
                        guy->state = GUY_STATE_IDLE;
                    }
                }
                else
                    guy->state = GUY_STATE_IDLE;
            }
            else
                guy->state = GUY_STATE_IDLE;
            break;
    }
}

static int last_guy_id = 0;

guy_t* guy_new(GHashTable* guy_table, guy_t guy, struct civ_info* civs)
{
    struct civ_info* guy_civ = &civs[guy.civ_id];
    guy_t* guy_ref = (guy_t*)malloc2(sizeof(guy_t));
    memcpy(guy_ref, &guy, sizeof(guy_t));
    int guy_id = last_guy_id++;
    g_hash_table_insert(guy_table, &guy_id, guy_ref);

    guy_ref->wanted_position[0] = 0;
    guy_ref->wanted_position[1] = 0;
    guy_ref->guy_id = guy_id;
    guy_ref->state = GUY_STATE_IDLE;
    guy_civ->civ_population++;

    return guy_ref;
}