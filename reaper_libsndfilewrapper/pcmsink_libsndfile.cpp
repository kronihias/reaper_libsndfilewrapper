#ifdef _WIN32
#include <windows.h>
#include<Commctrl.h>
#else
#include "swell/swell.h"
// #include "swell/swell-dlggen.h"
#endif

#include <stdio.h>
#include <math.h>
#include <sys/stat.h>

#include "reaper_plugin.h"

#include "wdlstring.h"

#include "sndfile.h"
#include "libsndfileImport.h"

#include "resource.h"


#define SINK_FOURCC REAPER_FOURCC('s','n','d','f')


// set the default format and quality...
int SF_DEFAULT_FORMAT = SF_FORMAT_PAF | SF_FORMAT_PCM_24;
float SF_DEFAULT_VBR = 0.5f;


extern void (*format_timestr)(double tpos, char *buf, int buflen);
extern REAPER_PeakBuild_Interface *(*PeakBuild_Create)(PCM_source *src, const char *fn, int srate, int nch);
extern void (*update_disk_counters)(int read, int write);
extern const char *(*get_ini_file)();
extern HWND g_main_hwnd;


extern int		(*ptr_sf_command)	(SNDFILE *sndfile, int command, void *data, int datasize) ;
extern SNDFILE* 	(*ptr_sf_open)		(const char *path, int mode, SF_INFO *sfinfo) ;
extern int		(*ptr_sf_close)		(SNDFILE *sndfile) ;
extern sf_count_t	(*ptr_sf_read_float)	(SNDFILE *sndfile, float *ptr, sf_count_t items) ;
extern sf_count_t	(*ptr_sf_read_double)	(SNDFILE *sndfile, double *ptr, sf_count_t items) ;
extern sf_count_t	(*ptr_sf_seek) 		(SNDFILE *sndfile, sf_count_t frames, int whence) ;
extern sf_count_t	(*ptr_sf_readf_double)(SNDFILE *sndfile, double *ptr, sf_count_t frames) ;
extern const char * (*ptr_sf_version_string)(void);
extern int		(*ptr_sf_format_check)	(const SF_INFO *info) ;
extern sf_count_t	(*ptr_sf_write_float)	(SNDFILE *sndfile, const float *ptr, sf_count_t items) ;
extern sf_count_t	(*ptr_sf_write_double)	(SNDFILE *sndfile, const double *ptr, sf_count_t items) ;


extern HINSTANCE g_hInst;
#define WIN32_FILE_IO


class PCM_sink_libsndfile : public PCM_sink
{
  public:
    static HWND showConfig(void *cfgdata, int cfgdata_l, HWND parent);

