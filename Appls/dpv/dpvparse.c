/* dpv -- ditroff previewer.  Parser for ditroff output. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"
#include "dpvdoc.h" /* Descripton of ditroff output format */

/* Parser main loop.  Read input from fp.
   Stop when EOF hit, 'x stop' found, or nextpage returns < 0.
   This makes calls to the machine module to update the
   virtual machine's state, and to the output module to perform
   actual output.
   Code derived from a (not very robust) ditroff filter by BWK. */

parse(fp)
	register FILE *fp;
{
	register int c, k;
	int m, n, n1, m1;
	char str[100], buf[300];
	
	while ((c = getc(fp)) != EOF) {
		switch (c) {
		case '\n':	/* when input is text */
		case '\0':	/* occasional noise creeps in */
		case '\t':
		case ' ':
			break;
		case '{':	/* push down current environment */
			t_push();
			break;
		case '}':
			t_pop();
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			/* two motion digits plus a character */
			k= (c-'0')*10;
			c= getc(fp);
			k += c-'0';
			hmot(k);
			c= getc(fp);
			put1(c);
			break;
		case 'c':	/* single ascii character */
			c= getc(fp);
			put1(c);
			break;
		case 'C':
			fscanf(fp, "%s", str);
			put1s(str);
			break;
		case 't':	/* straight text */
			if (fgets(buf, sizeof(buf), fp) == NULL)
			    error(FATAL, "unexpected end of input");
			t_text(buf);
			break;
		case 'D':	/* draw function */
			if (fgets(buf, sizeof(buf), fp) == NULL)
			    error(FATAL, "unexpected end of input");
			switch (buf[0]) {
			case 'l':	/* draw a line */
			    sscanf(buf+1, "%d %d", &n, &m);
			    drawline(n, m);
			    break;
			case 'c':	/* circle */
			    sscanf(buf+1, "%d", &n);
			    drawcirc(n);
			    break;
			case 'e':	/* ellipse */
			    sscanf(buf+1, "%d %d", &m, &n);
			    drawellip(m, n);
			    break;
			case 'a':	/* arc */
			    sscanf(buf+1, "%d %d %d %d", &n, &m, &n1, &m1);
			    drawarc(n, m, n1, m1);
			    break;
			case '~':	/* wiggly line */
			    /* HIRO: how to scan? */
			    drawwig(buf+1, fp, 1);
			    break;
			default:
			    error(FATAL, "unknown drawing function %s", buf);
			    break;
			}
			recheck();
			break;
		case 's':
			fscanf(fp, "%d", &n);
			setsize(t_size(n));
			break;
		case 'f':
			fscanf(fp, "%s", str);
			setfont(t_font(str));
			break;
		case 'H':	/* absolute horizontal motion */
			while ((c = getc(fp)) == ' ')
				;
			k = 0;
			do {
				k = 10 * k + c - '0';
			} while (isdigit(c = getc(fp)));
			ungetc(c, fp);
			hgoto(k);
			break;
		case 'h':	/* relative horizontal motion */
			while ((c = getc(fp)) == ' ')
				;
			k = 0;
			do {
				k = 10 * k + c - '0';
			} while (isdigit(c = getc(fp)));
			ungetc(c, fp);
			hmot(k);
			break;
		case 'w':	/* word space */
			break;
		case 'V':
			fscanf(fp, "%d", &n);
			vgoto(n);
			break;
		case 'v':
			fscanf(fp, "%d", &n);
			vmot(n);
			break;
		case 'P':	/* new spread (Versatec hack) */
			break;
		case 'p':	/* new page */
			fscanf(fp, "%d", &n);
			if (t_page(n) < 0)
				return;
			break;
		case 'n':	/* end of line */
			t_newline();
		/* Fall through */
		case '#':	/* comment */
			do {
				c = getc(fp);
			} while (c != EOL && c != EOF);
			break;
		case 'x':	/* device control */
			if (devcntrl(fp) < 0)
				return;
			break;
		default:
			error(FATAL, "unknown input character %o %c", c, c);
		}
	}
	error(WARNING, "read till EOF");
}

/* Parse rest of device control line.
   Returns -1 upon receiving EOF or "stop" command. */

static int
devcntrl(fp)
	FILE *fp;
{
        char str[20], str1[50], buf[50];
	extern char *namemap(); /* ??? */
	int c, n;

	fscanf(fp, "%s", str);
	switch (str[0]) {	/* crude for now */
	case 'i':	/* initialize */
		t_init();
		break;
	case 't':	/* trailer */
		break;
	case 'p':	/* pause -- can restart */
		t_reset('p');
		break;
	case 's':	/* stop */
		t_reset('s');
		return -1;
	case 'r':	/* resolution assumed when prepared */
		fscanf(fp, "%d", &res);
		break;
	case 'f':	/* font used */
		fscanf(fp, "%d %s", &n, str);
		(void) fgets(buf, sizeof buf, fp);   /* in case of filename */
		ungetc(EOL, fp);		/* fgets goes too far */
		str1[0] = 0;			/* in case nothing comes in */
		(void) sscanf(buf, "%s", str1);
		loadfont(n, str, str1);
		break;
	case 'H':	/* char height */
		fscanf(fp, "%d", &n);
		t_charht(n);
		break;
	case 'S':	/* slant */
		fscanf(fp, "%d", &n);
		t_slant(n);
		break;
	case 'T':	/* device name */
		buf[0]= EOS;
		fscanf(fp, "%s", buf);
		if (buf[0] != EOS)
			devname= strdup(buf);
		break;

	}
	while ((c = getc(fp)) != EOL) {	/* skip rest of input line */
		if (c == EOF)
			return -1;
	}
	return 0;
}
