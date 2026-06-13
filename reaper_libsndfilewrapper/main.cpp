#include "reaper_plugin.h"
// #include "reaper_plugin_functions.h"
#include "libsndfileImport.h"
#ifdef _WIN32
    #include <windows.h>
#endif
#include "wrapperclass.h"
#define IMPAPI(x) if (!((*((void **)&(x)) = (void *)rec->GetFunc(#x)))) impapierrcnt++;

REAPER_PLUGIN_HINSTANCE g_hInst=0;

PCM_source *(*PCM_Source_CreateFromSimple)(ISimpleMediaDecoder *dec, const char *fn);
void (*format_timestr)(double tpos, char *buf, int buflen);
void (*update_disk_counters)(int read, int write);
void (*ShowConsoleMsg)(const char* msg);

REAPER_PeakBuild_Interface *(*PeakBuild_Create)(PCM_source *src, const char *fn, int srate, int nch);
void (*GetPreferredDiskWriteMode)(int *mode, int nb[2], int *bs);
const char *(*get_ini_file)();

// output diagnostics messages using Reaper's currently available console
#define REAPER_DEBUG_OUTPUT_TRACING

PCM_source *CreateFromType(const char *type, int priority)
{
    // "XLSFW" is our own unique source type, so claim it at any priority. (The
    // historical `priority > 4` gate assumed REAPER passes priorities up to 7;
    // REAPER 7.x only goes up to 5, which would have left saved-project items
    // using our decoder unable to reload.)
    if (type && !strcmp(type,"XLSFW"))
        return PCM_Source_CreateFromSimple(new LSFW_SimpleMediaDecoder,NULL);

    return NULL;
}

PCM_source *CreateFromFile(const char *filename, int priority)
{
    int lfn = filename ? (int)strlen(filename) : 0;

    // Last-resort fallback: try libsndfile on any file a higher-priority
    // (built-in) handler did not already claim. REAPER 7.x calls this with
    // priorities 0-5 (highest .. lowest); the previous threshold (priority > 6)
    // was never reached, so .paf/.caf/etc. were never handed to our decoder and
    // REAPER fell back to its generic "Unknown file header" source. We only ever
    // RETURN a source when libsndfile can actually open the file (IsAvailable),
    // so claiming at the lowest priority cannot steal a format a built-in
    // handler already claimed higher up.
    if (priority >= 5 && lfn > 4)
    {
        PCM_source *w = PCM_Source_CreateFromSimple(new LSFW_SimpleMediaDecoder, filename);
        if (w && w->IsAvailable()) return w;
        delete w;
    }

    return NULL;
}

// this is used for UI only (builds REAPER's importable-media extension list).
// Enumerate EVERY major format the statically-linked libsndfile supports, so the
// reader advertises exactly what the sink (pcmsink_libsndfile.cpp) can write.
// Previously only PAF/CAF/sd2 were listed, so a file exported in any other
// libsndfile format (w64, au, voc, htk, mat, aiff, ...) was not recognised by
// REAPER on import. call increasing i until returns a string; returning NULL
// terminates enumeration, so the list must be contiguous (no gaps).
const char *EnumFileExtensions(int i, const char **descptr)
{
    if (i < 0)
    {
        if (descptr) *descptr = NULL;
        return NULL;
    }

    int major_count = 0;
    sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int));
    if (i >= major_count)
    {
        if (descptr) *descptr = NULL;
        return NULL;
    }

    SF_FORMAT_INFO info;
    info.format = i; // index, not a format id, for SFC_GET_FORMAT_MAJOR
    if (sf_command(NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof(info)) != 0 || !info.extension)
    {
        if (descptr) *descptr = NULL;
        return NULL;
    }

    // info.name / info.extension point at libsndfile's persistent static strings.
    if (descptr) *descptr = info.name;
    return info.extension;
}


pcmsrc_register_t myRegStruct={CreateFromType,CreateFromFile,EnumFileExtensions};
// extern pcmsink_register_t mySinkRegStruct; // from pcmsink_libsndfile.cpp
extern pcmsink_register_ext_t mySinkRegStruct; // from pcmsink_libsndfile.cpp


const char *(*GetExePath)();

extern "C"
{

REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
{
    // ShowConsoleMsg("Entering entrypoint function in libsndfile wrapper extension");
    
    g_hInst=hInstance;
    //g_EntrypointCallCount++;
    if (rec)
    {
        if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
            return 0;
        int impapierrcnt=0;
        IMPAPI(GetExePath);
        IMPAPI(PCM_Source_CreateFromSimple);
        IMPAPI(format_timestr);
        IMPAPI(update_disk_counters);
        IMPAPI(ShowConsoleMsg);
        IMPAPI(get_ini_file);
        IMPAPI(GetPreferredDiskWriteMode);
        IMPAPI(PeakBuild_Create);
        if (impapierrcnt)
        {
            ShowConsoleMsg("Errors importing Reaper API functions, aborting loading Xenakios Libsndfile wrapper");
            return 0;
        }
        
#ifdef REAPER_DEBUG_OUTPUT_TRACING
        //ShowConsoleMsg(lsfpath);
#endif
        if (ImportLibSndFileFunctions())
        {
            // loading libsndfile/resolving functions failed
            //ShowConsoleMsg("Errors loading libsndfile, aborting loading wrapper");
            return 0;
        }
        rec->Register("pcmsrc",&myRegStruct);
      
        if (!rec->Register("pcmsink_ext",&mySinkRegStruct))
        {
          printf("Failed to register extended Sink\n");
        
          if (rec->Register("pcmsink",&mySinkRegStruct))
            printf("Registered normal Sink!\n");
        
        }
      
        return 1;
    }
    return 0;
}
}
