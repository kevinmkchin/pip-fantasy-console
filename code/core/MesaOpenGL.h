#pragma once

#if MESA_WINDOWS
    #include <gl3w.h>
    #define MESA_USING_GL3W
#elif MESA_MACOSX
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#endif
