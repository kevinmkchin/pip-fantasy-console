#include "GfxShader.h"

#include "MesaCommon.h"
#include "FileSystem.h"

namespace Gfx
{
    static i32 GetCachedUniformLocation(const Shader& shader, const char* uniformName)
    {
        auto location_iter = shader.uniformLocationsMap.find(uniformName);
        if (location_iter != shader.uniformLocationsMap.end())
        {
            return location_iter->second;
        }
        return -1;
    }

    static void warningUniformNotFound(const Shader& shader, const char* uniformName)
    {
        if (shader.bPrintWarnings)
        {
            printf("Warning: Uniform '%s' doesn't exist or isn't active on shader %d.\n", uniformName, shader.idShaderProgram);
        }
    }

    static void cacheUniformLocation(Shader& shader, const char* uniformName)
    {
        i32 location = glGetUniformLocation(shader.idShaderProgram, uniformName);
        if (location != 0xffffffff)
        {
            shader.uniformLocationsMap[std::string(uniformName)] = location;
        }
        else
        {
            printf("Warning! Unable to get the location of uniform '%s' for shader id %d...\n", uniformName, shader.idShaderProgram);
        }
    }

    static void cacheUniformLocations(Shader& shader)
    {
        shader.uniformLocationsMap.clear();

        GLint longest_uniform_name_length;
        GLint number_of_uniforms;
        glGetProgramiv(shader.idShaderProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &longest_uniform_name_length);
        glGetProgramiv(shader.idShaderProgram, GL_ACTIVE_UNIFORMS, &number_of_uniforms);
        //printf("number of active uniforms for shader %d:  %d\n", shader.id_shader_program, number_of_uniforms);

        GLint readlength;
        GLint size;
        GLenum type;
        GLchar uniformName[128]; ASSERT(longest_uniform_name_length <= 128);

        /**If one or more elements of an array are active, the name of the array is returned in name, the type is returned
            * in type, and the size parameter returns the highest array element index used, plus one, as determined by the compiler
            * and/or linker. Only one active uniform variable will be reported for a uniform array.
        */
        for (GLint i = 0; i < number_of_uniforms; ++i)
        {
            glGetActiveUniform(shader.idShaderProgram, i, longest_uniform_name_length, &readlength, &size, &type, uniformName);
            cacheUniformLocation(shader, uniformName);
        }
    }

    static void GLCompileShader(u32 program_id, const char* shader_code, GLenum shaderType)
    {
        GLuint id_shader = glCreateShader(shaderType);             // Create an empty shader of given type and get id
        GLint code_length = (GLint)strlen(shader_code);
        glShaderSource(id_shader, 1, &shader_code, &code_length);   // Fill the empty shader with the shader code
        glCompileShader(id_shader);                                 // Compile the shader source

        GLint result = 0;
        GLchar eLog[1024] = {};
        glGetShaderiv(id_shader, GL_COMPILE_STATUS, &result);       // Make sure the shader compiled correctly
        if (!result)
        {
            glGetProgramInfoLog(id_shader, sizeof(eLog), nullptr, eLog);
            printf("Error compiling the %d shader: '%s' \n", shaderType, eLog);
            return;
        }

        glAttachShader(program_id, id_shader);
    }

    static bool GLCheckErrorAndValidate(GLuint program_id)
    {
    #if INTERNAL_BUILD
        GLint result = 0;
        GLchar eLog[1024] = {};
        glGetProgramiv(program_id, GL_LINK_STATUS, &result); // Make sure the program was created
        if (!result)
        {
            glGetProgramInfoLog(program_id, sizeof(eLog), nullptr, eLog);
            printf("Error linking program: '%s'! Aborting.\n", eLog);
            return true;
        }

        // Validate the program will work
        glValidateProgram(program_id);
        glGetProgramiv(program_id, GL_VALIDATE_STATUS, &result);
        if (!result)
        {
            glGetProgramInfoLog(program_id, sizeof(eLog), nullptr, eLog);
            printf("Error validating program %u: %s", program_id, eLog);
        }
    #endif
        return false;
    }




