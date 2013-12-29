
#include "modrepo.h"

#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <glob.h>
#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <dlfcn.h>
#endif

struct modrepo
{
    Modrepo *next;
    void *so;
    const char *id;
    IModule *(*instance)(void);
    void (*delete)(IModule *instance);
};

static Modrepo *
findModule(Modrepo *this, const char *id)
{
    Modrepo *current;

    for (current = this; current; current = current->next)
    {
        if (!strcmp(id, current->id))
        {
            return current;
        }
    }
    return 0;
}

Modrepo *
modrepo_new(const char *exe)
{
    Modrepo *this = 0;
    Modrepo *current = 0;
    Modrepo *next;

#ifdef WIN32

    HANDLE findHdl;
    HMODULE modso;
    FARPROC modid, modinst, moddel;
    WIN32_FIND_DATA findData;

    char *modpat = malloc(4096);
    GetModuleFileName(GetModuleHandle(0), modpat, 4096);
    PathRemoveFileSpec(modpat);
    strcat(modpat, "\\*.dll");

    findHdl = FindFirstFile(modpat, &findData);
    if (findHdl == INVALID_HANDLE_VALUE)
    {
        free(modpat);
        return this;
    }

    do
    {
        modso = LoadLibrary(findData.cFileName);

        if (!modso) continue;

        modid = GetProcAddress(modso, "id");
        if (!modid)
        {
            FreeLibrary(modso);
            continue;
        }

        modinst = GetProcAddress(modso, "instance");
        if (!modinst)
        {
            FreeLibrary(modso);
            continue;
        }

        moddel = GetProcAddress(modso, "delete");
        if (!moddel)
        {
            FreeLibrary(modso);
            continue;
        }

        next = malloc(sizeof(Modrepo));
        next->next = 0;
        next->so = modso;
        next->id = ((const char *(*)(void))modid)();
        next->instance = (IModule *(*)(void))modinst;
        next->delete = (void (*)(IModule *))moddel;

        if (current)
        {
            current->next = next;
        }
        else
        {
            this = next;
        }
        current = next;

        fprintf(stderr, "Found module: %s\n", current->id);

    } while (FindNextFile(findHdl, &findData) != 0);

    FindClose(findHdl);
    free(modpat);

#else

    glob_t glb;
    char **pathvp;
    char *modpat;
    void *modso, *modid, *modinst, *moddel;

    char *exefullpath = realpath(exe, 0);
    char *dir = dirname(exefullpath);
    modpat = malloc(strlen(dir) + 6);
    strcpy(modpat, dir);
    strcat(modpat, "/*.so");
    glob(modpat, 0, 0, &glb);
    free(modpat);
    free(exefullpath);

    for (pathvp = glb.gl_pathv; pathvp && *pathvp; ++pathvp)
    {
        modso = dlopen(*pathvp, RTLD_NOW);

        if (!modso) continue;

        modid = dlsym(modso, "id");
        if (!modid)
        {
            dlclose(modso);
            continue;
        }

        modinst = dlsym(modso, "instance");
        if (!modinst)
        {
            dlclose(modso);
            continue;
        }

        moddel = dlsym(modso, "delete");
        if (!moddel)
        {
            dlclose(modso);
            continue;
        }

        next = malloc(sizeof(Modrepo));
        next->next = 0;
        next->so = modso;
        next->id = ((const char *(*)(void))modid)();
        next->instance = (IModule *(*)(void))modinst;
        next->delete = (void (*)(IModule *))moddel;

        if (current)
        {
            current->next = next;
        }
        else
        {
            this = next;
        }
        current = next;

        fprintf(stderr, "Found module: %s\n", current->id);
    }

    globfree(&glb);

#endif

    return this;
}

void
modrepo_delete(Modrepo *this)
{
    Modrepo *current = this;
    Modrepo *tmp;

    while (current)
    {
        tmp = current;
        current = current->next;
#ifdef WIN32

#else

        dlclose(tmp->so);

#endif
        free(tmp);
    }
}

IModule *
modrepo_moduleInstance(Modrepo *this, const char *id)
{
    Modrepo *found = findModule(this, id);
    if (!found) return 0;
    return found->instance();
}

void
modrepo_deleteInstance(Modrepo *this, IModule *instance)
{
    Modrepo *found = findModule(this, instance->id());
    if (found) found->delete(instance);
}

/* vim: et:si:ts=8:sts=4:sw=4
*/
