
#include <Pilot.h>
#include <string.h>
#include "MensaRsc.h"

#define HelloWorldAppType 'Mnsa'

/////////////////////////
// Function Prototypes
/////////////////////////

static void    StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/////////////////////////
// StartApplication
/////////////////////////

static MenuBarPtr CurrentMenu = NULL;
DateType date;
int mode, mensa;
DateFormatType dateformat;

extern char *mampf[], *beethoven[];

#define EssenX        0
#define EssenY       16
#define EssenWidth  160
#define EssenHeight 132

#define EssenSpace    3
#define EssenIndent  10

char entfaellt[]="-- entf\344llt --";

int show_one_essen(int no, int essen_no, int x, int y, int width) {
  char essen_t[16],*msg;
  int  rest,start=0;

  if(mensa==0) msg=mampf[essen_no];
  else {
    if(beethoven[essen_no][0] == 0)
      msg = entfaellt;
    else
      msg = beethoven[essen_no];
  }

  /* erstmal in schwarz die Essennummer ausgeben */
  FntSetFont(1);
  if(no==0)       StrPrintF(essen_t, "EXTRA: ");
  else if(no==-1) StrPrintF(essen_t, "Beilagen: ");
  else            StrPrintF(essen_t, "Essen %d: ",no);

  WinDrawChars(essen_t, StrLen(essen_t), x, y);
  rest = EssenWidth - FntLineWidth(essen_t, StrLen(essen_t));

  FntSetFont(0);

  do {
    /* don't draw out outside the Essen-area */
    if(y<=(EssenHeight-FntLineHeight()))
      WinDrawChars(&msg[start], FntWordWrap(&msg[start], rest), x + width - rest, y);

    start+=FntWordWrap(&msg[start],rest);
    while(msg[start]==' ') start++;  /* spaces ueberlesen */ 
    y+=FntLineHeight();
    rest=width-EssenIndent;
  } while(msg[start]!=0);

  return y+EssenSpace;
}

const char *sonntag[]={ "Am Sonntag bleiben", "alle Mensen geschlossen!",""};
const char *samstag[]={ "Am Samstag gibt", "es nur Essen 1,2 und 3!",""};
const char *freitag[]={ "Am Freitag bleibt", "die Mensa abends", "geschlossen!",""};
const char *beetsam[]={ "Am Samstag bleibt die", "Mensa Beethovenstra\337e", "geschlossen", "" };
const char abend[]="Abendmensa:";

void extra_message(const char *msg[]) {
  int i=0; 
  FntSetFont(1);
  while(msg[i][0]!=0) { 
    WinDrawChars(msg[i], StrLen(msg[i]), 
		 80 - (FntLineWidth(msg[i], StrLen(msg[i]))/2), 
		 60+FntLineHeight()*i);
    i++;
  }
}

char *weekdays[]={"So","Mo","Di","Mi","Do","Fr","Sa","So"};

