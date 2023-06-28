#include "model.h"
#include "graphic.h"
#include "texture.h"
#include "sglthing.h"
#include "io.h"
#include <string.h>
#include <cglm/cglm.h>
#ifndef HEADLESS
#include <glad/glad.h>
#endif
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct model models[256] = {0};
int models_loaded = 0;

static void model_load_textures(struct mesh* mesh, struct aiMesh* mesh_2, const struct aiScene* scene)
{
    if(mesh_2->mMaterialIndex >= 0)
    {
        struct aiMaterial* material = scene->mMaterials[mesh_2->mMaterialIndex];
        struct aiString path;
        for(unsigned int i = 0; i < MIN(3,aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE)); i++)
        {
            aiGetMaterialTexture(material, aiTextureType_DIFFUSE, i, &path, NULL, NULL, NULL, NULL, NULL, NULL);
            char fpath[256];
            char rsc_dir[256] = {0};
            char rsc_path[256];
            strncpy(rsc_path, mesh->path, 256);
            char* last = strrchr(rsc_path, '/');
            if(last != NULL)
            {
                int len = strlen(rsc_path) - strlen(last);
                strncpy(rsc_dir, rsc_path, MIN(len,256));
            }
            if(path.data[0] != '/')
                snprintf(fpath,256,"%s/%s",rsc_dir,path.data);
            else
                strncpy(fpath, path.data, 256);
            if(!get_texture2(&fpath))
            {
                if(path.data[0] == '*')
                    printf("sglthing: embedded textures not supported\n");
                else
                {
                    load_texture(fpath);
                    mesh->diffuse_texture[mesh->diffuse_textures] = get_texture(fpath);
                    mesh->diffuse_textures++;
                }
            }
            else
            {
                mesh->diffuse_texture[mesh->diffuse_textures] = get_texture(fpath);
                mesh->diffuse_textures++;
            }
        }
        for(unsigned int i = 0; i < MIN(3,aiGetMaterialTextureCount(material, aiTextureType_SPECULAR)); i++)
        {
            aiGetMaterialTexture(material, aiTextureType_SPECULAR, i, &path, NULL, NULL, NULL, NULL, NULL, NULL);
            if(!get_texture(&path.data))
            {
                if(path.data[0] == '*')
                    printf("sglthing: embedded textures not supported\n");
                else
                {
                    char fpath[64];
                    snprintf(fpath,64,"../%s",&path.data);
                    load_texture(fpath);
                    mesh->specular_texture[mesh->specular_textures] = get_texture(fpath);
                    mesh->specular_textures++;
                }
            }
            else
            {
                mesh->specular_texture[mesh->specular_textures] = get_texture(path.data);
                mesh->specular_textures++;
            }    
        }
    }
}

static void model_reset_vertex_bone_data(struct model_vertex* vtx_array, int vertex)
{
    for(int i = 0; i < MAX_BONE_WEIGHTS; i++)
    {
        vtx_array[vertex].bone_ids[i] = -1;
        vtx_array[vertex].weights[i] = 0.f;
    }
}

void model_find_bone_data(struct aiMesh* mesh, char* name, struct model_bone_info* info_out)
{
    for(int i = 0; i < mesh->mNumBones; i++)
    {
        if(strncmp(mesh->mBones[i]->mName.data, name, 64) == 0)
        {
            info_out->id = i;
            strncpy(info_out->name,mesh->mBones[i]->mName.data,64);
            ASSIMP_TO_GLM(mesh->mBones[i]->mOffsetMatrix, info_out->offset);
            return;
        }
    }
    info_out->id = -1;
}

void model_find_bone_info(struct mesh* mesh, char* name, struct model_bone_info* info_out)
{
    for(int i = 0; i < mesh->bone_infos; i++)
    {
        if(strncmp(mesh->bone_info[i].name,name,64)==0)
        {
            *info_out = mesh->bone_info[i];
            return;
        }
    }
    info_out->id = -1;
}

static void model_set_vertex_bone_data(struct model_vertex* vtx_array, int vertex, int bone_id, float weight)
{
    for(int i = 0; i < MAX_BONE_WEIGHTS; i++)
    {
        if(vtx_array[vertex].bone_ids[i] < 0)
        {
            vtx_array[vertex].bone_ids[i] = bone_id;
            vtx_array[vertex].weights[i] = weight;
            break;
        }
    }
}