    PCM_sink_libsndfile(const char *fn, void *cfgdata, int cfgdata_l, int nch, int srate, bool buildpeaks)
    {
        m_outfile = 0;
        
        m_peakbuild=0;
        

        if (cfgdata_l >= 32 && *((int *)cfgdata) == SINK_FOURCC)
        {
            m_format = REAPER_MAKELEINT(((int *)(((unsigned char *)cfgdata)+4))[0]);
            m_vbr = ((float *)(((unsigned char *)cfgdata)+4))[1];
        }
        
        
        m_nch=nch;
        m_srate=srate;
        m_lensamples=0;
        m_filesize=0;
        
        
        SF_INFO		sfinfo;
        
        memset (&sfinfo, 0, sizeof (sfinfo)) ;
        
        sfinfo.samplerate	= m_srate ;
        sfinfo.channels		= m_nch;
        sfinfo.format		= m_format;
        
        // printf("Chose format: 0x%08x \n", m_format);
        
        // Check if the format is viable and get the extension
        if (!ptr_sf_format_check (&sfinfo))
        {
            printf("Something might have gone wrong... Format not available: 0x%08x for %d channels \n", m_format, m_nch);
            /* it would be nice to have some kind of popup warning... */
            return;
        }
        
        
        // get the extension for the format
        SF_FORMAT_INFO	info ;
        
        info.format = (m_format & SF_FORMAT_TYPEMASK);
        
        
        if (ptr_sf_command (NULL, SFC_GET_FORMAT_INFO, &info, sizeof (info)))
        {
            printf("can't get major format info: 0x%08x\n", info.format);
        }
        
        // printf("Writing format: %s  ( %s ); 0x%08x\n", info.name, info.extension, info.format);
        
        /*
        char fn_no_ext[strlen(fn) - 4];
        strncpy(fn_no_ext, fn, strlen(fn) - 4);
        
        char buf[strlen(fn_no_ext) + strlen(info.extension)];
        
        wsprintf(buf,"%s.%s", fn_no_ext, info.extension);

        printf("filename: %s\n", buf);
        */
        
        m_fn.Set(fn); // the extension gets already set by reaper, hopefully correct...
        
        m_format_string.Set(info.name);
        
        
        info.format = (m_format & SF_FORMAT_SUBMASK);
        if (ptr_sf_command (NULL, SFC_GET_FORMAT_INFO, &info, sizeof (info)))
        {
            printf("can't get subtype format info: 0x%08x\n", info.format);
        }
        
        m_encoding_string.Set(info.name);
        
        
        /* OPEN THE FILE */
        m_outfile = ptr_sf_open(m_fn.Get(), SFM_WRITE, &sfinfo);
        
        // set quality
        double q = (double)m_vbr;
        
        ptr_sf_command (m_outfile, SFC_SET_COMPRESSION_LEVEL, &q, sizeof (double));
        ptr_sf_command (m_outfile, SFC_SET_VBR_ENCODING_QUALITY, &q, sizeof (double));
        
        
        if (buildpeaks)
        {
          m_peakbuild=PeakBuild_Create(NULL,fn,m_srate,m_nch);
        }

    }

    bool IsOpen()
    {
      
      return m_outfile;
    }

    ~PCM_sink_libsndfile()
    {
        if (IsOpen())
        {
            ptr_sf_close(m_outfile);
        }
      
        m_outfile = 0;

        delete m_peakbuild;
        m_peakbuild=0;
    }

    const char *GetFileName() { return m_fn.Get(); }
    int GetNumChannels() { return m_nch; } // return number of channels
    double GetLength() { return m_lensamples / (double) m_srate; } // length in seconds, so far
    INT64 GetFileSize()
    {
      return m_filesize;
    }
    int GetLastSecondPeaks(int sz, ReaSample *buf)
    {
      if (m_peakbuild)
        return m_peakbuild->GetLastSecondPeaks(sz,buf);
      return 0;
    }
    void GetPeakInfo(PCM_source_peaktransfer_t *block)
    {
      if (m_peakbuild) m_peakbuild->GetPeakInfo(block);
      else block->peaks_out=0;
    }

    void GetOutputInfoString(char *buf, int buflen)
    {
      char tmp[512];
      sprintf(tmp,"%s, %s, Sr: %d Hz, Ch: %dch", m_format_string.Get(), m_encoding_string.Get(), m_srate, m_nch);
      strncpy(buf,tmp,buflen);
    }

    void WriteMIDI(MIDI_eventlist *events, int len, double samplerate) { }
    void WriteDoubles(double **samples, int len, int nch, int offset, int spacing)
    {
        // printf("i am writing %d samples, my samples are %d, spacing %d, offset %d \n", len, sizeof(ReaSample), spacing, offset);
      if (m_peakbuild)
        m_peakbuild->ProcessSamples(samples,len,nch,offset,spacing);
        
        /* luckily libsndfile treats interleaved buffers the same way as reaper  s1_ch1 s1_ch2 s2_ch1 s2_ch2 ... */
        ptr_sf_write_double(m_outfile, samples[0], len*nch);
        
        
        /*  DO THE WRITING IN HERE!!!  */
      
    }

    int Extended(int call, void *parm1, void *parm2, void *parm3) 
    {
      return 0;
    }


 private:
  
    SNDFILE     *m_outfile;
    SF_INFO		m_sfinfo;
  
    int m_format; // this is the configured libsndfile format
    float m_vbr; // this is the quality for ogg
    
    WDL_String m_format_string;
    WDL_String m_encoding_string;
    
    int m_bitrate;
    int m_vbrq, m_abr, m_vbrmax, m_quality, m_stereomode, m_vbrmethod;
    
    
    WDL_TypedBuf<float> m_inbuf;
    int m_nch,m_srate;
    INT64 m_filesize;
    INT64 m_lensamples;
    WDL_String m_fn;
    
    REAPER_PeakBuild_Interface *m_peakbuild;
};

static unsigned int GetFmt(char **desc) 
{
  if (desc) *desc="libsndfile (.au .avr .caf .htk .iff .mat .mpc .paf .pcm .pvf .sd2 ...)";
  return SINK_FOURCC;
}

/* Reaper queries the extension before creating the sink, therefore the correct extension for the settings is already passed to the sink! */
static const char *GetExtension(const void *cfg, int cfg_l)
{
    //printf("queried extesion...\n");
    
    // the extension will be set regarding the selected format!
    if (cfg_l <= 4 && *((int *)cfg) == SINK_FOURCC)
        return "var";
  
    if (cfg_l>=8 && ((int*)cfg)[0] == SINK_FOURCC)
    {
        int format = REAPER_MAKELEINT(((int *)(((unsigned char*)cfg)+4))[0]);
        // printf("got format: 0x%08x and getting extension...", format);
        
        // get the extension for the format
        SF_FORMAT_INFO	info ;
        
        info.format = (format & SF_FORMAT_TYPEMASK);
        
        
        if (ptr_sf_command (NULL, SFC_GET_FORMAT_INFO, &info, sizeof (info)))
        {
            printf("can't get major format info: 0x%08x\n", info.format);
            
            return "var";
        }
        
        return info.extension;
        
    }
    
    return NULL;
}

// config stuff

static int LoadDefaultConfig(void **data, const char *desc)
{
    // printf("LoadDefaultConfig\n");
    
    static WDL_HeapBuf m_hb;
    const char *fn=get_ini_file();
    int l=GetPrivateProfileInt(desc,"default_size",0,fn);
    if (l<1) return 0;
    
    if (GetPrivateProfileStruct(desc,"default",m_hb.Resize(l),l,fn))
    {
        *data = m_hb.Get();
        return l;
    }
    return 0;
}

int SinkGetConfigSize() {
    // printf("SinkGetConfigSize\n");
    
    return 8;
}


void SinkInitDialog(HWND hwndDlg, void *cfgdata, int cfgdata_l)
{
    
    // printf("SinkInitDialog\n");
    
    int format = SF_DEFAULT_FORMAT;
    
    if (cfgdata_l < 8 || *((int *)cfgdata) != SINK_FOURCC)
        cfgdata_l=LoadDefaultConfig(&cfgdata,"libsndfile sink defaults");
    
    if (cfgdata_l>=8 && ((int*)cfgdata)[0] == SINK_FOURCC)
    {
        format = REAPER_MAKELEINT(((int *)(((unsigned char*)cfgdata)+4))[0]);
        printf("init settings: 0x%08x", format);
    }
    
    // todo: show conifguration
    
}



void SinkSaveState(HWND hwndDlg, void *_data)
{
    printf("SinkSaveState\n");
    
    // get the format selection
    int id = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETCURSEL, 0, 0);
    int format = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETITEMDATA, id, 0);
    
    // todo: get state from dialog
    
    ((int *)_data)[0] = SINK_FOURCC;
    ((int *)(((unsigned char *)_data)+4))[0]=REAPER_MAKELEINT(format);
}



void SaveDefaultConfig(HWND hwndDlg)
{
    printf("SaveDefaultConfig\n");
    
    char data[1024];
    SinkSaveState(hwndDlg,data);
    int l=SinkGetConfigSize();
    char *desc="libsndfile sink defaults";
    const char *fn=get_ini_file();
    char buf[64];
    sprintf(buf,"%d",l);
    WritePrivateProfileString(desc,"default_size",buf,fn);
    WritePrivateProfileStruct(desc,"default",data,l,fn);
    
}

/* add entry to major format selection */
static void SetFormatStr(HWND hwndDlg, const char* txt, int idx)
{
    int n = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETCOUNT, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_ADDSTRING, n, (LPARAM)txt);
    SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_SETITEMDATA, n, idx);
}

/* add entry to dropdown box for encoding */
static void SetEncodingStr(HWND hwndDlg, const char* txt, int idx)
{
    int n = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETCOUNT, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_ADDSTRING, n, (LPARAM)txt);
    SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_SETITEMDATA, n, idx);
}

/* add entry to dropdown box for byte order*/
static void SetByteorderStr(HWND hwndDlg, const char* txt, int idx)
{
    int n = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETCOUNT, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_ADDSTRING, n, (LPARAM)txt);
    SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_SETITEMDATA, n, idx);
}

/* update byte order dropdown */
static void updateByteOrder(HWND hwndDlg, int wanted_format)
{
    
    /* First delete all entries */
    SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_RESETCONTENT, 0, 0);
    
    SF_INFO 		sfinfo ;
    
    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    sfinfo.channels = 1 ;
    
    int s_id = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETCURSEL, 0, 0);
    int format = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETITEMDATA, s_id, 0);
    
    /* sanity check... */
    sfinfo.format = format;
    
    if (!ptr_sf_format_check (&sfinfo))
    {
        printf("Something might have gone wrong... Format not available: %06x \n", format);
    }
    
    
    /* Default file endian-ness. */
    sfinfo.format = format | SF_ENDIAN_FILE;
    if (ptr_sf_format_check (&sfinfo))
    {
        //printf ("Default Byteorder available \n");
        
        SetByteorderStr(hwndDlg, "Default Byte Order", sfinfo.format);
    }
    
    /* Force little endian-ness. */
    sfinfo.format = format | SF_ENDIAN_LITTLE;
    if (ptr_sf_format_check (&sfinfo))
    {
        //printf ("Little Endian available \n");
        
        SetByteorderStr(hwndDlg, "Little-Endian (Intel Byte Order)", sfinfo.format);
    }
    
    /* Force big endian-ness. */
    sfinfo.format = format | SF_ENDIAN_BIG;
    if (ptr_sf_format_check (&sfinfo))
    {
        //printf ("Big Endian available \n");
        
        SetByteorderStr(hwndDlg, "Big-Endian (PowerPC Byte Order)", sfinfo.format);
    }
    
    /* select default endianess */
    // SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_SETCURSEL, 0, 0);
    
    /* select saved byteorder */
    if (wanted_format < 0)
    {
        // select first choice for default
        SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_SETCURSEL, 0, 0);
        
    } else {
        
        int num_b = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETCOUNT, 0, 0);
        
        for (int i=0; i<num_b; i++)
        {
            int sel_format = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETITEMDATA, i, 0);
            
            // if wanted format and itemdata match -> set selected!
            if (sel_format == ((wanted_format & SF_FORMAT_TYPEMASK) | (wanted_format & SF_FORMAT_SUBMASK) | (wanted_format & SF_FORMAT_ENDMASK)) )
                SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_SETCURSEL, i, 0);
        }
    }
    
}

