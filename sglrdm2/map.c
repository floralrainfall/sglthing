#include "map.h"
#include <sglthing/shader.h>
#include <sglthing/texture.h>
#include <sglthing/net.h>
#include <sglthing/sglthing.h>
#include "rdm_net.h"
#include "rdm2.h"

static int last_render_chunk_id;

struct __render_chunk_block
{
    vec3 position;
    char obscure;
    char blk_id;
    char air;
};

struct __render_chunk
{
    int chunk_x;
    int chunk_y;
    int chunk_z;

    int chunk_id;

    struct {
        struct {
            struct __render_chunk_block z[RENDER_CHUNK_SIZE];
        } y[RENDER_CHUNK_SIZE_Y];
    } x[RENDER_CHUNK_SIZE];

    struct graphic_bref bref;
};

static void __map_render_chunk_init(struct __render_chunk* chunk)
{
    sizeof(struct __render_chunk);
    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
        for(int y = 0; y < RENDER_CHUNK_SIZE_Y; y++)
            for(int z = 0; z < RENDER_CHUNK_SIZE; z++)
            {
                vec3 t_position;
                t_position[0] = x * CUBE_SIZE;
                t_position[1] = y * CUBE_SIZE;
                t_position[2] = z * CUBE_SIZE;
                
                glm_vec3_copy(t_position,chunk->x[x].y[y].z[z].position);

                chunk->x[x].y[y].z[z].obscure = 0;
                chunk->x[x].y[y].z[z].blk_id = 0;
            }
}

static void __map_render_chunk_update(struct __render_chunk* chunk)
{
    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
        for(int y = 0; y < RENDER_CHUNK_SIZE_Y; y++)
            for(int z = 0; z < RENDER_CHUNK_SIZE; z++)
            {                
                struct __render_chunk_block* block = &chunk->x[x].y[y].z[z];
                if(block->air)
                {
                    block->obscure = false;
                }
                else
                {
                    bool not_obscured = false;
                    
                    struct __render_chunk_block* test_block;
                    if(y != RENDER_CHUNK_SIZE_Y - 1)
                    {
                        test_block = &chunk->x[x].y[y+1].z[z];
                        if(test_block->air)
                        {
                            not_obscured = true;
                        }
                    }
                    else
                    {
                        not_obscured = true;
                    }
                    if(x != RENDER_CHUNK_SIZE - 1)
                    {
                        test_block = &chunk->x[x+1].y[y].z[z];
                        if(test_block->air)
                        {
                            not_obscured = true;
                        }
                    }
                    else
                    {
                        not_obscured = true;
                    }
                    if(z != RENDER_CHUNK_SIZE - 1)
                    {
                        test_block = &chunk->x[x].y[y].z[z+1];
                        if(test_block->air)
                        {
                            not_obscured = true;
                        }
                    }
                    else
                    {
                        not_obscured = true;
                    }

                    if(y != 0)
                    {
                        test_block = &chunk->x[x].y[y-1].z[z];
                        if(test_block->air)
                        {
                            not_obscured = true;
                        }
                    }
                    else
                    {
                        not_obscured = true;
                    }
                    if(x != 0)
                    {
                        test_block = &chunk->x[x-1].y[y].z[z];
                        if(test_block->air)
                        {
                            not_obscured = true;
                        }
                    }
                    else
                    {
                        not_obscured = true;
                    }
                    if(z != 0)
                    {
                        test_block = &chunk->x[x].y[y].z[z-1];
                        if(test_block->air)
                        {
                            not_obscured = true;
                        }
                    }
                    else
                    {
                        not_obscured = true;
                    }
                    
                    block->obscure = not_obscured;
                }
            }

    graphic_bref_upload(&chunk->bref, &chunk->x, sizeof(chunk->x));
}

static struct __render_chunk* __map_chunk_position(struct map_manager* map, int c_x, int c_y, int c_z)
{
    for(int i = 0; i < map->chunk_list->len; i++)
    {
        struct __render_chunk* chunk = g_array_index(map->chunk_list, struct __render_chunk*, i);
        if(chunk->chunk_x == c_x && chunk->chunk_y == c_y && chunk->chunk_z == c_z)
        {
            return chunk;
        }
    }
    return NULL;
}

