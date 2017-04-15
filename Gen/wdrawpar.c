/* STDWIN -- PARAGRAPH DRAWING OPERATIONS.
   N.B. This is portable to other implementations of stdwin. */

#include "stdwin.h"
#include "tools.h"

/* Draw a paragraph of text.
   An EOL forces a new line, otherwise lines are broken at spaces
   (if at all possible).
   Parameters are the top left corner, the width, the text and its length.
   Return value is the v coordinate of the bottom line.
   (Note that an empty string is drawn as one blank line.) */

int
wdrawpar(left, top, text, width)
	int left, top;
	char *text;
	int width;
{
	return _wdrawpar(left, top, text, width, TRUE);
}

/* Measure the height of a paragraph of text, when drawn with wdrawpar. */

int
wparheight(text, width)
	char *text;
	int width;
{
	return _wdrawpar(0, 0, text, width, FALSE);
}

/* Routine to do the dirty work for the above two. */

static int
_wdrawpar(left, top, text, width, draw)
	int left, top;
	char *text;
	int width;
	bool draw;
{
	int len= strlen(text);
	int len1= 0; /* Len1 counts characters until next EOL */
	
	while (len1 < len && text[len1] != EOL)
		++len1;
	for (;;) {
		int n= wtextbreak(text, len1, width);
		if (n < len1) {
			char *cp= text+n;
			while (cp > text && *cp != ' ')
				--cp;
			if (cp > text)
				n= cp-text;
		}
		if (draw)
			(void) wdrawtext(left, top, text, n);
		top += wlineheight();
		text += n;
		len -= n;
		len1 -= n;
		while (len1 > 0 && *text == ' ') {
			++text;
			--len;
			--len1;
		}
		if (len1 == 0) {
			if (len == 0)
				break;
			++text;
			--len;
			while (len1 < len && text[len1] != EOL)
				++len1;
		}
	}
	return top;
}
