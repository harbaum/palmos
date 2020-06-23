/*
    run.c - axxPac explorer launch functions

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "../api/axxPac.h"

#include "explorerRsc.h"
#include "explorer.h"
#include "callback.h"
#include "list_dir.h"
#include "db.h"
#include "run.h"

#define CARD 0     /* only work with card 0 */

/* Zwei Moeglichkeiten: 
   Start ueber prc file
   Start ueber run file

   prc: Lies und installier prc, extrahiere creator und
        installiere alle Datenbanken mit gleichem Creator
        - abscannen der Datenbanken dauert ggf. recht lange
	Danach PRC starten.

   run: Lies run-file:
        Run:  nasenbaer.prc
        Uses: nasebaer.pdb
        Uses: nase2.pdb
	Save: unused.pdb

        Installiert Run und Uses-Teile und startet das.


   Run-Files from Memo. Dafuer 'import memo'.
*/

/* execute program (in working directory) */
Boolean run(char *filename) {
  DEBUG_MSG("exec %s\n", filename);

  return true;     /* success */
}
