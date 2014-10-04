#ifndef LIBSNDFILEIMPORT_H
#define LIBSNDFILEIMPORT_H

#include "sndfile.h"

#ifdef _WIN32
    #include <windows.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <dlfcn.h>
#endif

int ImportLibSndFileFunctions();

#endif // LIBSNDFILEIMPORT_H
