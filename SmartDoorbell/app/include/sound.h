#ifndef SOUND_H
#define SOUND_H

// Initialize the sound module (starts background thread)
void sound_init(void);

// Clean up resources and stop thread
void sound_cleanup(void);

// --- Sound Effects ---
void sound_play_doorbell(void);  // dingdong.wav
void sound_play_alarm(void);     // alarm.wav
void sound_play_correct(void);   // correct.wav
void sound_play_incorrect(void); // incorrect.wav

// Stop any currently playing sound
void sound_stop(void);

#endif