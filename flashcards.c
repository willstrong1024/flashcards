#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define PAGESPERSIDE 8
#define STARTBLANK "{{"
#define HINTDELIM "::"
#define ENDBLANK "}}"

typedef struct {
	char *answer;
	char *hint;
	char *sentence;
} Flashcard;


static char *cat(char **dest, size_t *size, const char *src);
static char *ccmd(Flashcard f);
static void cfcards(const char *s);
static void cfcmds(void);
static void cleanup(void);
static void cocmds(void);
static void compile(void);
static void copy(const char *src, const char *dest);
static void csentence(const char *s, Flashcard *f);
static void extrblank(const char *s, Flashcard *f);
static void fillblank(char *s);
static void fillbuf(int start, int offset);
static int find(const char *h, const char *n);
static void initbuf(void);
static void insert(const char *s, int n, const char *f);
static int line(const char *s, const char *f);
static int  nblanks(const char *s);
static void print(char **s, const char *fmt, ...);
static void read(const char *f);
static void reorder(void);
static void replblank(char *s);
static size_t rlines(char ***buf, FILE *fp);
static int rndup(int n, int m);
static char *tok(char *s, const char *delim);

static int fcardcnt = 0;
static Flashcard *fcards;
static char *fcmdbuf = NULL;
static size_t fsize = 0;
static char *ocmdbuf = NULL;
static char **pagebuf;
static int pages;

static char *
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

static char *
ccmd(Flashcard f)
{
	char *answer, *back, *fcard, *front, *hint;

	if (f.answer == NULL && f.hint == NULL) {
		print(&fcard, "\\flashcard\n{%s}\n{}\n\n", f.sentence);
		return fcard;
	}

	print(&answer, "\\answer{%s}", f.answer);
	print(&hint, "\\hint{%s}", f.hint);

	print(&back, f.sentence, answer);
	print(&front, f.sentence, hint);

	print(&fcard, "\\flashcard\n{%s}\n{%s}\n\n", front, back);

	free(answer);
	free(back);
	free(front);
	free(hint);

	return fcard;
}

static void
cfcards(const char *s)
{
	int i;

	for (i = 0; i < nblanks(s) || i < 1; ++i) {
		char *cpy;
		int j;

		if (sizeof(*fcards) * (fcardcnt + i + 1) > fsize)
			fcards = erealloc(fcards, fsize += BUFSIZ);

		cpy = strdup(s);

		for (j = 0; j < i; ++j)
			fillblank(cpy);

		extrblank(cpy, &fcards[fcardcnt + i]);
		csentence(cpy, &fcards[fcardcnt + i]);

		free(cpy);
	}

	fcardcnt += i;
}

static void
cfcmds(void)
{
	int i;
	size_t size = 0;

	for (i = 0; i < fcardcnt; ++i) {
		char *tmp;

		tmp = ccmd(fcards[i]);
		cat(&fcmdbuf, &size, tmp);

		free(tmp);
	}

}

static void
cleanup(void)
{
	int i;

	free(fcmdbuf);
	free(ocmdbuf);

	for (i = 0; i < fcardcnt; ++i) {
		free(fcards[i].answer);
		free(fcards[i].hint);
		free(fcards[i].sentence);
	}
	free(fcards);

	for (i = 0; i < pages; ++i)
		free(pagebuf[i]);
	free(pagebuf);
}

static void
cocmds(void)
{
	int i;
	size_t size = 0;

	for (i = 0; i < pages; i += PAGESPERSIDE) {
		int j;
		char *tmp;

		cat(&ocmdbuf, &size, "\\includepdf[pages={");

		for (j = i; j < i + PAGESPERSIDE; ++j) {
			cat(&ocmdbuf, &size, pagebuf[j]);

			if (j < (i + PAGESPERSIDE) - 1)
				cat (&ocmdbuf, &size, ",");
		}

		print(&tmp, "},nup=2x%d]{flashcards.pdf}\n", PAGESPERSIDE / 2);
		cat(&ocmdbuf, &size, tmp);

		free(tmp);
	}
}

static void
compile(void)
{
	initbuf();
	reorder();
}

static void
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

static void
csentence(const char *s, Flashcard *f)
{
	char *cpy;
	int i;

	cpy = strdup(s);
	replblank(cpy);

	for (i = 0; i < nblanks(s) - 1; ++i)
		fillblank(cpy);

	cpy[strlen(cpy) - 1] = 0;

	f->sentence = strdup(cpy);
	free(cpy);
}

static void
extrblank(const char *s, Flashcard *f)
{
	char *cpy, *tmp;

	if (nblanks(s) < 1) {
		f->answer = NULL;
		f->hint = NULL;

		return;
	}

	cpy = strdup(s);
	tmp = strstr(cpy, STARTBLANK) + 2;
	tmp = tok(tmp, ENDBLANK);

	f->answer = strdup(tok(tmp, "::"));
	f->hint = (tmp = tok(NULL, "::")) ? strdup(tmp) : strdup("...");

	free(cpy);
}

