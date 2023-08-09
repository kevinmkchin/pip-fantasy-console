cmake_minimum_required(VERSION 3.20)
set (CMAKE_CXX_STANDARD 14)

project(mesa)
set(PROJECT_BUILD_VERSION "v0.0.a1")
set(PROJECT_WORKING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wd/")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/code/core/cmake_MesaProjectDefines.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/code/core/cmake_MesaProjectDefines.h")


set(SOURCE_FILES
        code/MesaMain.cpp
        code/core/MesaUtility.cpp
        code/core/Timer.cpp
        code/core/Console.cpp
        code/core/GfxRenderer.cpp
        code/core/GfxDataTypesAndUtility.cpp
        code/core/GfxShader.cpp
        code/core/FileSystem.cpp
        code/core/PrintLog.cpp
        code/core/InputSystem.cpp
        code/core/MesaIMGUI.cpp
        code/core/MemoryAllocator.cpp
        code/core/EditorState.cpp
        code/core/EditorGUI.cpp
        code/core/EditorCodeEditor.cpp
        code/game/script/MesaScript.cpp
        code/game/Game.cpp
        code/game/Space.cpp
        code/game/AssetManager.cpp
        )


add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_compile_definitions(INTERNAL_BUILD=1)

# Build options, additional include directories, library linking, copying DLLs to executable directory

if(INTERNAL_BUILD MATCHES 0)
    message(STATUS "External build: " ${PROJECT_BUILD_VERSION})
else()
    message(STATUS "INTERNAL_BUILD: " ${PROJECT_BUILD_VERSION})
endif()

## Windows
if(WIN32)

    if(MSVC)
      add_definitions(/MP)
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
    target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES})

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