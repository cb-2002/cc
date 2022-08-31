@echo off

pushd build
for /f "tokens=1,*" %%a in (..\test.tsv) do (
	set expected=%%a
	if %%a == break goto :exit
	if not %%a == continue (
		echo %%b
		echo %%b > tmp.c
		cc.exe tmp.c > tmp.asm || goto :compile_error
		nasm -f win64 tmp.asm -o tmp.obj || goto :compile_error
		link /nologo /entry:main /out:tmp.exe tmp.obj || goto :exit
		tmp.exe
		if not errorlevel == %%a goto :run_error
	)
)
:exit
popd
exit /b

:compile_error
type tmp.asm
popd
goto :exit

:run_error
echo got %errorlevel%, expected %expected%
nasm -g -F cv8 -f win64 tmp.asm -o tmp.obj 
link /pdb:tmp.pdf /debug /nologo /entry:main /out:tmp.exe tmp.obj 
type tmp.asm
popd
goto :exit
