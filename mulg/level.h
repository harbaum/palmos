// level.h

// def's to simplify level design

// attributes
#define ATTRIB_ST    0xff   // start for path
#define ATTRIB_RV    0x80   // reversed for path & ice
#define ATTRIB_UR    0x40   // unreversed for path & ice
#define ATTRIB_VA    0x20   // vanish for path
#define ATTRIB_R     0x80   // rotating for ventilator
#define ATTRIB_PT    PATH   // BOX with PATH under it

#define VANSH   (PATH | (ATTRIB_VA<<8))
#define START   (PATH | 0xff00)
#define SW      SWITCH_ON
#define SLIFE   (SWITCH_ON | 0xff00)  // start game of life
#define KH      KEYHOLE
#define DOR     HDOOR_1
#define DOOR    HDOOR_1
#define VDOR    VDOOR_1
#define SLT     SLOT
#define ODOOR   (HDOOR_4   | 0x8000)
#define ODOR    (HDOOR_4   | 0x8000)
#define HDOOR   HDOOR_1
#define VENT    (HVENT_1   | 0x8000)
#define VNT     (HVENT_1   | 0x8000)
#define LOCH    SPACE
#define VDOOR   VDOOR_1
#define DC      DOC
#define DIC1    DICE1
#define DIC2    DICE2
#define DIC3    DICE3
#define DIC4    DICE4
#define DIC5    DICE5
#define DIC6    DICE6
#define BPATH   (BOX | (PATH<<8))
#define LBPATH  (LBOX | (PATH<<8))
#define LBKEY   (LBOX | (KEY<<8))
#define PTH     PATH
#define SPC     SPACE
#define BUT     BUT0
#define F0      FLIP0
#define F1      FLIP1

#define GRH     GRLR
#define GRV     GRTB
#define GRX     GRLTRB

#define WAH     WALR
#define WAV     WATB
#define WAX     WALTRB

// short versions
#define O START
#define S STONE
#define X GOAL
#define P PATH
#define Sc0   (SCARAB0 | (PATH<<8))
#define Sc90  (SCARAB90| (PATH<<8))
#define Sc180 (SCARAB180| (PATH<<8))
#define Sc270 (SCARAB270| (PATH<<8))

// memorize tiles
#define MEM0  (MEM_Q)
#define MEM1  (MEM_Q | (0x40<<8))
#define MEM2  (MEM_Q | (0x80<<8))
#define MEM3  (MEM_Q | (0xc0<<8))

// walkers with path
#define PMOV_R  (MOVE_R | (PATH<<8))
#define PMOV_L  (MOVE_L | (PATH<<8))
#define PMOV_U  (MOVE_U | (PATH<<8))
#define PMOV_D  (MOVE_D | (PATH<<8))
