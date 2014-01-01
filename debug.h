#ifndef MKD64_DEBUG_H
#define MKD64_DEBUG_H

#ifdef DEBUG
#define DBG(x) fprintf(stderr, "[DEBUG] %s\n", x)
#define DBGd1(x,a) fprintf(stderr, "[DEBUG] %s %d\n", x, a)
#define DBGd2(x,a,b) fprintf(stderr, "[DEBUG] %s %d %d\n", x, a, b)
#define DBGs1(x,a) fprintf(stderr, "[DEBUG] %s %s\n", x, a)
#else
#define DBG(x)
#define DBGd1(x,a)
#define DBGd2(x,a,b)
#define DBGs1(x,a)
#endif

#endif
/* vim: et:si:ts=8:sts=4:sw=4
*/
