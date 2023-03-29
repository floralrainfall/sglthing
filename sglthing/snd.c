#include "snd.h"
#include "io.h"
#include "sglthing.h"

#define CACHE_SIZE 512
struct snd snd_cache[CACHE_SIZE];
int snd_count = 0;

void snd_init(struct sndmgr* mgr)
{
    mgr->device = alcOpenDevice(NULL);
    ASSERT(mgr->device);
    mgr->context = alcCreateContext(mgr->device, NULL);
    ASSERT(alcMakeContextCurrent(mgr->context));
    // ASSERT(alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") != AL_FALSE);
    snd_meta_view(mgr, (vec3){0.f, 0.f, 0.f}, (vec3){0.f, 0.f, 0.f}, (vec3){0.f, 0.f, -1.f}, (vec3){0.f, 1.f, 0.f});
    alGenSources((ALuint)1, &mgr->global_source);
    alSourcef(mgr->global_source, AL_PITCH, 1);
    alSourcef(mgr->global_source, AL_GAIN, 1);
    alSource3f(mgr->global_source, AL_POSITION, 0, 0, 0);
    alSource3f(mgr->global_source, AL_VELOCITY, 0, 0, 0);
    alSourcei(mgr->global_source, AL_LOOPING, AL_FALSE);
}

void snd_meta_view(struct sndmgr* mgr, vec3 p, vec3 v, vec3 u, vec3 l)
{
    alListener3f(AL_POSITION, p[0], p[1], p[2]);
    alListener3f(AL_VELOCITY, v[0], v[1], v[2]);
    vec3 orient[2] = {{l[0], l[1], l[2]}, {u[0], u[1], u[2]}};
    alListenerfv(AL_ORIENTATION, orient);
}

struct snd* get_snd(char* file_name)
{
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
}

void load_snd(char* file_name)
{
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
    n_snd->format_context = avformat_alloc_context();
    ASSERT(n_snd->format_context);
    int ret = avformat_open_input(&n_snd->format_context, dest_path, NULL, NULL);
    if(ret != 0)
    {
        printf("sglthing: could not open snd %s (%s)\n", file_name, av_err2str(ret));
        return;
    }
    ret = avformat_find_stream_info(n_snd->format_context,  NULL);
    if(ret != 0)
    {
        printf("sglthing: could not find stream info (%s)\n", file_name, av_err2str(ret));
        return;
    }
    printf("sglthing: format '%s'\n", n_snd->format_context->iformat->name);    
    for(int i = 0; i < n_snd->format_context->nb_streams; i++)
    {
        AVCodecParameters *local_params = NULL;
        local_params = n_snd->format_context->streams[i]->codecpar;
        AVCodec *local_codec;
        local_codec = (AVCodec*)avcodec_find_decoder(local_params->codec_id);
        printf("sglthing: codec %s, id: %d, bit rate: %lld\n", local_codec->name, local_codec->id, local_params->bit_rate);
        if(local_codec == NULL)
        {
            printf("sglthing: unsupported codec %i\n", i);
            continue;
        }
        if(local_params->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("sglthing: codec is audio; sample rate: %d\n", local_params->sample_rate);
            n_snd->codec = local_codec;
            n_snd->codec_params = local_params;
            n_snd->audio_stream_index = i;
        }
        else
        {
            printf("sglthing: i dont know what codec type is %i\n", local_params->codec_type);
        }
    }
    n_snd->codec_context = avcodec_alloc_context3(n_snd->codec);
    ASSERT(n_snd->codec_context);
    ASSERT(avcodec_parameters_to_context(n_snd->codec_context, n_snd->codec_params) >= 0);  
    ASSERT(avcodec_open2(n_snd->codec_context, n_snd->codec, NULL) >= 0);
    alGenBuffers(MAX_BUFFER_COUNT, n_snd->al_buffers);  
    
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    int rfret = 0;
    while((rfret = av_read_frame(n_snd->format_context, packet)) >= 0){
        if(packet->stream_index == n_snd->audio_stream_index)
        {
            int ret = avcodec_send_packet(n_snd->codec_context, packet);
            if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                printf("sglthing: audio error %i\n", ret);
                break;
            }
            while(ret >= 0)
            {
                ret = avcodec_receive_frame(n_snd->codec_context, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_frame_unref(frame);
                    break;
                }
                printf("sglthing: [AUDIO DECODE] frame %i\n", n_snd->codec_context->frame_number);
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_packet_free(&frame);
}

void play_snd(struct sndmgr* mgr, int source, char* file_name, float delta_time)
{
    struct snd* snd = get_snd(file_name);
    ASSERT(snd);
    ALint buf_processed;
    alGetSourcei(source,  AL_BUFFERS_PROCESSED,  &buf_processed);

    if(buf_processed)
    {

    }

    ALint play_status;
    alGetSourcei(source, AL_SOURCE_STATE, &play_status);
    if(play_status != AL_PLAYING)
    {
        alSourcePlay(source);
    }
}