void __map_render_chunk(struct world* world, struct map_manager* map, struct __render_chunk* chunk)
{
    int render_range = map->map_render_range;
    int p_c_x = floorf((world->cam.position[0]+0.5f)/(RENDER_CHUNK_SIZE * CUBE_SIZE));
    int p_c_y = floorf(world->cam.position[1]/(RENDER_CHUNK_SIZE_Y * CUBE_SIZE));
    int p_c_z = floorf((world->cam.position[2]+0.5f)/(RENDER_CHUNK_SIZE * CUBE_SIZE));

    if(chunk->chunk_x < p_c_x - render_range)
        return;
    if(chunk->chunk_x > p_c_x + render_range)
        return;

    if(chunk->chunk_y < p_c_y - render_range)
        return;
    if(chunk->chunk_y > p_c_y + render_range)
        return;

    if(chunk->chunk_z < p_c_z - render_range)
        return;
    if(chunk->chunk_z > p_c_z + render_range)
        return;

    vec3 chunk_position = {chunk->chunk_x*(RENDER_CHUNK_SIZE*CUBE_SIZE),chunk->chunk_y*(RENDER_CHUNK_SIZE_Y*CUBE_SIZE),chunk->chunk_z*(RENDER_CHUNK_SIZE*CUBE_SIZE)};

    mat4 matrix;
    glm_mat4_identity(matrix);
    glm_translate(matrix, chunk_position);

    int shader = world->gfx.shadow_pass ? map->cube_program_light : map->cube_program;

    sglc(glUseProgram(shader));

    //world_uniforms(world, map->cube_program, matrix);

    struct mesh* cube_mesh = &map->cube->meshes[0];
    graphic_varray_bind(&cube_mesh->vertex_array);
    graphic_bref_bind(&chunk->bref);

    sglc(glEnableVertexAttribArray(6));
    sglc(glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(struct __render_chunk_block), (void*)0)); // vec3: offset
    sglc(glVertexAttribDivisor(6, 1)); // had to update the entirety of sglthing to opengl 3.3 because of this...
    
    sglc(glEnableVertexAttribArray(7));
    sglc(glVertexAttribPointer(7, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct __render_chunk_block), (void*)(offsetof(struct __render_chunk_block,obscure)))); // char: obscure
    sglc(glVertexAttribDivisor(7, 1));  

    sglc(glEnableVertexAttribArray(8));
    sglc(glVertexAttribPointer(8, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct __render_chunk_block), (void*)(offsetof(struct __render_chunk_block,blk_id)))); // char[3]: color
    sglc(glVertexAttribDivisor(8, 1));  
    if(world->gfx.shadow_pass)
        sglc(glUniform1i(glGetUniformLocation(shader,"sel_map"), world->gfx.current_map));

    graphic_render_varray_instanced(&cube_mesh->vertex_array, shader, matrix, RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE_Y*RENDER_CHUNK_SIZE);
}

void map_render_chunks(struct world* world, struct map_manager* map)
{
    profiler_event("map_render_chunks");
    sglc(glActiveTexture(GL_TEXTURE0));
    sglc(glBindTexture(GL_TEXTURE_2D, map->map_texture_atlas));                
    for(int i = 0; i < map->chunk_list->len; i++)
        __map_render_chunk(world, map, g_array_index(map->chunk_list, struct __render_chunk*, i));
    profiler_end();
}

