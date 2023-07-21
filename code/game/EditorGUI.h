#pragma once

#include "../core/MesaCommon.h"

#include <vector>

/*


[ tab 1 ][ tab 2 ][ tab 3 ]
___________________________
below the line is the code editor


*/

struct CodeEditor
{
    std::vector<char> codeBuf;
    //std::string?
    u64 cursorPos;
};

void DoEditorGUI();
