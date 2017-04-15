/* Text Item Lists */

struct _textitem {
	TEXTEDIT *tp;
	int left, top, right, bottom;
	int active;
	struct _textitem *next;
	struct _textitemlist *back;
};

struct _textitemlist {
	WINDOW *win;
	struct _textitem *list;
	struct _textitem *focus;
};

#define TEXTITEM struct _textitem
#define TILIST struct _textitemlist

TILIST *tilcreate();
void tildestroy();
void tildraw();
int tilevent();
void tilnextfocus();

TEXTITEM *tiladd();
TEXTITEM *tilinsert();
void tilremove();

void tilsetactive();
void tilfocus();
void tilsettext();
char *tilgettext();
