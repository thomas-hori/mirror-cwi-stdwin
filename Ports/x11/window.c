/* X11 STDWIN -- Window operations */

#include "x11.h"
#include "llevent.h"
#include <X11/Xutil.h> /* For Rectangle{In,Out,Part} */

/* Forward */
static void _wsizesubwins();


/* Margin size around inner window (space for scroll and menu bars) */

#define LMARGIN		16
/*
#define TMARGIN		(MIN((_wmf->ascent + _wmf->descent), 16) + 2)
*/
#define TMARGIN		18
#define RMARGIN		0
#define BMARGIN		16

#define IMARGIN		0	/* Extra left/right margin in inner window */


/* Size of outer window border */

#define OBORDER		2


/* XXX IMARGIN and OBORDER should be settable with properties and command
   line options.  Maybe the other margins as well. */


/* Event masks */

/* Mask for 'wo' */
#define OUTER_MASK	( KeyPressMask \
			| FocusChangeMask \
			| EnterWindowMask \
			| LeaveWindowMask \
			| StructureNotifyMask \
			)

/* Mask for other windows except 'wi' */
#define OTHER_MASK	( ButtonPressMask \
			| ButtonReleaseMask \
			| ButtonMotionMask \
			| ExposureMask \
			)

	
/* Private globals */

static int def_h = 0, def_v = 0;
static int def_width = 0, def_height = 0;
static int def_hbar = 0;
static int def_vbar = 1;
static int def_mbar = 1;

#define DEF_WIDTH (def_width > 0 ? def_width : 40*wtextwidth("in", 2))
#define DEF_HEIGHT (def_height > 0 ? def_height : 24*wlineheight())


/* WINDOW list.
   Each WINDOW must be registered here, so it can be found back
   by _whichwin */

static WINDOW **winlist = 0;
static int nwins = 0;


/* Find a WINDOW pointer, given a Window.
   Hunt through the WINDOW list, for each element comparing the given wid
   with the wids of all (sub)Windows */

WINDOW *
_whichwin(w)
	register Window w;
{
	register int i;
	
	for (i = nwins; --i >= 0; ) {
		register WINDOW *win = winlist[i];
		register int j;
		for (j = NSUBS; --j >= 0; ) {
			if (w == win->subw[j].wid)
				return win;
		}
	}
	_wdebug(2, "_whichwin: can't find Window %x", w);
	return NULL;
}


/* Return some window */

WINDOW *
_w_somewin()
{
	if (nwins <= 0)
		return NULL;
	return winlist[0];
}


/* Set the max size of windows created later (ignored for now) */

/*ARGSUSED*/
void
wsetmaxwinsize(width, height)
	int width, height;
{
}


/* Set the initial size of windows created later */

void
wsetdefwinsize(width, height)
	int width, height;
{
	def_width = width;
	def_height = height;
}

void
wgetdefwinsize(pwidth, pheight)
	int *pwidth, *pheight;
{
	*pheight = def_height;
	*pwidth = def_width;
}


/* Set the initial position of windows created later */

void
wsetdefwinpos(h, v)
	int h, v;
{
	def_h = h;
	def_v = v;
}

void
wgetdefwinpos(ph, pv)
	int *ph, *pv;
{
	*ph = def_h;
	*pv = def_v;
}


/* Set the scroll bar options */

void
wsetdefscrollbars(need_hbar, need_vbar)
	int need_hbar, need_vbar;
{
	def_hbar = need_hbar;
	def_vbar = need_vbar;
}

void
wgetdefscrollbars(phbar, pvbar)
	int *phbar, *pvbar;
{
	*phbar = def_hbar;
	*pvbar = def_vbar;
}


/* Set the menu bar options */

void
wsetdefmenubar(need_mbar)
	int need_mbar;
{
	def_mbar = need_mbar;
}

int
wgetdefmenubar()
{
	return def_mbar;
}


/* Read a Bitmap from a file and convert it to a Pixmap.
   XXX Actually I don't convert it to a Pixmap; this may mean that perhaps
   you won't be able to set an icon on a color display, depending
   on the intelligence of the window manager.
   XXX Note that to fix this you must create separate functions to read
   Bitmaps and Pixmaps, as Bitmaps have a real use (for icon masks). */

#define readpixmap readbitmap

/* Read a bitmap from file */

static Pixmap
readbitmap(filename)
	char *filename;
{
	unsigned int width, height;
	int xhot, yhot;
	Pixmap bitmap;
	int err = XReadBitmapFile(_wd, RootWindowOfScreen(_ws), filename,
		&width, &height, &bitmap, &xhot, &yhot);
	if (err != BitmapSuccess) {
		_wwarning("can't read bitmap file %s, error code %d",
			filename, err);
		return None;
	}
	return bitmap;
}


/* Forward */
static bool _wmakesubwins _ARGS((WINDOW *win));


/* Open a WINDOW.
   Some defaults should only be used for the first window opened,
   e.g., window geometry (otherwise all windows would overlay each other!)
   and the "iconic" property.  Icon bitmaps will be used by all windows. */

