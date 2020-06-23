#ifndef __FTP_H__
#define __FTP_H__

/* file display columns */
#define NAME_POS    10
#define LEN_POS    110
#define LAST_POS   152

#define SHOW_ENTRIES 12

struct file_info {
  char name[64];
  long length;
  Boolean is_dir;
};

extern UInt axxPacRef;

extern Boolean open_netlib(void);
extern void    close_netlib(void);

extern Boolean connect(char *host, int port, char *user, char *pass);
extern void    disconnect(Boolean interactive);

extern Boolean list(void);
extern void    list_select(int x, int y, int sel);
extern void    list_scroll(Boolean, int pos);

extern Boolean upload(void);
extern Boolean change_dir(char *path);

#endif /* __FTP_H__ */


