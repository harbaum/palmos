%{
/*
    makelevel - (c) 1999-2001 by Till Harbaum
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define  DB_VERSION_MAJOR  '2'
#define  DB_VERSION_MINOR  '0'

#define IN_MAKELEVEL
#include "tiles.h"
#include "makelevel.h"
#include "mulg.h"

#include "tiles/coltab.h"

#define noDEBUG_CONST

#define MAX_IDENT  256
#define STR_LEN    32

#define MAX_LINES 33
#define MAX_ROWS 37
#define MAX_LEVELS 256

/* Palm<->Unix Time conversion */
#define PALM2UNIX(a)  (a - 2082844800)
#define UNIX2PALM(a)  (a + 2082844800)

char ident_name[MAX_IDENT][STR_LEN];
int  ident_value[MAX_IDENT];

int db_debug   = 0;
int db_version = 0;

char string_buffer[256];
char dbname[32];

void yyerrorf(int offset, const char *fmt, ...);
void enter_file(char *name);

int err=0;

typedef struct {
  char name[32];
  int width, height;
  unsigned short data[MAX_LINES][MAX_ROWS];
} level;

char doc[256][256];

/* the hole level data */
int   levels=0;
level *level_data[MAX_LEVELS];

/* level buffer */
level level_buffer;

/* line buffer */
int line_len, line_buffer[MAX_ROWS];

/* custom tiles */
#define MAX_CUSTOM_TILES 16
int custom_tiles=0;
char custom_tile_name[MAX_CUSTOM_TILES][64];
unsigned short custom_solid=0;

int get_ident_value(int *ret, char *name) {
  int i;

  for(i=0;i<MAX_IDENT;i++) {
    if(strncmp(ident_name[i],name,STR_LEN)==0) {
      *ret = ident_value[i];
      return 1;
    }
  }
  *ret=0;
  return 0;
}

int add_ident_value(int value, char *name) {
  int i;

  /* search for existing entry */
  for(i=0;i<MAX_IDENT;i++) {
    if(strncmp(ident_name[i],name,STR_LEN)==0) {
      ident_value[i]=value;
#ifdef DEBUG_CONST
      printf("stored %s=%d\n", name, value);
#endif
      return 1;
    }
  }

  /* create new entry */
  for(i=0;i<MAX_IDENT;i++) {
    if(ident_name[i][0]==0) {
      strncpy(ident_name[i],name,32);
      ident_value[i]=value;
#ifdef DEBUG_CONST
      printf("stored new %s=%d\n", name, value);
#endif
      return 1;
    }
  }
  return 0;
}

%}
%union {
  int val;
  char name[STR_LEN];
}
%left '+' '-' '&' '|' LEFTSHIFT, RIGHTSHIFT
%left '*' '/'
%right '~'
%token IDENTIFIER, INTEGER, DEFINE, EOL, HEXINT, EXTERN, STRING, LEVEL, DOCUMENT, AUTHOR
%token BININT, DBNAME, DBDEBUG, TSOLID, TPATH, TILE
%type <name> IDENTIFIER STRING
%type <val> expr tile INTEGER HEXINT attribute constant BININT ctype
%%
file    :
        | preproc file
        | error file
        ;

preproc : define
        | dbname
        | dbauthor
        | ctile
        | level
        | doc
        | EXTERN
        | EOL
        | DBDEBUG  { db_debug=1; }
        ;

ws      :
        | EOL ;

dbname  :  DBNAME STRING { if(dbname[0]!=0) yyerrorf(0, "database name redefined"); 
                           if(strlen(string_buffer)>32) yyerrorf(0, "database name must have 32 chars or less");
                           strcpy(dbname, string_buffer); }
        ;

dbauthor : AUTHOR STRING   {   if(doc[0][2]!=0) yyerrorf(0, "author name redefined"); 
                                  strcpy(&doc[0][2], string_buffer); 
                                  doc[0][0]=DB_VERSION_MAJOR; doc[0][1]=DB_VERSION_MINOR; }
        ;

