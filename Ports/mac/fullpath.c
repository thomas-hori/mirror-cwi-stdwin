/* GET FULL PATHNAME OF A FILE. */

#include "macwin.h"
#include <Files.h>

/* Mac file system parameters */
#define MAXPATH 256	/* Max path name length+1 */
#define SEP ':'		/* Separator in path names */
#define ROOTID 2	/* DirID of a volume's root directory */

/* Macro to find out whether we can do HFS-only calls: */
#define FSFCBLen (* (short *) 0x3f6)
#define hfsrunning() (FSFCBLen > 0)

char *
getdirname(dir)
	int dir; /* WDRefNum */
{
	union {
		HFileInfo f;
		DirInfo d;
		WDPBRec w;
		VolumeParam v;
	} pb;
	static char cwd[2*MAXPATH];
	unsigned char namebuf[MAXPATH];
	short err;
	long dirid= 0;
	char *next= cwd + sizeof cwd - 1;
	int len;
	
	if (!hfsrunning())
		return "";
	
	for (;;) {
		pb.d.ioNamePtr= namebuf;
		pb.d.ioVRefNum= dir;
		pb.d.ioFDirIndex= -1;
		pb.d.ioDrDirID= dirid;
		err= PBGetCatInfo((CInfoPBPtr)&pb.d, FALSE);
		if (err != noErr) {
			dprintf("PBCatInfo err %d", err);
			return NULL;
		}
		*--next= SEP;
		len= namebuf[0];
		/* XXX There is no overflow check on cwd here! */
		strncpy(next -= len, (char *)namebuf+1, len);
		if (pb.d.ioDrDirID == ROOTID)
			break;
		dirid= pb.d.ioDrParID;
	}
	return next;
}

void
fullpath(buf, wdrefnum, file)
	char *buf;
	int wdrefnum;
	char *file;
{
	strcpy(buf, getdirname(wdrefnum));
	strcat(buf, file);
}
