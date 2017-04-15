/* STDWIN -- KEY MAPPING TABLES. */

#include "alfa.h"

#define CTLX_MAP	1
#define ESC_MAP		2
#define PC_MAP		3

/* The key map is set up to provide shortcuts for the system menu. */

struct keymap _wprimap[256]= {
#ifdef MSDOS
	/* ^@ */ { 0, SECONDARY, PC_MAP, 0},	/* ^@ map */
#else /* !MSDOS */
	/* ^@ */ { 0, ORDINARY, 0, 0},
#endif
	/* ^A */ { 1, ORDINARY, 0, 0},
	/* ^B */ { 2, SHORTCUT, 0, FIRST_CMD+WC_LEFT},
	/* ^C */ { 3, SHORTCUT, 0, FIRST_CMD+WC_CANCEL},
	/* ^D */ { 4, ORDINARY, 0, 0},
	/* ^E */ { 5, ORDINARY, 0, 0},
	/* ^F */ { 6, SHORTCUT, 0, FIRST_CMD+WC_RIGHT},
	/* ^G */ { 7, SHORTCUT, 0, MOUSE_DOWN},
	/* ^H */ { 8, SHORTCUT, 0, FIRST_CMD+WC_BACKSPACE},
	/* ^I */ { 9, SHORTCUT, 0, FIRST_CMD+WC_TAB},
	/* ^J */ {10, SHORTCUT, 0, FIRST_CMD+WC_RETURN},
	/* ^K */ {11, ORDINARY, 0, 0},
	/* ^L */ {12, SHORTCUT, 0, REDRAW_SCREEN},
	/* ^M */ {13, SHORTCUT, 0, FIRST_CMD+WC_RETURN},
	/* ^N */ {14, SHORTCUT, 0, FIRST_CMD+WC_DOWN},
	/* ^O */ {15, ORDINARY, 0, 0},
	/* ^P */ {16, SHORTCUT, 0, FIRST_CMD+WC_UP},
	/* ^Q */ {17, ORDINARY, 0, 0},
	/* ^R */ {18, SHORTCUT, 0, REDRAW_SCREEN},
	/* ^S */ {19, ORDINARY, 0, 0},
	/* ^T */ {20, ORDINARY, 0, 0},
	/* ^U */ {21, ORDINARY, 0, 0},
	/* ^V */ {22, SHORTCUT, 0, LITERAL_NEXT},
	/* ^W */ {23, ORDINARY, 0, 0},
	/* ^X */ {24, SECONDARY, CTLX_MAP, 0},	/* ^X map */
	/* ^Y */ {25, ORDINARY, 0, 0},
	/* ^Z */ {26, SHORTCUT, 0, SUSPEND_PROC},
	/* ^[ */ {27, SECONDARY, ESC_MAP, 0},	/* ESC map */
	/* ^\ */ {28, ORDINARY, 0, 0},
	/* ^] */ {29, ORDINARY, 0, 0},
	/* ^^ */ {30, ORDINARY, 0, 0},
	/* ^_ */ {31, ORDINARY, 0, 0},
	
	/* Followed by 224 entries with type == ORDINARY. */
	
	/* NB: DEL (0177) should be mapped to BS_KEY. */
};

static struct keymap dummy_map[]= {
	{0, SENTINEL, 0, 0}
};

static struct keymap ctlx_map[]= {
	{'N', SHORTCUT, 0, NEXT_WIN},
	{'P', SHORTCUT, 0, PREV_WIN},
	{'D', SHORTCUT, 0, CLOSE_WIN},
	{'\003', SHORTCUT, 0, CLOSE_WIN},	/* ^X-^C */
	{0, SENTINEL, 0, 0}
};

static struct keymap esc_map[]= {
	{'\033', SHORTCUT, 0, MENU_CALL},	/* ESC-ESC */
	{'\007', SHORTCUT, 0, MOUSE_UP},	/* ESC-^G */
	{0, SENTINEL, 0, 0}
};

#ifdef MSDOS
static struct keymap pc_map[]= {
	/* {0003, ORDINARY, 0, 0}, */ /* ^@ should map to true ^@ */
	{0107, SHORTCUT, 0, FIRST_CMD+WC_HOME},
	{0110, SHORTCUT, 0, FIRST_CMD+WC_UP},
	{0113, SHORTCUT, 0, FIRST_CMD+WC_LEFT},
	{0115, SHORTCUT, 0, FIRST_CMD+WC_RIGHT},
	{0120, SHORTCUT, 0, FIRST_CMD+WC_DOWN},
	{0111, SHORTCUT, 0, FIRST_CMD+WC_PAGE_UP},
	{0117, SHORTCUT, 0, FIRST_CMD+WC_END},
	{0121, SHORTCUT, 0, FIRST_CMD+WC_PAGE_DOWN},
	{0123, SHORTCUT, 0, FIRST_CMD+WC_CLEAR},
	{0163, SHORTCUT, 0, FIRST_CMD+WC_META_LEFT},
	{0164, SHORTCUT, 0, FIRST_CMD+WC_META_RIGHT},
	{0165, SHORTCUT, 0, FIRST_CMD+WC_META_END},
	{0166, SHORTCUT, 0, FIRST_CMD+WC_META_PAGE_DOWN},
	{0167, SHORTCUT, 0, FIRST_CMD+WC_META_HOME},
	{0204, SHORTCUT, 0, FIRST_CMD+WC_META_PAGE_UP},
	{0, SENTINEL, 0, 0}
};
#endif /* MSDOS */

static struct keymap *sec_map_list[SECMAPSIZE]= {
	dummy_map,
	ctlx_map,
	esc_map,
#ifdef MSDOS
	pc_map,
#endif
};

struct keymap **_wsecmap= sec_map_list;
