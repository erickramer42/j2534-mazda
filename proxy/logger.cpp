#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <share.h>

#include "logger.h"

#define DATA_CAP    4128   // full buffer: never truncate flash payloads

static std::wstring g_traceDir; // directory of this proxy DLL
static HANDLE g_mutex = NULL;
static FILE*  g_file = NULL;


void LoggerInit(const std::wstring& iniPath)
{
    if (!g_mutex) g_mutex = CreateMutexA(NULL, FALSE, NULL);

    std::wstring selfDir(iniPath);
    size_t slash = selfDir.find_last_of(L'\\');
    if (slash != std::wstring::npos) selfDir.resize(slash);

    g_traceDir = selfDir + L"\\traces";
    CreateDirectoryW(g_traceDir.c_str(), NULL);   // ok if it already exists
    g_traceDir += L"\\";
}

static void OpenLogFileLocked()
{
    if (g_file) return;
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t wpath[MAX_PATH];
    swprintf(wpath, MAX_PATH, L"%strace_%04d%02d%02d_%02d%02d%02d.log",
             g_traceDir.c_str(), st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);
    // _wfopen_s(&g_file, wpath, L"w"); // doesn't allow sharing, so other processes can't read the log while we're writing
    g_file = _wfsopen(wpath, L"w", _SH_DENYNO);
}

void LogCall(const char* fmt, ...)
{
    if (!g_mutex) return;
    WaitForSingleObject(g_mutex, INFINITE);
    OpenLogFileLocked();
    if (g_file) {
        SYSTEMTIME st; GetLocalTime(&st);
        fprintf(g_file, "[%02d:%02d:%02d.%03d] ",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        va_list ap; va_start(ap, fmt);
        vfprintf(g_file, fmt, ap);
        va_end(ap);
        fprintf(g_file, "\n");
        fflush(g_file);
    }
    ReleaseMutex(g_mutex);
}

void LogMsgs(const char* dir, unsigned long channelId,
             const PASSTHRU_MSG* msgs, unsigned long count)
{
    if (!g_mutex) return;
    WaitForSingleObject(g_mutex, INFINITE);
    OpenLogFileLocked();
    if (g_file) {
        for (unsigned long i = 0; i < count; ++i) {
            const PASSTHRU_MSG* m = &msgs[i];

            char hex[DATA_CAP * 3 + 1]; // one big line, built then written once
            size_t pos = 0;
            unsigned long n = m->DataSize < DATA_CAP ? m->DataSize : DATA_CAP;
            for (unsigned long b = 0; b < n; ++b)
                pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", m->Data[b]);

            fprintf(g_file, "  MSG %s ch=%lu proto=0x%lX txflags=0x%lX rxstatus=0x%lX "
                            "ts=%lu len=%lu edi=%lu data=%.*s\n",
                    dir, channelId, m->ProtocolID, m->TxFlags, m->RxStatus,
                    m->Timestamp, m->DataSize, m->ExtraDataIndex, (int)pos, hex);
        }
        fflush(g_file);
    }
    ReleaseMutex(g_mutex);
}

const char* HexBytes(const PASSTHRU_MSG& msg)
{
    static char bufs[4][256];
    static int slot = 0;
    char* buf = bufs[slot];
    slot = (slot + 1) % 4;

    unsigned long n = msg.DataSize < 64 ? msg.DataSize : 64; // masks/patterns are short
    size_t pos = 0;
    for (unsigned long i = 0; i < n; ++i)
        pos += snprintf(buf + pos, sizeof(bufs[0]) - pos, "%02X ", msg.Data[i]);
    return buf;
}