doc     :  DOCUMENT expr STRING { if(($2<1)||($2>255)) yyerrorf(0, "document %d out of range (must be 1..255)", $2); 
                                  if(doc[$2][0]!=0) yyerrorf(0, "document %d redefined", $2); 
                                  strcpy(doc[$2], string_buffer); }
        ;

ctype   :  TSOLID { $$ = 1; }
        |  TPATH  { $$ = 0; };

ctile   :  TILE IDENTIFIER ctype STRING {
                                  if(db_version < 3) db_version = 3;

                                  if(custom_tiles >= MAX_CUSTOM_TILES-1)
                                     yyerrorf(0, "too many custom tiles (max %d)\n",
					         MAX_CUSTOM_TILES);

                                  if(!add_ident_value(255-custom_tiles, $2)) 
                                     yyerrorf(0, "too many identifiers\n");

				  strcpy(custom_tile_name[custom_tiles], string_buffer);

				  if($3) custom_solid |= (1<<custom_tiles);
				  
                                  custom_tiles++; };

level   :  LEVEL STRING ws '{' ws lines '}' 
                    { 
		      level_data[levels]=(level*)malloc(sizeof(level));
		      if(level_data[levels]==NULL) { yyerrorf(0,"out of memory"); exit(1); }

		      /* copy level into buffer */
		      memcpy(level_data[levels],&level_buffer,sizeof(level));

		      /* copy name */
		      strcpy(level_data[levels]->name, string_buffer);

		      levels++;
                    };

lines   :  line EOL {
                      level_buffer.width=line_len;
                      level_buffer.height=1;
                      /* copy line to level buffer */
                      { int i; for(i=0;i<line_len;i++) level_buffer.data[0][i]=line_buffer[i]; } 
                    }
        |  lines line EOL {
                      if(line_len!=level_buffer.width) 
                        yyerrorf(-1, "illegal line length of %d objects, must be %d", line_len, level_buffer.width);

                      /* copy line to level buffer */
                      { int i; for(i=0;i<line_len;i++) level_buffer.data[level_buffer.height][i]=line_buffer[i]; }
                      level_buffer.height++;
                    }
        ;

line    :  tile { line_len=0; line_buffer[line_len++]=$1; }
        |  line tile { line_buffer[line_len++]=$2; } 
        ;

tile    :  IDENTIFIER {   if(!get_ident_value(&$$, $1)) { 
                          yyerrorf(0, "undefined constant %s",$1);
                          $$=STONE; } }
        |  IDENTIFIER '.' attribute {
                          if(!get_ident_value(&$$, $1)) { 
                          yyerrorf(0, "undefined constant %s",$1);
                          $$=STONE; }
			  /* add attribute to upper 8 bit */
                          $$ |= ($3<<8);
                      }
        ;

attribute      :  IDENTIFIER
                          { char modname[32];
                            strcpy(modname,"ATTRIB_"); 
                            strcat(modname, $1);

			    if(!get_ident_value(&$$, modname)) { 
                              yyerrorf(0, "undefined attribute %s",modname);
                              $$=0; 
                            } }
               |  constant  { $$ = $1 }
               ;

define  :  DEFINE IDENTIFIER expr EOL { if(!add_ident_value($3, $2)) yyerror(0,"too many identifiers");  }
        ;

expr  	:  '(' expr ')'           { $$ = $2; }
	|  expr '+' expr          { $$ = $1 + $3; }
	|  expr '-' expr          { $$ = $1 - $3; }
	|  expr '*' expr          { $$ = $1 * $3; }
	|  expr '/' expr          { $$ = $1 / $3; }
	|  expr '&' expr          { $$ = $1 & $3; }
	|  expr '|' expr          { $$ = $1 | $3; }
	|  expr LEFTSHIFT  expr   { $$ = $1 << $3; }
	|  expr RIGHTSHIFT expr   { $$ = $1 >> $3; }
	|  '~' expr               { $$ = ~$2; }
	|  constant
	|  IDENTIFIER      { if(!get_ident_value(&$$, $1)) yyerrorf(0,"undefined constant %s",$1); }
	;

