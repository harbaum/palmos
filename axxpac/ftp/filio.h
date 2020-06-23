/*

  filio.h  - (c) 2000 by Till Harbaum

  ftp client for axxpac and palm VFS

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#define FS_CREAT  0
#define FS_RDONLY 1
#define FS_RDWR   2

extern Boolean fs_driver_open(void);
extern void    fs_driver_close(void);
extern UInt16  fs_fsel(char *title, char *name, Boolean new);
extern Boolean fs_open(char *name, int cmode);
extern void    fs_close(void);
extern void    fs_delete(char *name);
extern long    fs_tell(char *name);
extern long    fs_read(void *buf, long num);
extern long    fs_write(void *buf, long num);

