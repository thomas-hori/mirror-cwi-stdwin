/* Repeat a command forever, displaying the output in a window.
   This is useful to have a continuously update from status programs
   like 'ps' or 'lpq'.
   
   Usage: repeat [-d delay] command [arg] ...
   
   To do:
	- adapt textedit width to window width?
	- allocate image buffer dynamically
	- more sensible error handling (allow to cancel)
	- add a menu option to change the delay
	- display the delay somehow
	- use d.ddd seconds notation instead of deciseconds
	- avoid flickering screen in X11 version
	  (possibly by using tereplace on the smallest changed section,
	  except need a way to turn the caret off???)
*/

#include <stdio.h>
#include <string.h>
#include <stdwin.h>

extern char *malloc();

WINDOW *win;
TEXTEDIT *tp;

#define MAXSIZE 10000

#define DELAY 5 /* retry after success in 0.5 seconds */
#define ERRDELAY 50 /* retry after popen error in 5 seconds */

int delay = DELAY; /* normal delay */
int errdelay = ERRDELAY; /* delay after popen error */
int tflag = 0; /* Display time above output when set */

char *progname = "REPEAT";

void usage() {
	wdone();
	fprintf(stderr,
		"usage: %s [-d ddd.d] [-e ddd.d] [-t] cmd [arg] ...\n",
		progname);
	fprintf(stderr, "-d ddd.d: delay N seconds after command\n");
	fprintf(stderr, "-e ddd.d: delay N seconds after error\n");
	fprintf(stderr, "-t      : display current time above output\n");
	exit(2);
}

extern int optind;
extern char *optarg;

void mainloop();
void rerun();
void drawproc();
char *mktitle();
long getfract();

int main(argc, argv)
	int argc;
	char **argv;
{
	int c;
	winitargs(&argc, &argv); /* Must be first for X11 */
	if (argc > 0 && argv[0] != NULL && argv[0][0] != '\0')
		progname = argv[0];
	while ((c = getopt(argc, argv, "d:e:t")) != EOF) {
		switch (c) {
		case 'd':
			delay = getfract(&optarg, 10);
			if (*optarg != '\0') {
				fprintf(stderr, "-d argument format error\n");
				usage();
			}
			if (delay <= 0)
				delay = DELAY;
			break;
		case 'e':
			errdelay = getfract(&optarg, 10);
			if (*optarg != '\0') {
				fprintf(stderr, "-e argument format error\n");
				usage();
			}
			if (errdelay <= 0)
				errdelay = ERRDELAY;
			break;
		case 't':
			tflag = 1;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	if (optind >= argc) {
		usage();
		/* NOTREACHED*/
	}
	win = wopen(mktitle(argc-optind, argv+optind), drawproc);
	if (win == NULL) {
		wdone();
		fprintf(stderr, "can't open window\n");
		return 1;
	}
	tp = tecreate(win, 0, 0, 10000, 10000);
	if (tp == NULL) {
		wclose(win);
		wdone();
		fprintf(stderr, "can't create textedit block\n");
		return 1;
	}
	mainloop(argc-optind, argv+optind);
	wdone();
	return 0;
}

void mainloop(argc, argv)
	int argc;
	char **argv;
{
	rerun(argc, argv);
	
	for (;;) {
		EVENT e;
		wgetevent(&e);
		switch (e.type) {
		case WE_TIMER:
			rerun(argc, argv);
			break;
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
			case WC_CANCEL:
				return;
			case WC_RETURN:
				rerun(argc, argv);
				break;
			}
			break;
		}
	}
}

void rerun(argc, argv)
	int argc;
	char **argv;
{
	int fd;
	char *image, *p;
	int left, n;
	
	if ((image = malloc(MAXSIZE)) == NULL) {
		wmessage("can't malloc output buffer");
		wsettimer(win, errdelay);
		return;
	}
	if ((fd = readfrom(argc, argv)) < 0) {
		wmessage("can't start command");
		wsettimer(win, errdelay);
		return;
	}
	left = MAXSIZE;
	p = image;
	if (tflag) {
		extern char *ctime();
		extern long time();
		long t;
		time(&t);
		strcpy(p, ctime(&t));
		p = strchr(p, '\0');
	}
	while (left > 0 && (n = read(fd, p, left)) > 0) {
		p += n;
		left -= n;
	}
	close(fd);
	(void) wait((int *)0); /* Get rid of child -- naively */
	tesetbuf(tp, image, (int)(p - image));
	wsetdocsize(win, 0, tegetbottom(tp));
	wnocaret(win); /* Hack */
	wsettimer(win, delay);
}

void drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	tedrawnew(tp, left, top, right, bottom);
}

char *mktitle(argc, argv)
	int argc;
	char **argv;
{
	static char buf[1000];
	int i;
	char *p;
	
	p = buf;
	for (i = 0; i < argc; ++i) {
		if (i > 0) *p++ = ' ';
		strcpy(p, argv[i]);
		p = strchr(p, '\0');
	}
	return buf;
}

/* Function to get a number of the form ddd.ddd from a string, without
   using floating point variables.  Input is the address of a string
   pointer, which is incremented by the function to point after the
   last character that was part of the number.
   The function return value is X * scale, where X is the exact value
   read from the string, e.g., for input "3.14" and scale 1000, 3140 is
   returned.
   There is no provision for a leading sign or to leading spaces.
   Overflow goes undetected.
*/

long getfract(pstr, scale)
	char **pstr;
	long scale;
{
	char *str = *pstr;
	long x = 0;
	int in_fraction = 0;
	char c;
	
	for (; (c = *str) != '\0'; str++) {
		if (c >= '0' && c <= '9') {
			if (in_fraction) {
				if (scale < 10)
					break;
				scale /= 10;
			}
			x = x*10 + (c - '0');
		}
		else if (c == '.' && !in_fraction)
			in_fraction = 1;
		else
			break;
	}
	*pstr = str;
	return x*scale;
}

/*
                  Copyright 1989 by Douglas A. Young

                           All Rights Reserved

   Permission to use, copy, modify, and distribute this software 
   for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation. The author disclaims all warranties with 
   regard to this software, including all implied warranties of 
   merchantability and fitness.

   Comments and additions may be sent the author at:

    dayoung@hplabs.hp.com

   Modified to resemble popen(..., "r").  Returns the file descriptor
   from which to read, or negative if an error occurred.  (The pid is lost.)
   Name changed from talkto() to readfrom(). --Guido van Rossum, CWI
*/

int
readfrom(argc, argv)
	int argc;
	char **argv;
{
	int to_parent[2]; /* pipe descriptors from child->parent */
	int pid;

	if (pipe(to_parent) < 0) {
		perror("pipe");
		return -1;
	}
	pid = fork();
	if (pid == 0) {                      /* in the child */
		close(0);                    /* redirect stdin */
		open("/dev/null", 0);
		close(1);                    /* redirect stdout */
		dup(to_parent[1]);
		close(2);                    /* redirect stderr */
		dup(to_parent[1]);

		close(to_parent[0]);
		close(to_parent[1]);

		execvp(argv[0], argv);       /* exec the new cmd */
		perror(argv[0]);             /* error in child! */
		_exit(127);
	}
	else if (pid > 0) {                  /* in the parent */
		close(to_parent[1]);
		return to_parent[0];
	}
	else {                               /* error! */
		perror("vfork");
		return -1;
	}
}