/* Fill Encoding Dropdown Box */
static void updateSubFormats(HWND hwndDlg, int wanted_format)
{
    
    /* First delete all entries*/
    SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_RESETCONTENT, 0, 0);
    
    SF_FORMAT_INFO	info ;
    SF_INFO 		sfinfo ;
    
    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    sfinfo.channels = 1 ;
    
    int subtype_count, format;
    
    ptr_sf_command (NULL, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof (int)) ;
    
    
    // get current selection of format combobox
    int id = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETCURSEL, 0, 0);
    int majorformat = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETITEMDATA, id, 0);
    
    
    info.format = majorformat;
    ptr_sf_command (NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof (info)) ;
    
    // printf ("Got %i subformats\n", subtype_count);
    
    majorformat = info.format;
    
    // printf ("Getting subformats of %s\n", info.name);
    
    for (int s = 0 ; s < subtype_count ; s++)
    {
        info.format = s ;
        ptr_sf_command (NULL, SFC_GET_FORMAT_SUBTYPE, &info, sizeof (info)) ;
        
        
        // printf ("Subformats: %s\n", info.name);
        
        format = (majorformat & SF_FORMAT_TYPEMASK) | info.format ;
        
        sfinfo.format = format ;
        
        // check if the subformat is suitable for this major format
        if (ptr_sf_format_check (&sfinfo))
        {
            // printf ("Found subformat: %s\n", info.name);
            char buf[100];
            wsprintf(buf,"%s", info.name);
            
            SetEncodingStr(hwndDlg, buf, sfinfo.format);
        }
        
    }
    
    
    /* select saved subformat and update byteorder */
    if (wanted_format < 0)
    {
        // select first choice for default
        SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_SETCURSEL, 0, 0);
    } else {
        int num_s = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETCOUNT, 0, 0);
        
        for (int i=0; i<num_s; i++)
        {
            int sel_format = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETITEMDATA, i, 0);
            
            // if wanted format and itemdata match -> set selected!
            if (sel_format == ((wanted_format & SF_FORMAT_TYPEMASK) | (wanted_format & SF_FORMAT_SUBMASK)) )
                SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_SETCURSEL, i, 0);
        }
    }
    
    
    updateByteOrder(hwndDlg, wanted_format);
}



