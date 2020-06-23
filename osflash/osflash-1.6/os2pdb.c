/*

  os2pdb  -  Convert a 2MByte PalmOS rom image into a database 
             for use with osflash.

  WARNING: Only the first 40 blocks a 32k (1.25 MByte) are
           included. This is sufficient with the american
           and the german OS3 and should be ok for most 
           other OS3s. BUT THIS DOES NOT WORK FOR THE
           JAPANESE OS!!!!

       Version 1.0 - Mar 1999 Till Harbaum (T.Harbaum@tu-bs.de)
       Version 1.1 - added support for rom images from 3coms web page (does not work)
       Version 1.2 - increased image length (44 blocks) for french OS 3.1
       Version 1.5 - increased image length (52 blocks)
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "osflash.h"
#include "os2pdb.h"

#define  DB_VERSION_MAJOR  '2'
#define  DB_VERSION_MINOR  '0'

/* Palm<->Unix Time conversion */
#define PALM2UNIX(a)  (a - 2082844800)
#define UNIX2PALM(a)  (a + 2082844800)

unsigned char os_buffer[BLOCK_LEN];

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

main(int argc, char **argv) {
    FILE *outfile, *infile;
    int i,offset,j,na;
    time_t tim;
    PdbHeader pdb_header;
    RecHeader rec_header;
    unsigned short us;
    char *p;
    unsigned long checksum=0,h, length;

    printf("os2pdb V1.5 - (c) 2000 by Till Harbaum\n");

    if ((argc!=2)&&(argc!=3)) {
        puts("usage: os2pdb infile [outfile]");
        exit(1);
    }

    infile=fopen(argv[1], "rb");

    if (infile==0) {
        fprintf(stderr,"error opening input file '%s'\n", argv[1]);
        exit(1);
    }

    /* check file length */
    fseek(infile, 0l, SEEK_END);
    length = ftell(infile);
    fseek(infile, 0l, SEEK_SET);

    /* pilot-xfer image */
    if (length != 2097152) {
        printf("ERROR: Suspicious input file length (%ld bytes).\n", length);
        fclose(infile);
    }

    if (argc==2) {
        if ((p=strrchr(argv[1],'.'))!=NULL) {
            strcpy(p,".pdb");
        } else
            strcat(argv[1],".pdb");    
        na = 1;
    } else {
        na = 2;
    }

    outfile=fopen(argv[na],"wb");
    printf("writing %s\n", argv[na]);

    if (outfile==NULL) {
        printf("error opening output file\n");
        exit(1);
    }

    /* this is more a joke, since palmos overwrites this value anyway */
    tim=UNIX2PALM(time(0));

    /* create/write pdb header */
    memset(&pdb_header,0,sizeof(PdbHeader));
    strcpy((char *)pdb_header.name, "PalmOS ROM image");
    WriteWord(&(pdb_header.version),0x0001);
    WriteDWord(&(pdb_header.creationDate),tim);
    WriteDWord(&(pdb_header.modificationDate),tim);
    WriteDWord(&(pdb_header.databaseType),DB_TYPE);
    WriteDWord(&(pdb_header.creatorID),CREATOR);
    WriteWord(&(pdb_header.numberOfRecords), OS_BLOCKS);
    fwrite(&pdb_header,sizeof(PdbHeader),1,outfile);

    /* offset to data inside db */
    offset = sizeof(PdbHeader) + OS_BLOCKS*sizeof(RecHeader);

    /* write BLOCKS record headers */
    for (i=0;i<OS_BLOCKS;i++) {
        WriteDWord(&(rec_header.recordDataOffset), offset);

        rec_header.recordAttributes = 0x60;
        rec_header.uniqueID[0] = rec_header.uniqueID[1] = rec_header.uniqueID[2] = 0x00;
        fwrite(&rec_header,sizeof(RecHeader),1,outfile);

        /* offset to next block */
        offset+=BLOCK_LEN;
    }

    /* now copy data */
    for (i=0;i<OS_BLOCKS;i++) {

        if ((length/32768)<i) {
            printf("empty block %d\n", i);
        } else {
            /* this does not work under djgpp */
            if ((j=fread(os_buffer, 1, BLOCK_LEN, infile))!=BLOCK_LEN) {
                printf("read error in source file (%d,%d)\n", i,j);
                exit(1);
            }
        }

        /* block no 0 contains some values indicating whether this */
        /* really is palmos image */
        if ((i==0)||(i==1)) {
            if ( (os_buffer[0x08]!= 0xFE)||
                 (os_buffer[0x09]!= 0xED)||
                 (os_buffer[0x0A]!= 0xBE)||
                 (os_buffer[0x0B]!= 0xEF)) {

                printf("Error, the input file does not contain a valid PalmOS image.\n");

                fclose(outfile);
                fclose(infile);

                /* delete output file */
                remove(argv[na]);

                exit(1);

            }
        }

        /* block no 1 contains main rom reset vector */
        if (i==1) {

            /* check if this isn't already used by flashbuilder */
            /* and don't convert the image then */

            h = (((unsigned long)os_buffer[4])<<24) 
                | (((unsigned long)os_buffer[5])<<16) 
                | (((unsigned long)os_buffer[6])<<8) 
                | ((unsigned long)os_buffer[7]);

            if (h>0x10d00000) {

                printf("Error, the file to be converted seems to be modified\n"
                       "by flashbuilder/flashpro. It can't be used for osflash. (vector=%08lx)",h);

                fclose(outfile);
                fclose(infile);

                /* delete output file */
                remove(argv[na]);

                exit(1);
            }
        }

        /* checksum over everything except block 0 addr 0x6000 - 0x7fff (protected serial number) */

        /* New, now we ignore the entire first 32k.  This is the small rom, and does not need to be flashed */
        /* -- Tim V1.5, Sept 24/2000 */
        if (i!=0) {
            for (j=0;j<BLOCK_LEN/4;j++)
                checksum ^= *(unsigned long*)&os_buffer[4*j];
        } else {
        //    for (j=0;j<BLOCK_LEN/4;j++)
        //        if (((4*j) <0x6000) || ((4*j)>0x7fff) )
        //            checksum ^= *(unsigned long*)&os_buffer[4*j];

            /* erase serial number (can't be flashed anyway) */
            memset(&os_buffer[0x6006], '0', 12);
        }
        fwrite(os_buffer, BLOCK_LEN, 1, outfile);
    }

    fclose(outfile);
    fclose(infile);

    /* force big endian output */
    printf("ok, checksum = %02lx%02lx%02lx%02lx\n", 
           (checksum&0x000000ff)>>0,
           (checksum&0x0000ff00)>>8,
           (checksum&0x00ff0000)>>16,
           (checksum&0xff000000)>>24);

    return 0;
}

