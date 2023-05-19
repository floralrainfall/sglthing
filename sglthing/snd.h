#ifndef SND_H
#define SND_H
#include <portaudio.h>
#include <sndfile.h>

#include <stdbool.h>
#include <cglm/cglm.h>
#include <glib.h>

struct sndmgr {
#ifdef SOUND_ENABLED
    bool mute;
    GArray* sounds_loaded;
#endif
};

#define MAX_BUFFER_COUNT 8
struct snd {
#ifdef SOUND_ENABLED
    char name[64];
    int id;

    SF_INFO libsndfile_info;
    SNDFILE* file;
    PaStream* stream;

    int position;
    bool loop;
    bool stop;
    bool lock;

    float multiplier;

    bool sound_3d;    
    vec3 sound_position;
    vec3* camera_position;
#endif
};  

void snd_init(struct sndmgr* mgr);
void snd_deinit(struct sndmgr* mgr);
struct snd* get_snd(char* file_name);
struct snd* load_snd(char* file_name);
struct snd* play_snd(char* file_name);
void resume_snd(struct snd* snd);
void stop_snd(struct snd* snd);

#endif