#include "map.h"
#include <sglthing/shader.h>
#include <sglthing/texture.h>
#include <sglthing/net.h>
#include "rdm_net.h"
#include "rdm2.h"

#define CUBE_SIZE 1

static int last_render_chunk_id;

struct __render_chunk_block
{
    vec3 position;
    char obscure;
    char color;
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
        } y[RENDER_CHUNK_SIZE];
    } x[RENDER_CHUNK_SIZE];

    unsigned int vbo;
};

static void __map_render_chunk_init(struct __render_chunk* chunk)
{
    sizeof(struct __render_chunk);
    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
        for(int y = 0; y < RENDER_CHUNK_SIZE; y++)
            for(int z = 0; z < RENDER_CHUNK_SIZE; z++)
            {
                vec3 t_position;
                t_position[0] = x * CUBE_SIZE;
                t_position[1] = y * CUBE_SIZE;
                t_position[2] = z * CUBE_SIZE;
                
                glm_vec3_copy(t_position,chunk->x[x].y[y].z[z].position);
            }
    sglc(glGenBuffers(1,&chunk->vbo));
}

static void __map_render_chunk_update(struct __render_chunk* chunk)
{
    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
        for(int y = 0; y < RENDER_CHUNK_SIZE; y++)
            for(int z = 0; z < RENDER_CHUNK_SIZE; z++)
            {
                struct __render_chunk_block bottom_block = chunk->x[x].y[y-1].z[z];
                chunk->x[x].y[y].z[z].obscure = (bottom_block.color != 0);
            }
    
    sglc(glBindBuffer(GL_ARRAY_BUFFER,chunk->vbo));
    sglc(glBufferData(GL_ARRAY_BUFFER,sizeof(chunk->x),&chunk->x,GL_STATIC_DRAW));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, 0)); 
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
    mat4 matrix;
    glm_mat4_identity(matrix);
    glm_translate(matrix, (vec3){chunk->chunk_x*(RENDER_CHUNK_SIZE*CUBE_SIZE),chunk->chunk_y*(RENDER_CHUNK_SIZE*CUBE_SIZE),chunk->chunk_z*(RENDER_CHUNK_SIZE*CUBE_SIZE)});

    sglc(glUseProgram(map->cube_program));

    world_uniforms(world, map->cube_program, matrix);

    struct mesh* cube_mesh = &map->cube->meshes[0];
    model_bind_vbos(cube_mesh);

    sglc(glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo));
        
    sglc(glEnableVertexAttribArray(6));
    sglc(glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(struct __render_chunk_block), (void*)0)); // vec3: offset
    sglc(glVertexAttribDivisor(6, 1)); // had to update the entirety of sglthing to opengl 3.3 because of this...
    
    sglc(glEnableVertexAttribArray(7));
    sglc(glVertexAttribPointer(7, 1, GL_BYTE, GL_FALSE, sizeof(struct __render_chunk_block), (void*)(offsetof(struct __render_chunk_block,obscure)))); // char: obscure
    sglc(glVertexAttribDivisor(7, 1));  

    sglc(glEnableVertexAttribArray(8));
    sglc(glVertexAttribPointer(8, 1, GL_BYTE, GL_FALSE, sizeof(struct __render_chunk_block), (void*)(offsetof(struct __render_chunk_block,color)))); // char: color
    sglc(glVertexAttribDivisor(8, 1));  

    sglc(glBindBuffer(GL_ARRAY_BUFFER, 0)); 

    sglc(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_mesh->element_buffer));
    sglc(glDrawElementsInstanced(GL_TRIANGLES, cube_mesh->element_count, GL_UNSIGNED_INT, 0, RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE));
}

void map_render_chunks(struct world* world, struct map_manager* map)
{
    for(int i = 0; i < map->chunk_list->len; i++)
        __map_render_chunk(world, map, g_array_index(map->chunk_list, struct __render_chunk*, i));
}

