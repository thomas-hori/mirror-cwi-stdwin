/* Hema watch */

#include "stdwin.h"
#include "metaclock.h"

void
hema_1drawbg(n, points)
	int n;
	POINT *points;
{
	wfillelarc(points[0].h, points[0].v,
		points[1].h - points[0].h, points[0].v - points[1].v,
		0, 360);
}

void
hema_2drawbg(n, points)
	int n;
	POINT *points;
{
	wfillelarc(points[0].h, points[0].v,
		points[1].h - points[0].h, points[0].v - points[1].v,
		0, 360);
}

POINT hema_1background[] = {
	{   0,    0},
	{1000, 1000},
};

POINT hema_2background[] = {
	{   0,    0},
	{ 300,  300},
};

polydef hema_background[] = {
	{"black", "white", hema_1drawbg, AREF(hema_1background)},
	{"gray25", "white", hema_2drawbg, AREF(hema_2background)},
};

POINT hema_1hours[] = {
	{   0, 1000},
	{ 200,  654},
	{-200,  654},
};

polydef hema_hours[] = {
	{"gray50", "white", wfillpoly, AREF(hema_1hours)},
};

POINT hema_1minutes[] = {
	{  40,  300},
	{  40, 1000},
	{ -40, 1000},
	{ -40,  300},
};

polydef hema_minutes[] = {
	{"gray75", "white", wfillpoly, AREF(hema_1minutes)},
};

POINT hema_1seconds[] = {
	{  10,  300},
	{  10,  320},
	{ -10,  320},
	{ -10,  300},
};

polydef hema_seconds[] = {
	{"black", "white", wxorpoly, AREF(hema_1seconds)},
};

POINT hema_1alarm[] = {
	{   0, 1000},
	{ 100, 1100},
	{-100, 1100},
};

polydef hema_alarm[] = {
	{"black", "white", wxorpoly, AREF(hema_1alarm)},
};

clockdef hema_clock = {
	"Hema watch",		/* title */
	"white", "black",	/* fg, bg */
	{AREF(hema_background)},
	{AREF(hema_hours)},
	{AREF(hema_minutes)},
	{AREF(hema_seconds)},
	{AREF(hema_alarm)},
};
