# Mesa Fantasy Console
### By Kevin Myungkeon Chin

- Easy to learn
- Frictionless
- Limitations to scope ambition (USEFUL CONSTRAINTS - "useful" for gamedev)
- Engine is only as good as its tools, documentation, and error messages
- For Mac, Windows, Linux

## What is it?

The **Mesa Computer** is a game development environment.

## MVP
- Sprites and import pngs (objects?)
- Game logic scripting engine

## Add after MVP
- Screen system and screen switching
- Collision and dispatch system
- Sound effects and music system


## Build on Windows

### Visual Studio
Open repo directory with Visual Studio (requires Cmake tools for VS to be installed).
Right click on CMakeLists.txt from the solution explorerer and "Set as Startup Item".


## Build on Mac

### Command line
```
cmake -S <cmakelists.txt source directory> -B <output directory>
cmake --build <output directory>
```
### CLion
Open CMakeLists.txt in root with CLion