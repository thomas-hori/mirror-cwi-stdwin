/* Text Edit Window Package */

typedef struct editwin {
	/* Fields can be read by the caller but shouldn't be changed */
	WINDOW *win;
	TEXTEDIT *tp;
	char *filename;
	char /*tbool*/ saved;
} EDITWIN;

EDITWIN *ewfind _ARGS((WINDOW *));
int ewcount _ARGS((void));
EDITWIN *ewcreate _ARGS((char *));
EDITWIN *ewnew _ARGS((void));
EDITWIN *ewopen _ARGS((void));
/*bool*/int ewclose _ARGS((EDITWIN *));
/*bool*/int ewsave _ARGS((EDITWIN *));
/*bool*/int ewsaveas _ARGS((EDITWIN *));
/*bool*/int ewsavecopy _ARGS((EDITWIN *));
/*bool*/int ewsaveprompt _ARGS((EDITWIN *, char *, /*bool*/int));
/*bool*/int ewrevert _ARGS((EDITWIN *));
/*bool*/int ewreadfile _ARGS((EDITWIN *, char *));
void ewsetdimensions _ARGS((EDITWIN *));
/*bool*/int ewwritefile _ARGS((EDITWIN *, char *));
/*bool*/int ewevent _ARGS((EDITWIN *, EVENT *, /*bool*/int *));
/*bool*/int ewsaveall _ARGS((void));
/*bool*/int ewcloseall _ARGS((void));
void ewreplace _ARGS((EDITWIN *, char *));
void ewundo _ARGS((EDITWIN *));		/* Not implemented */
void ewcopy _ARGS((EDITWIN *));
void ewpaste _ARGS((EDITWIN *));
