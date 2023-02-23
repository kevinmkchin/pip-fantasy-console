#include "ArcadiaUtility.h"
#include "CoreCommon.h"
#include "ArcadiaOpenGL.h"
#include "CorePrintLog.h"

#include <cassert>

bool GLHasErrors()
{
    GLenum error = glGetError();

    if (error == GL_NO_ERROR) return false;

    while (error != GL_NO_ERROR)
    {
        const char* error_str = "";
        switch (error)
        {
            case GL_INVALID_OPERATION:
                error_str = "INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                error_str = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error_str = "INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                error_str = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error_str = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }

        // TODO(Kevin): PrintLog.ErrorFmt("OpenGL: {}", error_str);
        error = glGetError();
        assert(false);
    }

    return true;
}

void PickThreeRandomInts(int* x, int* y, int* z, int cap)
{
    assert(cap >= 3);
    *x = rand()%cap;
    do {
        *y = rand()%cap;
    } while(*y == *x);
    do {
        *z = rand()%cap;
    } while(*z == *x || *z == *y);
}

int RandomInt(int min, int max)
{
    int retval = min + (rand() % static_cast<int>(max - min + 1));
    return retval;
}

i32 ModifyASCIIBasedOnModifiers(i32 keycodeASCII, bool shift)
{
    i32 keycode = keycodeASCII;

    if (shift)
    {
        if (97 <= keycode && keycode <= 122)
        {
            keycode -= 32;
        }
        else if (keycode == 50)
        {
            keycode = 64;
        }
        else if (49 <= keycode && keycode <= 53)
        {
            keycode -= 16;
        }
        else if (91 <= keycode && keycode <= 93)
        {
            keycode += 32;
        }
        else
        {
            switch (keycode)
            {
                case 48: { keycode = 41; } break;
                case 54: { keycode = 94; } break;
                case 55: { keycode = 38; } break;
                case 56: { keycode = 42; } break;
                case 57: { keycode = 40; } break;
                case 45: { keycode = 95; } break;
                case 61: { keycode = 43; } break;
                case 39: { keycode = 34; } break;
                case 59: { keycode = 58; } break;
                case 44: { keycode = 60; } break;
                case 46: { keycode = 62; } break;
                case 47: { keycode = 63; } break;
            }
        }
    }

    return keycode;
}

std::string& RemoveCharactersFromEndOfString(std::string& str, const char c)
{
    while (str.back() == c)
    {
        str.pop_back();
    }
    return str;
}