static void
fillblank(char *s)
{
	char *buf;
	int i, j;

	buf = ecalloc(strlen(s), sizeof(*buf));

	for (i = 0, j = 0; i < find(s, STARTBLANK); ++i, ++j)
		buf[j] = s[i];

	for (i += 2; i < find(s, HINTDELIM) && i < find(s, ENDBLANK); ++i, ++j)
		buf[j] = s[i];

	for (i = find(s, ENDBLANK) + 2; i < strlen(s); ++i, ++j)
		buf[j] = s[i];

	strcpy(s, buf);
	free(buf);
}

static void
fillbuf(int start, int offset)
{
	int i, j, k;

	for (i = start; i < pages; i += PAGESPERSIDE) {
		for (j = i, k = offset; i - j < PAGESPERSIDE; ++i, k += 2) {
			if (j + k - start > fcardcnt * 2)
				pagebuf[i] = strdup("{}");
			else
				print(&pagebuf[i], "%d", j + k - start);
		}
	}
}

static int
find(const char *h, const char *n)
{
	char *p;

	if ((p = strstr(h, n)) == NULL)
		return strlen(h);

	return p - h;
}

static void
initbuf(void)
{
	pages = rndup(fcardcnt * 2, PAGESPERSIDE * 2);
	pagebuf = emalloc(sizeof(*pagebuf) * pages);

	fillbuf(0, 1);
	fillbuf(PAGESPERSIDE, 2);
}

static void
insert(const char *s, int n, const char *f)
{
	char **buf = NULL;
	FILE *fp;
	int i;
	size_t lines;

	if ((fp = fopen(f, "r")) == NULL)
		die("flashcards: %s:", f);

	lines = rlines(&buf, fp);

	fclose(fp);

	if ((fp = fopen(f, "w")) == NULL)
		die("flashcards: %s:", f);

	for (i = 0; i < lines; ++i) {
		if (i == n)
			fputs(s, fp);

		fputs(buf[i], fp);
	}

	fclose(fp);

	for (i = 0; i < lines; ++i)
		free(buf[i]);
	free(buf);
}

static int
line(const char *s, const char *f)
{
	FILE *fp;
	int i;
	size_t len = 0;
	char *tmp = NULL;

	if ((fp = fopen(f, "r")) == NULL)
		die("flashcards: %s:", f);

	for (i = 1; getline(&tmp, &len, fp) != -1; ++i)
		if (strstr(tmp, s) != NULL)
			break;

	free(tmp);
	fclose(fp);

	return i;
}

static int
nblanks(const char *s)
{
	const char *p = s;
	int i = 0;

	while ((p = strstr(p, STARTBLANK)) && (p = strstr(p, ENDBLANK)))
		++i;

	return i;
}

static void
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

static void
read(const char *f)
{
	FILE *fp;
	int i;
	size_t len = 0;
	char *tmp = NULL;

	if ((fp = fopen(f, "r")) == NULL)
		die("flashcards: %s:", f);

	for (i = 0; getline(&tmp, &len, fp) != -1; ++i) {
		if (strcmp(tmp, "\n") == 0)
			continue;

		if (tmp[0] == '#')
			continue;

		cfcards(tmp);
	}

	free(tmp);
	fclose(fp);
}

static void
reorder(void)
{
	int i, j;

	for (i = PAGESPERSIDE; i < pages; i += PAGESPERSIDE * 2) {
		for (j = 0; j < PAGESPERSIDE; j += 2) {
			char *tmp;

			tmp = pagebuf[i + j];
			pagebuf[i + j] = pagebuf[i + j + 1];
			pagebuf[i + j + 1] = tmp;
		}
	}
}

static void
replblank(char *s)
{
	char *buf;
	int i, j;

	if (nblanks(s) < 1)
		return;

	buf = ecalloc(strlen(s), sizeof(*buf));

	for (i = 0, j = 0; i < find(s, STARTBLANK); ++i, ++j)
		buf[j] = s[i];

	buf[j++] = '%';
	buf[j++] = 's';

	for (i = find(s, ENDBLANK) + 2; i < strlen(s); ++i, ++j)
		buf[j] = s[i];

	strcpy(s, buf);
	free(buf);
}

static size_t
rlines(char ***buf, FILE *fp)
{
	int i;
	size_t len = 0, size = 0;
	char *tmp = NULL;

	for (i = 0; getline(&tmp, &len, fp) != -1; ++i) {
		if (*buf == NULL || sizeof(**buf) * (i + 1) > size)
			*buf = erealloc(*buf, size += BUFSIZ);

		(*buf)[i] = strdup(tmp);
	}

	free(tmp);

	return i;
}

static int
rndup(int n, int mult)
{
	int rmndr;

	if (mult == 0)
		return 0;

	rmndr = n % mult;

	if (rmndr == 0)
		return n;

	return n + mult - rmndr;
}

static char *
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

int
main(int argc, char **argv)
{
	int n;

	if (argc != 2)
		die("usage: flashcards [file]");

	copy("/usr/local/share/flashcards/flashcards.tex", "flashcards.tex");
	copy("/usr/local/share/flashcards/output.tex", "output.tex");

	n = line("\\begin{document}", "flashcards.tex");
	read(argv[1]);
	cfcmds();
	insert(fcmdbuf, n + 1, "flashcards.tex");

	n = line("\\begin{document}", "output.tex");
	compile();
	cocmds();
	insert(ocmdbuf, n, "output.tex");

	cleanup();

	return EXIT_SUCCESS;
}
