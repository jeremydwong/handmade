@echo off
REM TODO - can we just build both with one exe?
echo "Build script starting..."
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

set CommonCompilerFlags=-nologo -MT -Oi -GR- -Gm -EHa- -W4 -WX -wd4201 -wd4100 -wd4101 -wd4189 -Zi /Od -DHANDMADE_INTERNAL=1
set CommonLinkerFlags=-opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM Compile with debugging info (Zi)

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link -subsystem:window,5.1 %CommonLinkerFlags%

REM 64-bit build
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags%
popd
