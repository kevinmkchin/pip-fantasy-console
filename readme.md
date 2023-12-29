# name ideas
- pip
- blit
- "Mesa is a nice name."

### By Kevin Myungkeon Chin

- Easy to learn
- Frictionless
- Limitations to scope ambition (USEFUL CONSTRAINTS - "useful" for gamedev)
- Engine is only as good as its tools, documentation, and error messages
- For Mac, Windows, Linux

## What is it?

pip is a game making tool.

SNES sprite graphics and Newgrounds (Dad n' Me, Alien Hominid) 2D graphics both possible.

Build Diablo 2 / Skyrim like Roguelike along the way.
https://store.steampowered.com/app/2218750/Halls_of_Torment/


## Build on Windows

```
build
build clean - clean build directory
build release - release build 

run
run release
run vs - start vs sln
run subl - start sublime project
```

### Visual Studio
Open repo directory with Visual Studio (requires Cmake tools for VS to be installed).
Right click on CMakeLists.txt from the solution explorerer and "Set as Startup Item".

### CLion
Open CMakeLists.txt in root with CLion

## Build on Mac

### Command line
```
cmake -S <cmakelists.txt source directory> -B <output directory>
cmake --build <output directory>
```
### CLion
Open CMakeLists.txt in root with CLion