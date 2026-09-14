#ifndef LOGGER_H
#define LOGGER_H

#include "j2534_types.h"

void LoggerInit(const std::wstring& iniPath);
void LogCall(const char* fmt, ...);
void LogMsgs(const char* dir, unsigned long channelId,
             const PASSTHRU_MSG* msgs, unsigned long count, bool suspect);
const char* HexBytes(const PASSTHRU_MSG& msg);

#endif
