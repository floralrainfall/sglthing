#ifndef MUS_H
#define MUS_H

#include "snd.h"

#define MUS_TRACK_COUNT 8
#define FRAMES_PER_PATTERN 32
#define SAMPLES_PER_FILE 255
#define PATTERNS_PER_FILE 255
#define LOADED_SAMPLE_ID 0x7373

struct mus_frame {
    char sample;    
    char note;
    char pitch;
    char gain;
    int cmd;
};

struct mus_pattern {
    struct mus_frame frames[FRAMES_PER_PATTERN][MUS_TRACK_COUNT];
};

struct mus_sample {
    char sample_path[64];
    uint16_t load;
};

struct mus_file {
    char name[64];
    char copyright[64];
    int patterns;
    int speed;    
    int pattern_map[PATTERNS_PER_FILE];
    struct mus_pattern pattern_data[PATTERNS_PER_FILE];
    struct mus_sample sample_data[SAMPLES_PER_FILE];
};

struct musmgr {
    struct mus_file* current_music;
    struct mus_file* scratch_song;
    struct sndmgr* sndmgr;
    void* world_ptr;

    int current_pattern;
    int pattern_offset;
    bool dbgmgr;

    bool playing;

    double next_tick;

    ALuint sources[MUS_TRACK_COUNT];
    struct mus_sample* playing_samples[MUS_TRACK_COUNT];
};

void mus_init(struct musmgr* mgr, struct sndmgr* s_mgr);
struct mus_file* load_mus_file(char* file_name);
void mus_tick(struct musmgr* mgr);
void mus_set_file(struct musmgr* mgr, struct mus_file* file);

#endif