WDL_DLGRET wavecfgDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
          
    // this is called when the dialog is initialized
    case WM_INITDIALOG:
      {
          
          
          /* Get the saved parameters */
          void *cfgdata=((void **)lParam)[0];
          int configLen = (int) (INT_PTR) ((void **) lParam)[1];
          
          int format = 0x020002;
          
          float vbrq = 0.5f;
          
          if (configLen < 32 || *((int *)cfgdata) != SINK_FOURCC)
              configLen=LoadDefaultConfig(&cfgdata,"libsndfile sink defaults");
          
          // parameters have been sent by reaper -> parse them and set gui accordingly
          if (configLen >= 32 && *((int *)cfgdata) == SINK_FOURCC)
          {
              format=REAPER_MAKELEINT(((int *)(((unsigned char *)cfgdata)+4))[0]);
              vbrq=((float *)(( (unsigned char *)cfgdata)+4))[1];
              
              // printf("Got parameters: format: 0x%08x, vbrq: %f\n", format, vbrq);
          }
          
          /* Set Range for slider and value for slider and vbr text*/
          SendDlgItemMessage(hwndDlg, IDC_VBR_SLIDER, TBM_SETRANGE, false, MAKELONG(0, 100));
          SendDlgItemMessage(hwndDlg, IDC_VBR_SLIDER, TBM_SETPOS, true, (int)(vbrq*100.f));
          
          char q_text[5];
          sprintf(q_text, "%.2f", vbrq);
          SetDlgItemText(hwndDlg, IDC_VBR_VAL, q_text);
          
          
          /* fill the format combobox */
          SF_FORMAT_INFO	info ;
          
          int major_count;
          
          ptr_sf_command (NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof (int)) ;
          
          
          for (int m = 0 ; m < major_count ; m++)
          {
              info.format = m ;
              ptr_sf_command (NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof (info)) ;
              
              char buf[100];
              wsprintf(buf,".%s  ( %s )", info.extension, info.name);
              
              SetFormatStr(hwndDlg, buf, info.format);
          }
          
          
          
          /* select saved parameter and update subformats */
          int num_m = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETCOUNT, 0, 0);
          
          for (int i=0; i<num_m; i++)
          {
              int sel_format = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETITEMDATA, i, 0);
              
              // if wanted format and itemdata match -> set selected!
              if (sel_format == (format & SF_FORMAT_TYPEMASK))
                  SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_SETCURSEL, i, 0);
          }
          
          updateSubFormats(hwndDlg, format); // this will also update byte order
          
          
          return 0;
      }
          
    // this gets called in case something changed
    case WM_COMMAND:
          {
              if (LOWORD(wParam) == IDC_FORMAT && HIWORD(wParam) == CBN_SELCHANGE)
              {
                  int id = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETCURSEL, 0, 0);
                  int sel_format = SendDlgItemMessage(hwndDlg, IDC_FORMAT, CB_GETITEMDATA, id, 0);
                  
                  updateSubFormats(hwndDlg, -1);
                      
              }
              else if (LOWORD(wParam) == IDC_ENCODING && HIWORD(wParam) == CBN_SELCHANGE)
              {
                  int id = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETCURSEL, 0, 0);
                  int sel_format = SendDlgItemMessage(hwndDlg, IDC_ENCODING, CB_GETITEMDATA, id, 0);
                  
                  updateByteOrder(hwndDlg, -1);
              }
              
              break;
          }
          
      // this gets called when slider is moved..
      case WM_HSCROLL:
      {
          
          int pos = SendDlgItemMessage(hwndDlg, IDC_VBR_SLIDER, TBM_GETPOS, 0, 0);
          
          float val = (float)pos/100.f;
          char q_text[5];
          // wsprintf(q_text, "%.2f", (float)pos/100.f);
          
          sprintf(q_text, "%.2f", val);
          
          SetDlgItemText(hwndDlg, IDC_VBR_VAL, q_text);
          
          // printf("Slider moved, pos: %.2f!!\n", val);
          
          
          break;
      }
      
    // this gets called to retrieve the settings!
    case WM_USER+1024:
      {
          if (wParam) *((int *)wParam)=32;
          if (lParam)
          {
              
              // get the format selection
              int id = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETCURSEL, 0, 0);
              int format = SendDlgItemMessage(hwndDlg, IDC_BYTEORDER, CB_GETITEMDATA, id, 0);
              
              // get the vbr quality
              int pos = SendDlgItemMessage(hwndDlg, IDC_VBR_SLIDER, TBM_GETPOS, 0, 0);
              
              
              ((int *)lParam)[0] = SINK_FOURCC;
              ((int *)(((unsigned char *)lParam)+4))[0]=REAPER_MAKELEINT(format);
              ((float *)(((unsigned char *)lParam)+4))[1]=(float)pos/100.f;
              
          }
          
          return 0;
      }
    case WM_DESTROY:
      {
          
          
          
          
         return 0;
      }
      
  }
  return 0;
}


static HWND ShowConfig(const void *cfg, int cfg_l, HWND parent)
{
  
    if (cfg_l >= 4 && *((int *)cfg) == SINK_FOURCC)
    {
        const void *x[2]={cfg,(void *)cfg_l};
        return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_LIBSNDFILESINK_CFG),parent,wavecfgDlgProc,(LPARAM)x);
    }
   
    return 0;
   
}

static PCM_sink *CreateSink(const char *filename, void *cfg, int cfg_l, int nch, int srate, bool buildpeaks)
{
  if (cfg_l >= 4 && *((int *)cfg) == SINK_FOURCC)
  {
    if (ImportLibSndFileFunctions())
    {
        printf("could not load libsndfile\n");
        return 0;
    }
    PCM_sink_libsndfile *v=new PCM_sink_libsndfile(filename,cfg,cfg_l,nch,srate,buildpeaks);
    if (v->IsOpen()) return v;
    delete v;
  }
  return 0;
}



pcmsink_register_t mySinkRegStruct={GetFmt,GetExtension,ShowConfig,CreateSink};


// import the resources. Note: if you do not have these files, run "php ../WDL/swell/mac_resgen.php res.rc" from this directory
#ifndef _WIN32 // MAC resources
#include "swell/swell-dlggen.h"
#include "res.rc_mac_dlg"
#undef BEGIN
#undef END
#include "swell/swell-menugen.h"
#include "res.rc_mac_menu"
#endif

