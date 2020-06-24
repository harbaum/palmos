#include <stdio.h>
#include <time.h>
#include <ctype.h>

// #define PALM2UNIX(a)  (a - 2082844800)
#define UNIX2PALM(a)  (a + 2082844800 + 7200)

main(int argc, char **argv) {
  struct tm tm;
  time_t tim;
  int i,j;
  char str[64];

  if(argc == 1) {
    printf("/* no expiration date */\n");
    return 0;
  }

  if(argc != 3) {
    printf("Usage: %s day.month.year hour:minute:second\n", argv[0]);
    return 1;
  }

  i = sscanf(argv[1], "%d.%d.%d", &tm.tm_mday, &tm.tm_mon, &tm.tm_year);
  j = sscanf(argv[2], "%d:%d:%d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
  
  if((i != 3)||(j!=3)) {
    printf("Usage: %s day.month.year hour:minute:second\n", argv[0]);
    return 1;
  }

  tm.tm_year -= 1900;
  tm.tm_mon -= 1;

  tm.tm_wday = 0;
  tm.tm_yday = 0;
  tm.tm_isdst = 1;

  /* convert time into seconds unix time */
  tim = mktime(&tm);

  strcpy(str, ctime(&tim));
  while(!isdigit(str[strlen(str)-1])) str[strlen(str)-1] = 0;

  /* show time for verification */
  printf("/*\n Time: %s\n", str);
  printf(" Seconds since 1.1.1970 (Unix time): %lu\n", tim);
  printf(" Seconds since 1.1.1900 (Palm time): %lu\n*/\n", UNIX2PALM(tim));
  printf("#define EXPIRE_TIME_ASC \"%s\"\n", str);
  printf("#define EXPIRE_TIME_SEC %luU\n", UNIX2PALM(tim));

  return 0;
}
