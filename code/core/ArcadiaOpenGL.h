#pragma once

#if _WIN32
    #include <gl3w.h>
    #define MESA_USING_GL3W
#elif __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#endif
