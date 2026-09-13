#include "j2534_types.h"
#include <cstring>
#include <cstdio>
#include <vector>

static std::vector<PASSTHRU_MSG> g_rxQueue;
static unsigned long g_nextChannel = 0;

extern "C" {
__declspec(dllexport) long __stdcall PassThruOpen(const char*, unsigned long* id)
{ *id = 1; return 0; }

__declspec(dllexport) long __stdcall PassThruClose(unsigned long) { return 0; }

__declspec(dllexport) long __stdcall PassThruConnect(unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long* ch)
{ *ch = ++g_nextChannel; return 0; }

__declspec(dllexport) long __stdcall PassThruDisconnect(unsigned long) { return 0; }

__declspec(dllexport) long __stdcall PassThruWriteMsgs(unsigned long, PASSTHRU_MSG* msgs,
    unsigned long* num, unsigned long)
{
    // echo response queued for subsequent reads: positive response to service byte
    PASSTHRU_MSG resp = {};
    resp.ProtocolID = msgs[0].ProtocolID;
    resp.DataSize = msgs[0].DataSize < 8 ? msgs[0].DataSize : 8;
    resp.Data[0] = (unsigned char)(msgs[0].Data[0] + 0x40); // UDS-style positive
    for (unsigned long i = 1; i < resp.DataSize; ++i) resp.Data[i] = msgs[0].Data[i];
    g_rxQueue.push_back(resp);
    return 0;
}

__declspec(dllexport) long __stdcall PassThruReadMsgs(unsigned long, PASSTHRU_MSG* msgs,
    unsigned long* num, unsigned long)
{
    if (g_rxQueue.empty()) { *num = 0; return 0xB0; }   // mock-defined timeout
    msgs[0] = g_rxQueue.front(); g_rxQueue.erase(g_rxQueue.begin());
    *num = 1; return 0;
}

__declspec(dllexport) long __stdcall PassThruReadVersion(unsigned long, char* fw,
    char* dll, char* api)
{ strcpy(fw, "1.0-MOCK"); strcpy(dll, "1.0-MOCK"); strcpy(api, "4.04"); return 0; }

__declspec(dllexport) long __stdcall PassThruGetLastError(char* desc)
{ strcpy(desc, "MOCK: no error"); return 0; }

// filters / periodic / progvol / ioctl: trivially succeed
__declspec(dllexport) long __stdcall PassThruStartMsgFilter(unsigned long, unsigned long,
    PASSTHRU_MSG*, PASSTHRU_MSG*, PASSTHRU_MSG*, unsigned long* fid)
{ *fid = 1; return 0; }
// ...StopMsgFilter, Start/StopPeriodicMsg, SetProgrammingVoltage, Ioctl: return 0
}
