cmake_minimum_required(VERSION 3.20)
set (CMAKE_CXX_STANDARD 14)

project(arcadia_fantasy_console)


set(SOURCE_FILES
	    code/ArcadiaFantasyConsole.cpp
        code/core/ArcadiaUtility.cpp
        code/core/ArcadiaTimer.cpp
        code/core/CoreRenderer.cpp
        code/core/CoreShader.cpp
        code/core/CoreFileSystem.cpp
        code/core/CorePrintLog.cpp
        code/core/CoreInput.cpp
        code/core/ArcadiaIMGUI.cpp
        code/core/CoreMemoryAllocator.cpp

        code/game/script/ArcadiaScriptInterpreter.cpp
        )


add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set(MY_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wd")


# Build options, additional include directories, library linking, copying DLLs to executable directory

## MacOS

#set_target_properties(${PROJECT_NAME} PROPERTIES
#        MACOSX_BUNDLE TRUE
#        MACOSX_FRAMEWORK_IDENTIFIER org.cmake.${PROJECT_NAME}
#        XCODE_GENERATE_SCHEME TRUE
#        XCODE_SCHEME_WORKING_DIRECTORY ${MY_WORKING_DIRECTORY})

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
