/* A hack to compensate the absence of <dirent.h> on BSD systems.
   Remove this file if it bothers you. */

#ifdef SYSV
#include "/usr/include/dirent.h"
#else
#include <sys/dir.h>
#define dirent direct
#endif
