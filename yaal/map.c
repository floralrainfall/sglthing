#include "map.h"

bool determine_tile_collision(struct map_tile_data tile)
{
    if(tile.tile_graphics_ext_id)
        return true;
    switch(tile.map_tile_type)
    {
        case TILE_AIR:
        case TILE_WALL:
        case TILE_WALL_CHEAP:
        case TILE_WATER:
            return true;
        default:
        case TILE_WALKAIR:
        case TILE_FLOOR:
        case TILE_DEBUG:
            return false;
    }
}

void map_render(struct world* world, struct map_tile_data map[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y])
{
    for(int map_x = 0; map_x < MAP_SIZE_MAX_X; map_x++)
    {
        for(int map_y = 0; map_y < MAP_SIZE_MAX_Y; map_y++)
        {
            struct map_tile_data* map_tile = &map[map_x][map_y];
            if(map_tile->map_tile_type == TILE_AIR || map_tile->map_tile_type == TILE_WALKAIR || !yaal_state.current_player)
            {

            }
            else
            {
                vec3 map_pos = { map_x * MAP_TILE_SIZE, 0.f, map_y * MAP_TILE_SIZE };
                float dist = glm_vec3_distance(map_pos, yaal_state.current_player->player_position);
                float distx = fabsf(map_pos[0] - yaal_state.current_player->player_position[0]);
                float disty = fabsf(map_pos[2] - yaal_state.current_player->player_position[2]);
                if(distx > 20.f)
                    continue;
                if(disty > 20.f)
                    continue;
                vec3 direction;
                glm_vec3_sub(map_pos,world->cam.position,direction);
                float angle = glm_vec3_dot(direction, world->cam.front);
                //printf("%f %f\n", angle, world->cam.fov);
                if(angle < world->cam.fov * M_PI_180f)
                    continue;
                mat4 model_matrix;
                glm_mat4_identity(model_matrix);
                glm_translate(model_matrix, map_pos);
                switch(map_tile->map_tile_type)
                {
                case TILE_WALL_CHEAP:
                    glm_translate_y(model_matrix, 4.f);
                    glm_scale(model_matrix, (vec3){1.f, 4.f, 1.f});
                    break;
                case TILE_WATER:
                    glm_translate_y(model_matrix, sinf(glfwGetTime()+map_x+map_y)*0.5f);
                    break;
                default:
                    break;
                }
                
                /*if(yaal_state.current_player)
                    if(map_x == yaal_state.current_player->map_tile_x && map_y == yaal_state.current_player->map_tile_y)
                        {
                            glm_translate_y(model_matrix, 1.f);
                        }*/
                // glm_rotate(model_matrix, (map_tile->direction * 90.f) * (180 / M_PIf), (vec3){0.f,1.f,0.f});
                struct model* mdl = yaal_state.map_tiles[map_tile->tile_graphics_id];
                bool has_tex = map_tile->tile_graphics_tex;
                int tex_id = yaal_state.map_graphics_tiles[map_tile->tile_graphics_tex];
                if(map_tile->map_tile_type == TILE_DEBUG)   
                {
                    mdl = yaal_state.map_tiles[1];
                    tex_id = 0;
                    has_tex = true;
                }
                if(mdl)
                    if(has_tex)
                    {
                        if(tex_id < MAP_TEXTURE_IDS)
                            tex_id = 0;
                        if(tex_id == 0)
                            tex_id = yaal_state.map_graphics_tiles[0];
                        sglc(glUseProgram(yaal_state.object_shader));
                        sglc(glActiveTexture(GL_TEXTURE0));
                        sglc(glBindTexture(GL_TEXTURE_2D, tex_id));                
                        sglc(glUniform1i(glGetUniformLocation(yaal_state.object_shader,"diffuse0"), 0));
                        sglc(glUniform4f(glGetUniformLocation(yaal_state.object_shader,"color"), 0.5, 0.5, 0.5, 1.0));
                        world_draw_model(world, mdl, yaal_state.object_shader, model_matrix, false);
                    }
                    else
                        world_draw_model(world, mdl, yaal_state.object_shader, model_matrix, true);
                struct model* mdl2 = yaal_state.map_tiles[map_tile->tile_graphics_ext_id];
                if(mdl2 && dist < 32.f)
                    world_draw_model(world, mdl2, yaal_state.object_shader, model_matrix, true);
            }
        }
    }    
}