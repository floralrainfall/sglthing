#ifndef SND_H
#define SND_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <stdbool.h>
#include <cglm/cglm.h>

struct sndmgr {
    ALCdevice* device;
    ALCcontext* context;
    ALuint global_source;
};

#define MAX_BUFFER_COUNT 8
struct snd {
    char name[64];
    AVFormatContext* format_context;
    AVCodec *codec;
    AVCodecParameters *codec_params;
    AVCodecContext* codec_context;
    int audio_stream_index;
    ALuint al_buffers[MAX_BUFFER_COUNT];
};  

void snd_init(struct sndmgr* mgr);
void snd_stop(struct sndmgr* mgr);
void snd_meta_view(struct sndmgr* mgr, vec3 p, vec3 v, vec3 u, vec3 l);
void load_snd(char* file_name);
struct snd* get_snd(char* file_name);
void play_snd(struct sndmgr* mgr, int source, char* file_name, float delta_time);

#endif