WINDOW *
wopen(title, drawproc)
	char *title;
	void (*drawproc)();
{
	static bool used_defaults;
	WINDOW *win;
	XSizeHints sizehints;
	char *geom;
	
	/* Allocate zeroed storage for the WINDOW structure
	   and fill in the easy non-zero values */
	win = (WINDOW*) calloc(sizeof(WINDOW), 1);
	if (win == NULL) {
		_werror("wopen: can't alloc storage for window");
		return NULL;
	}
	win->drawproc = drawproc;
	win->careth = win->caretv = -1;
	win->attr = wattr;
	win->tmargin = def_mbar ? TMARGIN : 0;
	win->rmargin = RMARGIN;
	win->bmargin = def_hbar ? BMARGIN : 0;
	win->lmargin = def_vbar ? LMARGIN : 0;

	sizehints.x = def_h <= 0 ? 0 : def_h - win->lmargin - OBORDER;
	sizehints.y = def_v <= 0 ? 0 : def_v - win->tmargin - OBORDER;
	sizehints.width = DEF_WIDTH + win->lmargin + win->rmargin
			 + 2*IMARGIN;
	sizehints.height = DEF_HEIGHT + win->tmargin + win->bmargin;
	sizehints.flags = PSize;
	if (def_h > 0 || def_v > 0)
		sizehints.flags |= PPosition | USPosition;
		/* USPosition added to fool twm */

#ifndef PRE_R4
	/* Set base size and resize increment */
	sizehints.base_width = win->lmargin + win->rmargin + 2*IMARGIN;
	sizehints.base_height = win->tmargin + win->bmargin;
	sizehints.width_inc = 1;
	sizehints.height_inc = 1;
	sizehints.flags |= PBaseSize | PResizeInc;
#endif

	/* Parse user-specified geometry default.
	   This overrides what the application specified.
	   Note that the x and y stored internally are exclusive or borders,
	   while X geometries specify x and y including the border.
	   XXX Also note that the obsolete sizehints members x, y, width and
	   height are used later to actually create the window. */

	if (!used_defaults &&
		(geom = _wgetdefault("geometry", "Geometry")) != NULL) {
		unsigned int width, height;

		int flags = XParseGeometry(geom,
			&sizehints.x, &sizehints.y, &width, &height);
		if (flags & WidthValue)
			sizehints.width = width
				+ win->lmargin + win->rmargin
				+ 2*IMARGIN;
		if (flags & HeightValue)
			sizehints.height = height
				+ win->tmargin + win->bmargin;
		if (flags & XNegative)
			sizehints.x =
				WidthOfScreen(_ws) + sizehints.x
				- sizehints.width - OBORDER;
		if (flags & YNegative)
			sizehints.y =
				HeightOfScreen(_ws) + sizehints.y
				- sizehints.height - OBORDER;

		/* Use the user-specified size as the default
		   size for future windows */

		if (flags & WidthValue)
			def_width = width;
		if (flags & HeightValue)
			def_height = height;

		/* If the user has given as position,
		   pretend a size is also given, otherwise
		   UWM will still ask for interactive
		   window placement.  I'm in good company:
		   "the" Toolkit also does this. */

		if (flags & (XValue|YValue))
			sizehints.flags |= USPosition|USSize;
		else if (flags & (WidthValue|HeightValue))
			sizehints.flags |= USSize;

#ifndef PRE_R4
		/* Derive the gravity hint from the geometry */

		if ((flags & XNegative) || (flags & YNegative)) {
			sizehints.flags |= PWinGravity;
			if (flags & XNegative) {
				if (flags & YNegative)
					sizehints.win_gravity =
						SouthEastGravity;
				else
					sizehints.win_gravity =
						NorthEastGravity;
			}
			else
				sizehints.win_gravity =
					SouthWestGravity;
		}
#endif			
	}
	
	/* Set the initial geometry from the size hints just computed */
	win->wo.border = OBORDER;
	win->wo.x = sizehints.x + win->wo.border;
	win->wo.y = sizehints.y + win->wo.border;
	win->wo.width = sizehints.width;
	win->wo.height = sizehints.height;
	
	/* Set the foreground and background pixel values */
	win->fga = _w_fgcolor;
	win->bga = _w_bgcolor;
	win->fgo = _wgetpixel("menuForeground", "MenuForeground", _w_fgcolor);
	win->bgo = _wgetpixel("menuBackground", "MenuBackground", _w_bgcolor);
	
	/* Create the outer Window */
	if (!_wcreate(&win->wo, RootWindowOfScreen(_ws), 0, FALSE,
		win->fgo, win->bgo)) {
		FREE(win);
		return NULL;
	}
	
	/* Create the inner subWindows */
	if (!_wmakesubwins(win)) {
		FREE(win);
		return NULL;
	}
	
	/* Create the Graphics Contexts */
	win->gc = _wgcreate(win->wo.wid, _wmf->fid, win->fgo, win->bgo);
	win->gca = _wgcreate(win->wa.wid, _wf->fid, win->fga, win->bga);

	/* Set the plane mask so _winvert keeps working... */
	XSetPlaneMask(_wd, win->gc, win->fgo ^ win->bgo);

	/* Change selected Window properties */
	_wsetmasks(win);
	_w_setgrayborder(win);
	
	/* Set the "invalid" region to empty (rely on Expose events) */
	win->inval = XCreateRegion();
	
	/* Initialize the menu bar stuff */
	_waddmenus(win);
	
	/* Set window properties */
	
	/* Set window and icon names */
	{
		char *windowname = title;
		char *iconname = NULL;
		char *p;
		
		/* Resources may override these for the first window */
		if (!used_defaults) {
			if ((p = _wgetdefault("title", "Title")) != NULL)
				windowname = iconname = p;
			if ((p = _wgetdefault("iconName", "IconName")) != NULL)
				iconname = p;
		}
		
		XStoreName(_wd, win->wo.wid, windowname);
		/* Only store the icon name if specified -- the WM will
		   derive a default from the title otherwise. */
		if (iconname != NULL)
			XSetIconName(_wd, win->wo.wid, iconname);
	}
	
	/* Set command line (computed by winitargs()) */
	XChangeProperty(_wd, win->wo.wid,
		XA_WM_COMMAND, XA_STRING, 8, PropModeReplace,
		(unsigned char *)_wm_command, _wm_command_len);
		/* XXX The ICCCM prescribes that exactly one window
		   of a given client has this property.  Later. */
	
	/* Set normal size hints (computed above) */
#ifdef PRE_R4
	XSetNormalHints(_wd, win->wo.wid, &sizehints);
#else
	/* XSetNormalHints carefully masks away the base size and
	   window gravity, so we must call XChangeProperty directly. */
	/* XXX This is not correct on machines where int != long. Sigh. */
	XChangeProperty(_wd, win->wo.wid,
		XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32, PropModeReplace,
		(unsigned char *) &sizehints, sizeof sizehints / 4);
#endif
	
	/* Set WM Hints */
	{
		XWMHints wmhints;
		char *value;
		wmhints.flags = InputHint | StateHint;
		wmhints.input = _wgetbool("input", "Input", 1);
		if (!used_defaults &&
			_wgetbool("iconic", "Iconic", 0))
			wmhints.initial_state = IconicState;
		else
			wmhints.initial_state = NormalState;
		if (!used_defaults && (value =
			_wgetdefault("iconGeometry", "IconGeometry"))
				!= NULL) {
			unsigned int width, height;
			int flags = XParseGeometry(value,
				&wmhints.icon_x, &wmhints.icon_y,
				&width, &height);
			if (flags & XNegative)
				wmhints.icon_x =
					WidthOfScreen(_ws) - wmhints.icon_x;
			if (flags & YNegative)
				wmhints.icon_y =
					HeightOfScreen(_ws) - wmhints.icon_y;
			if (flags & (XValue|YValue)) {
				wmhints.flags |= IconPositionHint;
				_wdebug(1, "icon pos: %d,%d",
					wmhints.icon_x, wmhints.icon_y);
			}
			else
				_wdebug(1, "no icon pos");
		}
		value = _wgetdefault("iconBitmap", "IconBitmap");
		if (value != NULL)  {
			wmhints.icon_pixmap = readpixmap(value);
			if (wmhints.icon_pixmap != None)
				wmhints.flags |= IconPixmapHint;
		}
		value = _wgetdefault("iconMask", "IconMask");
		if (value != NULL)  {
			wmhints.icon_mask = readbitmap(value);
			if (wmhints.icon_mask != None)
				wmhints.flags |= IconMaskHint;
		}
		/* XXX What about window groups? */
		XSetWMHints(_wd, win->wo.wid, &wmhints);
	}
	
	/* Set class (same as strings used by _wgetdefault() */
	{
		XClassHint classhint;
		classhint.res_name = _wprogname;
		classhint.res_class = "Stdwin";
		XSetClassHint(_wd, win->wo.wid, &classhint);
	}
	
	/* Set client machine */
	XChangeProperty(_wd, win->wo.wid,
		XA_WM_CLIENT_MACHINE, XA_STRING, 8, PropModeReplace,
		(unsigned char *)_whostname, strlen(_whostname));
	
	/* Set protocols property */
	{
		static Atom protocols[] = {
			0 /*XA_WM_DELETE_WINDOW*/,
		};
		protocols[0] = _wm_delete_window;
		XChangeProperty(_wd, win->wo.wid,
			_wm_protocols, XA_ATOM, 32, PropModeReplace,
			(unsigned char *)protocols,
			sizeof(protocols) / sizeof(protocols[0]));
	}
	
	/* Store the WINDOW pointer in the list of windows */
	L_APPEND(nwins, winlist, WINDOW *, win);
	
	/* Now we're ready, finally show the window, on top of others */
	XMapRaised(_wd, win->wo.wid);
	
	/* Note that we've used the once-only defaults */
	used_defaults = TRUE;
	
	/* Don't forget to return the WINDOW pointer */
	return win;
}


