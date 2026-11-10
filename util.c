/* SPDX-License-Identifier: GPL-3.0-only */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

char *xstrdup(const char *s) {
	if (s == NULL) {
#ifdef EINVAL
		errno = EINVAL;
#endif
		return NULL;
	}
	size_t len = strlen(s) + 1;
	char *new = malloc(len);
	if (new != NULL) {
		memcpy(new, s, len);
	}
#ifdef ENOMEM
	else {
		errno = ENOMEM;
	}
#endif
	return new;
}
