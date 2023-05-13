#ifndef SND_H
#define SND_H
#include <portaudio.h>
#include <sndfile.h>

#include <stdbool.h>
#include <cglm/cglm.h>
#include <glib.h>

struct sndmgr {
#ifdef SOUND_ENABLED

#endif
};

#define MAX_BUFFER_COUNT 8
struct snd {
#ifdef SOUND_ENABLED
    char name[64];

    SF_INFO libsndfile_info;
    SNDFILE* file;
    PaStream* stream;

    int position;
    bool loop;
#endif
};  

void snd_init(struct sndmgr* mgr);
struct snd* get_snd(char* file_name);
void load_snd(char* file_name);
void play_snd(char* file_name);

#endif