static void model_extract_bone_weights(struct mesh* i_mesh, struct model_vertex* vtx_array, struct aiMesh* mesh, const struct aiScene* scene)
{
    if(mesh->mNumBones == 0)
        printf("sglthing: mesh has no bones\n");
    for(int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
    {
        int bone_id = -1;
        struct model_bone_info bone_data;
        char* bone_name = mesh->mBones[bone_index]->mName.data;
        printf("sglthing: %s bone added\n", bone_name);
        model_find_bone_data(mesh, bone_name, &bone_data);
        ASSERT(bone_data.id != -1);
        if(strncmp(bone_name,i_mesh->bone_info[i_mesh->bone_infos].name,64)==0 || i_mesh->bone_infos == bone_data.id)
        {
            struct model_bone_info new_bone_info;
            new_bone_info.id = i_mesh->bone_infos;
            strncpy(new_bone_info.name,bone_name,64);
            ASSIMP_TO_GLM(mesh->mBones[bone_index]->mOffsetMatrix,new_bone_info.offset);
            bone_id = new_bone_info.id;
            i_mesh->bone_info[i_mesh->bone_infos++] = new_bone_info;
        }
        else
        {
            bone_id = bone_data.id;
        }

        ASSERT(bone_id != -1);

        struct aiVertexWeight* weights = mesh->mBones[bone_index]->mWeights;
        int num_weights = mesh->mBones[bone_index]->mNumWeights;

        for(int weight_index = 0; weight_index < num_weights; ++weight_index)
        {
            int vertex_id = weights[weight_index].mVertexId;
            float weight =  weights[weight_index].mWeight;
            ASSERT(vertex_id <= i_mesh->vtx_data_count);
            model_set_vertex_bone_data(i_mesh->vtx_data, vertex_id, bone_id, weight);
        }
    }
    printf("sglthing: %i bones extracted\n", mesh->mNumBones);
}

static void model_parse_mesh(struct model_vertex* vtx_array, int* vtx_count, unsigned int* idx_array, int* idx_count, struct aiMesh* mesh, const struct aiScene* scene)
{
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        struct model_vertex* curr_vtx = &vtx_array[i];
        (*vtx_count)++;

        model_reset_vertex_bone_data(vtx_array, i);

        curr_vtx->position[0] = mesh->mVertices[i].x;
        curr_vtx->position[1] = mesh->mVertices[i].y;
        curr_vtx->position[2] = mesh->mVertices[i].z; 

        curr_vtx->normal[0] = mesh->mNormals[i].x;
        curr_vtx->normal[1] = mesh->mNormals[i].y;
        curr_vtx->normal[2] = mesh->mNormals[i].z;        

        if(mesh->mColors[0])
        {
            curr_vtx->color[0] = mesh->mColors[0][i].r;
            curr_vtx->color[1] = mesh->mColors[0][i].g;
            curr_vtx->color[2] = mesh->mColors[0][i].b;
            curr_vtx->color[3] = mesh->mColors[0][i].a;
        }
        else
        {
            curr_vtx->color[0] = atan2(mesh->mVertices[i].z,mesh->mVertices[i].x);
            curr_vtx->color[1] = atan2(mesh->mVertices[i].x,mesh->mVertices[i].y);
            curr_vtx->color[2] = atan2(mesh->mVertices[i].y,mesh->mVertices[i].z);
            curr_vtx->color[3] = 1.f;
        }

        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            curr_vtx->tex_coords[0] = mesh->mTextureCoords[0][i].x; 
            curr_vtx->tex_coords[1] = mesh->mTextureCoords[0][i].y;
        }
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        struct aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
        {
            idx_array[*idx_count] = face.mIndices[j];
            (*idx_count)++;
        }
    }

    // printf("parsed mesh with %i vertices %i faces... \n", mesh->mNumVertices, mesh->mNumFaces);
}

