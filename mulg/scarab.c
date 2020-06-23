/* 
  Scarab bots code.

   (c) 1999    Pat Kane
*/

#ifdef OS35HDR
#include <PalmOS.h>
#else
#include <Pilot.h>
#endif

#include "winx.h"

#include "rsc.h"
#include "mulg.h"
#include "tiles.h"

extern unsigned int marble_x,  marble_y;
extern unsigned int marble_xp,  marble_yp;
extern          int marble_sx,  marble_sy;
extern unsigned char  level_buffer[MAX_HEIGHT][MAX_WIDTH];  
extern unsigned char attribute_buffer[MAX_HEIGHT][MAX_WIDTH];
extern unsigned short level_width, level_height;
extern void change_tile(unsigned short tile, int attribute, int x, int y);
extern void snd_clic(void);
extern char debug_msg[];

#define DebugPr(format, args...) {StrPrintF(debug_msg, format, ## args);}

#define MAX_SCARABS 4
#define N_JITTERBUG 3                     /* how soon bug gets bored    */
#define MAX_JITTER  ((N_JITTERBUG*2)-1)   /* do let bug get too wired   */
#define BUGACC      300                   /* how hard bug pushes marble */
 
int n_scarabs = 0;



struct Scarab
{
  int  x,  y;
  int  vx, vy;
  int  jitter;     /* boredom count */
  char alive;
  int  hold;       /* can hold one thing */
} scarabs[MAX_SCARABS];



init_scarabs( )
{
  int i, j;

  for ( i = 0; i < MAX_SCARABS; i++)
  {
    scarabs[i].x      = 0;
    scarabs[i].y      = 0;
    scarabs[i].vx     = 0;
    scarabs[i].vy     = 0;
    scarabs[i].jitter = 0;
    scarabs[i].alive  = 0;
    scarabs[i].hold   = 0;
  }  
  n_scarabs = 0;

  for ( i = 0; i < MAX_HEIGHT; i++ )
  {
   for ( j = 0; j < MAX_WIDTH; j++ )
   {
     if ( level_buffer[i][j] == SCARAB0
       || level_buffer[i][j] == SCARAB90
       || level_buffer[i][j] == SCARAB180
       || level_buffer[i][j] == SCARAB270)
     {
       add_scarab( j, i );
     }  
   }
  }
}



add_scarab( int x, int y )
{
  int i;

  if ( n_scarabs >= MAX_SCARABS )
    return 0;
  for ( i = 0; i < MAX_SCARABS; i++)
  {
    if (scarabs[i].alive)
      continue;
    else
    {
      scarabs[i].x      = x;
      scarabs[i].y      = y;
      scarabs[i].vx     = 0;
      scarabs[i].vy     = 0;
      scarabs[i].jitter = 0;
      scarabs[i].alive  = 1;
      scarabs[i].hold   = 0;
      n_scarabs++;
      break;
    }
  }  
}


do_scarabs()
{
  int i;
  int x, y;
  int vx, vy;
  int dx, dy;
  int nx, ny;
  int mx, my;   /* marble tile position */
  int ntile;
  static int z;
  if ( n_scarabs == 0 )
    return;

  for ( i = 0; i < MAX_SCARABS; i++)
  {
    unsigned char st;
    /*
     * Scarab AI  (they are pretty dumb)
     */
    if( scarabs[i].alive )
    {
      x = scarabs[i].x;
      y = scarabs[i].y;
      mx = (marble_x>>8)/16 + marble_xp;
      my = (marble_y>>8)/16 + marble_yp;

      /*
       * delta to marble
       */
      dx =  mx - x ;
      dy =  my - y ;
      if ( ABS(dx)  > 1 || ABS(dy) > 1)
      {
	vx  = sign( dx );
	vy  = sign( dy );
      }
      else 
	vx = vy = 0;

      /* 
       * Is bug bored of not moving? 
       * if so it moves in  random direction
       */
      if ( scarabs[i].jitter > N_JITTERBUG )
      {
	vx = 1 - (SysRandom(0) % 3);
	vy = 1 - (SysRandom(0) % 3);
	if (scarabs[i].jitter > MAX_JITTER )
	  scarabs[i].jitter = MAX_JITTER;     /* not too jittery */
      }

      /*
       * Don't let them move on a diagonal,
       * I do this for two reasons: 
       *   2) too lazy to make 45 degree tiles
       *   1) it kinda looked odd ...
       */
      if ( vx && vy ) {
	if (SysRandom(0)%2)
	  vy = 0;
	else
	  vx = 0;
      }


      if ( vx ||  vy )
      {
	nx = x + vx;
	ny = y + vy;

	/* wrap coordinate (is this needed?) */
	nx = nx  % level_width;
	ny = ny  % level_height;

        /*
         * get tile based on direction bug might move
         */
	if ( vx < 0 ) ntile = SCARAB270;
	if ( vx > 0 ) ntile = SCARAB90;
	if ( vy < 0 ) ntile = SCARAB0;
	if ( vy > 0 ) ntile = SCARAB180;
	level_buffer[y][x] = ntile;

	/* 
         * If bug will hit it, nudge the marble.
         * 
         * If bug is holding a key it will drop it near marble
         */
	if ( nx == mx && ny == my )
	{
	  marble_sx += vx * BUGACC;
	  marble_sy += vy * BUGACC;
	  scarabs[i].jitter = 0;  /* that was a lot of fun */
	  if (scarabs[i].hold && (attribute_buffer[y][x] == PATH) )
	  {
	    attribute_buffer[y][x] = scarabs[i].hold;
	    scarabs[i].hold = 0;
	  }
	}

	/*
         * check what bug might move onto
         */
        st = level_buffer[ny][nx];
	if (  st == PATH || st == KEY 
           || st == VAN0 || st == VAN1 || st == VAN2 )
	{
	  /* 
           * remove old
           */
	  level_buffer[y][x] = attribute_buffer[y][x];
  	  attribute_buffer[y][x] = 0;

	  /*
           * if bug is bored, he will pick up keys to play with
           */
	  if ( st == KEY && scarabs[i].hold == 0 && scarabs[i].jitter)
	  {
	    scarabs[i].hold = KEY;
	    attribute_buffer[ny][nx] = PATH;
	    if ( scarabs[i].jitter-- < 0)  /* fun, not as bored now */
	      scarabs[i].jitter = 0;
	  }
	  else
	  {
	    attribute_buffer[ny][nx] = level_buffer[ny][nx];
	  }
          level_buffer[ny][nx] = ntile;

  	  scarabs[i].x = nx;
	  scarabs[i].y = ny;
          scarabs[i].vx = 0;
          scarabs[i].vy = 0;
	  if ( scarabs[i].jitter-- < 0)  /* not as bored now */
	    scarabs[i].jitter = 0;

        }
	/*
         * bugs can hit switches
         */
	else if ( st == SWITCH_ON || st == SWITCH_OFF )
	{
	  switch_it(attribute_buffer[ny][nx]);

	  /* change visible state of switch */
	  swap_tile(SWITCH_ON, SWITCH_OFF, nx, ny);

	  if ( scarabs[i].jitter--  < 0)  /* fun, not as bored now */
	    scarabs[i].jitter = 0;
	}
	else
	  scarabs[i].jitter++; /* getting bored of being blocked*/
      }
      else
	scarabs[i].jitter++; /* getting bored of not moving*/
    }
  }
}


int 
sign(int v)
{
   if (v < 0)
     return -1;
   else if (v > 0)
     return 1;
   else
     return 0;
 }

