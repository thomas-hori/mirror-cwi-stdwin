/* Set the date and time -- 4.3 BSD Unix version */

#ifdef _AIX
#include <time.h>
#endif
#include <sys/time.h>

#define isleap(y) ((y)%4 == 0 && ((y)%100 != 0 || (y)%400 == 0))

/* Convert a struct tm to seconds since Jan. 1, 1970.
   This knows nothing about time zones or daylight saving time. */

static unsigned long
tm2tv(tp)
	struct tm *tp;
{
	static short mdays[12]=
		{31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	unsigned long s= 0;
	int y, m, d;
	
	for (y= 1970; y < tp->tm_year + 1900; ++y) {
		s += 365;
		if (isleap(y))
			++s;
	}
	mdays[1]= 28 + isleap(y); /* Months have origin 0 */
	for (m= 0; m < tp->tm_mon; ++m)
		s += mdays[m];
	s += tp->tm_mday - 1;
	return ((s*24 + tp->tm_hour)*60 + tp->tm_min)*60 /*+ tp->tm_sec*/;
}

/* Set the date and time from a struct tm.
   The Input time is in local time.
   If 'minchange' is zero, minutes and seconds are not taken
   from the input but from the current system time. */

setdatetime(tp, minchange)
	struct tm *tp;
	int minchange; /* nonzero if we must reset minutes and seconds, too */
{
	struct timeval tv;
	struct timezone tz;
	unsigned long t;
	
	t= tm2tv(tp);				/* t is local time */
	if (gettimeofday(&tv, &tz) != 0)
		return -1;
	if (tp->tm_isdst)
		t -= 3600;			/* t is local time less DST */
	t += tz.tz_minuteswest*60;		/* t is GMT time */
	if (minchange)
		t= t/60 * 60;			/* Clear seconds */
	else
		t= t/3600 * 3600 + tv.tv_sec % 3600; /* Use current min/sec */
	tv.tv_sec= t;
	if (settimeofday(&tv, &tz) != 0)
		return -1;
	return 0;
}

#ifdef SYSV

/* XXX How do you set the date/time on system V? */

int
settimeofday(ptv, ptz)
	struct timeval *ptv;
	struct timezone *ptz;
{
	return -1;
}

#endif
