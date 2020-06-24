/*
  mktexts.c

  text ressource generator for Elite for PalmOS
*/

/*
  and generate compressed and hidden code for time/user limit 

  The generated code has to contain 4e56ff (link a6,#-xx) 
  in the first few bytes. This is the entry point.
*/

#include <stdio.h>

#ifdef LIMIT_TIME
#include "limit.h"

#define TIME_LIMIT EXPIRE_TIME_ASC
#endif

char *messages[]= {
  /* id           text */
  "INCOMING",     "INCOMING MESSAGE",
  "BUT_CONT",     "Tap screen to continue.",
  "COBRA_POCKET", "The Cobra in Your Pocket",

  /* mission texts */
  "MPD1",         "THE CONSTRICTOR WAS LAST SEEN AT REESDICE, COMMANDER.",
  "MPD2",         "A STRANGE LOOKING SHIP LEFT HERE A WHILE BACK. "
                  "LOOKED BOUND FOR AREXE.",
  "MPD3",         "YEP, AN UNUSUAL NEW SHIP HAD A GALACTIC HYPERDRIVE "
                  "FITTED HERE, USED IT TOO.",
  "MPD4",         "I HEAR A WEIRD LOOKING SHIP WAS SEEN AT ERRIUS.",
  "MPD5",         "THIS STRANGE SHIP DEHYPED HERE FROM NOWHERE, SUN "
                  "SKIMMED AND JUMPED. I HEAR IT WENT TO INBIBE.",
  "MPD6",         "ROGUE SHIP WENT FOR ME AT AUSAR. MY LASERS DIDN'T "
                  "EVEN SCRATCH ITS HULL.",
  "MPD7",         "OH DEAR ME YES. A FRIGHTFUL ROGUE WITH WHAT I BELIEVE "
                  "YOU PEOPLE CALL A LEAD POSTERIOR SHOT UP LOTS OF THOSE "
                  "BEASTLY PIRATES AND WENT TO USLERI.",
  "MPD8",         "YOU CAN TACKLE THE VICIOUS SCOUNDREL IF YOU LIKE. HE'S "
                  "AT ORARRA.",
  "MPD9",         "THERE'S A REAL DEADLY PIRATE OUT THERE.",
  "MPD10",        "BOY ARE YOU IN THE WRONG GALAXY!",

  /* constrictor mission briefing */
#ifdef LIMIT_NAME
  "CMBA",         "Greetings Commander, I am Captain " USER_NAME " of "
#else
  "CMBA",         "Greetings Commander, I am Captain Curruthers of "
#endif
                  "Her Majesty's Space Navy and I beg a moment of your "
                  "valuable time.  We would like you to do a little job "
                  "for us.  The ship you'll see on the next screen is a "
                  "new model, the Constrictor, equiped with a top secret "
                  "new shield generator. Unfortunately it has been stolen.",

  "CMBB",         "The Constrictor went missing from our ship yard on Xeer "
                  "five months ago and was last seen at Reesdice. Your "
                  "mission should you decide to accept it, is to seek "
                  "and destroy this ship. You are cautioned that only "
                  "Military Lasers will get through the new shields and "
                  "that the Constrictor is fitted with an E.C.M. System. "
                  "Good Luck, Commander. ---MESSAGE ENDS.",
  
  "CMBC",         "The Constrictor went missing from our ship yard on "
                  "Xeer five months ago and is believed to have jumped "
                  "to this galaxy. Your mission should you decide to accept "
                  "it, is to seek and destroy this ship. You are cautioned "
                  "that only Military Lasers will get through the new "
                  "shields and that the Constrictor is fitted with an "
                  "E.C.M. System. Good Luck, Commander. ---MESSAGE ENDS.",

  /* constrictor mission debriefing */
  "CMD",          "There will always be a place for you in Her Majesty's "
                  "Space Navy. And maybe sooner than you think... "
                  "---MESSAGE ENDS.",

  /* thargoid mission first briefing */
  "TMFB",         "Attention Commander, I am Captain Fortesque of Her "
                  "Majesty's Space Navy. We have need of your services "
                  "again. If you would be so good as to go to Ceerdi "
                  "you will be briefed.If succesful, you will be rewarded. "
                  "---MESSAGE ENDS.",

  /* thargoid mission second briefing */
  "TMSBA",        "Good Day Commander. I am Agent Blake of Naval "
                  "Intelligence. As you know, the Navy have been keeping "
                  "the Thargoids off your ass out in deep space for many "
                  "years now. Well the situation has changed. Our boys are "
                  "ready for a push right to the home system of those "
                  "murderers.",

  "TMSBB",        "I have obtained the defence plans for their Hive Worlds. "
                  "The beetles know we've got something but not what. If "
                  "I transmit the plans to our base on Birera they'll "
                  "intercept the transmission. I need a ship to make the "
                  "run. You're elected. The plans are unipulse coded within "
                  "this transmission. You will be paid. Good luck Commander. "
                  "---MESSAGE ENDS.",

  /* thargoid mission debriefing */
  "TMDB",         "You have served us well and we shall remember. "
                  "We did not expect the Thargoids to find out about you."
                  "For the moment please accept this Navy Extra Energy "
                  "Unit as payment. ---MESSAGE ENDS.",

  "CONGRAT",      "Congratulations Commander!",

  "WELLDONE",     "Well done Commander.",

  "URL",          "http:\015www.harbaum.org\014till\014palm\014elite",

  "VERSION",      "Elite for Palm OS " VERSION ,
  "VERSION_DATE", "Elite for Palm OS " VERSION " - " __DATE__ ,

  "COPYRIGHT1",   "Original Game (c) 1984 I.Bell & D.Braben",
  "COPYRIGHT2",   "Re-engineered 1999-2001 C.J.Pinder",
  "COPYRIGHT3",   "Palm OS Version (c) 2002 T.Harbaum",

#ifdef LIMIT_TIME
  "LIM_TIM_MSG",  "Limited to " TIME_LIMIT,
  "LIMIT",        "This version of Elite for Palm OS is time limited "
                  "and has expired. Please contact palm@harbaum.org "
                  "to obtain a new version.",
#endif

#ifdef LIMIT_NAME
  "LIM_USR_MSG",  "Registered to " USER_NAME,
  "LIMIT",        "This version of Elite for Palm OS is registered "
                  "to a different user. Please contact palm@harbaum.org "
                  "to obtain a version registered to you.",
#endif

#ifdef INTERNAL_TEXT
  "INTERN",       "Internal version, do not distribute!",
#endif

  "",             ""
};