/* Make a window the active window -- ICCCM version.
   - De-iconify.
   - Raise the window.
   Any events that may follow from this are handled later when we get them.
*/

void
wsetactive(win)
	WINDOW *win;
{
	XMapRaised(_wd, win->wo.wid); /* The ICCCM way to de-iconify */
	XRaiseWindow(_wd, win->wo.wid);
}


/* Fetch a color, by name */

COLOR
wfetchcolor(cname)
	char *cname;
{
	return (COLOR) _w_fetchpixel(cname, (unsigned long)BADCOLOR);
}


/* Get a pixel value from the resource database */

unsigned long
_wgetpixel(resname, resclassname, defpixel)
	char *resname;
	char *resclassname;
	unsigned long defpixel;
{
	char *cname;
	Colormap cmap;
	XColor hard, exact;
	
	cname = _wgetdefault(resname, resclassname);
	if (cname == NULL)
		return defpixel;
	return _w_fetchpixel(cname, defpixel);
}


/* Translate a color name or #RGB spec into a pixel value */

unsigned long
_w_fetchpixel(cname, defpixel)
	char *cname;
	unsigned long defpixel;
{
	Colormap cmap;
	XColor hard, exact;
	
	cmap = DefaultColormapOfScreen(_ws);
	if (!XParseColor(_wd, cmap, cname, &exact)) {
		_wdebug(1, "%s: no such color", cname);
		return defpixel;
	}
	hard = exact;
	if (!XAllocColor(_wd, cmap, &hard)) {
		_wdebug(1, "%s: can't allocate color cell", cname);
		return defpixel;
	}
	return hard.pixel;
}


