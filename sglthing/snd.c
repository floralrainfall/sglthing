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

static int snd_decode_buf(AVPacket *packet, AVCodecContext *codec_context, AVFrame *frame, char** buf, int* length)
{
    int response = avcodec_send_packet(codec_context, packet);
    if (response < 0) {
        printf("sglthing: error sending packet to encoder: '%s'\n", av_err2str(response));
        return response;
    }
    while(response >= 0)
    {
        response = avcodec_receive_frame(codec_context, frame);
        if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            break;
        else if(response < 0)
        {
            printf("sglthing: error receiving frame from encoder: '%s'\n", av_err2str(response));
            return response;
        }
        if (response >= 0) {
            int bytes;
            switch(codec_context->sample_fmt)
            {
                case AV_SAMPLE_FMT_U8:
                    bytes = 1;
                    break;
                case AV_SAMPLE_FMT_S16:
                    bytes = 2;
                    break;
                case AV_SAMPLE_FMT_S32:
                    bytes = 4;
                    break;
                default:
                    bytes = 0;
                    break;
            }
            *length = bytes;
            *buf = (char*)malloc(length);
            memcpy(*buf, frame->data, *length);
        }
    }
    return 0;
}

void load_snd(char* file_name)
{
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
}

void play_snd(struct sndmgr* mgr, int source, char* file_name, float delta_time)
{
    struct snd* snd = NULL;
    for(int i = 0; i < snd_count; i++)
    {
        if(strncmp(file_name,snd_cache[i].name,64)==0)
        {
            snd = &snd_cache[i];
        }
    }
    ASSERT(snd);
    ALint buf_processed;
    alGetSourcei(source,  AL_BUFFERS_PROCESSED,  &buf_processed);

    if(buf_processed)
    {
        AVFrame* frame = av_frame_alloc();
        AVPacket* packet = av_packet_alloc();

        int buf_to_load = MAX_BUFFER_COUNT;
        ALuint buffer;
        for(int i = 0; i < buf_to_load; i++)
        {
            if(av_read_frame(snd->format_context, packet))
            {
                if(packet->stream_index == snd->audio_stream_index)
                {
                    char* data = 0;
                    int data_sz;

                    int response = snd_decode_buf(packet, snd->codec_context, frame, &data, &data_sz);
                    if(response < 0)
                        break;
                    alSourceUnqueueBuffers(source,  1,  &buffer);
                    alBufferData(buffer,  AL_FORMAT_STEREO16,  data,  data_sz,  snd->codec_context->sample_rate);
                    alSourceQueueBuffers(source,  1,  &buffer);
                    if(data)
                        free(data);
                }
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        av_frame_free(&frame);
    }

    ALint play_status;
    alGetSourcei(source, AL_SOURCE_STATE, &play_status);
    if(play_status != AL_PLAYING)
    {
        alSourcePlay(source);
        alSourceQueueBuffers(source, MAX_BUFFER_COUNT, snd->al_buffers);
    }
}