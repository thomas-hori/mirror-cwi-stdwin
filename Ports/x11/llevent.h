/* X11 STDWIN -- Variables set by low-level event handling */

extern WINDOW *_w_new_active;	/* New active window */
extern struct button_state _w_bs; /* Mouse button state */
extern KeySym _w_keysym;	/* Keysym of last non-modifier key pressed */
extern int _w_state;		/* Modifiers in effect at key press */

extern bool _w_moved;		/* Set if button moved */
extern bool _w_bs_changed;	/* Set if button up/down state changed */
extern bool _w_dirty;		/* Set if any window needs a redraw */
extern bool _w_resized;		/* Set if any window resized */
extern Time _w_lasttime;	/* Set to last timestamp seen */
extern bool _w_focused;		/* Set between FocusIn and FocsOut events */
extern WINDOW *_w_close_this;	/* Window to close (WM_DELETE_WINDOW) */