void map_update_chunks(struct map_manager* map, struct world* world)
{
    for(int i = 0; i < map->chunk_list->len; i++)
    {
        struct __render_chunk* chunk = g_array_index(map->chunk_list, struct __render_chunk*, i);
        vec3 chunk_position = {
            chunk->chunk_x * (RENDER_CHUNK_SIZE * CUBE_SIZE),
            chunk->chunk_y * (RENDER_CHUNK_SIZE * CUBE_SIZE),
            chunk->chunk_z * (RENDER_CHUNK_SIZE * CUBE_SIZE),
        };
        float chunk_distance = glm_vec3_distance(chunk_position, (vec3){world->cam.position[0],0.f,world->cam.position[2]});
        if(chunk_distance > 8 * (RENDER_CHUNK_SIZE * CUBE_SIZE))
        {
            if(chunk->vbo)
                sglc(glDeleteBuffers(1, &chunk->vbo));
            g_array_remove_index(map->chunk_list, i);
            i--;
        }
    }

    int p_c_x = roundf(world->cam.position[0]/(RENDER_CHUNK_SIZE * CUBE_SIZE));
    int p_c_y = roundf(world->cam.position[1]/(RENDER_CHUNK_SIZE * CUBE_SIZE));
    int p_c_z = roundf(world->cam.position[2]/(RENDER_CHUNK_SIZE * CUBE_SIZE));

    int request_range = 2;

    if(client_state.local_player)
        for(int x = -request_range+1; x <= request_range; x++)
            for(int y = -request_range+1; y <= request_range; y++)
            {
                struct __render_chunk* c = __map_chunk_position(map, p_c_x + x, 0, p_c_z + y);
                if(!c)
                {    
                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                    _pak.meta.packet_type = RDM_PACKET_REQUEST_CHUNK;
                    _pak.meta.acknowledge = true;
                    _data->update_chunk.chunk_x = MAX(p_c_x + x, 0);
                    _data->update_chunk.chunk_y = 0;
                    _data->update_chunk.chunk_z = MAX(p_c_z + y, 0);
                    network_transmit_packet(client_state.local_player->client->owner, &client_state.local_player->client->owner->client, _pak);
                }
            }
}

void map_update_chunk(struct map_manager* map, int c_x, int c_y, int c_z, int d_x, int d_y, char* chunk_data)
{
    struct __render_chunk* new_chunk = __map_chunk_position(map, c_x, c_y, c_z);
    if(!new_chunk)
    {
        struct __render_chunk* _chunk = (struct __render_chunk*)malloc(sizeof(struct __render_chunk));
        _chunk->chunk_x = c_x;
        _chunk->chunk_y = c_y;
        _chunk->chunk_z = c_z;
        _chunk->chunk_id = last_render_chunk_id++;
        _chunk->vbo = 0;
        // printf("rdm2: creating new render chunk at %ix%ix%i\n", c_x, c_y, c_z);
        new_chunk = _chunk;
        __map_render_chunk_init(_chunk);
        g_array_append_val(map->chunk_list, _chunk);
    }
    
    for(int i = 0; i < RENDER_CHUNK_SIZE; i++)
    {
        struct __render_chunk_block* block = &new_chunk->x[d_x].y[d_y].z[i];
        block->color = chunk_data[i];
        block->obscure = chunk_data[i];
    }

    __map_render_chunk_update(new_chunk);
}

void map_init(struct map_manager* map)
{
    load_model("rdm2/cube.obj");
    map->cube = get_model("rdm2/cube.obj");

    int cube_vertex = compile_shader("rdm2/shaders/instance_cube.vs", GL_VERTEX_SHADER);
    int cube_fragment = compile_shader("shaders/fragment_simple.fs", GL_FRAGMENT_SHADER);
    map->cube_program = link_program(cube_vertex, cube_fragment);

    map->chunk_list = g_array_new(true, true, sizeof(struct __render_chunk*));
}

void map_server_init(struct map_server* map)
{
    for(int x = 0; x < MAP_SIZE; x++)
        for(int y = 0; y < MAP_SIZE; y++)
        {
            struct map_chunk* chunk = &map->chunk_x[x].chunk_y[y];
            for(int _x = 0; _x < RENDER_CHUNK_SIZE; _x++)
                for(int _y = 0; _y < RENDER_CHUNK_SIZE; _y++)
                    for(int _z = 0; _z < RENDER_CHUNK_SIZE; _z++)
                    {
                        int map_size = RENDER_CHUNK_SIZE/MAP_SIZE;
                        float true_x = _x+(x*RENDER_CHUNK_SIZE);
                        float true_z = _z+(y*RENDER_CHUNK_SIZE);
                        float noise = perlin2d((float)true_x/map_size,(float)true_z/map_size,0.1,4)*RENDER_CHUNK_SIZE;
                        chunk->x[_x].y[_y].z[_z] = (noise > _y) ? _y*2 : 0;
                    }
        }   
}

void map_determine_collision_client(struct map_manager* map, vec3 position)
{   
    position[0] /= CUBE_SIZE;
    position[1] /= CUBE_SIZE;
    position[2] /= CUBE_SIZE;

    position[0] = roundf(position[0]);
    position[1] = roundf(position[1]);
    position[2] = roundf(position[2]);


}

void map_determine_collision_server(struct map_server* map, vec3 position)
{
    position[0] /= CUBE_SIZE;
    position[1] /= CUBE_SIZE;
    position[2] /= CUBE_SIZE;

    position[0] = roundf(position[0]);
    position[1] = roundf(position[1]);
    position[2] = roundf(position[2]);

}