void show_essen(void) {
  static char str[longDateStrLength+4];
  RectangleType rect;
  int i,y, offset,week, day;
  FormPtr frm;

  /* dateFormatType sollte aus Pref geholt werden */
  FntSetFont(0);

  StrPrintF(str, "%s. ", weekdays[DayOfWeek(date.month, date.day, date.year + firstYear)]);
  DateToAscii(date.month, date.day, date.year + firstYear, dateformat, &str[4]);

  frm = FrmGetActiveForm ();
  CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaSelect)), str);

  day  = DayOfWeek(date.month, date.day, date.year+firstYear) ;

  if(mensa==0)  week = (((DateToDays(date)-3)/7)+2)%7;
  else          week = (((DateToDays(date)-3)/7)+1)%9;

  rect.topLeft.x=EssenX; rect.topLeft.y=EssenY; 
  rect.extent.x=EssenWidth; rect.extent.y=EssenHeight;
  WinEraseRectangle(&rect,0);

  if(day==0)
    extra_message(sonntag);

  /* Katharinenstrasse */
  if(mensa==0) {
    /* Montag bis Fr. Mittag sind einfach */
    if(((day>0)&&(day<5))||((day==5)&&(mode<2))) {
      
      if(mode==0) {
	/* Essen 1 bis 3 anzeigen */
	for(y=EssenY,i=0;i<3;i++)
	  y=show_one_essen(1+i, week*40+(day-1)*8+i, EssenX, y, EssenWidth);
      }

      if(mode==1) {
	/* Essen 4 + 5 */
	for(y=EssenY,i=0;i<2;i++)
	  y=show_one_essen(4+i, week*40+(day-1)*8+3+i, EssenX, y, EssenWidth);
      }

      if(mode==2) {
	/* Mo-Do abends */
	FntSetFont(1);
	WinDrawChars(abend,StrLen(abend),EssenX,EssenY);
	
	for(y=EssenY+FntLineHeight()+EssenSpace,i=0;i<3;i++)
	  y=show_one_essen(1+i, week*40+(day-1)*8+i+5, EssenX, y, EssenWidth);
      }
    }
    
    /* Freitag abend */
    if((day==5)&&(mode==2)) 
      extra_message(freitag);
    
    /* Samstag */
    if(day==6) {
      if(mode==0) {
	for(y=EssenY,i=0;i<2;i++)
	  y=show_one_essen(1+i, week*40+(day-1)*8+i-3, EssenX, y, EssenWidth);
	show_one_essen(0, week*40+(day-1)*8-1, EssenX, y, EssenWidth);
      } else {
	extra_message(samstag);
      }
    }
  } else {
    /* Beethovenstrasse */
    if(day==6)
      extra_message(beetsam);
    else {
      /* Mo - Fr */
      if((day>0)&&(day<6)) {
	for(y=EssenY,i=0;i<3;i++)
	  y=show_one_essen(1+i, week*20+(day-1)*4+i, EssenX, y, EssenWidth);

	show_one_essen(-1, week*20+(day-1)*4+3, EssenX, y, EssenWidth);	
      }
    }
  }
}

void add_days(int offset) {
  /* Datum in Tage umrechnen */
  DateDaysToDate(DateToDays(date)+offset,&date);
  show_essen();
}

static void StartApplication(void)
{
  int day;
  SystemPreferencesType sysPrefs;

  FrmGotoForm(frm_Main);

  mode=0;  /* default: Tagesmensa anzeigen */
  mensa=0; /* Katharinenstrasse */

  /* get system preferences for short date format */
  PrefGetPreferences(&sysPrefs);
  dateformat = sysPrefs.dateFormat;

  /* get todays date */
  DateSecondsToDate(TimGetSeconds(), &date);

  day=DayOfWeek(date.month, date.day, date.year+firstYear);
  if((day>0)&&(day<5)) { /* Mo - Do */
    if(((TimGetSeconds()/1800)%48)>=29) /* 29 = ab 14.30 Abendmensa als Default */
      mode=2;
  }
}

/////////////////////////
// MainFormHandleEvent
/////////////////////////

