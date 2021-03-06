#include <mkd64/common.h>
#include <mkd64/util.h>

#include "cmdline.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FILEOPT_CHUNKSIZE 256

struct Cmdline
{
    int count;
    int pos;
    char *opts;
    char **args;
    char *exe;
};

static void
clear(Cmdline *self)
{
    int i;
    if (self->args)
    {
        for (i = 0; i < self->count; ++i)
        {
            free(self->args[i]);
        }
        free(self->args);
        self->args = 0;
    }
    free(self->opts);
    self->opts = 0;
    self->count = 0;
    self->pos = -1;
}

SOLOCAL size_t
Cmdline_objectSize(void)
{
    return sizeof(Cmdline);
}

SOLOCAL Cmdline *
Cmdline_init(Cmdline *self)
{
    memset(self, 0, sizeof(Cmdline));
    self->pos = -1;
    return self;
}

SOLOCAL void
Cmdline_done(Cmdline *self)
{
    clear(self);
}

static void
_warnLooseArg(const char *arg)
{
    fprintf(stderr, "Warning: loose argument `%s' ignored.\n", arg);
}

SOLOCAL void
Cmdline_parse(Cmdline *self, int argc, char **argv)
{
    char **argvp;

    clear(self);

    self->exe = *argv;
    self->opts = mkd64Alloc(argc * sizeof(char));
    self->args = mkd64Alloc(argc * sizeof(char *));

    for (argvp = argv+1; *argvp; ++argvp)
    {
        if (strlen(*argvp) > 1 && **argvp == '-')
        {
            self->opts[self->count] = (*argvp)[1];
            if (strlen(*argvp) > 2)
            {
                self->args[self->count] = copyString((*argvp)+2);
            }
            else if (*(argvp+1) && **(argvp+1) != '-')
            {
                self->args[self->count] = copyString(*++argvp);
            }
            else
            {
                self->args[self->count] = 0;
            }
            ++(self->count);
        }
        else _warnLooseArg(*argvp);
    }
}

static int
_charIn(const char *set, char c)
{
    const char *ptr = set;
    while (*ptr)
    {
        if (*ptr == c) return 1;
        ++ptr;
    }
    return 0;
}

static void
_rmFirstChar(char *s)
{
    char *ptr = s;
    while (ptr[1])
    {
        *ptr = ptr[1];
        ++ptr;
    }
}

static char *
_cmdtok(char *str, const char *delim, const char *quote)
{
    static char *start;
    char *ptr, *tok;
    char inquot;

    if (str) start = str;
    if (*start == '\0') return 0;

    inquot = '\0';

    while (_charIn(delim, *start)) ++start;
    if (*start == '\0') return 0;

    ptr = start;
    while (*ptr != '\0' && !_charIn(delim, *ptr))
    {
        if (_charIn(quote, *ptr))
        {
            inquot = *ptr;
            _rmFirstChar(ptr);
            while (*ptr != '\0' && *ptr != inquot)
            {
                ++ptr;
            }
            if (*ptr == inquot) _rmFirstChar(ptr);
            inquot = '\0';
        }
        else
        {
            ++ptr;
        }
    }
    tok = start;
    if (*ptr == '\0')
    {
        start = ptr;
    }
    else
    {
        *ptr = '\0';
        start = ptr+1;
    }
    return tok;
}

SOLOCAL int
Cmdline_parseFile(Cmdline *self, const char *cmdfile)
{
    static const char *delim = " \t\r\n";
    static const char *quote = "\"'";
    size_t optSize = FILEOPT_CHUNKSIZE * sizeof(char);
    size_t argSize = FILEOPT_CHUNKSIZE * sizeof(char *);
    char *buf, *tok;
    FILE *f;
    int64_t fSize;
    size_t bufSize;

    clear(self);

    if (!(f = fopen(cmdfile, "rb"))) return 0;
    fSize = getFileSize(f);
    if (fSize < 1 || (uint64_t)fSize > (uint64_t)SIZE_MAX)
    {
        fclose(f);
        return 0;
    }
    bufSize = (size_t) fSize;

    buf = mkd64Alloc(bufSize);

    if (fread(buf, 1, bufSize, f) != bufSize)
    {
        fclose(f);
        free(buf);
        return 0;
    }

    self->opts = mkd64Alloc(optSize);
    self->args = mkd64Alloc(argSize);

    tok = _cmdtok(buf, delim, quote);
    while (tok)
    {
        if (strlen(tok) > 1 && *tok == '-')
        {
            self->opts[self->count] = tok[1];
            if (strlen(tok) > 2)
            {
                self->args[self->count] = copyString(tok+2);
                tok = _cmdtok(0, delim, quote);
            }
            else
            {
                tok = _cmdtok(0, delim, quote);
                if (tok && tok[0] != '-')
                {
                    self->args[self->count] = copyString(tok);
                    tok = _cmdtok(0, delim, quote);
                }
                else
                {
                    self->args[self->count] = 0;
                }
            }
            if (!(++(self->count) % FILEOPT_CHUNKSIZE))
            {
                optSize += FILEOPT_CHUNKSIZE * sizeof(char);
                self->opts = realloc(self->opts, optSize);
                argSize += FILEOPT_CHUNKSIZE * sizeof(char *);
                self->args = realloc(self->args, argSize);
            }
        }
        else
        {
            _warnLooseArg(tok);
            tok = _cmdtok(0, delim, quote);
        }
    }
    fclose(f);
    free(buf);
    return 1;
}

SOLOCAL char
Cmdline_opt(const Cmdline *self)
{
    if (self->pos < 0) return '\0';
    return self->opts[self->pos];
}

SOLOCAL const char *
Cmdline_arg(const Cmdline *self)
{
    if (self->pos < 0) return 0;
    return self->args[self->pos];
}

SOLOCAL int
Cmdline_moveNext(Cmdline *self)
{
    ++(self->pos);
    if (self->pos == self->count)
    {
        self->pos = -1;
        return 0;
    }
    return 1;
}

SOLOCAL const char *
Cmdline_exe(const Cmdline *self)
{
    return self->exe;
}

SOLOCAL int
Cmdline_count(const Cmdline *self)
{
    return self->count;
}
/* vim: et:si:ts=4:sts=4:sw=4
*/