/* Return a gray pixmap */

Pixmap
_w_gray() {
#include <X11/bitmaps/gray>
/* defines gray_bits, gray_width, gray_height */
	
	static Pixmap gray;
	
	if (gray == 0) {
		gray = XCreatePixmapFromBitmapData(_wd,
			RootWindowOfScreen(_ws),
			gray_bits, gray_width, gray_height,
			BlackPixelOfScreen(_ws),
			WhitePixelOfScreen(_ws),
			DefaultDepthOfScreen(_ws));
	}
	
	return gray;
}


/* Rasters used by _w_raster below */

#define raster_width 4
#define raster_height 4
static char raster_bits[][4] = {
	{0x00, 0x00, 0x00, 0x00},
	{0x01, 0x00, 0x00, 0x00},
	{0x01, 0x00, 0x04, 0x00},
	{0x08, 0x00, 0x04, 0x01},
	{0x05, 0x00, 0x05, 0x00},
	{0x01, 0x0a, 0x00, 0x0a},
	{0x05, 0x02, 0x05, 0x08},
	{0x05, 0x02, 0x05, 0x0a},
	{0x05, 0x0a, 0x05, 0x0a},
	{0x07, 0x0a, 0x05, 0x0a},
	{0x07, 0x0a, 0x0d, 0x0a},
	{0x0a, 0x0f, 0x05, 0x0e},
	{0x0f, 0x0a, 0x0f, 0x0a},
	{0x07, 0x0f, 0x0b, 0x0e},
	{0x0e, 0x0f, 0x0b, 0x0f},
	{0x0f, 0x0f, 0x0f, 0x0b},
	{0xff, 0xff, 0xff, 0xff},
};
#define NRASTERS (sizeof raster_bits / sizeof raster_bits[0])

/* Return a raster of a certain percentage */

Pixmap
_w_raster(percent)
	int percent;
{
	static Pixmap raster[NRASTERS];
	int i = (percent * NRASTERS + 50) / 100;

	if (i < 0)
		i = 0;
	if (i >= NRASTERS)
		i = NRASTERS-1;
	if (raster[i] == 0) {
		raster[i] = XCreateBitmapFromData(_wd,
			RootWindowOfScreen(_ws),
			raster_bits[i], raster_width, raster_height);
	}
	return raster[i];
}


/* Set the border pixmap of a window to a gray pattern */

void
_w_setgrayborder(win)
	WINDOW *win;
{
	XSetWindowBorderPixmap(_wd, win->wo.wid, _w_gray());
}


/* Create a Graphics Context using the given Window and Font ids and colors.
   Don't set the font if Font id is zero. */

GC
_wgcreate(wid, fid, fg, bg)
	Window wid;
	Font fid;
	unsigned long fg, bg;
{
	int mask = GCForeground|GCBackground;
	XGCValues v;
	
	v.foreground = fg;
	v.background = bg;
	if (fid != 0) {
		v.font = fid;
		mask |= GCFont;
	}
	return XCreateGC(_wd, wid, mask, &v);
}


/* Create a Window for a given windata struct and set its event mask.
   If map is TRUE, also map it.
   Initially, a window is not dirty (it'll get Expose events for that) */

bool
_wcreate1(wp, parent, cursor, map, fg, bg, nowm)
	struct windata *wp;
	Window parent;
	int cursor;
	bool map;
	unsigned long fg, bg;
	bool nowm;
{
	XSetWindowAttributes attributes;
	unsigned long valuemask;

	/* Don't create empty windows */
	if (wp->width == 0 || wp->height == 0) {
		wp->wid = None;
		return TRUE;
	}

	valuemask = CWBackPixel|CWBorderPixel|CWBitGravity|CWBackingStore;
	attributes.background_pixel = bg;
	attributes.border_pixel = fg;
	attributes.bit_gravity = NorthWestGravity;
	if (nowm) {
		attributes.override_redirect = 1;
		valuemask |= CWOverrideRedirect;
	}
	attributes.backing_store = NotUseful; /* Seems to be Harmful... */

	if (cursor > 0) {
		attributes.cursor = XCreateFontCursor(_wd, cursor);
		valuemask |= CWCursor;
	}
	
	/* We must subtract border width from x and y before
	   passing them to WCreateSimpleWindow, since
	   they refer to the upper left corner of the border! */
	
	wp->wid = XCreateWindow(
			_wd,
			parent,
			wp->x - wp->border,	/* x */
			wp->y - wp->border,	/* y */
			wp->width,
			wp->height,
			wp->border,		/* border width */
			CopyFromParent,		/* depth */
			InputOutput,		/* class */
			CopyFromParent,		/* visual */
			valuemask,
			&attributes
		);
	if (!wp->wid) {
		_werror("_wcreate: can't create (sub)Window");
		return FALSE;
	}
	_wdebug(2, "_wcreate: wid=0x%x", wp->wid);
	if (map)
		XMapWindow(_wd, wp->wid);
	wp->dirty = FALSE;
	return TRUE;
}


/* Set the save-under property for a window given by a windata struct */

