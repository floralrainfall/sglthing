#include "map.h"
#include <sglthing/shader.h>
#include <sglthing/texture.h>

struct __render_chunk test_chunk;

#define CUBE_SIZE 2

#define RENDER_CHUNK_SIZE 16

struct __render_chunk_block
{
    vec3 position;
    char obscure;
    char color;
};

struct __render_chunk
{
    struct {
        struct {
            struct __render_chunk_block z[RENDER_CHUNK_SIZE];
        } y[RENDER_CHUNK_SIZE];
    } x[RENDER_CHUNK_SIZE];

    unsigned int vbo;
};

void __map_render_chunk_init(struct __render_chunk* chunk)
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

                chunk->x[x].y[y].z[z].obscure = x>y;
                chunk->x[x].y[y].z[z].color = (x+y+z)%127;
            }
    glGenBuffers(1,&chunk->vbo);
}

void __map_render_chunk_update(struct __render_chunk* chunk)
{
    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
        for(int y = 0; y < RENDER_CHUNK_SIZE; y++)
            for(int z = 0; z < RENDER_CHUNK_SIZE; z++)
            {

            }
    
    glBindBuffer(GL_ARRAY_BUFFER,chunk->vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(chunk->x),&chunk->x,GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
}

void __map_render_chunk(struct world* world, struct map_manager* map, struct __render_chunk* chunk)
{
    mat4 matrix;
    glm_mat4_identity(matrix);

    glUseProgram(map->cube_program);

    world_uniforms(world, map->cube_program, matrix);

    struct mesh* cube_mesh = &map->cube->meshes[0];
    model_bind_vbos(cube_mesh);

    glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
        
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(struct __render_chunk_block), (void*)0); // vec3: offset
    glVertexAttribDivisor(6, 1); // had to update the entirety of sglthing to opengl 3.3 because of this...
    
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_BYTE, GL_FALSE, sizeof(struct __render_chunk_block), (void*)(offsetof(struct __render_chunk_block,obscure))); // char: obscure
    glVertexAttribDivisor(7, 1);  

    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 1, GL_BYTE, GL_FALSE, sizeof(struct __render_chunk_block), (void*)(offsetof(struct __render_chunk_block,color))); // char: color
    glVertexAttribDivisor(8, 1);  

    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    sglc(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_mesh->element_buffer));
    glDrawElementsInstanced(GL_TRIANGLES, cube_mesh->element_count, GL_UNSIGNED_INT, 0, RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);
}

void map_render_chunks(struct world* world, struct map_manager* map)
{
    __map_render_chunk(world, map, &test_chunk);

    for(int x = -(MAP_STORAGE_SIZE-1); x < MAP_STORAGE_SIZE-1; x++)
    {
        for(int y = -(MAP_STORAGE_SIZE-1); y < MAP_STORAGE_SIZE-1; y++)
        {

        }
    }
}

void map_init(struct map_manager* map)
{
    load_model("rdm2/cube.obj");
    map->cube = get_model("rdm2/cube.obj");

    int cube_vertex = compile_shader("rdm2/shaders/instance_cube.vs", GL_VERTEX_SHADER);
    int cube_fragment = compile_shader("shaders/fragment_simple.fs", GL_FRAGMENT_SHADER);
    map->cube_program = link_program(cube_vertex, cube_fragment);

    __map_render_chunk_init(&test_chunk);
    __map_render_chunk_update(&test_chunk);
}