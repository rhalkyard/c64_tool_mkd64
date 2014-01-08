#ifndef MKD64_MKD64_H
#define MKD64_MKD64_H

#include <mkd64/common.h>

#include <mkd64/modrepo.h>

#define MKD64_VERSION "0.4b"

DECLEXPORT Modrepo *mkd64_modrepo(void);

DECLEXPORT void mkd64_suggestOption(IModule *mod, int fileNo,
        char opt, const char *arg, const char *reason);

#endif

/* vim: et:si:ts=4:sts=4:sw=4
*/