void
_wsaveunder(wp, flag)
	struct windata *wp;
	Bool flag;
{
	static int saveUnder = -1;
	XSetWindowAttributes attrs;

	if (wp->wid == None)
		return;
	
	/* The user may explicitly turn off save-unders by specifying
		Stdwin*SaveUnder: off
	   since they are broken on some servers */
	
	if (saveUnder < 0) {
		/* First time here: check resource database */
		saveUnder = _wgetbool("saveUnder", "SaveUnder", 1);
		if (!saveUnder)
			_wdebug(1, "user doesn't want save-unders");
	}
	
	if (!saveUnder)
		return;
	
	attrs.save_under = flag;
	XChangeWindowAttributes(_wd, wp->wid, CWSaveUnder, &attrs);
}


/* Set gravity for a window given by a windata struct */

static void
_wgravity(wp, grav)
	struct windata *wp;
	int grav;
{
	XSetWindowAttributes attrs;
	if (wp->wid == None)
		return;
	attrs.win_gravity = grav;
	XChangeWindowAttributes(_wd, wp->wid, CWWinGravity, &attrs);
}


/* Move and resize a window given by a windata struct */

void
_wmove(wp)
	struct windata *wp;
{
	if (wp->wid == None)
		return;
	XMoveResizeWindow(_wd, wp->wid,
	    wp->x - wp->border, wp->y - wp->border, wp->width, wp->height);
}


/* (Re)compute the sizes and positions of all subwindows.
   Note a check (in SZ) to prevent windows ever to get a size <= 0 */

static void
_wsizesubwins(win)
	WINDOW *win;
{
	int bmargin = win->wi.height - win->doc.height - win->wa.y;
	int rmargin = win->wi.width - IMARGIN - win->doc.width - win->wa.x;

#define SZ(elem, nx, ny, nw, nh, nb) \
		(win->elem.x = (nx), \
		win->elem.y = (ny), \
		win->elem.width = (nw) > 0 ? (nw) : 1, \
		win->elem.height = (nh) > 0 ? (nh) : 1, \
		win->elem.border = (nb))

	/* Interior window in the middle */
	SZ(wi, win->lmargin, win->tmargin,
		win->wo.width - win->lmargin - win->rmargin,
		win->wo.height - win->tmargin - win->bmargin, 0);
	/* Menu bar at the top */
	if (win->tmargin)
		SZ(mbar, 0, 0, win->wo.width, win->tmargin - IBORDER, IBORDER);
	else
		SZ(mbar, 0, 0, 0, 0, 0);
	/* Vbar left */
	if (win->lmargin)
		SZ(vbar, 0, win->wi.y, win->lmargin - IBORDER,
		   win->wi.height, IBORDER);
	else
		SZ(vbar, 0, 0, 0, 0, 0);
	/* Hbar at the bottom */
	if (win->bmargin)
		SZ(hbar, win->wi.x, win->wo.height - win->bmargin + IBORDER,
		   win->wi.width, win->bmargin - IBORDER, IBORDER);
	else
		SZ(hbar, 0, 0, 0, 0, 0);

#undef SZ
	
	/* The document window (wa) is sized differently.
	   If it fits in the window, it is made the same size
	   and aligned at (0,0) no questions asked.
	   Otherwise, it is moved so that the window never shows more
	   outside it than before (if at all possible). */
	
	if (win->doc.width <= win->wi.width - 2*IMARGIN) {
		win->wa.x = IMARGIN;
		win->wa.width = win->wi.width - 2*IMARGIN;
	}
	else {
		CLIPMIN(rmargin, IMARGIN);
		CLIPMIN(win->wa.x, win->wi.width - win->wa.width - rmargin);
		CLIPMAX(win->wa.x, IMARGIN);
		win->wa.width = win->doc.width;
		CLIPMIN(win->wa.width, win->wi.width - win->wa.x);
	}
	if (win->doc.height <= win->wi.height) {
		win->wa.y = 0;
		win->wa.height = win->wi.height;
	}
	else {
		CLIPMIN(bmargin, 0);
		CLIPMIN(win->wa.y, win->wi.height - win->wa.height - bmargin);
		CLIPMAX(win->wa.y, 0);
		win->wa.height = win->doc.height;
		CLIPMIN(win->wa.height, win->wi.height - win->wa.y);
	}
	win->wa.border = IMARGIN;
}


/* Create the permanently visible subwindows.
   Return TRUE if all creations succeeded.
   The inner window is created after all bars, so it lies on top,
   and will receive clicks in its border */

static bool
_wmakesubwins(win)
	WINDOW *win;
{
	Window w = win->wo.wid;
	unsigned long fg = win->fgo, bg = win->bgo;
	
	_wsizesubwins(win);
	if (!(	_wcreate(&win->mbar, w, XC_arrow, TRUE, fg, bg) &&
		_wcreate(&win->vbar, w, XC_sb_v_double_arrow, TRUE, fg, bg) &&
		_wcreate(&win->hbar, w, XC_sb_h_double_arrow, TRUE, fg, bg) &&
		_wcreate(&win->wi, w, 0, TRUE, fg, bg) &&
		_wcreate(&win->wa, win->wi.wid, XC_crosshair, TRUE,
			win->bga, win->bga)))
		return FALSE;
	_wgravity(&win->hbar, SouthWestGravity);
	return TRUE;
}


/* Resize all subwindows and move them to their new positions.
   The application and current menu windows (WA, MWIN) are not resized.
   The application window may be moved, however, to prevent exposing
   parts outside the document that weren't visible earlier. */

