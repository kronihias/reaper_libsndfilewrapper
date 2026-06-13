#ifndef LIBSNDFILEIMPORT_H
#define LIBSNDFILEIMPORT_H

#include "sndfile.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// Binds the ptr_sf_* indirection pointers to the statically-linked libsndfile
// symbols. Always returns 0 (kept as int for backwards compatibility with the
// historical runtime-loading version that could fail). See libsndfileimport.cpp.
int ImportLibSndFileFunctions();

#endif // LIBSNDFILEIMPORT_H
