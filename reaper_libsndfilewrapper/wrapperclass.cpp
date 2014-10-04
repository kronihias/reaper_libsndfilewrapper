#include "wrapperclass.h"
//#include "libsndfileImport.h"

extern int		(*ptr_sf_command)	(SNDFILE *sndfile, int command, void *data, int datasize) ;
extern SNDFILE* 	(*ptr_sf_open)		(const char *path, int mode, SF_INFO *sfinfo) ;
extern int		(*ptr_sf_close)		(SNDFILE *sndfile) ;
extern sf_count_t	(*ptr_sf_read_float)	(SNDFILE *sndfile, float *ptr, sf_count_t items) ;
extern sf_count_t	(*ptr_sf_read_double)	(SNDFILE *sndfile, double *ptr, sf_count_t items) ;
extern sf_count_t	(*ptr_sf_seek) 		(SNDFILE *sndfile, sf_count_t frames, int whence) ;
extern sf_count_t	(*ptr_sf_readf_double)(SNDFILE *sndfile, double *ptr, sf_count_t frames) ;
extern const char * (*ptr_sf_version_string)(void);

extern void (*format_timestr)(double tpos, char *buf, int buflen);
extern void (*update_disk_counters)(int read, int write);
extern void (*ShowConsoleMsg)(const char* msg);

LSFW_SimpleMediaDecoder::LSFW_SimpleMediaDecoder()
{
    // ShowConsoleMsg("constructing LSFW_SimpleMediaDecoder");
    m_filename=0;
    m_isopened=0;
    m_nch=0;
    m_bps=0;
    m_srate=0.0;
    m_length=0;
    m_lastpos=0;
    m_lastblocklen=0;
    m_fh=0;
}

LSFW_SimpleMediaDecoder::~LSFW_SimpleMediaDecoder()
{
    // ShowConsoleMsg("destroying LSFW_SimpleMediaDecoder");
    Close(true);
    free(m_filename);
}

bool LSFW_SimpleMediaDecoder::IsOpen()
{
    return m_fh && m_bps && m_nch && m_srate>0; // todo: check to make sure decoder is initialized properly
}

void LSFW_SimpleMediaDecoder::Open(const char *filename, int diskreadmode, int diskreadbs, int diskreadnb)
{
    m_isreadingblock=false;
    Close(filename && strcmp(filename,m_filename?m_filename:""));
    if (filename)
    {
        free(m_filename);
        m_filename=strdup(filename);
    }
    
    
    m_isopened=false;
    m_length=0;
    m_bps=0;
    m_nch=0;
    m_srate=0;
    m_sfinfo.format=SF_ENDIAN_BIG; // this doesn't do appear to do anything in this context
    // this call on Windows probably does not handle file names with non-ascii characters correctly
    // libsndfile has added sf_wchar_open for Windows which can be given a 16 bit string as the file name
    // however...Reaper does not use 16 bit strings. So I am not sure how this should be solved...
    m_fh=ptr_sf_open(m_filename,SFM_READ,&m_sfinfo);
    if (!m_fh)
    {
#ifdef REAPER_DEBUG_OUTPUT_TRACING
        ShowConsoleMsg("Failed to open file with libsndfile");
#endif
        ptr_sf_close(m_fh);
        m_fh=0;
    } else
    {
#ifdef REAPER_DEBUG_OUTPUT_TRACING
        ShowConsoleMsg("succeeded to open file with libsndfile");
#endif
        m_isopened=true;
        m_length=m_sfinfo.frames;
        //TODO : this based on libsndfile's format, it should not actually matter to Reaper
        //for playback purposes what this is. However, to update the disk bandwidth counters, it would be nice
        //to have this correctly initialized
        m_bps=0;
        int foo=m_sfinfo.format & 0x0000FFFF;
        if (foo==SF_FORMAT_PCM_16)
            m_bps=16;
        else if (foo==SF_FORMAT_PCM_24)
            m_bps=24;
        else if (foo==SF_FORMAT_PCM_32 || foo==SF_FORMAT_FLOAT)
            m_bps=32;
        else if (foo==SF_FORMAT_PCM_S8 || foo==SF_FORMAT_PCM_U8)
            m_bps=8;
        m_nch=m_sfinfo.channels;
        m_srate=m_sfinfo.samplerate;
    }

}

void LSFW_SimpleMediaDecoder::Close(bool fullClose)
{
    if (fullClose)
    {
        // delete any decoder data, but we have nothing dynamically allocated

    }
    // if (m_fh)
        ptr_sf_close(m_fh);
    m_fh=0;
    m_isopened=false;
}

void LSFW_SimpleMediaDecoder::GetInfoString(char *buf, int buflen, char *title, int titlelen)
{
    // lstrcpyn(title,"libsndfile supported File Properties",titlelen);
    strncpy(title,"libsndfile supported File Properties",titlelen);
    if (IsOpen())
    {
        // todo: add any decoder specific info
        char temp[4096],lengthbuf[128];
        format_timestr((double) m_length / (double)m_srate,lengthbuf,sizeof(lengthbuf));
        SF_FORMAT_INFO fi;
        //fi.name=new char[512];
        fi.format=m_sfinfo.format & SF_FORMAT_TYPEMASK;
        ptr_sf_command(0,SFC_GET_FORMAT_INFO,&fi,sizeof(fi));
        //ShowConsoleMsg(fi.name);
        fi.format=m_sfinfo.format & SF_FORMAT_SUBMASK;
        ptr_sf_command(0,SFC_GET_FORMAT_INFO,&fi,sizeof(fi));
        //ShowConsoleMsg(fi.name);
        sprintf(temp,"Length: %s:\r\n"
                "Samplerate: %.0f\r\n"
                "Channels: %d\r\n"
                "Bits/sample: %d\r\n"
                "libsndfile version: %s",
                lengthbuf,m_srate,m_nch,m_bps,ptr_sf_version_string());

        // lstrcpyn(buf,temp,buflen);
        strncpy(buf,temp,buflen);
        //delete[] fi.name;
    } else
        sprintf(buf,"Error: File not has been opened succesfully");

}

void LSFW_SimpleMediaDecoder::SetPosition(INT64 pos)
{
    if (m_isreadingblock)
    {
        // hopefully this won't ever happen. libsndfile does actually explode
        // if seeking while also reading the file. we could solve this with
        // a mutex but hmm...
        //ShowConsoleMsg("SetPosition() called while reading block"); // this could be A Bad Thing(tm) if it happened
    }
    if (m_fh)
    {
        // todo: if decoder, seek decoder (rather than file)

        if (pos!=m_lastpos+m_lastblocklen) // this condition prevents glitches when Reaper plays this media decoder resampled
            ptr_sf_seek(m_fh,pos,SEEK_SET);
        m_lastpos=pos;
        //char buf[200];
        //sprintf(buf,"seeked to %d",pos);
        //ShowConsoleMsg(buf);
    }
}

int LSFW_SimpleMediaDecoder::ReadSamples(double *buf, int length)
{
    if (m_fh)
    {
        m_isreadingblock=true;
        int rdframes=ptr_sf_readf_double(m_fh,buf,length);
        m_isreadingblock=false;
        update_disk_counters(rdframes*m_sfinfo.channels*m_bps,0);
        m_lastpos+=rdframes;
        return rdframes;
    }
    return 0;

}

ISimpleMediaDecoder* LSFW_SimpleMediaDecoder::Duplicate()
{
    LSFW_SimpleMediaDecoder *r=new LSFW_SimpleMediaDecoder;
    free(r->m_filename);
    r->m_filename = m_filename ? strdup(m_filename) : NULL;
    return r;
}