void
_wmovesubwins(win)
	WINDOW *win;
{
	int i;
	_wsizesubwins(win);
	
	for (i = 1; i <= WA; ++i)
		_wmove(&win->subw[i]);
	
	/* Invalidate scroll bars after a resize */
	win->hbar.dirty = win->vbar.dirty = _w_dirty = TRUE;
}


/* Set normal event masks for all (sub)Windows of a WINDOW */

void
_wsetmasks(win)
	WINDOW *win;
{
	int i;
	
	XSelectInput(_wd, win->wo.wid, OUTER_MASK);
	for (i = 1; i <= WA; ++i) {
		if (win->subw[i].wid != 0)
			XSelectInput(_wd, win->subw[i].wid,
				(i == WI) ? NoEventMask : OTHER_MASK);
	}
}


/* Generate any pending size event. */

bool
_w_doresizes(ep)
	EVENT *ep;
{
	int i;

	for (i = nwins; --i >= 0; ) {
		WINDOW *win = winlist[i];
		if (win->resized) {
			win->resized = FALSE;
			ep->type = WE_SIZE;
			ep->window = win;
			if (i == 0)
				_w_resized = FALSE;
			return TRUE;
		}
	}
	_w_resized = FALSE;
	return FALSE;
}


/* Forward */
static bool update _ARGS((WINDOW *win, EVENT *ep));


/* Perform any pending window updates.
   If the application subwindow needs an update,
   either call its draw procedure or generate a WE_DRAW event
   in the given event record (and then stop).
   Return TRUE if an event was generated */

bool
_w_doupdates(ep)
	EVENT *ep;
{
	int i;

	for (i = nwins; --i >= 0; ) {
		if (update(winlist[i], ep)) {
			if (i == 0)
				_w_dirty = FALSE;
			return TRUE;
		}
	}
	_w_dirty = FALSE;
	return FALSE;
}

void
wupdate(win)
	WINDOW *win;
{
	(void) update(win, (EVENT *)NULL);
}

/* Update any parts of a window that need updating.
   If the application window needs redrawing and there is no drawproc
   and the 'ep' argument is non-nil,
   construct a DRAW event and return TRUE. */

static bool
update(win, ep)
	WINDOW *win;
	EVENT *ep;
{
	bool ret = FALSE;
	
	if (win->mbar.dirty)
		_wdrawmbar(win);
	if (win->hbar.dirty)
		_wdrawhbar(win);
	if (win->vbar.dirty)
		_wdrawvbar(win);
	win->mbar.dirty = win->hbar.dirty = win->vbar.dirty = FALSE;
	/* wi and wo have nothing that can be drawn! */
	
	if (win->wa.dirty && (win->drawproc != NULL || ep != NULL)) {
		XRectangle clip;
		int left, top, right, bottom;
		XClipBox(win->inval, &clip);
		left = clip.x;
		top = clip.y;
		right = left + clip.width;
		bottom = top + clip.height;
		CLIPMIN(left, -win->wa.x);
		CLIPMIN(top, -win->wa.y);
		CLIPMAX(right, win->wi.width - win->wa.x);
		CLIPMAX(bottom, win->wi.height - win->wa.y);
		_wdebug(3, "clip: (%d,%d) to (%d,%d)",
			left, top, right, bottom);
		if (left < right && top < bottom) {
			_whidecaret(win);
			if (win->drawproc != NULL) {
				/* A bug in X11R2 XSetRegion prevents this
				   from working: */
#ifndef PRE_R3
				/* Version for R3 or later */
				XSetRegion(_wd, win->gca, win->inval);
#else
				/* Version for R2 */
				XSetClipRectangles(_wd, win->gca,
					0, 0, &clip, 1, Unsorted);
#endif
				wbegindrawing(win);
				werase(left, top, right, bottom);
				(*win->drawproc)(win,
					left, top, right, bottom);
				wenddrawing(win);
				XSetClipMask(_wd, win->gca, (Pixmap)None);
			}
			else {
				XClearArea(_wd, win->wa.wid,
					clip.x, clip.y,
					clip.width, clip.height, FALSE);
				ep->type = WE_DRAW;
				ep->window = win;
				ep->u.area.left = left;
				ep->u.area.top = top;
				ep->u.area.right = right;
				ep->u.area.bottom = bottom;
				ret = TRUE;
			}
			_wshowcaret(win);
		}
		XDestroyRegion(win->inval);
		win->inval = XCreateRegion();
		win->wa.dirty = FALSE;
	}
	return ret;
}


/* Close a window */

void
wclose(win)
	WINDOW *win;
{
	int i;
	for (i = 0; i < nwins; ++i) {
		if (winlist[i] == win)
			break;
	}
	if (i >= nwins) {
		_werror("wclose: unknown window");
		return;
	}
	L_REMOVE(nwins, winlist, WINDOW *, i);
	_w_deactivate(win);
	_w_delmenus(win);
	XFreeGC(_wd, win->gc);
	XFreeGC(_wd, win->gca);
	XDestroyWindow(_wd, win->wo.wid);
	XFlush(_wd); /* Make the effect immediate */
	FREE(win);
}


/* Change a window's title */

void
wsettitle(win, title)
	WINDOW *win;
	char *title;
{
	XStoreName(_wd, win->wo.wid, title);
	XFlush(_wd); /* Make the effect immediate */
	/* The icon name will not change */
}


/* Return a window's current title -- straight from the window property */

char *
wgettitle(win)
	WINDOW *win;
{
	static char *title = NULL;
	
	if (title != NULL)
		free(title); /* Free result of previous call */
	title = NULL; /* Just in case */
	if (!XFetchName(_wd, win->wo.wid, &title))
		_wwarning("wgettitle: no title set");
	return title;
}


