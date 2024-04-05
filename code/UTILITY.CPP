#include "UTILITY.H"
#include "MesaCommon.h"
#include "MesaOpenGL.h"
#include "Console.h"

#include <cassert>
#include <chrono>


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

        // TODO(Kevin): CorePrintLog.ErrorFmt("OpenGL: {}", error_str);
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
                case 96: { keycode = 126; } break;
            }
        }
    }

    return keycode;
}

std::string& RemoveCharactersFromEndOfString(std::string& str, char c)
{
    while (str.back() == c)
    {
        str.pop_back();
    }
    return str;
}

vec3 HSVToRGB(float h, float s, float v)
{
    // https://www.rapidtables.com/convert/color/hsv-to-rgb.html
    u16 huedegree = u16(h * 359.f);
    float chroma = v*s;
    float m = v-chroma;
    float normalizedx = chroma * (1.f - abs(fmod((h * 359.f) / 60.f, 2.f) - 1.f));

    vec3 rgb;
    if (huedegree < 60)
        rgb = { chroma, normalizedx, 0.f };
    else if (huedegree < 120)
        rgb = { normalizedx, chroma, 0.f };
    else if (huedegree < 180)
        rgb = { 0.f, chroma, normalizedx };
    else if (huedegree < 240)
        rgb = { 0.f, normalizedx, chroma };
    else if (huedegree < 300)
        rgb = { normalizedx, 0.f, chroma };
    else if (huedegree < 360)
        rgb = { chroma, 0.f, normalizedx };
    rgb += vec3(m,m,m);
    return rgb;
}

vec3 RGBToHSV(float r, float g, float b)
{
    // https://en.wikipedia.org/wiki/Hue
    // https://www.rapidtables.com/convert/color/rgb-to-hsv.html

    float h = atan2(1.7320508f * (g - b), 2 * r - g - b);
    if (h < 0)
        h = GM_TWOPI + h;
    h = abs(h) / GM_TWOPI;

    float cmax = GM_max(r, GM_max(g, b));
    float cmin = GM_min(r, GM_min(g, b));
    float cdelta = cmax - cmin;
    float s = cmax == 0.f ? 0.f : cdelta / cmax;
    
    vec3 hsv = vec3(h, s, cmax);
    return hsv;
}

Timer Time;

using Clock = std::chrono::high_resolution_clock;

static std::chrono::time_point<std::chrono::high_resolution_clock> programStartTime;

Timer::Timer()
        : deltaTime(-1.f)
        , time(0.f)
        , timeSinceStart(0.f)
        , timeScale(1.f)
        , unscaledDeltaTime(-1.f)
{
    programStartTime = Clock::now();
}

float Timer::UpdateDeltaTime()
{
    static Clock::time_point timeAtLastUpdate = Clock::now();

    auto now = Clock::now();
    float elapsedMs = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - timeAtLastUpdate)).count() * 0.001f;
    timeAtLastUpdate = now;
    float deltaTimeInSeconds = elapsedMs * 0.001f; // elapsed time in SECONDS
    float currentTimeInSeconds = (float)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() * 0.001f);
    time = currentTimeInSeconds;
    unscaledDeltaTime = deltaTimeInSeconds;
    timeSinceStart += unscaledDeltaTime;
    deltaTime = unscaledDeltaTime * Time.timeScale;

    return deltaTime;
}

float Timer::TimeStamp()
{
    static Clock::time_point timeAtLastTimeStamp = Clock::now();
    auto now = Clock::now();
    float elapsedMs = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - timeAtLastTimeStamp)).count() * 0.001f;
    timeAtLastTimeStamp = now;
    float difference = elapsedMs * 0.001f;
    return difference;
}

double Timer::TimeSinceProgramStartInSeconds()
{
    auto now = Clock::now();
    return std::chrono::duration<double>(now - programStartTime).count();
}

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

