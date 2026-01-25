@echo off
echo "Build script starting..."
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM Compile with debugging info (Zi)
cl /Zi /Od ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd
