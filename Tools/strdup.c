#include "tools.h"

char *
strdup(str)
	const char *str;
{
	if (str != NULL) {
		register char *copy = malloc(strlen(str) + 1);
		if (copy != NULL)
			return strcpy(copy, str);
	}
	return NULL;
}
