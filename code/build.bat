@echo off
REM TODO - can we just build both with one exe?
echo "Build script starting..."
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM Compile with debugging info (Zi)
cl -nologo -MT -Oi -GR- -Gm -EHa- -W4 -WX -wd4201 -wd4100 -wd4189 -Zi /Od ..\handmade\code\win32_handmade.cpp /link -opt:ref -subsystem:windows,5.1 user32.lib gdi32.lib
popd