constant   :  INTEGER  { $$=$1; }
	   |  HEXINT   { $$=$1; }
           |  BININT   { $$=$1; }

%%
#include "ml_lex.c"

yyerror(char *s) {
  printf("error in file '%s' line %d: %s\n",fname[include_stack_ptr],line[include_stack_ptr],s);
  err=1;
}

void 
yyerrorf(int offset, const char *fmt, ...) {
  char p[128];
  va_list ap;
  va_start(ap, fmt);
  (void) vsprintf(p, fmt, ap);
  va_end(ap);
  printf("error in file '%s' line %d: %s\n",fname[include_stack_ptr],line[include_stack_ptr]+offset,p);
  err=1;
}

void 
WriteWord(void *addr, uword data) {
  ubyte *to = (ubyte*)addr;
  ubyte *from = (ubyte*)&data;

  to[0] = from[1]; to[1] = from[0];
}

void 
WriteDWord(void *addr, udword data) {
  ubyte *to = (ubyte*)addr;
  ubyte *from = (ubyte*)&data;

  to[0] = from[3]; to[1] = from[2];
  to[2] = from[1]; to[3] = from[0];
}

int solid_tiles[]={
  STONE, SWITCH_ON, SWITCH_OFF, BUMP, SKULL, SPACE, KEYHOLE, 
  DICE1, DICE2, DICE3, DICE4, DICE5, DICE6, BOMBD, FLIP0, FLIP1,
  FRATZE, SLOT, SLOT1, SLOT2, SLOT3, SLOT4, SLOT5, SLOT6, SLOT7, 
  SLOT8, MEM_Q, MEM_0C, MEM_1C, MEM_2C, MEM_3C, MEM_0O, MEM_1O, 
  MEM_2O, MEM_3O, MAGNET_P, MAGNET_N, 0};

is_walkable(int tile) {
  int i=0;

  /* check custom tiles */
  if((tile&0xff)>=(255-16)) {
    if(custom_solid & (1<<(255-(tile&0x0ff)))) {
      return 0;
    }
  }

  while(solid_tiles[i]!=0)
    if((tile&0xff)==solid_tiles[i++])
      return 0;

  return 1;
}

