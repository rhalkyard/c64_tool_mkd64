#ifndef CMDLINE_H
#define CMDLINE_H

#include <mkd64/cmdline.h>

Cmdline *cmdline_new(void);
void cmdline_delete(Cmdline *this);

void cmdline_parse(Cmdline *this, int argc, char **argv);
int cmdline_moveNext(Cmdline *this);

#endif
/* vim: et:si:ts=8:sts=4:sw=4
*/
