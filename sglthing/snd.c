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

static int snd_callback(void* input, void* output, unsigned long frame_count, const PaStreamCallbackTimeInfo* pa_time, PaStreamCallbackFlags status, void* user)
{
    struct snd* snd = (struct snd*)user;

    int *cursor;
    int *out = (int*)output;
    int sz = frame_count;
    int rd = 0;

    cursor = out;

    bool stop;
    while(sz > 0)
    {
        sf_seek(snd->file, snd->position, SEEK_SET);
        if (sz > (snd->libsndfile_info.frames - snd->position))
        {
            rd = snd->libsndfile_info.frames - snd->position;
            snd->position = 0;            
            if(!snd->loop)
                stop = true;
        }
        else
        {
            rd = sz;
            snd->position += rd;
        }

        sf_readf_int(snd->file, cursor, rd);
        cursor += rd;
        sz -= rd;
    }

    return stop ? paComplete : paContinue;
}

void load_snd(char* file_name)
{
#ifdef SOUND_ENABLED
    struct snd* o_snd = get_snd(file_name);
    if(o_snd)
        return;
        
    char dest_path[255];
    if(file_get_path(dest_path, 255, file_name) == -1)
    {
        printf("sglthing: file %s doesn't exist\n", file_name);
        return;
    }
    struct snd* n_snd = &snd_cache[snd_count];
    snd_count++;
    strncpy(n_snd->name, file_name, 64);

    n_snd->file = sf_open(dest_path, SFM_READ, &n_snd->libsndfile_info);
    printf("sglthing: sound file rate %i, %i channels, %i frames\n", 
        n_snd->libsndfile_info.samplerate,
        n_snd->libsndfile_info.channels,
        n_snd->libsndfile_info.frames);


    PaStreamParameters stream_param;
    stream_param.device = Pa_GetDefaultOutputDevice();
    stream_param.channelCount = n_snd->libsndfile_info.channels;
    stream_param.sampleFormat = paInt32;
    stream_param.suggestedLatency = 0.2;
    stream_param.hostApiSpecificStreamInfo = 0;

    PaError error = Pa_OpenStream(&n_snd->stream, NULL, &stream_param, n_snd->libsndfile_info.samplerate, paFramesPerBufferUnspecified, paNoFlag, snd_callback, n_snd);
    if(error)
        printf("sglthing: error opening stream (%i)\n", error);
#endif
}

void play_snd(char* file_name)
{
#ifdef SOUND_ENABLED
    struct snd* snd = get_snd(file_name);
    ASSERT(snd);
    Pa_StartStream(snd->stream);
#endif
}