void map_update_chunks(struct map_manager* map, struct world* world)
{
    profiler_event("map_update_chunks");
    int request_range = map->map_request_range;
    int render_range = map->map_dealloc_range;

    int p_c_x = floorf((world->cam.position[0]+0.5f)/(RENDER_CHUNK_SIZE * CUBE_SIZE));
    int p_c_y = floorf(world->cam.position[1]/(RENDER_CHUNK_SIZE_Y * CUBE_SIZE));
    int p_c_z = floorf((world->cam.position[2]+0.5f)/(RENDER_CHUNK_SIZE * CUBE_SIZE));
    
    for(int i = 0; i < map->chunk_list->len; i++)
    {
        struct __render_chunk* chunk = g_array_index(map->chunk_list, struct __render_chunk*, i);

        bool pass = true;
        if(chunk->chunk_x < p_c_x - render_range)
            pass = false;
        if(chunk->chunk_x > p_c_x + render_range)
            pass = false;

        if(chunk->chunk_y < p_c_y - render_range)
            pass = false;
        if(chunk->chunk_y > p_c_y + render_range)
            pass = false;

        if(chunk->chunk_z < p_c_z - render_range)
            pass = false;
        if(chunk->chunk_z > p_c_z + render_range)
            pass = false;

        if(!pass)
        {
            graphic_bref_delete(&chunk->bref);
            free2(chunk);
            g_array_remove_index(map->chunk_list, i);
            i--;
        }
    }

    if(client_state.local_player && world->time > map->next_map_rq)
    {
        if(map->map_data_wanted == 0)
        {
            for(int x = -request_range-1; x <= request_range; x++)
            {
                if(p_c_x + x < 0)
                    continue;
                if(p_c_x + x > MAP_SIZE)
                    continue;
                for(int y = -request_range-1; y <= request_range; y++)
                {
                    if(p_c_z + y < 0)
                        continue;
                    if(p_c_z + y > MAP_SIZE)
                        continue;
                    struct __render_chunk* c = __map_chunk_position(map, p_c_x + x, 0, p_c_z + y);
                    if(!c)
                    {    
                        if(map->map_data_wanted > (RENDER_CHUNK_SIZE * map->chunk_limit))
                            break;

                        struct network_packet _pak;
                        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                        _pak.meta.packet_type = RDM_PACKET_REQUEST_CHUNK;
                        _pak.meta.acknowledge = true;
                        _pak.meta.packet_size = sizeof(union rdm_packet_data);
                        _data->update_chunk.chunk_x = MAX(p_c_x + x, 0);
                        _data->update_chunk.chunk_y = 0;
                        _data->update_chunk.chunk_z = MAX(p_c_z + y, 0);

                        network_transmit_packet(client_state.local_player->client->owner, &client_state.local_player->client->owner->client, &_pak);

                        map->map_data_wanted += RENDER_CHUNK_SIZE;
                    }
                }
            }
            map->next_map_rq = world->time + 0.5f;        
        }
        if(world->time > (map->next_map_rq + 4.5f))
        {
            map->map_data_wanted = 0;
        }
    }
    profiler_end();
}

void map_update_chunk(struct map_manager* map, int c_x, int c_y, int c_z, int d_x, enum block_type* chunk_data)
{
    struct __render_chunk* new_chunk = __map_chunk_position(map, c_x, c_y, c_z);
    if(!new_chunk)
    {
        struct __render_chunk* _chunk = (struct __render_chunk*)malloc2(sizeof(struct __render_chunk));
        _chunk->chunk_x = c_x;
        _chunk->chunk_y = c_y;
        _chunk->chunk_z = c_z;
        _chunk->chunk_id = last_render_chunk_id++;
        graphic_bref_create(&_chunk->bref, SLOT_VERTEX_ARRAY);
        // printf("rdm2: creating new render chunk at %ix%ix%i\n", c_x, c_y, c_z);
        new_chunk = _chunk;
        __map_render_chunk_init(_chunk);
        g_array_append_val(map->chunk_list, _chunk);
    }
    
    for(int y = 0; y < RENDER_CHUNK_SIZE_Y; y++)
        for(int z = 0; z < RENDER_CHUNK_SIZE; z++)
        {
            struct __render_chunk_block* block = &new_chunk->x[d_x].y[y].z[z];
            enum block_type data = chunk_data[y*RENDER_CHUNK_SIZE+z];
            block->air = false;
            if(data == 0)
                block->air = true;
            block->blk_id = data;
            block->obscure = chunk_data[y*RENDER_CHUNK_SIZE+z] != BLOCK_AIR;
        }

    map->map_data_wanted--;
    if(map->map_data_wanted < 0)
        map->map_data_wanted = 0;

    __map_render_chunk_update(new_chunk);
}

void map_init(struct map_manager* map)
{
    load_model("rdm2/cube.obj");
    map->cube = get_model("rdm2/cube.obj");

    load_texture("rdm2/textures.png");

    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    sglc(glGenerateMipmap(GL_TEXTURE_2D));

    map->map_texture_atlas = get_texture("rdm2/textures.png");

    int cube_vertex = compile_shader("rdm2/shaders/instance_cube.vs", GL_VERTEX_SHADER);
    int cube_fragment = compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    map->cube_program = link_program(cube_vertex, cube_fragment);

    int cube_light_vertex = compile_shader("rdm2/shaders/instance_cube_light.vs", GL_VERTEX_SHADER);
    map->cube_program_light = link_program(cube_light_vertex, cube_fragment);

    map->chunk_list = g_array_new(true, true, sizeof(struct __render_chunk*));

    map->next_map_rq = 0.f;
    map->map_request_range = 4;
    map->map_dealloc_range = 5;
    map->map_render_range = 3;
    map->map_data_wanted = 0;
    map->chunk_limit = map->map_request_range * 2;
}

