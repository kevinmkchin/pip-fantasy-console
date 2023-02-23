#pragma once

#if _WIN32
#include <glew.h>
#define KEVIN_USING_GLEW
#elif __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#endif