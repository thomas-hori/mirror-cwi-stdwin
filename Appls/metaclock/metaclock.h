/* Clock definitions for metaclock */

/* Max number of points per polygon (for temp array in drawproc) */
#define MAXPOINTS 100

/* Radius of unit circle in clock definitions */
#define SCALE 1000

/* Definition for a single polygon */
typedef struct {
	char *pd_fgcolor;
	char *pd_bgcolor;
	void (*pd_func)(int, POINT []);
	int pd_npoints;
	POINT *pd_points;
	/* The rest are computed at initialization time */
	COLOR pd_fgpixel, pd_bgpixel;
} polydef;

/* Definition for a hand -- just a list of polygons */
typedef struct {
	int hd_npolys;
	polydef *hd_polys;
} handdef;

/* Definition for a clock -- some misc. variables and a bunch of hands */
typedef struct {
	char *cd_title;
	char *cd_fgcolor;
	char *cd_bgcolor;
	handdef cd_background, cd_hours, cd_minutes, cd_seconds, cd_alarm;
	/* The rest are computed at initialization time */
	COLOR cd_fgpixel, cd_bgpixel;
} clockdef;

/* Macro to compute the number of elements in a static array */
#define COUNT(array) (sizeof(array) / sizeof(array[0]))

/* Macro to initialize an array of polydefs */
#define AREF(array) COUNT(array), array
