//
// Created by Kevin Chin on 2023-02-22.
//

#pragma once

#include <string>

class CorePrintLog {
public:
    void Message(const std::string& message);
    void Warning(const std::string& warning_message);
    void Error(const std::string& error_message);
};

extern CorePrintLog PrintLog;
