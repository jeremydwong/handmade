@echo off

mkdir ..\..\build
pushd ..\..\build
REM Compile with debugging info (Zi)
cl -Zi ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd
