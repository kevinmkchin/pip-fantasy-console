@echo off

if not exist build mkdir build
echo Build started: %time%

if not "%1" == "clean" goto generate
cmake --build build --target clean
goto build

:generate
cmake -S . -B build
goto build

:build
cmake --build build
echo Build finished: %time%
