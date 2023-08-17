#pragma once

#include "MesaCommon.h"
#include "MesaOpenGL.h"

#include <unordered_map>

namespace Gfx
{
    struct Shader
    {
        GLuint idShaderProgram = 0; // id of this shader program in GPU memory

        bool bPrintWarnings = true;
        std::unordered_map<std::string, i32> uniformLocationsMap;
    };

    void GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* fragmentPath);
    void GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* geometryPath, const char* fragmentPath);
    void GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* fragmentShaderStr);
    void GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* geometryShaderStr, const char* fragmentShaderStr);
#if GL_VERSION_4_3
    void GLLoadComputeShaderProgramFromFile(Shader& shader, const char* computePath);
    void GLCreateComputeShaderProgram(Shader& shader, const char* computeShaderStr);
#endif
    void GLDeleteShader(Shader& shader);

    void UseShader(const Shader& shader);

    void GLBind1i(const Shader& shader, const char* uniformName, GLint v0);
    void GLBind2i(const Shader& shader, const char* uniformName, GLint v0, GLint v1);
    void GLBind3i(const Shader& shader, const char* uniformName, GLint v0, GLint v1, GLint v2);
    void GLBind4i(const Shader& shader, const char* uniformName, GLint v0, GLint v1, GLint v2, GLint v3);
    void GLBind1f(const Shader& shader, const char* uniformName, GLfloat v0);
    void GLBind2f(const Shader& shader, const char* uniformName, GLfloat v0, GLfloat v1);
    void GLBind3f(const Shader& shader, const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2);
    void GLBind4f(const Shader& shader, const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void GLBindMatrix3fv(const Shader& shader, const char* uniformName, GLsizei count, const GLfloat* value);
    void GLBindMatrix4fv(const Shader& shader, const char* uniformName, GLsizei count, const GLfloat* value);
}
