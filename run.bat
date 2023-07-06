@echo off

IF "%1"=="vs" goto run-vs
IF "%1"=="subl" goto run-subl
goto run-bin

:run-vs
REM build\Debug\mesa.exe
devenv .
goto common-exit

:run-subl
subl source.sublime-project
goto common-exit

:run-bin
START build\Debug\mesa.exe
goto common-exit

:common-exit