/* Change a window's icon name */

void
wseticontitle(win, title)
	WINDOW *win;
	char *title;
{
	XSetIconName(_wd, win->wo.wid, title);
}


/* Return a window's current icon name -- straight from the window property */

char *
wgeticontitle(win)
	WINDOW *win;
{
	static char *title = NULL;
	
	if (title != NULL)
		free(title); /* Free result of previous call */
	title = NULL; /* Just in case */
	if (!XGetIconName(_wd, win->wo.wid, &title))
		_wdebug(1, "wgeticontitle: no icon name set");
		/* This may occur normally so don't make it a warning */
	return title;
}


/* Get a window's size.
   This is really the size of the visible part of the document. */

void
wgetwinsize(win, pwidth, pheight)
	WINDOW *win;
	int *pwidth, *pheight;
{
	*pwidth = win->wi.width - 2*IMARGIN;
	*pheight = win->wi.height;
}


/* Get a window's position relative to the root */

void
wgetwinpos(win, ph, pv)
	WINDOW *win;
	int *ph, *pv;
{
	Window child;
	
	if (!XTranslateCoordinates(	_wd,
					win->wi.wid,
					RootWindowOfScreen(_ws),
					IMARGIN, 0,
					ph, pv,
					&child)) {
		
		_wwarning("wgetwinpos: XTranslateCoordinates failed");
		*ph = 0;
		*pv = 0;
	}
}


/* Set a window's size */

void
wsetwinsize(win, width, height)
	WINDOW *win;
	int width, height;
{
	width = width + win->lmargin + win->rmargin + 2*IMARGIN;
	height = height + win->tmargin + win->bmargin;
	XResizeWindow(_wd, win->wo.wid, width, height);
	XFlush(_wd); /* Make the effect immediate */
}


/* Set a window's position */

void
wsetwinpos(win, h, v)
	WINDOW *win;
	int h, v;
{
	h = h - win->lmargin - OBORDER;
	v = v - win->tmargin - OBORDER;
	XMoveWindow(_wd, win->wo.wid, h, v);
	XFlush(_wd); /* Make the effect immediate */
}


/* "Change" part of a document, i.e., post a WE_DRAW event */

void
wchange(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	_wdebug(3, "wchange: %d %d %d %d", left, top, right, bottom);
	if (left < right && top < bottom) {
		XRectangle r;
		r.x = left;
		r.y = top;
		r.width = right - left;
		r.height = bottom - top;
		XUnionRectWithRegion(&r, win->inval, win->inval);
		win->wa.dirty = TRUE;
		_w_dirty = TRUE;
	}
}


/* "Scroll" part of a document, i.e., copy bits and erase the place
   where they came from.  May cause WE_DRAW events if source bits are
   covered. */

void
wscroll(win, left, top, right, bottom, dh, dv)
	WINDOW *win;
	int left, top, right, bottom;
	int dh, dv;
{
	int src_x = left, src_y = top;
	int dest_x = left, dest_y = top;
	int width = right - left - ABS(dh);
	int height = bottom - top - ABS(dv);
	
	if (dh == 0 && dv == 0 || left >= right || top >= bottom)
		return;
	
	if (dh < 0)
		src_x += -dh;
	else
		dest_x += dh;
	if (dv < 0)
		src_y += -dv;
	else
		dest_y += dv;
	
	_wdebug(2, "wscroll: src(%d,%d)size(%d,%d)dest(%d,%d)",
		src_x, src_y, width, height, dest_x, dest_y);
	if (width > 0 && height > 0) {
		_whidecaret(win);
		XCopyArea(_wd, win->wa.wid, win->wa.wid, win->gca,
			  src_x, src_y, width, height, dest_x, dest_y);
		_wshowcaret(win);
	}
	
	if (XRectInRegion(win->inval, left, top, right-left, bottom-top)
							!= RectangleOut) {
		/* Scroll the overlap between win->inval and the
		   scrolled rectangle:
			scroll := <the entire scrolling rectangle>
			overlap := inval * scroll
			inval -:= overlap
			shift overlap by (dh, dv)
			overlap := overlap * scroll
			inval +:= overlap
		*/
		Region scroll = XCreateRegion();
		Region overlap = XCreateRegion();
		XRectangle r;
		r.x = left;
		r.y = top;
		r.width = right-left;
		r.height = bottom-top;
		XUnionRectWithRegion(&r, scroll, scroll);
		XIntersectRegion(win->inval, scroll, overlap);
		XSubtractRegion(win->inval, overlap, win->inval);
		XOffsetRegion(overlap, dh, dv);
		XIntersectRegion(overlap, scroll, overlap);
		XUnionRegion(win->inval, overlap, win->inval);
		XDestroyRegion(overlap);
		XDestroyRegion(scroll);

		/* There's still a bug left: exposure events in the queue
		   (like GraphicsExposures caused be previous scrolls!)
		   should be offset as well, bu this is too complicated
		   to fix right now... */
	}
	
	/* Clear the bits scrolled in */
	
	if (dh > 0)
		XClearArea(_wd, win->wa.wid,
			left, top, dh, bottom-top, FALSE);
	else if (dh < 0)
		XClearArea(_wd, win->wa.wid,
			right+dh, top, -dh, bottom-top, FALSE);
	if (dv > 0)
		XClearArea(_wd, win->wa.wid,
			left, top, right-left, dv, FALSE);
	else if (dv < 0)
		XClearArea(_wd, win->wa.wid,
			left, bottom+dv, right-left, -dv, FALSE);
}