void map_server_init(struct map_server* map)
{
    GTimer* generation_timer = g_timer_new();
    double start_time = g_timer_elapsed(generation_timer, NULL);
    float map_size = 1.0;
    int highway_pos = MAP_SIZE/2;
    for(int x = 0; x < MAP_SIZE; x++)
        for(int y = 0; y < MAP_SIZE; y++)
        {
            struct map_chunk* chunk = &map->chunk_x[x].chunk_y[y];
            perlin_seed = map->map_seed + 1;
            float selector = perlin2d(x/0.1,y/0.1,0.1,4);
            perlin_seed = map->map_seed + 2;
            float selector2 = perlin2d(x/0.1,y/0.1,0.1,4);
            if(x == 0 || x == MAP_SIZE-1 || y == 0 || y == MAP_SIZE-1)
                selector = 0.67f;
            float city_value = (selector2 * (RENDER_CHUNK_SIZE_Y));
            int pavement_color = BLOCK_CONCRETE;
            int ground_color = BLOCK_CONCRETE;
            bool city_tile = false;
            int underground_color = BLOCK_DIRT;
            for(int _x = 0; _x < RENDER_CHUNK_SIZE; _x++)
                for(int _z = 0; _z < RENDER_CHUNK_SIZE; _z++)
                {
                    float true_x = _x+(x*RENDER_CHUNK_SIZE);
                    float true_z = _z+(y*RENDER_CHUNK_SIZE);
                    perlin_seed = map->map_seed + x+(y*MAP_SIZE);
                    float noise = perlin2d(true_x/map_size,true_z/map_size,0.1,4);
                    float noise_2 = (noise * (RENDER_CHUNK_SIZE_Y/2)+2);
                    perlin_seed = map->map_seed;
                    float gnoise = perlin2d(true_x/map_size,true_z/map_size,0.1,4);
                    float gnoise_2 = (gnoise * (RENDER_CHUNK_SIZE_Y/2)+2);
                    bool is_road = false;

                    float value = noise_2;
                    
                    city_tile = false;
                    if(selector < 0.58f)
                    {
                        ground_color = BLOCK_GRASS;
                        is_road = false;
                        value = gnoise_2;
                        chunk->attr = CHUNK_TERRAIN;
                    }
                    else if(selector > 0.78f)
                    {
                        pavement_color = BLOCK_GRASS;
                        is_road = true;
                        chunk->attr = CHUNK_FLAT;
                    }
                    else
                    {
                        underground_color = BLOCK_SKYSCRAPERWALL;
                        ground_color = BLOCK_SKYSCRAPERWALL;
                        city_tile = true;
                        if(floorf(fmodf(true_x/2,4)) == 0)
                            is_road = true;
                        else if(floorf(fmodf(true_z/2,4)) == 0)
                            is_road = true;
                        chunk->attr = CHUNK_CITY;
                    }
                    if(x == 0 && y == 0)
                    {
                        is_road = true;
                        pavement_color = BLOCK_REDSPAWNBLOCK;
                        chunk->attr = CHUNK_RED_SPAWN;
                    }
                    if(x == MAP_SIZE - 1 && y == MAP_SIZE - 1)
                    {
                        is_road = true;
                        pavement_color = BLOCK_BLUESPAWNBLOCK; 
                        chunk->attr = CHUNK_BLUE_SPAWN;
                    }
                    if(x == highway_pos || y == highway_pos)
                    {
                        is_road = true;
                        pavement_color = BLOCK_CONCRETE;
                        chunk->attr = CHUNK_HIGHWAY;
                    }

                    for(int _y = 0; _y < RENDER_CHUNK_SIZE_Y; _y++)
                    {
                        float actual_value = city_tile ? city_value : value;
                        int actual_color = (floorf(value) == _y) ? ground_color : underground_color;

                        if(!is_road)
                            chunk->x[_x].y[_y].z[_z] = (actual_value > _y) ? actual_color : 0;
                        else
                            chunk->x[_x].y[_y].z[_z] = (3 > _y) ? pavement_color : 0;
                    }
                }
        }   
    map->map_ready = true;
    double time_generating = g_timer_elapsed(generation_timer, NULL) - start_time;
    printf("rdm2: generated a map %ix%i, %i bytes in memory (1 chunk is %i bytes), took %f seconds\n", MAP_SIZE, MAP_SIZE, sizeof(map->chunk_x), sizeof(struct map_chunk), time_generating);
    g_timer_destroy(generation_timer);
}

