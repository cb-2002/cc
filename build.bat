@echo off
set CF=-std=gnu99 -g -Wno-parentheses -Wno-switch -Wno-deprecated-declarations
if not exist build mkdir build
pushd build
clang %CF% -g -o cc.exe ..\main.c
popd
