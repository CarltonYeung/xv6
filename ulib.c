#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#include "vdso.h"

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

uint
vdso_getticks()
{
  static vdso_getticks_t _getticks_func = 0;

  // upon the first use, get the entry from the kernel
  if (0 == _getticks_func) {
    _getticks_func = vdso_entry(VDSO_GETTICKS);
  }

  // call the function
  return _getticks_func();
}

uint
vdso_getpid()
{
  static vdso_getpid_t _getpid_func = 0;

  // upon the first use, get the entry from the kernel
  if (0 == _getpid_func) {
    _getpid_func = vdso_entry(VDSO_GETPID);
  }

  // call the function
  return _getpid_func();
}

void mutex_init(mutex_t *m) {
	m->flag = 0; // 0 = available
}

void mutex_lock(mutex_t *m) {
	do {
		futex_wait((int *)&m->flag, 1); // sleep if unavailable (1)
//		printf(1, "%d woke up, lock = %d\n", getpid(), m->flag);
	} while (__sync_lock_test_and_set(&m->flag, 1));
//	printf(1, "%d took the lock, lock = %d\n", getpid(), m->flag);
//	printf(1, "%d count is now %d\n", getpid(), *(int *)((char *)m + sizeof(mutex_t)));
}

int  mutex_trylock(mutex_t *m) {
	if (__sync_lock_test_and_set(&m->flag, 1))
		return -1;
	return 0;
}

void mutex_unlock(mutex_t *m) {
//	printf(1, "%d unlocking, lock = %d\n", getpid(), m->flag);
//	printf(1, "%d count is now %d\n", getpid(), *(int *)((char *)m + sizeof(mutex_t)));
	m->flag = 0; // set flag to available (0)
	futex_wake((int *)&m->flag);
}

void cv_init(cond_var_t *cv) {
	cv->done = 0; // 0 = not done
}

void cv_wait(cond_var_t *cv, mutex_t *m) {
	mutex_lock(m);

	while (0 == cv->done) {
		mutex_unlock(m);
		futex_wait((int *)&cv->done, 0);
		mutex_lock(m);
	}
}

void cv_bcast(cond_var_t *cv) {
	// Caller should have acquired mutex
	cv->done = 1;
	futex_wake((int *)&cv->done);
	// Caller should release mutex
}

