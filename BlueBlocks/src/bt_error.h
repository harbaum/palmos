/*
  bt_error.h
*/

#ifndef BT_ERROR_H
#define BT_ERROR_H

typedef struct { Err err; char *str; } err_entry;
extern const err_entry err_entries[];

extern void bt_error(char *msg, Err err);

static inline char *bt_error_str(Err err) {
  int i = 0;

  while((err_entries[i].err != err)&&(err_entries[i].err != -1))
    i++;
  
  return err_entries[i].str;
}

#endif // BT_ERROR_H
