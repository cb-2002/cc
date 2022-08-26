@echo off

pushd build
for /f "tokens=1,*" %%a in (..\test.tsv) do (
	set expected=%%a
	if not %%a == SKIP (
		echo %%b
		cc.exe %%b > tmp.asm || exit /b
		nasm -f win64 tmp.asm -o tmp.obj || exit /b
		link /nologo /entry:main /out:tmp.exe tmp.obj || exit /b
		tmp.exe
		if not errorlevel == %%a goto :error
	)
)
popd
exit /b

:error
echo got %errorlevel%, expected %expected%
nasm -g -F cv8 -f win64 tmp.asm -o tmp.obj 
link /pdb:tmp.pdf /debug /nologo /entry:main /out:tmp.exe tmp.obj 
type tmp.asm
popd
