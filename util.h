char *cat(char **dest, size_t *size, const char *src);
void copy(const char *src, const char *dest);
void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);
void *emalloc(size_t size);
void *erealloc(void *p, size_t size);
void print(char **s, const char *fmt, ...);
