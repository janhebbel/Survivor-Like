@echo off

if not exist bin mkdir bin

pushd bin
cl.exe -nologo -std:c11 -FC -Zi -Od -W4 -WX -wd4090 -wd4100 -external:W0 -I..\inc -Fesurvivor ..\src\main.c -link -incremental:no -subsystem:windows user32.lib gdi32.lib opengl32.lib
popd