check_level(int lev) {
  int x,y,i,j;

  /* check level size */
  if((level_data[lev]->width>MAX_WIDTH)||(level_data[lev]->height>MAX_HEIGHT)) {
    printf("error: size of level '%s' exceeds %dx%d tiles", level_data[lev]->name, MAX_WIDTH, MAX_HEIGHT); 
    return 0;
  }

  i = level_data[lev]->width; while(i>10) i-=9;
  if(i!=10) { printf("error: width of level '%s' is not 10+(n*9)\n", level_data[lev]->name); return 0; }

  i = level_data[lev]->height; while(i>9) i-=8;
  if(i!=9) { printf("error: height of level '%s' is not 9+(n*8)\n", level_data[lev]->name); return 0; }

  /* check for start/goal field */
  for(j=0,i=0,y=0;y<level_data[lev]->height;y++) {
    for(x=0;x<level_data[lev]->width;x++) {

      /* check for new PEK tiles */
      if(((level_data[lev]->data[y][x]&0xff)>112) && (db_version<1)) 
	db_version = 1;

      /* check for new Mulg II n */
      if(((level_data[lev]->data[y][x]&0xff)>132) && (db_version<2)) 
	db_version = 2;

      if(level_data[lev]->data[y][x]==START) i++;
      if(level_data[lev]->data[y][x]==GOAL)  j++;

      /* see if doc really available */
      if((level_data[lev]->data[y][x]&0xff)==DOC) {
	if((doc[(level_data[lev]->data[y][x])>>8][0]==0)||(((level_data[lev]->data[y][x])>>8)==0)) {
	  printf("error: level '%s' contains undeclared DOC no. %d\n", 
		 level_data[lev]->name,
		 (level_data[lev]->data[y][x])>>8
		 ); 

	  return 0; 
	}
      }
    }
  }

  if(i<1) { printf("error: level '%s' contains no start\n", level_data[lev]->name); return 0; }
  if(i>1) { printf("error: level '%s' contains more than one start\n", level_data[lev]->name); return 0; }
  if(j<1) { printf("error: level '%s' contains no goal\n", level_data[lev]->name); return 0; }

#if 0
  /* check if border can be walked on */
  for(i=1,y=0;y<level_data[lev]->height;y++)
    if((is_walkable(level_data[lev]->data[y][0]))||
       (is_walkable(level_data[lev]->data[y][level_data[lev]->width-1]))) i=0;

  for(x=0;x<level_data[lev]->width;x++)
    if((is_walkable(level_data[lev]->data[0][x]))||
       (is_walkable(level_data[lev]->data[level_data[lev]->height-1][x]))) i=0;

  if(!i) { printf("error: level '%s' contains walkable border tiles\n", level_data[lev]->name); return 0; }
#endif

  /* rotate doors (make vdoor of hdoor if needed, so user need not to care about) */
  for(j=0,i=0,y=0;y<level_data[lev]->height;y++) {
    for(x=0;x<level_data[lev]->width;x++) {
      if((level_data[lev]->data[y][x]&0xff)==HDOOR_1) {
	/* rotate */
	if((is_walkable(level_data[lev]->data[y][x-1]))&&
	   (is_walkable(level_data[lev]->data[y][x+1]))&&
	   (!is_walkable(level_data[lev]->data[y-1][x]))&&
	   (!is_walkable(level_data[lev]->data[y+1][x])))
	  level_data[lev]->data[y][x]=(level_data[lev]->data[y][x]&0xff00)|VDOOR_1;
      }
      if((level_data[lev]->data[y][x]&0xff)==HDOOR_4) {
	/* rotate */
	if((is_walkable(level_data[lev]->data[y][x-1]))&&
	   (is_walkable(level_data[lev]->data[y][x+1]))&&
	   (!is_walkable(level_data[lev]->data[y-1][x]))&&
	   (!is_walkable(level_data[lev]->data[y+1][x])))
	  level_data[lev]->data[y][x]=(level_data[lev]->data[y][x]&0xff00)|VDOOR_4;
      }
      if(((level_data[lev]->data[y][x]&0xff)==HDOOR_2)||
	 ((level_data[lev]->data[y][x]&0xff)==HDOOR_3)||
	 ((level_data[lev]->data[y][x]&0xff)==VDOOR_2)||
	 ((level_data[lev]->data[y][x]&0xff)==VDOOR_3)) {
	printf("error: level '%s' contains half opened doors\n", level_data[lev]->name); 
	return 0; 
      }
    }
  }
  
  return 1;
}

