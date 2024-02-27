cmake_minimum_required(VERSION 3.20)
set (CMAKE_CXX_STANDARD 14)

project(mesa)
set(PROJECT_BUILD_VERSION "v0.0.a1")
set(PROJECT_WORKING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wd/")

if (WIN32)
    set(MESA_WINDOWS 1)
    set(MESA_MACOSX 0)
else(APPLE)
    set(MESA_WINDOWS 0)
    set(MESA_MACOSX 1)
endif()



set(SOURCE_FILES
        code/cmake_MesaProjectDefines.h.in
        code/MesaCommon.h
        code/MesaMath.h
        code/MesaOpenGL.h
        code/MesaMain.h
        code/MesaMain.cpp
        code/MesaUtility.h
        code/MesaUtility.cpp
        code/Timer.h
        code/Timer.cpp
        code/Console.h
        code/Console.cpp
        code/GfxRenderer.h
        code/GfxRenderer.cpp
        code/GfxDataTypesAndUtility.h
        code/GfxDataTypesAndUtility.cpp
        code/GfxShader.h
        code/GfxShader.cpp
        code/FileSystem.h
        code/FileSystem.cpp
        code/PrintLog.h
        code/PrintLog.cpp
        code/Input.h
        code/Input.cpp
        code/MesaIMGUI.h
        code/MesaIMGUI.cpp
        code/MemoryAllocator.h
        code/MemoryAllocator.cpp
        code/GameData.h
        code/GameData.cpp
        code/Game.h
        code/Game.cpp
        code/PipAPI.h
        code/PipAPI.cpp
        code/ByteBuffer.h

        code/piplang/VM.h
        code/piplang/VM.cpp
        code/piplang/PipLangCommon.h
        code/piplang/Scanner.h
        code/piplang/Scanner.cpp
        code/piplang/Compiler.h
        code/piplang/Compiler.cpp
        code/piplang/Chunk.h
        code/piplang/Chunk.cpp
        code/piplang/Object.h
        code/piplang/Object.cpp
        code/piplang/Debug.h
        code/piplang/Debug.cpp

        code/editor/Editor.h
        code/editor/Editor.cpp
        code/editor/CodeEditor.h
        code/editor/CodeEditor.cpp
        code/editor/SpriteEditor.h
        code/editor/SpriteEditor.cpp
        code/editor/SpriteEditorActions.cpp
        code/editor/SpriteEditorActions.h

)

set(OTHER_FILES_FOR_IDE
    todo.md
        code/editor/SpriteEditorActions.cpp
        code/editor/SpriteEditorActions.h
)

file(GLOB DOCUMENTATION_MARKDOWNS true code/manual/*.md)


add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${OTHER_FILES_FOR_IDE} ${DOCUMENTATION_MARKDOWNS})

add_compile_definitions(INTERNAL_BUILD=1)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/code/cmake_MesaProjectDefines.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/code/cmake_MesaProjectDefines.h")

# Build options, additional include directories, library linking, copying DLLs to executable directory

if(INTERNAL_BUILD MATCHES 0)
    message(STATUS "External build: " ${PROJECT_BUILD_VERSION})
else()
    message(STATUS "INTERNAL_BUILD: " ${PROJECT_BUILD_VERSION})
endif()

## Windows
if(WIN32)

    if(MSVC)
      add_definitions(/MP -D_CRT_SECURE_NO_WARNINGS)
      target_compile_options(${PROJECT_NAME} PUBLIC "/W3" "/EHsc" "/we4239")
    endif()

    # Find OpenGL
    find_package(OpenGL REQUIRED)
    if (OPENGL_FOUND)
       target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
       target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_gl_LIBRARY})
    endif()

    set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/include")
    set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2.lib;${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2main.lib")
    set(SDL_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2.dll")

    # Copy and rename dlls
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2.dll")

    target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS} ext/gl3w)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES} dwmapi.lib)

## MacOS
else(APPLE)

    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
        message(STATUS "SDL2 found.")
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
    else()
    	message(FATAL_ERROR "CMake couldn't find SDL2 on MacOSX.")
    endif()

    find_package(OpenGL REQUIRED)
    if(OpenGL_FOUND)
        message(STATUS "OpenGL found.")
        target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
    else()
        message(FATAL_ERROR "CMake couldn't find OpenGL on MacOSX.")
    endif()

endif()