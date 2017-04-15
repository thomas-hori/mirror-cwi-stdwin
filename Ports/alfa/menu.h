/* TERMCAP STDWIN -- MENU DEFINITIONS */

struct item {
	char *text;		/* The item's text */
	char *shortcut;		/* Visible representation of the shortcuts */
	tbool enabled;		/* Can be selected */
	tbool checked;		/* Check mark left of menu text */
};

struct _menu {
	int id;			/* Menu id, reported by wgetevent */
	char *title;		/* Menu title string */
	bool local;		/* Set if must explicitly attach to windows */
	bool dirty;		/* Set if items have changed */
	int left, right;	/* Left & right edge of title in menu bar */
	int maxwidth;		/* Max width of items */
	int nitems;		/* Number of items */
	struct item *itemlist;	/* List of items */
};

struct menubar {
	int nmenus;
	MENU **menulist;
};
