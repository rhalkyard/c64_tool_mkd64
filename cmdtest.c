
#include "cmdline.h"
#include "modrepo.h"

#include <stdio.h>

const char *noarg = "<EMPTY>";

int main(int argc, char **argv)
{
    Image *img;
    Modrepo *mr;
    Cmdline *cl = cmdline_new();

    cmdline_parse(cl, argc, argv);

    while (cmdline_moveNext(cl))
    {
        char opt = cmdline_opt(cl);
        const char *arg = cmdline_arg(cl);
        printf("%c: %s\n", opt, arg?arg:noarg);
    }

    mr = modrepo_new(cmdline_exe(cl));
    img = image_new();
    image_delete(img);
    modrepo_delete(mr);

    cmdline_delete(cl);
}

/* vim: et:si:ts=4:sts=4:sw=4
*/
