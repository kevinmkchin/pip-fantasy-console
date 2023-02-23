#include "CorePrintLog.h"

CorePrintLog PrintLog;

void CorePrintLog::Message(const std::string& message)
{
    printf("%s", std::string(message + "\n").c_str());
//    console_print(std::string(message + "\n").c_str());
}

void CorePrintLog::Warning(const std::string& warning_message)
{
    printf("%s", std::string("WARNING: " + warning_message + "\n").c_str());
//    console_print(std::string("WARNING: " + warning_message + "\n").c_str());
}

void CorePrintLog::Error(const std::string& error_message)
{
    printf("%s", std::string("ERROR: " + error_message + "\n").c_str());
//    console_print(std::string("ERROR: " + error_message + "\n").c_str());
}
