#define _GNU_SOURCE
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

// --- CONFIGURATION ---
#define DOORBELL_WAV  "audio-files/dingdong.wav"
#define ALARM_WAV     "audio-files/alarm.wav"
#define CORRECT_WAV   "audio-files/correct.wav"
#define INCORRECT_WAV "audio-files/incorrect.wav"

#define APLAY_CMD "aplay -q"
#define QUEUE_SIZE 16 

static pthread_t sound_thread;
static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sound_cond  = PTHREAD_COND_INITIALIZER;

static bool running = true;
static int command_queue[QUEUE_SIZE];
static int head = 0; 
static int tail = 0; 

static bool is_queue_empty() { return head == tail; }
static bool is_queue_full()  { return ((tail + 1) % QUEUE_SIZE) == head; }

static void* sound_thread_func(void* args) {
    (void)args;
    while (running) {
        int current_cmd = 0;

        pthread_mutex_lock(&sound_mutex);
        while (is_queue_empty() && running) {
            pthread_cond_wait(&sound_cond, &sound_mutex);
        }

        if (!running) {
            pthread_mutex_unlock(&sound_mutex);
            break;
        }

        current_cmd = command_queue[head];
        head = (head + 1) % QUEUE_SIZE;
        pthread_mutex_unlock(&sound_mutex);

        char cmd_buffer[512];
        const char* file_to_play = NULL;

        switch (current_cmd) {
            case 1: file_to_play = DOORBELL_WAV; break;
            case 2: file_to_play = ALARM_WAV; break;
            case 3: file_to_play = CORRECT_WAV; break;
            case 4: file_to_play = INCORRECT_WAV; break;
            case 5: // Stop
                system("killall -q aplay"); 
                continue; 
        }

        if (file_to_play) {
            snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s &", APLAY_CMD, file_to_play);
            system(cmd_buffer);
        }
    }
    return NULL;
}

void sound_init(void) {
    running = true;
    head = 0;
    tail = 0;
    pthread_create(&sound_thread, NULL, sound_thread_func, NULL);
}

void sound_cleanup(void) {
    pthread_mutex_lock(&sound_mutex);
    running = false;
    pthread_cond_signal(&sound_cond);
    pthread_mutex_unlock(&sound_mutex);
    pthread_join(sound_thread, NULL);
}

static void queue_sound(int cmd) {
    pthread_mutex_lock(&sound_mutex);
    if (!is_queue_full()) {
        command_queue[tail] = cmd;
        tail = (tail + 1) % QUEUE_SIZE;
        pthread_cond_signal(&sound_cond);
    }
    pthread_mutex_unlock(&sound_mutex);
}

void sound_play_doorbell(void)  { queue_sound(1); }
void sound_play_alarm(void)     { queue_sound(2); }
void sound_play_correct(void)   { queue_sound(3); }
void sound_play_incorrect(void) { queue_sound(4); }
void sound_stop(void)           { queue_sound(5); }