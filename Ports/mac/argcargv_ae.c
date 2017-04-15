/*
** argcargv_ae.c - Attempt to construct argc and argv by using apple events.
**
** Needed because the old argcargv.c stopped working on the PowerPC mac.
**
** This glue code was put together by Jack, mainly from information glanced from
** the mac programmers FAQ (where some of the code comes from, too) and from IM vol VI.
** The FAQ code, by the way, was dead wrong and needed quite a bit of polishing up.
** It uses high-level events and such to simulate argc/argv. This does mean, however,
** that the program can expect unexpected high-level events while it is running. It's
** anybody's guess what'll happen to these...
*/

#include "macwin.h"

#include <Memory.h>
#include <Files.h>
#include <Processes.h>
#include <Errors.h>
#include <AppleEvents.h>
#include <AEObjects.h>
#include <Desk.h>

#ifdef THINK_C
/* XXX This is really only for versions that don't know about UPPs yet */
#define NewAEEventHandlerProc(x) (x)
#define AEEventHandlerUPP EventHandlerProcPtr
#endif

static int in_wargs;		/* True while collecting arguments */
L_DECLARE(wargs_argc, wargs_argv, char *);

/*
** Return FSSpec of current application.
*/
static OSErr
CurrentProcessLocation(FSSpec *applicationSpec)
{
        ProcessSerialNumber currentPSN;
        ProcessInfoRec info;
        
        currentPSN.highLongOfPSN = 0;
        currentPSN.lowLongOfPSN = kCurrentProcess;
        info.processInfoLength = sizeof(ProcessInfoRec);
        info.processName = NULL;
        info.processAppSpec = applicationSpec;
        return ( GetProcessInformation(&currentPSN, &info) );
}

/*
** Given an FSSpec, return the FSSpec of the parent folder.
*/
static OSErr
GetFolderParent ( FSSpec * fss , FSSpec * parent ) {

CInfoPBRec rec ;
short err ;

        * parent = * fss ;
        rec . hFileInfo . ioNamePtr = parent -> name ;
        rec . hFileInfo . ioVRefNum = parent -> vRefNum ;
        rec . hFileInfo . ioDirID = parent -> parID ;
		rec.hFileInfo.ioFDirIndex = -1;
        rec . hFileInfo . ioFVersNum = 0 ;
        if (err = PBGetCatInfoSync ( & rec ) )
        	return err;
        parent -> parID = rec . dirInfo . ioDrParID ;
      /*        parent -> name [ 0 ] = 0 ; */
        return 0 ;
}

/*
** Given an FSSpec return a full, colon-separated pathname
*/
static OSErr
GetFullPath ( FSSpec * fss , char *buf ) {
	short err ;
	FSSpec fss_parent, fss_current ;
	char tmpbuf[256];
	int plen;

	fss_current = *fss;
	plen = fss_current.name[0];
	memcpy(buf, &fss_current.name[1], plen);
	buf[plen] = 0;
    while ( fss_current.parID > 1 ) {
    			/* Get parent folder name */
                if ( err =GetFolderParent( &fss_current , &fss_parent ) )
             		return err;
                fss_current = fss_parent;
                /* Prepend path component just found to buf */
    			plen = fss_current.name[0];
    			if ( strlen(buf) + plen + 1 > 256 ) {
    				/* Oops... Not enough space (shouldn't happen) */
    				*buf = 0;
    				return -1;
    			}
    			memcpy(tmpbuf, &fss_current.name[1], plen);
    			tmpbuf[plen] = ':';
    			strcpy(&tmpbuf[plen+1], buf);
    			strcpy(buf, tmpbuf);
        }
        return 0 ;
}

/*
** Return the full programname
*/
static char *
getappname()
{
	static char appname[256];
	FSSpec appspec;
	long size;
	
	if ( CurrentProcessLocation(&appspec) )
		return NULL;
	if ( GetFullPath(&appspec, appname) )
		return NULL;
	return appname;
}

/* Check that there aren't any args remaining in the event */
static OSErr 
GetMissingParams( AppleEvent  *theAppleEvent)
{
	DescType	theType;
	Size		actualSize;
	OSErr		err;
	
	err = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard,
								&theType, nil, 0, &actualSize);
	if (err == errAEDescNotFound)
		return(noErr);
	else
		return(errAEEventNotHandled);
}

/* Handle the "Open Application" event (by ignoring it) */
static pascal
OSErr HandleOpenApp(AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	#pragma unused (reply,refCon)
	WindowPtr	window;
	OSErr		err;
	
	err = GetMissingParams(theAppleEvent);
	return(err);
}

/* Handle the "Open Document" event, by adding an argument */
static pascal
OSErr HandleOpenDoc(AppleEvent *theAppleEvent, AppleEvent *reply, long refCon)
{
	#pragma unused (reply,refCon)
	OSErr	err;
	AEDescList doclist;
	AEKeyword keywd;
	DescType rttype;
	long i, ndocs, size;
	FSSpec fss;
	char path[256];
	
	if ( !in_wargs )
		return(errAEEventNotHandled);
		
	if ( err=AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &doclist) )
		return err;
	if (err = GetMissingParams(theAppleEvent)) 
		return err;
	if (err = AECountItems(&doclist, &ndocs) )
		return err;
	for(i=1; i<=ndocs; i++ ) {
		err=AEGetNthPtr(&doclist, i, typeFSS, &keywd, &rttype, &fss, sizeof(fss), &size);
		if ( err )
			return err;
		GetFullPath(&fss, path);
		L_APPEND(wargs_argc, wargs_argv, char *, strdup(path));

	}
	return(err);
}

static pascal
OSErr DummyHandler( AppleEvent		*theAppleEvent,
					AppleEvent		*reply,
					long			refCon	)
{
	#pragma unused (theAppleEvent,reply,refCon)
	return(errAEEventNotHandled);
}

void InitAEHandlers()
{
	AEEventHandlerUPP upp_opena = NewAEEventHandlerProc(HandleOpenApp);
	AEEventHandlerUPP upp_opend = NewAEEventHandlerProc(HandleOpenDoc);
	AEEventHandlerUPP upp_printd = NewAEEventHandlerProc(DummyHandler);
	AEEventHandlerUPP upp_quit = NewAEEventHandlerProc(DummyHandler);
	
	AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, upp_opena, 0L, false);
	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, upp_opend, 0L, false);
	AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, upp_printd, 0L, false);
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, upp_quit, 0L, false);
}


static void 
wargs_EventLoop()
{
	EventRecord		event;
	int				gotEvent;
	int				count = 0;

	do {
		count++;
		SystemTask();
		gotEvent = GetNextEvent(everyEvent, &event);
		if (gotEvent && event.what == kHighLevelEvent ) {
			AEProcessAppleEvent(&event);
		}
	} while(  count < 100 );  /* Seems we get a few null events first... */
}

void
wargs(pargc, pargv)
	int *pargc;
	char ***pargv;
{
	char *apname;
	
	winit();
	apname = getappname();
	L_APPEND(wargs_argc, wargs_argv, char *, strdup(apname));
	
	in_wargs = 1;
	InitAEHandlers();
	wargs_EventLoop();
	in_wargs = 0;

	*pargc = wargs_argc;
	*pargv = wargs_argv;
}

