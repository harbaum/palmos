/*
 * Elite - The New Kind, PalmOS port
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder and T.Harbaum
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 * PalmOS port (c) 2002 by Till Harbaum <till@harbaum.org>
 *
 */

/*
 * sound.h
 */

#ifndef SOUND_H
#define SOUND_H

#define SND_LAUNCH		0
#define SND_CRASH		1
#define SND_DOCK		2
#define SND_GAMEOVER	3
#define SND_PULSE		4
#define SND_HIT_ENEMY	5
#define SND_EXPLODE		6
#define SND_ECM			7
#define SND_MISSILE		8
#define SND_HYPERSPACE	9
#define SND_INCOMMING_FIRE_1	10
#define SND_INCOMMING_FIRE_2	11
#define SND_BEEP		12
#define SND_BOOP		13

typedef struct {
  UInt16 freq;
  UInt16 duration;
} sound_type;

void snd_sound_startup (void);
void snd_sound_shutdown (void);
void snd_play_sample (int sample_no);
void snd_update_sound (void);

void snd_beep(void);
void snd_click(void);

#endif
