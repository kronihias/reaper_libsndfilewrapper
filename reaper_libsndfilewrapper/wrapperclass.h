#ifndef WRAPPERCLASS_H
#define WRAPPERCLASS_H
#include "libsndfileImport.h"
#include "reaper_plugin.h"
#include "sndfile.h"

class LSFW_SimpleMediaDecoder : public ISimpleMediaDecoder
{
public:
    LSFW_SimpleMediaDecoder();
    ~LSFW_SimpleMediaDecoder();
    ISimpleMediaDecoder *Duplicate();
    void Open(const char *filename, int diskreadmode, int diskreadbs, int diskreadnb);
    void Close(bool fullClose);
    const char *GetFileName() { return m_filename?m_filename:""; }
    const char *GetType() { return "XLSFW"; }
    void GetInfoString(char *buf, int buflen, char *title, int titlelen);
    bool IsOpen();
    int GetNumChannels() { return m_nch; }
    int GetBitsPerSample() { return m_bps; }
    double GetSampleRate() { return m_srate; }
    INT64 GetLength() { return m_length; }
    INT64 GetPosition() { return m_lastpos; }
    void SetPosition(INT64 pos);
    int ReadSamples(double *buf, int length);
private:
    SNDFILE *m_fh;
    SF_INFO m_sfinfo;
    char *m_filename;
    int m_isopened;
    double *m_audiordbuf;
    int m_nch, m_bps;
    double m_srate;
    INT64 m_lastpos;
    int m_lastblocklen;
    INT64 m_length; // length in sample-frames
    bool m_isreadingblock;
};

#endif // WRAPPERCLASS_H
