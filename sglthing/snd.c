#include "snd.h"
#include "io.h"
#include "sglthing.h"

#define CACHE_SIZE 512
struct snd snd_cache[CACHE_SIZE];
int snd_count = 0;

void snd_init(struct sndmgr* mgr)
{
#ifdef SOUND_ENABLED
    Pa_Initialize();

    printf("sglthing: sound init (%p)\n", mgr);
#endif
}

struct snd* get_snd(char* file_name)
{
#ifdef SOUND_ENABLED
    struct snd* snd = NULL;
    for(int i = 0; i < snd_count; i++)
    {
        if(strncmp(file_name,snd_cache[i].name,64)==0)
        {
            snd = &snd_cache[i];
            break;
        }
    }
    return snd;
#else
    return 0;
#endif
}

#ifdef SOUND_ENABLED
static int snd_callback(void* input, void* output, unsigned long frame_count, const PaStreamCallbackTimeInfo* pa_time, PaStreamCallbackFlags status, void* user)
{
    struct snd* snd = (struct snd*)user;

    float *cursor;
    float *out = (float*)output;
    int sz = frame_count;
    int rd = 0;

    cursor = out;

    bool stop = snd->stop;
    while(sz > 0)
    {
        sf_seek(snd->file, snd->position, SEEK_SET);
        if (sz > (snd->libsndfile_info.frames - snd->position))
        {
            rd = snd->libsndfile_info.frames - snd->position;
            snd->position = 0;            
            if(!snd->loop)
                snd->stop = true;
        }
        else
        {
            rd = sz;
            snd->position += rd;
        }

        sf_readf_float(snd->file, cursor, rd);

        float multiplier = snd->multiplier;
        float attenuation_l = 1.0;
        float attenuation_r = 1.0;

#define EAR_DISTANCE 3

        if(snd->sound_3d)
        {
            vec3 camera_position;
            vec3 l_ear_offset = {EAR_DISTANCE,0.f,0.f};
            vec3 r_ear_offset = {-EAR_DISTANCE,0.f,0.f};
            camera_position[0] = *(snd->camera_position)[0];
            camera_position[1] = 10.f;
            camera_position[2] = *(snd->camera_position)[1];

            vec3 ear_position;
            glm_vec3_add(camera_position, l_ear_offset, ear_position);
            float distance_l = glm_vec3_distance(ear_position, snd->sound_position);
            glm_vec3_add(camera_position, r_ear_offset, ear_position);
            float distance_r = glm_vec3_distance(ear_position, snd->sound_position);
            attenuation_l = 1.f/powf(distance_l/10.f,2.f);
            attenuation_r = 1.f/powf(distance_r/10.f,2.f);
            printf("%i %0.2f %0.2f %0.2f %0.2f %f %f %f %f %f %f\n", snd->id, attenuation_l, attenuation_r, distance_l, distance_r, snd->sound_position[0], snd->sound_position[1], snd->sound_position[2], camera_position[0], camera_position[1], camera_position[2]);
        }

        bool stereo = (snd->libsndfile_info.channels == 2);
        int frame_rd_count = stereo ? sz*2 : sz;
        int add_no = stereo ? 2 : 1;
        for(int i = 0; i < frame_rd_count; i+=add_no)
        {
            cursor[i] *= multiplier * attenuation_l;
            if(stereo)
                cursor[i+1] *= multiplier * attenuation_r;
        }

        cursor += rd;
        sz -= rd;
    }
    
    return stop ? paComplete : paContinue;
}
#endif

struct snd* load_snd(char* file_name)
{
#ifdef SOUND_ENABLED
    struct snd* o_snd = get_snd(file_name);
    if(o_snd)
        return o_snd;
        
    char dest_path[255];
    if(file_get_path(dest_path, 255, file_name) == -1)
    {
        printf("sglthing: file %s doesn't exist\n", file_name);
        return 0;
    }
    struct snd* n_snd = &snd_cache[snd_count];
    snd_count++;
    strncpy(n_snd->name, file_name, 64);
    n_snd->loop = false;
    n_snd->position = 0;
    n_snd->file = sf_open(dest_path, SFM_READ, &n_snd->libsndfile_info);
    n_snd->stop = false;
    n_snd->sound_3d = false;
    n_snd->id = snd_count-1;
    n_snd->multiplier = 1.0f;
    printf("sglthing: %s sound file rate %i, %i channels, %i frames\n", 
        dest_path,
        n_snd->libsndfile_info.samplerate,
        n_snd->libsndfile_info.channels,
        n_snd->libsndfile_info.frames);

    return n_snd;
#else
    return 0;
#endif
}

struct snd*  play_snd(char* file_name)
{
#ifdef SOUND_ENABLED
    struct snd* snd = get_snd(file_name);    
    if(!snd)
    {
        printf("sglthing: attempted to play nonexistent snd %s\n", file_name);
        return 0;
    }
    struct snd* new_snd = malloc(sizeof(struct snd));
    memcpy(new_snd, snd, sizeof(struct snd));

    new_snd->stop = false;
    new_snd->position = 0;
    PaStreamParameters stream_param;
    stream_param.device = Pa_GetDefaultOutputDevice();
    stream_param.channelCount = new_snd->libsndfile_info.channels;
    stream_param.sampleFormat = paFloat32;
    stream_param.suggestedLatency = 0.2;
    stream_param.hostApiSpecificStreamInfo = 0;
    PaError error = Pa_OpenStream(&new_snd->stream, NULL, &stream_param, new_snd->libsndfile_info.samplerate, paFramesPerBufferUnspecified, paNoFlag, snd_callback, new_snd);
    if(error)
        printf("sglthing: error opening stream (%i)\n", error);

    Pa_StartStream(new_snd->stream);
    return new_snd;
#else
    return 0;
#endif
}

void resume_snd(struct snd* snd)
{
#ifdef SOUND_ENABLED
    snd->stop = true;
    Pa_StartStream(snd->stream);
#endif
}

void stop_snd(struct snd* snd)
{
#ifdef SOUND_ENABLED
    snd->stop = true;
    Pa_StopStream(snd->stream);
#endif
}