main(int argc, char **argv) {
  FILE *outfile, *tilefile;
  int k,j,i,c,str_size,str_no, offset,x,y;
  time_t tim;
  char *p;
  PdbHeader pdb_header;
  RecHeader rec_header;
  unsigned short us;
  CUSTOM_TILE tile;
  unsigned char buffer[768];
  char tname[32];
  
  printf("Makelevel for Mulg IIp - (c) 1999-2001 by Till Harbaum\n");

  if((argc!=2)&&(argc!=3)) {
    puts("usage: makelevel infile [outfile]");
    exit(1);
  }

  yyin=fopen(argv[1],"r");

  strcpy(fname[0],argv[1]);
  line[0]=1;
  if(yyin==0) {
    fprintf(stderr,"error opening input file '%s'\n", argv[1]);
    exit(1);
  }

  /* clear arrays/strings */
  for(i=0;i<MAX_IDENT;i++)  ident_name[i][0]=0;
  for(i=0;i<256;i++)        doc[i][0]=0;
  dbname[0]=0;

  /* ok, parse the input file */
  yyparse();

  if(dbname[0]==0) {
    fprintf(stderr, "no database name given\n");
    exit(1);
  }

  if(doc[0][2]==0) {
    fprintf(stderr, "no author name given\n");
    exit(1);
  }

  /* append (D) to database name if in debug mode */
  if(db_debug) strcat(dbname, " (D)");

  /* more checks on the maze ... */
  for(i=0;i<levels;i++) {
    if(!check_level(i))
      exit(1);
  }  

  if(argc==2) {
    if((p=strrchr(argv[1],'.'))!=NULL) {
      strcpy(p,".pdb");
    } else
      strcat(argv[1],".pdb");    
    outfile=fopen(argv[1],"wb");
  } else
    outfile=fopen(argv[2],"wb");

  if(outfile==NULL) {
    printf("error opening output file\n");
    exit(1);
  }

  tim=UNIX2PALM(time(0));
  
  /* create/write pdb header */
  memset(&pdb_header,0,sizeof(PdbHeader));
  strcpy(pdb_header.name,dbname);
  WriteWord(&(pdb_header.version),0x0001);
  WriteDWord(&(pdb_header.creationDate),tim);
  WriteDWord(&(pdb_header.modificationDate),tim);

  switch(db_version) {
  case 3:
    WriteDWord(&(pdb_header.databaseType),DB_TYPE_P);
    printf("Some levels in this database require mulg version IIp or later.\n");
    break;

  case 2:
    WriteDWord(&(pdb_header.databaseType),DB_TYPE_N);
    printf("Some levels in this database require mulg version IIn or later.\n");
    break;

  case 1:
    WriteDWord(&(pdb_header.databaseType),DB_TYPE_F);
    printf("Some levels in this database require mulg version IIh or later.\n");
    break;

  default:
    WriteDWord(&(pdb_header.databaseType),DB_TYPE);
    printf("This level database will run with any version of mulg II\n");
    break;
  }

  WriteDWord(&(pdb_header.creatorID),CREATOR);
  WriteWord(&(pdb_header.numberOfRecords),levels+((db_version>=3)?3:2));
  fwrite(&pdb_header,sizeof(PdbHeader),1,outfile);

  /* find no/size of strings */
  for(str_no=0,str_size=0,i=0;i<256;i++) {
    if(doc[i][0]!=0) {
      str_no=i;
      str_size+=strlen(doc[i])+1;
    } 
  }

  offset = sizeof(PdbHeader) + (levels+((db_version>=3)?3:2))*sizeof(RecHeader);

  /* write record header (0=High-Scores, 1=Strings, 2,... = Levels) */
  for(c = 0; c < levels+((db_version>=3)?3:2);c++){
    WriteDWord(&(rec_header.recordDataOffset), offset);
    
    rec_header.recordAttributes = 0x60;
    rec_header.uniqueID[0] = rec_header.uniqueID[1] = rec_header.uniqueID[2] = 0x00;
    fwrite(&rec_header,sizeof(RecHeader),1,outfile);

    if(c==0) offset += 4 + levels*4; /* space for high scores, current level & max level */
    if(c==1) offset += (2*(str_no+1)) + str_size;  /* space for doc strings */
    if(db_version>=3) {
      if(c==2) offset += 2 + custom_tiles*sizeof(CUSTOM_TILE);
      /* space for level data (32 bytes name, 2 bytes size, 2*w*h bytes data */
      if(c>=3) offset += 32 + 2 + (2 * level_data[c-3]->width * level_data[c-3]->height);
    } else {
      /* space for level data (32 bytes name, 2 bytes size, 2*w*h bytes data */
      if(c>=2) offset += 32 + 2 + (2 * level_data[c-2]->width * level_data[c-2]->height);
    }
  }

  /* now write the records */
  if(db_debug) c=0x0000ffff;  /* all levels can be selected */
  else         c=0x00000000;  /* only first level can be selected */
  WriteDWord(&i, c);
  fwrite(&i,4,1,outfile);
  
  /* high scores */
  for(c=0xffffffff, i=0; i<levels;i++)  fwrite(&c,4,1,outfile);

  /* doc strings (first the offsets) */
  for(us=2*(str_no+1),i=0;i<=str_no;i++) {
    fputc(us>>8, outfile);
    fputc(us&0xff, outfile);

    if(doc[i][0]!=0)
      us+=strlen(doc[i])+1;
  }

  /* doc strings (data) */
  for(i=0;i<=str_no;i++)
    if(doc[i][0]!=0)
      fwrite(doc[i],strlen(doc[i])+1,1,outfile);

  if(db_version >= 3) {

    /* custom tile data */
    fputc(custom_solid>>8, outfile);
    fputc(custom_solid&0xff, outfile);

    for(i=0;i<custom_tiles;i++) {
      /* load files */
      
      /* read 2bpp grayscale */
      sprintf(tname, "%s_gray2.raw", custom_tile_name[i]);
      if((tilefile = fopen(tname, "rb")) == NULL) {
	printf("error reading tile data %s\n", tname);
	exit(1);
      }
      fread(buffer, 256, 1, tilefile);
      fclose(tilefile);
      
      /* and convert them */
      for(j=0;j<64;j++)
	tile.gray2[j] = 
	  ~(((buffer[j*4+0]&0xc0)>>0)|
	    ((buffer[j*4+1]&0xc0)>>2)|
	    ((buffer[j*4+2]&0xc0)>>4)|
	    ((buffer[j*4+3]&0xc0)>>6));
      
      /* read 4bpp grayscale */
      sprintf(tname, "%s_gray4.raw", custom_tile_name[i]);
      if((tilefile = fopen(tname, "rb")) == NULL) {
	printf("error reading tile data %s\n", tname);
	exit(1);
      }
      fread(buffer, 256, 1, tilefile);
      fclose(tilefile);
      
      /* and convert them */
      for(j=0;j<128;j++)
	tile.gray4[j] = 
	  ~((buffer[j*2+0]&0xf0)|
	   ((buffer[j*2+1]&0xf0)>>4));
      
      /* read color */
      sprintf(tname, "%s_color.raw", custom_tile_name[i]);
      if((tilefile = fopen(tname, "rb")) == NULL) {
	printf("error reading tile data %s\n", tname);
	exit(1);
      }
      fread(buffer, 768, 1, tilefile);
      fclose(tilefile);
      
      for(j=0;j<256;j++) {
	for(x=-1,k=0;k<256;k++) {
	  if((PalmPalette256[k][0]==buffer[3*j+0])&&
	     (PalmPalette256[k][1]==buffer[3*j+1])&&
	     (PalmPalette256[k][2]==buffer[3*j+2]))
	    x = k;
	}
	if(x<0) {
	  printf("color not mapped\n");
	  exit(1);
	}
	tile.color[j] = x;       
      }
      fwrite(&tile, sizeof(CUSTOM_TILE),1,outfile);
    }
  }

  /* and finally the levels */
  for(i=0;i<levels;i++) {
    fwrite(level_data[i]->name, 32, 1, outfile);
    fputc(level_data[i]->width,  outfile);
    fputc(level_data[i]->height, outfile);

    for(y=0;y<level_data[i]->height;y++) {
      for(x=0;x<level_data[i]->width;x++) {
	fputc((level_data[i]->data[y][x])>>8,   outfile);
	fputc((level_data[i]->data[y][x])&0xff, outfile);
      }
    }
  }

  fclose(outfile);

  if(db_debug)
    printf("wrote debug version\n");

  return err;
}













