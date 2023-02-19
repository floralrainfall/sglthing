#include "map.h"
#include "model.h"
#include "world.h"
#include "sglthing.h"
#include "texture.h"
#include <ode/ode.h>
#include <cglm/cglm.h>
#include <GLFW/glfw3.h>

#define RAND_TEXTURES 4

void new_map(struct world* world, struct map* map)
{
    srand(glfwGetTime());

    for(int i = 0; i < MAP_MESHES; i++)
    {
        char name[64];
        snprintf(name,64,"tile%i.obj",i);
        load_model(name);
        map->map_meshes[i] = get_model(name);
        if(!map->map_meshes[i])
            map->map_meshes[i] = get_model("test.obj");
    }

    int rand_textures[RAND_TEXTURES];
    for(int i = 0; i < RAND_TEXTURES; i++)
    {
        char name[64];
        snprintf(name,64,"tile_t%i.png",i);
        load_texture(name);
        rand_textures[i] = get_texture(name);
    }

    for(int i = 0; i < MAP_SIZE; i++)
    {
        for(int j = 0; j < MAP_SIZE; j++)
        {
            map->map_data[i][j] = rand()%MAP_MESHES;
            // create collision shape
            dGeomID floor_geom = dCreateBox(world->physics.space,32.0,56.0,32.0);
            //dBodyID tile_body = dBodyCreate(world->physics.world);
            //dBodyID mesh_body = dBodyCreate(world->physics.world);

            //dGeomSetBody(floor_geom, tile_body);
            dGeomSetPosition(floor_geom, i*32.0,-0.5 - (56.0/2.0),j*32.0);

            switch(map->map_data[i][j])
            {
                case 1:
                    dGeomID building_geom = dCreateBox(world->physics.space,32.0,56.0,32.0);
                    dGeomSetPosition(building_geom, i*32.0,28.0,j*32.0);
                    //dGeomSetBody(building_geom, tile_body);
                    break;
                case 4:
                    dGeomID cone_geom = dCreateBox(world->physics.space,1.0,1.0,1.0);
                    dGeomSetPosition(cone_geom, i*32.0,0.0,j*32.0);
                    //dGeomSetBody(cone_geom, tile_body);
                    break;
            }

            map->map_textures[i][j] = rand_textures[rand()%RAND_TEXTURES];
        }
    }
    map->map_data[0][0] = 2;
}

void draw_map(struct world* world, struct map* map, int shader)
{
    mat4 model_matrix;
    char txbuf[64];
    int drawn_frame = 0;
    for(int i = 0; i < MAP_SIZE; i++)
    {
        for(int j = 0; j < MAP_SIZE; j++)
        {
            vec3 position = {i*32.f,0.f,j*32.f};
            float distance = glm_vec3_distance(world->cam.position, position);
            if(distance < world->gfx.fog_maxdist + 32.f)
            {
                drawn_frame++;
                vec3 direction;
                vec3 mm_position;
                glm_vec3_sub(world->cam.position,position,direction);
                float angle = glm_vec3_dot(world->cam.front, direction) / M_PI_180f;
                if(angle < world->cam.fov || distance < world->gfx.fog_mindist)
                {
                    int tile = map->map_data[i][j];
                    int texture = map->map_textures[i][j];
                    glm_mat4_identity(model_matrix);
                    glm_translate(model_matrix,position);
                    glm_rotate_y(model_matrix,M_PIf*(0.50*(i*j)),model_matrix);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture);
                    world_draw_model(world,map->map_meshes[tile],shader,model_matrix,false);
                    snprintf(txbuf,64,"x=%i, y=%i, type=%i, %.2fu away", i, j, tile, distance);
                    ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,10.f,0.f}, world->cam.fov, model_matrix, world->vp, txbuf);
                }
            }
        }
    }
}