#include "PrintLog.h"
#include "Console.h"

CorePrintLog PrintLog;

void CorePrintLog::Message(const std::string& message)
{
    std::string msgFmt = std::string(message + "\n");
    SendMessageToConsole(msgFmt.c_str(), msgFmt.size());
}

void CorePrintLog::Warning(const std::string& warning_message)
{
    std::string msgFmt = std::string("WARNING: " + warning_message + "\n");
    SendMessageToConsole(msgFmt.c_str(), msgFmt.size());
}

void CorePrintLog::Error(const std::string& error_message)
{
    std::string msgFmt = std::string("ERROR: " + error_message + "\n");
    SendMessageToConsole(msgFmt.c_str(), msgFmt.size());
}
