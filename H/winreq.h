/* STDWIN Server -- Window (Manager) Requests. */

/* Command codes in transaction header. */

#define WIN_FIRST	1100

#define WIN_HELLO	(WIN_FIRST+1)	/* Say hello to server */
#define WIN_DROPDEAD	(WIN_FIRST+2)	/* Die, if idle */
#define WIN_OPEN	(WIN_FIRST+3)	/* Open new window */
#define WIN_CLOSE	(WIN_FIRST+4)	/* Close window */
#define WIN_DRAW	(WIN_FIRST+5)	/* Drawing commands; see subcodes */
#define WIN_GETEVENT	(WIN_FIRST+6)	/* Get event */
#define WIN_FLEEP	(WIN_FIRST+7)	/* Flash or beep */
#define WIN_ACTIVATE	(WIN_FIRST+8)	/* Make window active */
#define WIN_SETDOCSIZE	(WIN_FIRST+9)	/* Set document size */
#define WIN_SETTITLE	(WIN_FIRST+10)	/* Set window title */
#define WIN_SHOW	(WIN_FIRST+11)	/* Show part of document */
#define WIN_SETORIGIN	(WIN_FIRST+12)	/* Set window origin in document */
#define WIN_CHANGE	(WIN_FIRST+13)	/* Change part of document */
#define WIN_SCROLL	(WIN_FIRST+14)	/* Scroll part of document */
#define WIN_MESSAGE	(WIN_FIRST+15)	/* Output message */
#define WIN_ASKSTR	(WIN_FIRST+16)	/* Ask string */
#define WIN_ASKYNC	(WIN_FIRST+17)	/* Ask yes/no/cancel question */
#define WIN_SETCARET	(WIN_FIRST+18)	/* Set caret position */
#define WIN_STATUS	(WIN_FIRST+19)	/* Get window status */
#define WIN_GETFONTTAB	(WIN_FIRST+20)	/* Get font width table */
#define WIN_SETTIMER	(WIN_FIRST+21)	/* Set window timer */
#define WIN_SETCLIP	(WIN_FIRST+22)	/* Set clipboard string */
#define WIN_GETCLIP	(WIN_FIRST+23)	/* Get clipboard string */
#define WIN_POLLEVENT	(WIN_FIRST+24)	/* Poll for event */

#define MENU_FIRST	(WIN_FIRST+50)

#define MENU_CREATE	(MENU_FIRST+1)
#define MENU_DELETE	(MENU_FIRST+2)
#define MENU_ADDITEM	(MENU_FIRST+3)
#define MENU_SETITEM	(MENU_FIRST+4)
#define MENU_ATTACH	(MENU_FIRST+5)
#define MENU_DETACH	(MENU_FIRST+6)
#define MENU_ENABLE	(MENU_FIRST+7)
#define MENU_CHECK	(MENU_FIRST+8)


/* Subcodes in data buffer for WIN_DRAW. */

#define DRW_LINE	1
#define DRW_BOX		2
#define DRW_CIRCLE	3
#define DRW_ERASE	4
#define DRW_PAINT	5
#define DRW_SHADE	6
#define DRW_INVERT	7
#define DRW_TEXT	8
#define DRW_FONT	9
#define DRW_STYLE	10
#define DRW_ELARC	11
#define DRW_XORLINE	12

/* Error codes for h_status. */

#define WER_OK		0		/* ok */
#define WER_FAIL	-101		/* don't know why it failed */
#define WER_COMBAD	-102		/* bad command */
#define WER_CAPBAD	-103		/* bad capability */
#define WER_NOSPACE	-104		/* no space left */
#define WER_ABORT	-105		/* call aborted */
#define WER_NODATA	-106		/* No or insufficient data provided */

/* Event packing parameters.
   (This is a kludge!  Should be variable-length and have a length byte
   in the data!) */

#define EVSHORTS	7
#define EVPACKSIZE	(EVSHORTS * sizeof(short))

/* Server's buffer size. */

#define SVRBUFSIZE	4096
	/* I suppose this should really be negotiated between server and
	   client, e.g., WIN_HELLO should return the server's buffer
	   size. */

/* Font width table length. */

#define FONTTABLEN	256	/* Number of chars in a font */

/* Pseudo-event sent when SIGAMOEBA received */

#define WE_ALERT	22
