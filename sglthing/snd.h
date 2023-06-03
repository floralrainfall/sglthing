#ifndef SND_H
#define SND_H
#include <portaudio.h>
#include <sndfile.h>

#include <stdbool.h>
#include <cglm/cglm.h>
#include <glib.h>
#include "memory.h"

#define MAX_SOUNDS 2

struct sndmgr {
#ifdef SOUND_ENABLED
    bool mute;
    GArray* sounds_playing;
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
    bool auto_dealloc;
    bool callback;
    vec3 sound_position;
    vec3* camera_position;
#endif
};  

void snd_init(struct sndmgr* mgr);
void snd_deinit(struct sndmgr* mgr);
void snd_frame(struct sndmgr* mgr);
void snd_rmref(struct sndmgr* mgr, struct snd* snd);
struct snd* get_snd(char* file_name);
struct snd* load_snd(char* file_name);
struct snd* play_snd(char* file_name);
struct snd*  play_snd2(struct snd* snd);
void resume_snd(struct snd* snd);
void stop_snd(struct snd* snd);
void kill_snd(struct snd* snd);

#endif