bool map_determine_collision_client(struct map_manager* map, vec3 position)
{   
    if(position[1] < 0)
        return true;
    profiler_event("map_determine_collision_client");

    int map_global_tile_x = floorf((position[0]+0.5f) / CUBE_SIZE);
    int map_global_tile_y = floorf((position[1]) / CUBE_SIZE);
    int map_global_tile_z = floorf((position[2]+0.5f) / CUBE_SIZE);

    int map_chunk_x = floorf((float)map_global_tile_x / RENDER_CHUNK_SIZE);
    int map_chunk_y = floorf((float)map_global_tile_y / RENDER_CHUNK_SIZE_Y);
    int map_chunk_z = floorf((float)map_global_tile_z / RENDER_CHUNK_SIZE);

    // printf("%ix%ix%i %ix%ix%i\n", map_chunk_x, map_chunk_y, map_chunk_z, map_global_tile_x, map_global_tile_y, map_global_tile_z);

    struct __render_chunk* chunk = __map_chunk_position(map, map_chunk_x, map_chunk_y, map_chunk_z);
    if(chunk)
    {
        int map_chunk_tile_x = map_global_tile_x - (map_chunk_x * RENDER_CHUNK_SIZE);
        int map_chunk_tile_y = map_global_tile_y - (map_chunk_y * RENDER_CHUNK_SIZE_Y);
        int map_chunk_tile_z = map_global_tile_z - (map_chunk_z * RENDER_CHUNK_SIZE);

        struct __render_chunk_block block = chunk->x[map_chunk_tile_x].y[map_chunk_tile_y].z[map_chunk_tile_z];

        profiler_end();
        return !block.air;
    }
    else
    {
        profiler_end();
        return map_chunk_y == 0;
    }
}

bool map_determine_collision_server(struct map_server* map, vec3 position)
{
    if(position[1] < 0)
        return true;
    profiler_event("map_determine_collision_server");

    int map_global_tile_x = floorf((position[0]+0.5f) / CUBE_SIZE);
    int map_global_tile_y = floorf((position[1]) / CUBE_SIZE);
    int map_global_tile_z = floorf((position[2]+0.5f) / CUBE_SIZE);

    int map_chunk_x = floorf((float)map_global_tile_x / RENDER_CHUNK_SIZE);
    int map_chunk_y = floorf((float)map_global_tile_y / RENDER_CHUNK_SIZE_Y);
    int map_chunk_z = floorf((float)map_global_tile_z / RENDER_CHUNK_SIZE);

    // printf("%ix%ix%i %ix%ix%i\n", map_chunk_x, map_chunk_y, map_chunk_z, map_global_tile_x, map_global_tile_y, map_global_tile_z);

    if(map_chunk_y != 0)
        return false;
        
    struct map_chunk* chunk = &map->chunk_x[map_chunk_x].chunk_y[map_chunk_z];
    if(chunk)
    {
        int map_chunk_tile_x = map_global_tile_x - (map_chunk_x * RENDER_CHUNK_SIZE);
        int map_chunk_tile_y = map_global_tile_y - (map_chunk_y * RENDER_CHUNK_SIZE_Y);
        int map_chunk_tile_z = map_global_tile_z - (map_chunk_z * RENDER_CHUNK_SIZE);

        enum block_type block = chunk->x[map_chunk_tile_x].y[map_chunk_tile_y].z[map_chunk_tile_z];

        profiler_end();
        return block != BLOCK_AIR;
    }
    else
    {
        profiler_end();
        return false;
    }
}

void map_color_to_rgb(unsigned char color_id, vec3 output)
{
    int data_r = abs((color_id / 16) + 1);
    int data_g = abs((color_id / 4 % 4) + 1);
    int data_b = abs((color_id % 4) + 1);
    output[0] = (((float)data_r)/4.f);
    output[1] = (((float)data_g)/4.f);
    output[2] = (((float)data_b)/4.f);
}

void map_client_clear(struct map_manager* map)
{
    g_array_remove_range(map->chunk_list, 0, map->chunk_list->len);
}