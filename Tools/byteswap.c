/* Byte-swap an array of short ints (assuming a short is 2 8-bit bytes). */

shortswap(data, len)
	register short *data;
	register int len;
{
	while (--len >= 0) {
		register /*short*/ x= *data;
		*data++= ((x>>8) & 0xff) | ((x&0xff) << 8);
	}
}

/* Byte-swap an array of long ints (assuming a long is 4 8-bit bytes). */

longswap(data, len)
	register long *data;
	register int len;
{
	while (--len >= 0) {
		register long x= *data;
		*data++=	((x>>24) & 0x00ff) |
				((x>> 8) & 0xff00) |
				((x&0xff00) <<  8) |
				((x&0x00ff) << 24);
	}
}

/* Byte-swapping versions of memcpy.
   Note that the count is here a number of shorts or longs,
   not a number of bytes.
   This only works for short = 2 bytes, long = 4 bytes,
   byte = 8 bits. */

swpscpy(dst, src, nshorts)
	register short *dst, *src;
	register int nshorts;
{
	while (--nshorts >= 0) {
		register /*short*/ x= *src++;
		*dst++ = ((x>>8) & 0xff) | ((x&0xff) << 8);
	}
}

swplcpy(dst, src, nlongs)
	register long *dst, *src;
	register int nlongs;
{
	while (--nlongs >= 0) {
		register long x= *src++;
		*dst++ =	((x>>24) & 0x00ff) |
				((x>> 8) & 0xff00) |
				((x&0xff00) <<  8) |
				((x&0x00ff) << 24);
	}
}
