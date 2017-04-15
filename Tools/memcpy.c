char *
memcpy(dst, src, n)
	char *dst;
	char *src;
	int n;
{
	char *d= dst;
	while (--n >= 0) {
		*dst++ = *src++;
	}
	return d;
}
