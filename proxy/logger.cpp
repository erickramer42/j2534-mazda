#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#include "logger.h"

static HANDLE g_mutex = NULL;
static FILE* g_file = NULL;

void OpenLogFile()
{
    if (g_file) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH); // not DLL dir; set explicitly if preferred
    // TODO: choose your trace directory
    snprintf(path, sizeof(path), "C:\\j2534_traces\\trace_%04d%02d%02d_%02d%02d%02d.log",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    fopen_s(&g_file, path, "w");
}

void LogCall(const char* fmt, ...)
{
    if (!g_file) {
        if (!g_mutex) g_mutex = CreateMutexA(NULL, FALSE, NULL);
        WaitForSingleObject(g_mutex, INFINITE);
        OpenLogFile();
        ReleaseMutex(g_mutex);
    }
    if (!g_file) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    WaitForSingleObject(g_mutex, INFINITE);

    fprintf(g_file, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_file, fmt, ap);
    va_end(ap);
    fprintf(g_file, "\n");
    fflush(g_file);

    ReleaseMutex(g_mutex);
}

void LogMsgs(const char* dir, unsigned long channelId,
             const PASSTHRU_MSG* msgs, unsigned long count)
{
    for (unsigned long i = 0; i < count; ++i) {
        const PASSTHRU_MSG* m = &msgs[i];
        fprintf(g_file, "  MSG %s ch=%lu proto=0x%lX txflags=0x%lX rxstatus=0x%lX ts=%lu len=%lu edi=%lu data=",
                dir, channelId, m->ProtocolID, m->TxFlags, m->RxStatus,
                m->Timestamp, m->DataSize, m->ExtraDataIndex);
        for (unsigned long b = 0; b < m->DataSize && b < 64; ++b)
            fprintf(g_file, "%02X ", m->Data[b]);
        if (m->DataSize > 64) fprintf(g_file, "...");
        fprintf(g_file, "\n");
    }
    fflush(g_file);
}

const char* HexBytes(const PASSTHRU_MSG& msg)
{
    static char buf[128]; // static: fine for single-threaded logging use
    unsigned long n = msg.DataSize < 40 ? msg.DataSize : 40;
    unsigned long pos = 0;
    for (unsigned long i = 0; i < n; ++i)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%02X ", msg.Data[i]);
    return buf;
}