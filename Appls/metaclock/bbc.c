/* BBC breakfast television clock */

#include "stdwin.h"
#include "metaclock.h"

void
bbc_drawbg(n, points)
	int n;
	POINT *points;
{
}

POINT bbc_1background[] = {
	{   0,    0},
	{1000, 1000},
};

polydef bbc_background[] = {
	{0, 0, bbc_drawbg, AREF(bbc_1background)},
};

POINT bbc_1hours[] = {
	{   0,    0},
	{  40,   40},
	{  40,  700},
	{   0,  740},
	{ -40,  700},
	{ -40,   40},
	{   0,    0},
};

polydef bbc_hours[] = {
	{0, 0, wfillpoly, AREF(bbc_1hours)},
};

POINT bbc_1minutes[] = {
	{   0,    0},
	{  20,   20},
	{  20,  800},
	{   0,  820},
	{ -20,  800},
	{ -20,   20},
	{   0,    0},
};

polydef bbc_minutes[] = {
	{0, 0, wfillpoly, AREF(bbc_1minutes)},
};

POINT bbc_1seconds[] = {
	{   0, 1000},
	{  50,  800},
	{ -50,  800},
	{   0, 1000},
};

polydef bbc_seconds[] = {
	{0, 0, wxorpoly, AREF(bbc_1seconds)},
};

POINT bbc_1alarm[] = {
	{   0, 1000},
	{ 100, 1100},
	{-100, 1100},
	{   0, 1000},
	{   0, 1030},
	{  70, 1070},
	{ -70, 1070},
	{   0, 1030},
};

polydef bbc_alarm[] = {
	{0, 0, wxorpoly, AREF(bbc_1alarm)},
};

clockdef bbc_clock = {
	"BBC breakfast tv",	/* title */
	0, 0,			/* fg, bg */
	{AREF(bbc_background)},
	{AREF(bbc_hours)},
	{AREF(bbc_minutes)},
	{AREF(bbc_seconds)},
	{AREF(bbc_alarm)},
};