static Boolean MainFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  ListPtr lst;
  
  Boolean handled = false;
  SWord month, day, year;
  
  switch (event->eType) {
  case ctlSelectEvent:
    if(event->data.ctlEnter.controlID == MensaSelect) {
      /* Datumsauswahl */
      month = date.month; day=date.day; year=date.year+firstYear;
      if(SelectDayV10(&month, &day, &year, "Datumsauswahl")) {
	date.month=month; date.day=day; date.year=year-firstYear;
	show_essen();
      }
      handled=true;
    }
    break;
    
  case keyDownEvent:
    if(event->data.keyDown.chr == pageDownChr) {
      mode-=1;
      if(mensa==1) { mode=2; mensa=0; }
      if((mensa==0)&&(mode<0)) { mensa=1; mode=0; }
      
      frm = FrmGetActiveForm ();
      lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensenList));

      LstSetSelection(lst, mensa);
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensenPopup)), 
		  LstGetSelectionText(lst, mensa));

      if(mensa==1) 
	CtlEraseControl(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)));
      else {        
	lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaList));
	LstSetSelection(lst, mode);
	CtlShowControl(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)));
	CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)), 
		    LstGetSelectionText(lst, mode));
      }

      show_essen();
      handled = true;
    }
    
    if(event->data.keyDown.chr == pageUpChr) {
      mode+=1;
      if(mensa==1) { mode=0; mensa=0; }
      if((mensa==0)&&(mode>2)) { mensa=1; mode=2; }
      
      frm = FrmGetActiveForm ();
      lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensenList));

      LstSetSelection(lst, mensa);
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensenPopup)), 
		  LstGetSelectionText(lst, mensa));

      if(mensa==1)
	CtlEraseControl(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)));
      else {       
	lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaList));
	LstSetSelection(lst, mode);
	CtlShowControl(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)));
	CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)), 
		    LstGetSelectionText(lst, mode));
      }
      show_essen();
      handled = true;
    }
    break;
    
  case popSelectEvent: 
    if(event->data.ctlEnter.controlID == MensaPopup) {
      mode = event->data.popSelect.selection;
      show_essen();
    }

    if(event->data.ctlEnter.controlID == MensenPopup) {
      mensa = event->data.popSelect.selection;

      /* Bei Beethoven die Essenauswahl entfernen */
      frm = FrmGetActiveForm ();
      
      if(mensa==1) CtlEraseControl(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)));
      else         CtlShowControl(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)));
            
      show_essen();
    }
    break;
    
  case ctlRepeatEvent:
    if(event->data.ctlEnter.controlID == MensaLeft)
	add_days(-1);
    
    if(event->data.ctlEnter.controlID == MensaRight)
      add_days(1);
    break;
    
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    
    /* ggf. Popup aktualisieren */
    frm = FrmGetActiveForm ();
    CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaPopup)), 
		LstGetSelectionText(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, MensaList)),mode));
    
    show_essen();
    handled = true;
    break;
    
  case menuEvent:
    switch (event->data.menu.itemID) {
    case mit_About:
      frm=FrmInitForm(frm_About);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      handled = true;
      break;
    case mit_Info:
      FrmAlert(alt_Info);
      handled = true;
      break;
    case mit_Offen:
      frm=FrmInitForm(frm_Offen);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      handled = true;
      break;
    case mit_Preise:
      frm=FrmInitForm(frm_Preise);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      handled = true;
      break;
    }
  }
  
  return(handled);
}

/////////////////////////
// ApplicationHandleEvent
/////////////////////////

static Boolean ApplicationHandleEvent(EventPtr event)
{
  FormPtr form;
  Word    formId;
  Boolean handled = false;
  
  if (event->eType == frmLoadEvent) {
    formId = event->data.frmLoad.formID;
    form = FrmInitForm(formId);
    FrmSetActiveForm(form);
    
    if(formId ==frm_Main)
      FrmSetEventHandler(form, MainFormHandleEvent);

    handled = true;
  }
  
  return handled;
}

/////////////////////////
// EventLoop
/////////////////////////

static void EventLoop(void)
{
    EventType event;
    Word      error;

    do
    {
        EvtGetEvent(&event, evtWaitForever);
        if (SysHandleEvent(&event))
            continue;
        if (MenuHandleEvent(CurrentMenu, &event, &error))
            continue;
        if (ApplicationHandleEvent(&event))
            continue;
        FrmDispatchEvent(&event);
    }
    while (event.eType != appStopEvent);
}

/////////////////////////
// PilotMain
/////////////////////////

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        StartApplication();
        EventLoop();
        return(0);
    }
    return(0);
}











