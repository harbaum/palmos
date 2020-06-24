/*
 * Elite - The New Kind.
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder.
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 *
 */

/*
 * palm_snd.c
 */

#include <PalmOS.h>

#include "elite.h"
#include "sound.h"
#include "debug.h"
#include "options.h"
#include "gfx.h"

#define SND_OFF   -1
#define SND_START 0

int snd_state = SND_OFF;
sound_type *snd_which;

/* launch ship or escape capsule */
sound_type sound_data_launch[] = {
  {1000, 100}, {800, 100}, {1000, 100}, { 0, 0}
};

/* ship crashes into something */
sound_type sound_data_crash[] = {
  {30, 10}, {800, 10}, {0, 0}
};

/* we are docked */
sound_type sound_data_dock[] = {
  {100, 333}, {200, 333}, {300, 333}, {400, 1000}, {0, 0}
};

/* game over */
sound_type sound_data_over[] = {
  {392, 1000}, {349, 200}, {329, 1000}, {293, 200}, {261, 1000}, {0, 0}
};

/* laser has been fired */
sound_type sound_data_pulse[] = {
  {500, 10}, {0, 0}
};

/* enemy is hit (with pulse laser sound at start) */
sound_type sound_data_hit[] = {
  {500, 10}, {100, 100}, {0, 0}
};

/* something is exploding */
sound_type sound_data_explode[] = {
  {40, 100}, {30, 100}, {20, 100}, {10, 100}, 
  {0, 0}
};

/* ecm has been used */
sound_type sound_data_ecm[] = {
  { 400, 100}, { 500, 100}, { 600, 100}, { 700, 100}, 
  { 800, 100}, { 900, 100}, {1000, 100}, {1100, 100},
  {   0,   0}
};

/* a missile is laucnhed */
sound_type sound_data_msle[] = {
  {1100, 100}, {1000, 100}, { 900, 100}, { 800, 100}, 
  { 700, 100}, { 600, 100}, { 500, 100}, { 400, 100},
  {   0,   0}
};

/* hyperspace sound */
sound_type sound_data_hyper[] = { 
  {1200, 50}, {1100, 50}, {1000, 50}, { 900, 50},
  {1000, 50}, {1100, 50},
  {65535, SND_HYPERSPACE}       // this is a loop
};

/* we are hit and shields are up */
sound_type sound_data_inc1[] = {
  {400, 100}, {500, 100}, {400, 100}, {500, 100}, {0,0}
};

/* we are hit and shields are low */
sound_type sound_data_inc2[] = {
  {1200, 100}, {1300,100}, {1200, 100}, {1300, 100}, { 0,0}
};

/* high pitched sound */
sound_type sound_data_beep[] = {
  {1200, 100}, {0,0}
};

/* low pitched sound */
sound_type sound_data_boop[] = {
  {100, 100}, {0,0}
};

sound_type *sound[] = {
  sound_data_launch,  // 0  SND_LAUNCH
  sound_data_crash,   // 1  SND_CRASH
  sound_data_dock,    // 2  SND_DOCK
  sound_data_over,    // 3  SND_GAMEOVER
  sound_data_pulse,   // 4  SND_PULSE
  sound_data_hit,     // 5  SND_HIT_ENEMY
  sound_data_explode, // 6  SND_EXPLODE
  sound_data_ecm,     // 7  SND_ECM
  sound_data_msle,    // 8  SND_MISSILE
  sound_data_hyper,   // 9  SND_HYPERSPACE
  sound_data_inc1,    // 10 SND_INCOMMING_FIRE_1
  sound_data_inc2,    // 11 SND_INCOMMING_FIRE_2
  sound_data_beep,    // 12 SND_BEEP
  sound_data_boop     // 13 SND_BOOP
};

UInt16 game_volume;
Int16  snd_tick;

void
snd_sound_startup (void) {
  game_volume = PrefGetPreference(prefGameSoundVolume);

  DEBUG_PRINTF("sound volume = %d%% (%d/%d)\n", 
	       (100*game_volume)/sndMaxAmp, game_volume, sndMaxAmp);

  /* hmm, we need at least some volume, since the user may */
  /* enable sound manually, use 50% volume then ... */
  if(game_volume == 0) game_volume = sndMaxAmp/2;

  snd_state = SND_OFF;
}

void
snd_sound_shutdown (void) {
  SndCommandType silence = {sndCmdQuiet, 0,0,0,0};

  /* stop sound output */
  SndDoCmd(NULL, &silence, true); 
  snd_state = SND_OFF;
}

#define SND_TICKS (50)

void
snd_play_sample (int sample_no)
{
  DEBUG_PRINTF("play sample %d\n", sample_no);

  /* is this sound implemented? */
  if((sound[sample_no] != NULL)&&
     (options.bool[OPTION_SOUND])) {

    snd_tick  = 0;  // the first command is processed immediately
    snd_which = sound[sample_no];
    snd_state = SND_START;
  }
}

void
snd_update_sound (void)
{
  SndCommandType snd = {sndCmdFrqOn, 0, 0, 100, sndMaxAmp};

  if(snd_state != SND_OFF) {

    /* check if it's time to process the next sound thing */
    snd_tick -= ticks;  // use the acurate timer
    if(snd_tick <= 0) {

      /* is this a jump/loop command? */
      if(snd_which[snd_state].freq == 65535) {
	snd_which = sound[snd_which[snd_state].duration];
	snd_state = SND_START;	
      }

      /* timer for next sound event */
      snd_tick = snd_which[snd_state].duration * SND_TICKS / 100;

      if(snd_which[snd_state].freq) {
	/* process sound command */
	snd.param1 = snd_which[snd_state].freq;
	snd.param2 = snd_which[snd_state].duration;
	snd.param3 = game_volume;
	SndDoCmd(NULL, &snd, true); 
	snd_state++;
      } else
	snd_sound_shutdown();
    }
  }
}

/* just a system beep */
void snd_beep(void) {
  struct SndCommandType snd_beep =
    {sndCmdFreqDurationAmp,0,400,10,sndMaxAmp};

  if(options.bool[OPTION_SOUND]) {
    snd_beep.param3 = game_volume;
    SndDoCmd(NULL, &snd_beep, 0); 
  }
}

/* just a button click */
void snd_click(void) {
  struct SndCommandType snd_click =
    {sndCmdFreqDurationAmp,0,100,1,sndMaxAmp};

  if(options.bool[OPTION_SOUND]) {
    snd_click.param3 = game_volume;
    SndDoCmd(NULL, &snd_click, 0); 
  }
}
