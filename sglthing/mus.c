#include "mus.h"
#include "world.h"
#include "ui.h"

char mus_note_names[] = {
    'C',
    'D',
    'E',
    'F',
    'G',
    'A',
    'B',
};

void mus_init(struct musmgr* mgr, struct sndmgr* s_mgr)
{
    mgr->scratch_song = (struct mus_file*)malloc(sizeof(struct mus_file));
    mgr->scratch_song->speed = 1;
    strncpy(mgr->scratch_song->name, "scratch", 64);
    mgr->current_music = 0;
    mgr->current_pattern = 0;
    mgr->pattern_offset = 0;
    mgr->sndmgr = s_mgr;
    mgr->dbgmgr = true;
    mgr->playing = false;

    alGenSources(MUS_TRACK_COUNT, mgr->sources);
    for(int i = 0; i < MUS_TRACK_COUNT; i++)
    {
        
    }

    mus_set_file(mgr, mgr->scratch_song);
}

struct mus_file* load_mus_file(char* file_name)
{
    char dest_path[255];
    if(file_get_path(dest_path, 255, file_name) == -1)
    {
        printf("sglthing: file %s doesn't exist\n", file_name);
        return;
    }
    struct mus_file* file = (struct mus_file*)malloc(sizeof(struct mus_file));
    FILE* file_r = fopen(dest_path, "r");
    fread(file, 1, sizeof(struct mus_file), file_r);
    fclose(file_r);    
    printf("sglthing: loaded music '%s', copyright: '%s'\n", file->name, file->copyright);
}

void mus_set_file(struct musmgr* mgr, struct mus_file* file)
{
    for(int i = 0; i < SAMPLES_PER_FILE; i++)
        if(file->sample_data[i].load == LOADED_SAMPLE_ID)
        {
            load_snd(file->sample_data[i].sample_path);
        } 
    for(int i = 0; i < MUS_TRACK_COUNT; i++)
    {
        alSourcef(mgr->sources[i], AL_PITCH, 1);
        alSourcef(mgr->sources[i], AL_GAIN, 1);
        alSource3f(mgr->sources[i], AL_POSITION, 0, 0, 0);
        alSource3f(mgr->sources[i], AL_VELOCITY, 0, 0, 0);
        alSourcei(mgr->sources[i], AL_LOOPING, AL_FALSE);
    }
    mgr->current_music = file;
    mgr->current_pattern = 0;
    mgr->pattern_offset = 0;
    mgr->playing = true;
}

void mus_tick(struct musmgr* mgr)
{
    if(!mgr->world_ptr)
        return;
    struct world* world = (struct world*)mgr->world_ptr;

    if(mgr->current_music)
    {
        int pattern_id = mgr->current_music->pattern_map[mgr->current_pattern] % PATTERNS_PER_FILE;
        struct mus_pattern* current_pattern = &mgr->current_music->pattern_data[pattern_id];
        struct mus_frame *current_frame = current_pattern->frames[mgr->pattern_offset % FRAMES_PER_PATTERN];

        if(mgr->dbgmgr)
        {
            float dbgmgr_ui_base_x = 0;
            float dbgmgr_ui_base_y = 750;
            vec4 old_color;
            glm_vec4_copy(world->ui->foreground_color, old_color);
            for(int j = 0; j < MUS_TRACK_COUNT; j++)
            {
                world->ui->foreground_color[0] = 0.25f;
                world->ui->foreground_color[1] = 0.25f;
                world->ui->foreground_color[2] = 0.25f;
                char noteinfo[32];
                snprintf(noteinfo, 32, "TRACK %i", j);
                ui_draw_text(world->ui, dbgmgr_ui_base_x+((8.f*24.f)*j), dbgmgr_ui_base_y+(16.f), noteinfo, 1.f);
                for(int i = 0; i < FRAMES_PER_PATTERN; i++)
                {
                    struct mus_frame* frame = &current_pattern->frames[i][j];
                    snprintf(noteinfo, 32, "%c%02x:%02x-%c-%02x%02x, %08x", 
                        (i == mgr->pattern_offset)?'>':' ', 
                        i, 
                        frame->sample, 
                        mus_note_names[frame->note%7], 
                        frame->pitch, 
                        frame->gain, 
                        frame->cmd);
                    if(i%2)
                    {
                        world->ui->foreground_color[0] = 0.75f;
                        world->ui->foreground_color[1] = 0.75f;
                        world->ui->foreground_color[2] = 0.75f;
                    }
                    else
                    {
                        world->ui->foreground_color[0] = 0.5f;
                        world->ui->foreground_color[1] = 0.5f;
                        world->ui->foreground_color[2] = 0.5f;
                    }
                    if(i == mgr->pattern_offset)
                    {
                        world->ui->foreground_color[0] = 1.f;
                        world->ui->foreground_color[1] = 1.f;
                        world->ui->foreground_color[2] = 1.f;
                    }
                    ui_draw_text(world->ui, dbgmgr_ui_base_x+((8.f*24.f)*j), dbgmgr_ui_base_y-(16.f*i), noteinfo, 1.f);
                }
            }
            glm_vec4_copy(old_color, world->ui->foreground_color);
        }

        if(mgr->playing)
        {
            double tick_rate = 0.016 * (double)mgr->current_music->speed;
            if(glfwGetTime() - mgr->next_tick > tick_rate)
            {
                mgr->pattern_offset++;
                if(mgr->pattern_offset == FRAMES_PER_PATTERN)
                {
                    mgr->pattern_offset = 0;
                    mgr->current_pattern++;
                }
                if(mgr->current_pattern == PATTERNS_PER_FILE)
                {
                    mgr->current_pattern = 0;
                }
                for(int i = 0; i < MUS_TRACK_COUNT; i++)
                {
                    mgr->playing_samples[i] = &mgr->current_music->sample_data[current_frame[i].sample];
                    alSourcef(mgr->sources[i], AL_PITCH, ((float)current_frame[i].pitch)/10.f);
                    alSourcef(mgr->sources[i], AL_GAIN, ((float)current_frame[i].gain)/10.f);
                }
                mgr->next_tick = glfwGetTime() + tick_rate;
            }
        }

        for(int i = 0; i < MUS_TRACK_COUNT; i++)
        {

        }

        ui_draw_text(world->ui, 0.f, 128.f, "sglmustracker", 1.f);
        ui_draw_text(world->ui, 0.f, 128.f-16.f, mgr->current_music->name, 1.f);


    }
}