/* Change the "origin" of a window (position of document, really) */

void
wsetorigin(win, orgh, orgv)
	WINDOW *win;
	int orgh, orgv;
{
	bool moveit = FALSE;
	
	CLIPMIN(orgh, 0);
	CLIPMIN(orgv, 0);
	if (win->wa.x != IMARGIN - orgh) {
		win->hbar.dirty = moveit = TRUE;
		win->wa.x = IMARGIN - orgh;
	}
	if (win->wa.y != -orgv) {
		win->vbar.dirty = moveit = TRUE;
		win->wa.y = -orgv;
	}
	if (moveit)
		XMoveWindow(_wd, win->wa.wid,
			    IMARGIN - orgh - win->wa.border,
			    -orgv - win->wa.border);
}


/* Get the "origin" (see above) of a window */

void
wgetorigin(win, ph, pv)
	WINDOW *win;
	int *ph, *pv;
{
	*ph = IMARGIN - win->wa.x;
	*pv = -win->wa.y;
}


/* Set the document size.  Zero means don't use a scroll bar */

void
wsetdocsize(win, width, height)
	WINDOW *win;
	int width, height;
{
	bool dirty = FALSE;
	
	CLIPMIN(width, 0);
	CLIPMIN(height, 0);
	if (win->doc.width != width) {
		win->doc.width = width;
		if (width <= win->wo.width - IMARGIN) {
			win->wa.x = IMARGIN;
			win->wa.width = win->wi.width;
		}
		else {
			win->wa.width = width;
			CLIPMIN(win->wa.width, win->wi.width - win->wa.x);
		}
		win->hbar.dirty = dirty = TRUE;
	}
	if (win->doc.height != height) {
		win->doc.height = height;
		if (height <= win->wo.height) {
			win->wa.y = 0;
			win->wa.height = win->wi.height;
		}
		else {
			win->wa.height = height;
		}
		win->vbar.dirty = dirty = TRUE;
	}
	if (dirty) {
		_w_dirty = TRUE;
		_wmove(&win->wa);
	}
}


/* Return the document size last set by wsetdocsize() */

void
wgetdocsize(win, pwidth, pheight)
	WINDOW *win;
	int *pwidth, *pheight;
{
	*pwidth = win->doc.width;
	*pheight = win->doc.height;
}


/* Change the cursor for a window */

void
wsetwincursor(win, cursor)
	WINDOW *win;
	CURSOR *cursor;
{
	Cursor c;
	if (cursor == NULL)
		c = None;
	else
		c = (Cursor) cursor;
	XDefineCursor(_wd, win->wa.wid, c);
	XFlush(_wd); /* Make the effect immediate */
}


/* Scroll the document in the window to ensure that the given rectangle
   is visible, if at all possible.  Don't scroll more than necessary. */

void
wshow(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	int orgh, orgv;
	int winwidth, winheight;
	int extrah, extrav;
	
	_wdebug(3, "wshow: %d %d %d %d", left, top, right, bottom);

	wgetorigin(win, &orgh, &orgv);
	wgetwinsize(win, &winwidth, &winheight);
	
	if (left >= orgh &&
		top >= orgv &&
		right <= orgh + winwidth &&
		bottom <= orgv + winheight)
		return; /* Already visible */
	
	extrah = (winwidth - (right - left)) / 2;
	CLIPMAX(extrah, win->doc.width - right);
	CLIPMIN(extrah, 0);
	orgh = right + extrah - winwidth;
	CLIPMAX(orgh, left);
	CLIPMIN(orgv, 0);
	
	extrav = (winheight - (bottom - top)) / 2;
	CLIPMAX(extrav, win->doc.height - bottom);
	CLIPMIN(extrav, 0);
	orgv = bottom + extrav - winheight;
	CLIPMAX(orgv, top);
	CLIPMIN(orgv, 0);
	
	wsetorigin(win, orgh, orgv);
}


/* Sound the bell (beep) */

void
wfleep()
{
	XBell(_wd, 0);
}


/* Helper functions for the menu mananger: */

_waddtoall(mp)
	MENU *mp;
{
	int i;
	
	for (i = nwins; --i >= 0; )
		wmenuattach(winlist[i], mp);
}


_wdelfromall(mp)
	MENU *mp;
{
	int i;
	
	for (i = nwins; --i >= 0; )
		wmenudetach(winlist[i], mp);
}


/* Helper function for the timer manager: */

WINDOW *
_wnexttimer()
{
	int i;
	WINDOW *cand = NULL;
	
	for (i = nwins; --i >= 0; ) {
		WINDOW *win = winlist[i];
		long t = win->timer;
		if (t != 0) {
			if (cand == NULL || t < cand->timer)
				cand = win;
		}
	}
	return cand;
}


/* Delete all windows -- called by wdone() */

_wkillwindows()
{
	while (nwins > 0)
		wclose(winlist[nwins-1]);
}


/* X11-specific hack: get the window ID of a window's document window.
   This is used by the Ghostscript bridge. */

long
wgetxwindowid(win)
	WINDOW *win;
{
	return win->wa.wid;
}


_winsetup()
{
	def_mbar = _wgetbool("menubar", "Menubar", def_mbar);
	def_hbar = _wgetbool("horizontalScrollbar", "Scrollbar", def_hbar);
	def_vbar = _wgetbool("verticalScrollbar", "Scrollbar", def_hbar);
}