static void model_parse_node(struct model* model, struct aiNode* node, const struct aiScene* scene)
{
    for(int i = 0; i < node->mNumMeshes; i++)
    {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        struct model_vertex* vtx_array = (struct model_vertex*)malloc2(mesh->mNumVertices*sizeof(struct model_vertex));
        int idx_num = 0;
        for(int i = 0; i < mesh->mNumFaces; i++)
            idx_num += mesh->mFaces[i].mNumIndices;
        int* idx_array = (int*)malloc2(idx_num * sizeof(int));
        int idx_count = 0, vtx_count = 0;

        model_parse_mesh(vtx_array, &vtx_count, idx_array, &idx_count, mesh, scene);

        model->meshes[model->mesh_count].idx_data = idx_array;
        model->meshes[model->mesh_count].element_count = idx_count;
        model->meshes[model->mesh_count].vtx_data = vtx_array;
        model->meshes[model->mesh_count].vtx_data_count = vtx_count;
        model->meshes[model->mesh_count].path = model->path;
        model->meshes[model->mesh_count].model_parent = model;
        model->meshes[model->mesh_count].bones = mesh->mNumBones;
        model_load_textures(&model->meshes[model->mesh_count], mesh, scene);
        model_extract_bone_weights(&model->meshes[model->mesh_count], vtx_array, mesh, scene);    

        graphic_varray_create(&model->meshes[model->mesh_count].vertex_array, RT_ELEMENTS);

        model->meshes[model->mesh_count].vertex_array.bref_array = &model->meshes[model->mesh_count].vertex_buffer;
        model->meshes[model->mesh_count].vertex_array.bref_elements = &model->meshes[model->mesh_count].element_buffer;
        model->meshes[model->mesh_count].vertex_array.gl_count = idx_count;

        graphic_bref_create(&model->meshes[model->mesh_count].vertex_buffer, SLOT_VERTEX_ARRAY);
        graphic_bref_create(&model->meshes[model->mesh_count].element_buffer, SLOT_ELEMENT_ARRAY);

        graphic_bref_upload(&model->meshes[model->mesh_count].vertex_buffer, vtx_array, vtx_count * sizeof(struct model_vertex));
        graphic_bref_upload(&model->meshes[model->mesh_count].element_buffer, idx_array, idx_count * sizeof(int));

        graphic_varray_add(&model->meshes[model->mesh_count].vertex_array,(struct graphic_varray_entry){ // vec3: position
            .buff = &model->meshes[model->mesh_count].vertex_buffer,
            .layout_id = 0,
            .buff_array_len = 3,
            .buff_array_size = sizeof(struct model_vertex),
            .buff_array_stride = offsetof(struct model_vertex, position),
            .type = DT_FLOAT,
        });
        graphic_varray_add(&model->meshes[model->mesh_count].vertex_array,(struct graphic_varray_entry){ // vec3: normal
            .buff = &model->meshes[model->mesh_count].vertex_buffer,
            .layout_id = 1,
            .buff_array_len = 3,
            .buff_array_size = sizeof(struct model_vertex),
            .buff_array_stride = offsetof(struct model_vertex, normal),
            .type = DT_FLOAT,
        });
        graphic_varray_add(&model->meshes[model->mesh_count].vertex_array,(struct graphic_varray_entry){ // vec4: color
            .buff = &model->meshes[model->mesh_count].vertex_buffer,
            .layout_id = 2,
            .buff_array_len = 4,
            .buff_array_size = sizeof(struct model_vertex),
            .buff_array_stride = offsetof(struct model_vertex, color),
            .type = DT_FLOAT,
        });
        graphic_varray_add(&model->meshes[model->mesh_count].vertex_array,(struct graphic_varray_entry){ // vec2: tex_coords
            .buff = &model->meshes[model->mesh_count].vertex_buffer,
            .layout_id = 3,
            .buff_array_len = 2,
            .buff_array_size = sizeof(struct model_vertex),
            .buff_array_stride = offsetof(struct model_vertex, tex_coords),
            .type = DT_FLOAT,
        });
        graphic_varray_add(&model->meshes[model->mesh_count].vertex_array,(struct graphic_varray_entry){ // int4: bone_ids
            .buff = &model->meshes[model->mesh_count].vertex_buffer,
            .layout_id = 4,
            .buff_array_len = 4,
            .buff_array_size = sizeof(struct model_vertex),
            .buff_array_stride = offsetof(struct model_vertex, bone_ids),
            .type = DT_INT,
        });
        graphic_varray_add(&model->meshes[model->mesh_count].vertex_array,(struct graphic_varray_entry){ // vec4: weights
            .buff = &model->meshes[model->mesh_count].vertex_buffer,
            .layout_id = 5,
            .buff_array_len = 4,
            .buff_array_size = sizeof(struct model_vertex),
            .buff_array_stride = offsetof(struct model_vertex, weights),
            .type = DT_FLOAT,
        });
        graphic_varray_upload(&model->meshes[model->mesh_count].vertex_array);

        //int new_mesh = create_model_vao(vtx_array, vtx_count, idx_array, idx_count, &vertex_buffer, &element_buffer);

        model->mesh_count++;

        free2(idx_array);
    }
    for(int i = 0; i < node->mNumChildren; i++)
    {
        model_parse_node(model, node->mChildren[i], scene);
    }
}

