#pragma once

#include "CoreCommon.h"
#include "ArcadiaOpenGL.h"

#include <unordered_map>

struct Shader
{
    static void GLDeleteShader(Shader& shader);
    static void GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* fragmentPath);
    static void GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* geometryPath, const char* fragmentPath);
    static void GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* fragmentShaderStr);
    static void GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* geometryShaderStr, const char* fragmentShaderStr);
#if GL_VERSION_4_3
    static void GLLoadComputeShaderProgramFromFile(Shader& shader, const char* computePath);
    static void GLCreateComputeShaderProgram(Shader& shader, const char* computeShaderStr);
#endif

    void UseShader() const;

    void GLBind1i(const char* uniformName, GLint v0) const;
    void GLBind2i(const char* uniformName, GLint v0, GLint v1) const;
    void GLBind3i(const char* uniformName, GLint v0, GLint v1, GLint v2) const;
    void GLBind4i(const char* uniformName, GLint v0, GLint v1, GLint v2, GLint v3) const;
    void GLBind1f(const char* uniformName, GLfloat v0) const;
    void GLBind2f(const char* uniformName, GLfloat v0, GLfloat v1) const;
    void GLBind3f(const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2) const;
    void GLBind4f(const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const;
    void GLBindMatrix3fv(const char* uniformName, GLsizei count, const GLfloat* value) const;
    void GLBindMatrix4fv(const char* uniformName, GLsizei count, const GLfloat* value) const;

    i32 GetCachedUniformLocation(const char* uniformName) const;

    void SuppressWarningsPush();
    void SuppressWarningsPop();

private:
    bool bPrintWarnings = true;

    GLuint idShaderProgram = 0; // id of this shader program in GPU memory

    std::unordered_map<std::string, i32> uniformLocationsMap;

    static void cacheUniformLocations(Shader& shader);

    static void cacheUniformLocation(Shader& shader, const char* uniformName);

    void warningUniformNotFound(const char* uniformName) const;

    // Create shader on GPU and compile shader
    static void GLCompileShader(u32 program_id, const char* shader_code, GLenum shaderType);

    // Return true if there was a compile error
    static bool GLCheckErrorAndValidate(GLuint program_id);
};

