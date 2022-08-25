@echo off
if not exist build mkdir build
pushd build
cl /Zi /nologo /Fe:cc.exe ..\main.c
popd