void load_model_mem(void* mem, int length, char* name)
{
    struct model* sel_model = &models[models_loaded];
    sel_model->mesh_count = 0;
    const struct aiScene *scene = aiImportFileFromMemory(mem, length, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals, NULL);
    if(!scene)
    {
        printf("sglthing: model %s not loadable\n", name);
        return;
    }

    sel_model->path = "sglsiteassets";
    model_parse_node(sel_model, scene->mRootNode, scene);

    strncpy(&sel_model->name[0], name, 64);
    sel_model->scene = scene;

    printf("sglthing: model %s loaded from memory\n", name);

    models_loaded++;
}

void load_model(char* file)
{
    struct model* sel_model = &models[models_loaded];
    sel_model->mesh_count = 0;
    
    char path[256];
    file_get_path(path,256,file);
    const struct aiScene *scene = aiImportFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if(!scene)
    {
        printf("sglthing: model %s not found\n", file);
        return;
    }

    sel_model->path = path;
    model_parse_node(sel_model, scene->mRootNode, scene);

    strncpy(&sel_model->name[0], file, 64);
    sel_model->scene = scene;

    printf("sglthing: model %s loaded\n", file);

    models_loaded++;
}

struct model* get_model(char* file)
{
    for(int i = 0; i < 256; i++)
    {
        if(strncmp(file,models[i].name,64)==0)
            return &models[i];
    }
    return 0;
}

static int vertex_array = 0;

static void model_vao_pointers()
{
    // vec3: position
    sglc(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct model_vertex), (void*)0));
    sglc(glEnableVertexAttribArray(0));
    // vec3: normal
    sglc(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct model_vertex), (void*)offsetof(struct model_vertex, normal)));
    sglc(glEnableVertexAttribArray(1));
    // vec4: color
    sglc(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(struct model_vertex), (void*)offsetof(struct model_vertex, color)));
    sglc(glEnableVertexAttribArray(2));
    // vec2: uv
    sglc(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(struct model_vertex), (void*)offsetof(struct model_vertex, tex_coords)));
    sglc(glEnableVertexAttribArray(3));
    // ivec4: bone_ids
    sglc(glVertexAttribIPointer(4, 4, GL_INT, sizeof(struct model_vertex), (void*)offsetof(struct model_vertex, bone_ids)));
    sglc(glEnableVertexAttribArray(4));
    // vec4: weights
    sglc(glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(struct model_vertex), (void*)offsetof(struct model_vertex, weights)));
    sglc(glEnableVertexAttribArray(5));
}

void graphic_render_model(struct model* model, int shader, mat4 model_matrix)
{
    for(int i = 0; i < model->mesh_count; i++)
    {
        struct mesh* mesh = &model->meshes[i];
        int t_last = 0;
        for(int i = 0; i < mesh->diffuse_textures; i++)
            graphic_texture_bind(shader, TT_DIFFUSE, t_last++, mesh->diffuse_texture[i]);
        for(int i = 0; i < mesh->specular_textures; i++)
            graphic_texture_bind(shader, TT_SPECULAR, t_last++, mesh->specular_texture[i]);
        graphic_render_varray(&mesh->vertex_array, shader, model_matrix);
    }
}