int main(int argc, char **argv) {
  FILE *head, *data, *limit;
  int i,offset,msgs=0;

  printf("Elite text ressource generator\n");

  head = fopen(argv[1], "w");
  data = fopen(argv[2], "wb");

  for(msgs=0,i=0;messages[i][0]!=0;i+=2,msgs++);
  printf("converting %d messages ...\n", msgs);

  if(argc > 3)
    printf("adding/hiding one limit code segment\n");

  /* space for offset table */
  offset = 0;

  for(i=0;messages[i][0]!=0;i+=2) {
    /* write header file */
    fprintf(head, "#define TXT_%s %d\n", messages[i], offset);
    /* write the strings */
    fwrite(messages[i+1], 1l, strlen(messages[i+1])+1, data);

    offset += strlen(messages[i+1])+1;
  }

  if(argc > 3) {
    unsigned char cbuffer[1024];
    int clen, coffset;

    if((limit = fopen(argv[3], "rb")) == NULL) {
      fprintf(stderr, "unable to open limit file\n");
      return -1;
    }

    /* read limit code */
    memset(cbuffer, 1024, 0);
    clen = fread(cbuffer, 1l, 1024l, limit);
    coffset = 0;

    /* 4e56ff (link a6,#-xx) */
    while(((cbuffer[coffset+0]!=0x4e)||
	   (cbuffer[coffset+1]!=0x56)||
	   (cbuffer[coffset+2]!=0xff))&&
	  (coffset < 1000)) coffset++;
    
    if(coffset == 1000) {
      fprintf(stderr, "WARNING: unable to find limit code entry, using 0\n");
      coffset = 0;
    }

    /* enforce even address */
    if((offset + coffset) & 1) {
      offset += 1;
      fputc(0,data);
    }

    printf("limit: code length = %d, code offset = %d\n", clen, coffset);
    fprintf(head, "#define LIMIT_OFFSET %d\n", offset + coffset);

    /* write the code itself */
    fwrite(cbuffer, 1l, clen, data);

    fclose(limit);
  }

  fclose(head);
  fclose(data);

  return 0;
}
