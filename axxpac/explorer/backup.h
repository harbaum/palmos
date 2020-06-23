/*

*/

#ifndef __BACKUP_H__
#define __BACKUP_H__

#include "../api/axxPac.h"
#include "explorer.h"

extern Boolean backup_all(Boolean alarm, UInt lib);
extern void    restore_all(void);
extern void    backup_options(void);
extern void    backup_set_alarm(Boolean set);
extern void    backup_check_alarm(void);

#endif // __BACKUP_H__
