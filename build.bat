@echo off

if not exist bin mkdir bin

pushd bin
:: Debug
cl.exe -nologo -std:c11 -D DEBUG=1 -FC -Zi -Od -W4 -WX -wd4090 -wd4100 -wd4127 -external:W0 -I..\inc -Fesurvivor ..\src\main.c -link -incremental:no -subsystem:windows user32.lib gdi32.lib opengl32.lib

:: Release
:: cl.exe -nologo -std:c11 -D DEBUG=0 -FC -O2 -W4 -WX -wd4090 -wd4100 -wd4127 -external:W0 -I..\inc -Fesurvivor ..\src\main.c -link -incremental:no -subsystem:windows user32.lib gdi32.lib opengl32.lib
popd
