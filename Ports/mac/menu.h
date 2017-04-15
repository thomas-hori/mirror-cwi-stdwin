/* MAC STDWIN -- MENU DEFINITIONS. */

/* Note -- struct menu isn't defined here.
   MENU * is taken to be equivalent to MenuPtr, whenever appropriate.
   I know this is a hack -- I'll fix it later. */

struct menubar {
	int nmenus;		/* Number of menus in the list */
	MENU **menulist;	/* Pointer to list of MENU pointers */
};

#define APPLE_MENU 32000
