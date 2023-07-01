@echo off

IF "%1"=="vs" (
    devenv build\Debug\mesa.exe
) ELSE (
    START build\Debug\mesa.exe
)
