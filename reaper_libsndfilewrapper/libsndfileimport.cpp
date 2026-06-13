#include "libsndfileImport.h"

// libsndfile is now vendored as a git submodule and statically linked into the
// plugin (see top-level CMakeLists.txt). Historically these symbols were
// resolved at runtime with dlopen()/LoadLibrary() against a system-installed
// libsndfile, which silently failed if none was present. Now they simply point
// at the statically-linked sf_* functions. The ptr_sf_* indirection is kept so
// the rest of the code (pcmsink/wrapperclass) needs no changes.

int		(*ptr_sf_command)	(SNDFILE *sndfile, int command, void *data, int datasize) ;
SNDFILE* 	(*ptr_sf_open)		(const char *path, int mode, SF_INFO *sfinfo) ;
int		(*ptr_sf_close)		(SNDFILE *sndfile) ;
sf_count_t	(*ptr_sf_read_float)	(SNDFILE *sndfile, float *ptr, sf_count_t items) ;
sf_count_t	(*ptr_sf_read_double)	(SNDFILE *sndfile, double *ptr, sf_count_t items) ;
sf_count_t	(*ptr_sf_seek) 		(SNDFILE *sndfile, sf_count_t frames, int whence) ;
sf_count_t	(*ptr_sf_readf_double)(SNDFILE *sndfile, double *ptr, sf_count_t frames) ;
const char * (*ptr_sf_version_string)(void);
int		(*ptr_sf_format_check)	(const SF_INFO *info) ;
sf_count_t	(*ptr_sf_write_float)	(SNDFILE *sndfile, const float *ptr, sf_count_t items) ;
sf_count_t	(*ptr_sf_write_double)	(SNDFILE *sndfile, const double *ptr, sf_count_t items) ;

int ImportLibSndFileFunctions()
{
    // Bind the indirection pointers to the statically-linked libsndfile
    // symbols. This cannot fail, but we keep the int return so main.cpp's
    // existing guard (`if (ImportLibSndFileFunctions()) return 0;`) is happy.
    ptr_sf_command        = sf_command;
    ptr_sf_open           = sf_open;
    ptr_sf_close          = sf_close;
    ptr_sf_read_float     = sf_read_float;
    ptr_sf_read_double    = sf_read_double;
    ptr_sf_seek           = sf_seek;
    ptr_sf_readf_double   = sf_readf_double;
    ptr_sf_version_string = sf_version_string;
    ptr_sf_format_check   = sf_format_check;
    ptr_sf_write_float    = sf_write_float;
    ptr_sf_write_double   = sf_write_double;

    return 0;
}
