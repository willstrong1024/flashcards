#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char *
cat(char **dest, size_t *size, const char *src)
{
	size_t dsize, needed, ssize;

	if (*dest == NULL) {
		*dest = strdup("\0");
		*size = 1;
	}

	dsize = sizeof(**dest) * strlen(*dest);
	ssize = sizeof(*src) * strlen(src);
	needed = dsize + ssize + sizeof(**dest) * 1;

	if (*size < needed)
		*dest = erealloc(*dest, *size = needed);

	return strcat(*dest, src);
}

void
copy(const char *src, const char *dest)
{
	FILE *fin, *fout;
	int tmp;

	if ((fin = fopen(src, "r")) == NULL)
		die("flashcards: %s:", src);

	if ((fout = fopen(dest, "w")) == NULL)
		die("flashcards: %s:", dest);

	while ((tmp = fgetc(fin)) != EOF)
		fputc(tmp, fout);

	fclose(fin);
	fclose(fout);
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(EXIT_FAILURE);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if ((p = calloc(nmemb, size)) == NULL)
		die("flashcards: calloc:");

	return p;
}

void *
emalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		die("flashcards: malloc:");

	return p;
}

void *
erealloc(void *p, size_t size)
{
	if ((p = realloc(p, size)) == NULL)
		die("flashcards: realloc:");

	return p;
}

int
find(const char *haystack, const char *needle)
{
	char *p;

	if ((p = strstr(haystack, needle)) == NULL)
		return strlen(haystack);

	return p - haystack;
}

void
print(char **s, const char *fmt, ...)
{
	va_list ap, cpy;
	size_t len;

	va_start(ap, fmt);
	va_copy(cpy, ap);

	len = vsnprintf(NULL, 0, fmt, cpy) + 1;
	*s = emalloc(sizeof(**s) * len);
	vsprintf(*s, fmt, ap);

	va_end(cpy);
	va_end(ap);
}

char *
tok(char *s, const char *delim)
{
	char *end, *tok;
	static char *saveptr;

	if ((tok = s ? s : saveptr) == NULL)
		return tok;

	if ((end = strstr(tok, delim)) == NULL) {
		saveptr = end;
	} else {
		saveptr = end + strlen(delim);
		*end = '\0';
	}

	return tok;
}
