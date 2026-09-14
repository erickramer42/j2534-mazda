#include "j2534_types.h"

#include <cstring>
#include <map>
#include <vector>
#include <mutex>
#include <set>

static std::mutex g_lock;
static std::vector<PASSTHRU_MSG> g_rxQueue;
static std::map<unsigned long, unsigned long> g_config; // param -> value
static unsigned long g_nextChannel = 0;
static std::set<unsigned long> g_channels;
static unsigned long g_nextFilter = 0;
static unsigned long g_nextPeriodic = 0;
static std::vector<PASSTHRU_MSG> g_periodicQueue;
static unsigned long g_nextPeriodicID = 0x200;

static void StrCopy80(char* dst, const char* src)
{
    strncpy(dst, src, 79);
    dst[79] = '\0';
}

extern "C" {

__declspec(dllexport) long __stdcall 
PassThruOpen(const char*, unsigned long* pDeviceID)
{ 
    if (!pDeviceID) return 0xC1;
    *pDeviceID = 1; 
    return 0;
}

__declspec(dllexport) long __stdcall 
PassThruClose(unsigned long) { return 0; }

__declspec(dllexport) long __stdcall 
PassThruConnect(unsigned long, unsigned long, unsigned long, unsigned long,
                unsigned long* pChannelID)
{ 
    if (!pChannelID) return 0xC1;
    std::lock_guard<std::mutex> lk(g_lock);
    *pChannelID = ++g_nextChannel;
    g_channels.insert(*pChannelID);
    return 0;
}

__declspec(dllexport) long __stdcall PassThruDisconnect(unsigned long ChannelID) {
    std::lock_guard<std::mutex> lk(g_lock);
    g_channels.erase(ChannelID);
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs,
                  unsigned long)
{
    if (!pMsg || !pNumMsgs || *pNumMsgs == 0 || g_channels.find(ChannelID) == g_channels.end()) return 0xC1;
    PASSTHRU_MSG resp = {};
    resp.ProtocolID = pMsg[0].ProtocolID;
    resp.Timestamp  = 1000 + pMsg[0].DataSize;
    unsigned long n = pMsg[0].DataSize < 8 ? pMsg[0].DataSize : 8;
    resp.DataSize = n;
    resp.ExtraDataIndex = n;
    resp.Data[0] = (unsigned char)(pMsg[0].Data[0] + 0x40);   // UDS-style positive
    for (unsigned long i = 1; i < n; ++i) resp.Data[i] = pMsg[0].Data[i];
    std::lock_guard<std::mutex> lk(g_lock);
    g_rxQueue.push_back(resp);
    *pNumMsgs = 1;
    return 0;
}


__declspec(dllexport) long __stdcall
PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs,
                 unsigned long)
{
    if (!pMsg || !pNumMsgs || g_channels.find(ChannelID) == g_channels.end()) return 0xC1;
    std::lock_guard<std::mutex> lk(g_lock);

    // Yield periodic first, then queued responses
    if (!g_periodicQueue.empty()) {
        *pMsg = g_periodicQueue.front();
        g_periodicQueue.erase(g_periodicQueue.begin());  // pop it after one read, can modify to keep it in the queue for repeated reads if desired
        *pNumMsgs = 1;
        return 0;
    }

    if (g_rxQueue.empty()) { *pNumMsgs = 0; return 0xB0; }    // mock-defined timeout
    *pMsg = g_rxQueue.front();
    g_rxQueue.erase(g_rxQueue.begin());
    *pNumMsgs = 1;
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruReadVersion(unsigned long, char* pFirmwareVersion,
                    char* pDllVersion, char* pApiVersion)
{
    if (!pFirmwareVersion || !pDllVersion || !pApiVersion) return 0xC1;
    StrCopy80(pFirmwareVersion, "1.0-MOCK");
    StrCopy80(pDllVersion,     "1.0-MOCK");
    StrCopy80(pApiVersion,    "4.04");
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruGetLastError(char* pErrorDescription)
{
    if (!pErrorDescription) return 0xC1;
    StrCopy80(pErrorDescription, "MOCK: no error");
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruStartMsgFilter(unsigned long ChannelID, unsigned long, PASSTHRU_MSG*,
                       PASSTHRU_MSG*, PASSTHRU_MSG*, unsigned long* pFilterID)
{
    if (!pFilterID || g_channels.find(ChannelID) == g_channels.end()) return 0xC1;
    std::lock_guard<std::mutex> lk(g_lock);
    *pFilterID = ++g_nextFilter;
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruStopMsgFilter(unsigned long ChannelID, unsigned long FilterID)
{
    if (g_channels.find(ChannelID) == g_channels.end()) return 0xC1;
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pMsgID,
                         unsigned long)
{
    if (!pMsgID || !pMsg || g_channels.find(ChannelID) == g_channels.end()) return 0xC1;
    std::lock_guard<std::mutex> lk(g_lock);
    *pMsgID = ++g_nextPeriodicID;
    g_periodicQueue.push_back(*pMsg);
    return 0;
}

__declspec(dllexport) long __stdcall
PassThruStopPeriodicMsg(unsigned long, unsigned long) 
{ 
    std::lock_guard<std::mutex> lk(g_lock);
    for (auto it = g_periodicQueue.begin(); it != g_periodicQueue.end();) {
        // Mark as stopped; just clear the queue for simplicity, can be modified to remove only the specific ID if desired
        it = g_periodicQueue.erase(it);
    }
    return 0; 
}

__declspec(dllexport) long __stdcall
PassThruSetProgrammingVoltage(unsigned long, unsigned long, unsigned long)
{ return 0; }

// GET_CONFIG returns whatever was last SET_CONFIG'd, so the harness can
// round-trip a value end to end and assert on the proxy's decoded log lines.
__declspec(dllexport) long __stdcall
PassThruIoctl(unsigned long, unsigned long IoctlID, void* pInput, void* pOutput)
{
    std::lock_guard<std::mutex> lk(g_lock);
    if (IoctlID == 0x02 && pInput) {                        // SET_CONFIG
        SCONFIG_LIST* list = (SCONFIG_LIST*)pInput;
        for (unsigned long i = 0; i < list->NumOfParams; ++i)
            g_config[list->ConfigPtr[i].Parameter] = list->ConfigPtr[i].Value;
        return 0;
    }
    if (IoctlID == 0x01 && pOutput) {                       // GET_CONFIG
        SCONFIG_LIST* list = (SCONFIG_LIST*)pOutput;
        for (unsigned long i = 0; i < list->NumOfParams; ++i) {
            unsigned long p = list->ConfigPtr[i].Parameter;
            auto it = g_config.find(p);
            list->ConfigPtr[i].Value =
                (it != g_config.end()) ? it->second : 0xDEADBEEF;
        }
        return 0;
    }
    return 0;
}

// NOTE: PassThruReadVoltage is deliberately NOT exported.
// It exercises the proxy's unresolved-export path (expected 0xE2 +
// a 'GetProcAddress failed' log line, no crash).

} // extern "C"