    void GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* fragmentPath)
    {
        std::string v = ReadFileString(vertexPath);
        std::string f = ReadFileString(fragmentPath);
        GLCreateShaderProgram(shader, v.c_str(), f.c_str());
    }

    void GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* geometryPath, const char* fragmentPath)
    {
        std::string v = ReadFileString(vertexPath);
        std::string g = ReadFileString(geometryPath);
        std::string f = ReadFileString(fragmentPath);
        GLCreateShaderProgram(shader, v.c_str(), g.c_str(), f.c_str());
    }

    void GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* fragmentShaderStr)
    {
        shader.idShaderProgram = glCreateProgram(); // Create an empty shader program and get the id
        if (!shader.idShaderProgram)
        {
            printf("Failed to create shader program! Aborting.\n");
            return;
        }
        GLCompileShader(shader.idShaderProgram, vertexShaderStr, GL_VERTEX_SHADER); // Compile and attach the shaders
        GLCompileShader(shader.idShaderProgram, fragmentShaderStr, GL_FRAGMENT_SHADER);
        glLinkProgram(shader.idShaderProgram); // Actually create the exectuable shader program on the graphics card

#if INTERNAL_BUILD
        if (GLCheckErrorAndValidate(shader.idShaderProgram))
        {
            return;
        }
#endif

        cacheUniformLocations(shader);
    }

    void GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* geometryShaderStr, const char* fragmentShaderStr)
    {

        shader.idShaderProgram = glCreateProgram(); // Create an empty shader program and get the id
        if (!shader.idShaderProgram)
        {
            printf("Failed to create shader program! Aborting.\n");
            return;
        }
        GLCompileShader(shader.idShaderProgram, vertexShaderStr, GL_VERTEX_SHADER); // Compile and attach the shaders
        GLCompileShader(shader.idShaderProgram, geometryShaderStr, GL_GEOMETRY_SHADER);
        GLCompileShader(shader.idShaderProgram, fragmentShaderStr, GL_FRAGMENT_SHADER);
        glLinkProgram(shader.idShaderProgram); // Actually create the exectuable shader program on the graphics card

#if INTERNAL_BUILD
        if (GLCheckErrorAndValidate(shader.idShaderProgram))
        {
            return;
        }
#endif

        cacheUniformLocations(shader);
    }

#if GL_VERSION_4_3
    void GLLoadComputeShaderProgramFromFile(Shader& shader, const char* computePath)
    {
        std::string c = ReadFileString(computePath);
        GLCreateComputeShaderProgram(shader, c.c_str());
    }

    void GLCreateComputeShaderProgram(Shader& shader, const char* computeShaderStr)
    {
        shader.idShaderProgram = glCreateProgram();
        if (!shader.idShaderProgram)
        {
            printf("Failed to create shader program! Aborting.\n");
            return;
        }
        GLCompileShader(shader.idShaderProgram, computeShaderStr, GL_COMPUTE_SHADER);
        glLinkProgram(shader.idShaderProgram);
#if INTERNAL_BUILD
        if (GLCheckErrorAndValidate(shader.idShaderProgram))
        {
            return;
        }
#endif
        cacheUniformLocations(shader);
    }
#endif

    /** Delete the shader program off GPU memory */
    void GLDeleteShader(Shader& shader)
    {
        if (shader.idShaderProgram == 0)
        {
            printf("WARNING: Passed an unloaded shader program to GLDeleteShader! Aborting.\n");
            return;
        }
        glDeleteProgram(shader.idShaderProgram);
    }

    /** Telling opengl to start using this shader program */
    void UseShader(const Shader& shader)
    {
        if (shader.idShaderProgram == 0)
        {
            printf("WARNING: Passed an unloaded shader program to UseShader! Aborting.\n");
            return;
        }
        glUseProgram(shader.idShaderProgram);
    }

    void GLBind1i(const Shader& shader, const char* uniformName, GLint v0)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform1i(location, v0);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind2i(const Shader& shader, const char* uniformName, GLint v0, GLint v1)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform2i(location, v0, v1);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind3i(const Shader& shader, const char* uniformName, GLint v0, GLint v1, GLint v2)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform3i(location, v0, v1, v2);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind4i(const Shader& shader, const char* uniformName, GLint v0, GLint v1, GLint v2, GLint v3)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform4i(location, v0, v1, v2, v3);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind1f(const Shader& shader, const char* uniformName, GLfloat v0)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform1f(location, v0);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind2f(const Shader& shader, const char* uniformName, GLfloat v0, GLfloat v1)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform2f(location, v0, v1);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind3f(const Shader& shader, const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform3f(location, v0, v1, v2);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBind4f(const Shader& shader, const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniform4f(location, v0, v1, v2, v3);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBindMatrix3fv(const Shader& shader, const char* uniformName, GLsizei count, const GLfloat* value)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniformMatrix3fv(location, count, GL_FALSE, value);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }

    void GLBindMatrix4fv(const Shader& shader, const char* uniformName, GLsizei count, const GLfloat* value)
    {
        i32 location = GetCachedUniformLocation(shader, uniformName);
        if (location >= 0)
        {
            glUniformMatrix4fv(location, count, GL_FALSE, value);
        }
        else
        {
            warningUniformNotFound(shader, uniformName);
        }
    }
    
}
