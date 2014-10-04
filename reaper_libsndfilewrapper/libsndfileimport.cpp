#include "libsndfileImport.h"

int		(*ptr_sf_command)	(SNDFILE *sndfile, int command, void *data, int datasize) ;
SNDFILE* 	(*ptr_sf_open)		(const char *path, int mode, SF_INFO *sfinfo) ;
int		(*ptr_sf_close)		(SNDFILE *sndfile) ;
sf_count_t	(*ptr_sf_read_float)	(SNDFILE *sndfile, float *ptr, sf_count_t items) ;
sf_count_t	(*ptr_sf_read_double)	(SNDFILE *sndfile, double *ptr, sf_count_t items) ;
sf_count_t	(*ptr_sf_seek) 		(SNDFILE *sndfile, sf_count_t frames, int whence) ;
sf_count_t	(*ptr_sf_readf_double)(SNDFILE *sndfile, double *ptr, sf_count_t frames) ;
const char * (*ptr_sf_version_string)(void);

#ifdef _WIN32
HINSTANCE g_hLibSndFile=0;
#endif

int ImportLibSndFileFunctions()
{
    int errcnt=0;
    
#ifdef _WIN32
    if (g_hLibSndFile)
        return 0;
    g_hLibSndFile=LoadLibraryA("libsndfile-1.dll");

    if (!g_hLibSndFile)
        errcnt++;
    if (g_hLibSndFile)
    {
        //OutputDebugStringA("libsndfile dll loaded! now loading functions...");
        *((void **)&ptr_sf_command)=(void*)GetProcAddress(g_hLibSndFile,"sf_command");
        if (!ptr_sf_command) errcnt++;
        *((void **)&ptr_sf_open)=(void*)GetProcAddress(g_hLibSndFile,"sf_open");
        if (!ptr_sf_open) errcnt++;
        *((void **)&ptr_sf_close)=(void*)GetProcAddress(g_hLibSndFile,"sf_close");
        if (!ptr_sf_close) errcnt++;
        *((void **)&ptr_sf_read_float)=(void*)GetProcAddress(g_hLibSndFile,"sf_read_float");
        *((void **)&ptr_sf_read_double)=(void*)GetProcAddress(g_hLibSndFile,"sf_read_double");
        if (!ptr_sf_read_double) errcnt++;
        *((void **)&ptr_sf_seek)=(void*)GetProcAddress(g_hLibSndFile,"sf_seek");
        if (!ptr_sf_seek) errcnt++;
        *((void **)&ptr_sf_readf_double)=(void*)GetProcAddress(g_hLibSndFile,"sf_readf_double");
        if (!ptr_sf_readf_double) errcnt++;
        *((void **)&ptr_sf_version_string)=(void*)GetProcAddress(g_hLibSndFile,"sf_version_string");
        if (!ptr_sf_version_string) errcnt++;
        //OutputDebugStringA("libsndfile functions loaded!");
    } //else OutputDebugStringA("libsndfile DLL not loaded!");
    
#elif defined(__APPLE__)
    static int a;
    static void *dll;
    if (!dll&&!a)
    {
        a=1;
        if (!dll) dll=dlopen("libsndfile.1.dylib",RTLD_LAZY);
        if (!dll) dll=dlopen("/usr/local/lib/libsndfile.1.dylib",RTLD_LAZY);
        if (!dll) dll=dlopen("/usr/lib/libsndfile.1.dylib",RTLD_LAZY);
        
        if (!dll)
        {
            CFBundleRef bund=CFBundleGetMainBundle();
            if (bund)
            {
                CFURLRef url=CFBundleCopyBundleURL(bund);
                if (url)
                {
                    char buf[8192];
                    if (CFURLGetFileSystemRepresentation(url,true,(UInt8*)buf,sizeof(buf)-128))
                    {
                        char *p=buf;
                        while (*p) p++;
                        while (p>=buf && *p != '/') p--;
                        if (p>=buf)
                        {
                            strcat(buf,"/Contents/Plugins/libsndfile.1.dylib");
                            if (!dll) dll=dlopen(buf,RTLD_LAZY);
                            
                            if (!dll)
                            {
                                strcpy(p,"/libsndfile.1.dylib");
                                dll=dlopen(buf,RTLD_LAZY);
                            }
                            if (!dll)
                            {
                                strcpy(p,"/Plugins/libsndfile.1.dylib");
                                if (!dll) dll=dlopen(buf,RTLD_LAZY);
                            }
                        }          
                    }
                    CFRelease(url);
                }
            }
        }
        
        if (dll)
        {
            *(void **)(&ptr_sf_command) = dlsym(dll, "sf_command");
            *(void **)(&ptr_sf_open) = dlsym(dll, "sf_open");
            *(void **)(&ptr_sf_close) = dlsym(dll, "sf_close");
            *(void **)(&ptr_sf_read_float) = dlsym(dll, "sf_read_float");
            *(void **)(&ptr_sf_read_double) = dlsym(dll, "sf_read_double");
            *(void **)(&ptr_sf_readf_double) = dlsym(dll, "sf_readf_double");
            *(void **)(&ptr_sf_seek) = dlsym(dll, "sf_seek");
            *(void **)(&ptr_sf_version_string) = dlsym(dll, "sf_version_string");
        }
        if (!dll)
			errcnt++;
    }
#endif
    return errcnt;
}
