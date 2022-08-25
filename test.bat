@echo off

pushd build
call :assert "int main(){return 0;}" 0
call :assert "int main(){return 0;}" 0
call :assert "int main(){return 11;}" 11
call :assert "int main(){return 2+22;}" 24
call :assert "int main(){return  3 + 7 ;}" 10
call :assert "int main(){return 5-3;}" 2
call :assert "int main(){return 5+3+1;}" 9
call :assert "int main(){return 5+3-1+2;}" 9
call :assert "int main(){return (5+3)-(1+2);}" 5
call :assert "int main(){return 2*3;}" 6
call :assert "int main(){return 2*3*4;}" 24
call :assert "int main(){return 1+2*3-4;}" 3
call :assert "int main(){return 6/3;}" 2
call :assert "int main(){return 6/5;}" 1
call :assert "int main(){return 4/3*3;}" 3
call :assert "int main(){return 4/(3/3);}" 4
REM call :assert "int main(){return 10%2;}" 0
call :assert "int main(){return 8-1&3;}" 3
call :assert "int main(){return 7&3;}" 3
call :assert "int main(){return 2*4&5;}" 0
REM call :assert "int main(){return 7&3^3;}" 7
call :assert "int main(){return 10+2&20|9/3;}" 7
call :assert "int main(){return -5;}" -5
call :assert "int main(){return 6+-5;}" 1
call :assert "int main(){return ~3;}" -4
call :assert "int main(){return !-3+3;}" 3
call :assert "int main(){return !(-3+3);}" 1
call :assert "int main(){return 4<3;}" 0
call :assert "int main(){return 4>3;}" 1
call :assert "int main(){return 3>4;}" 0
call :assert "int main(){return 3<4;}" 1
call :assert "int main(){return 4<4;}" 0
call :assert "int main(){return 3<<2;}" 12
call :assert "int main(){return 15>>1;}" 7
call :assert "int main(){return 2==2;}" 1
call :assert "int main(){return 2!=2;}" 0
call :assert "int main(){return 0&&0;}" 0
call :assert "int main(){return 1&&0;}" 0
call :assert "int main(){return 0&&1;}" 0
call :assert "int main(){return 1&&1;}" 1
call :assert "int main(){return 0||0;}" 0
call :assert "int main(){return 1||0;}" 1
call :assert "int main(){return 0||1;}" 1
call :assert "int main(){return 1||1;}" 1
call :assert "int main(){return 0&&1&&0;}" 0
call :assert "int main(){return 0||1||0;}" 1
call :assert "int main(){return 0?1:2;}" 2
call :assert "int main(){return 1?1:2;}" 1
call :assert "int main(){return 0?1:0?2:3;}" 3
call :assert "int main(){return 0?1:1?2:3;}" 2
call :assert "int main(){return 1?1:0?2:3;}" 1
call :assert "int main(){return 1?1:1?2:3;}" 1
call :assert "int main(){return 1+2,2+3,3+4;}" 7
call :assert "int main(){return 1?1,2:3;}" 2
call :assert "int main(){return 0?1,2:3;}" 3
call :assert "int main(){return 0?1,2:3,4;}" 4
call :assert "int main(){return 1?1,2:3,4;}" 4
call :assert "int main(){int x,y;return x=2,y=3,x;}" 2
call :assert "int main(){int x,y;return x=2,y=3,x+y;}" 5
REM call :asseint main()rt "{int x,y;return x=4,*(&x-8)=5,y;}" 5
call :assert "int main(){1,2;2,3,4;return 5;}" 5
call :assert "int main(){return 0;}" 0
call :assert "int main(){return 0;1;}" 0
call :assert "int main(){int x=3;return x;}" 3
call :assert "int main(){int x=3;return x?1:2;}" 1
call :assert "int main(){int x=0;return x?1:2;}" 2
call :assert "int main(){if(0==1)1; else 2;}" 2
call :assert "int main(){if(1==1)1; else 2;}" 1
call :assert "int main(){if(0)if(0)return 1;else return 2;else return 3;}" 3
call :assert "int main(){if(0)if(1)return 1;else return 2;else return 3;}" 3
call :assert "int main(){if(1)if(0)return 1;else return 2;else return 3;}" 2
call :assert "int main(){if(1)if(1)return 1;else return 2;else return 3;}" 1
call :assert "int main(){if(0)return 1;else if(0)return 2;else return 3;}" 3
call :assert "int main(){if(0)return 1;else if(1)return 2;else return 3;}" 2
call :assert "int main(){if(1)return 1;else if(0)return 2;else return 3;}" 1
call :assert "int main(){if(1)return 1;else if(1)return 2;else return 3;}" 1
call :assert "int main(){for(int x=0,y=0;x<10;x=x+1)y=y+x;return y;}" 45
call :assert "int main(){for(int x=1,y=7;y>0;x=x*y,y=y-1);return x;}" 5040
call :assert "int main(){int x=1,y=7;for(;y>0;x=x*y,y=y-1);return x;}" 5040
call :assert "int main(){int x=1,y=7;for(;y>0;x=x*y,y=y-1);return x;}" 5040
call :assert "int main(){int x=1,y=7;for(;;x=x*y,y=y-1)if(y==1)return x;}" 5040
call :assert "int main(){int x=1,y=7;for(;;){if(y==0)return x;x=x*y,y=y-1;}}" 5040
call :assert "int main(){int x=0,y=1,z=0;while(z<100)z=x+y,x=y,y=z;return z;}" 144
call :assert "int main(){int x=0,y=1,z=0;while(1)if(z>100)return z;else z=x+y,x=y,y=z;return -1;}" 144
call :assert "int main(){int x=0,y=1,z;do z=x+y,x=y,y=z;while(z<100);return z;}" 144
call :assert "int main(){int x=0,y=1,z;do if(z>100)return z;else z=x+y,x=y,y=z;while(1);return -1;}" 144
call :assert "int main(){for(int x=0;x<10;x=x+1)if(x==5)break;return x;}" 5
call :assert "int main(){int y=0;for(int x=0;x<10;x=x+1)if(x&1)continue;else y=y+x;return y;}" 20
call :assert "int main(){for(int x=1,y=7;;x=x*y,y=y-1)if(y>0)continue;else return x;return -1;}" 5040
call :assert "int main(){int x=2,y=3;if(x!=y)goto L;else goto M;L:return 4;M:return 5;return -1;}" 4
call :assert "int main(){int x=2,y=3;if(x==y)goto L;else goto M;L:return 4;M:return 5;return -1;}" 5
call :assert "int main(){if(0)L:return 2;goto L;return -1;}" 2
call :assert "int main(){if(1)L:return 2;goto L;return -1;}" 2
call :assert "int main(){int x=2;{int x=3;}return x;}" 2
call :assert "int main(){int x=2;{x=3;}return x;}" 3
call :assert "int main(){return 0;}int f(){return 2;}" 0
call :assert "int f(){return 2;}int main(){return 3*f();}" 6
call :assert "int f(){return 2;}int g(){return 3;}int main(){return f()<<g();}" 16
call :assert "int f(){int x=3;return x;}int g(){int x=4;return x;}int main(){int x=5;return f()*g()+x;}" 17
call :assert "int f(int x){return 2;}int main(){return 0;}" 0
call :assert "int f(int x,int y){return 2;}int main(){return 0;}" 0
call :assert "int f(int x,int y){return 2;}int main(){return f(2,3);}" 2
call :assert "int f(int x,int y){return x*y;}int main(){return f(2,3);}" 6
call :assert "int f(int x,int y,int z){return x-y-z;}int main(){return f(2,3,4);}" -5
call :assert "int x;int f(){x=3;return 0;}int main(){f();return x;}" 3
call :assert "int x=3;int main(){return x;}" 3
call :assert "int main(){int x=2;return x++;}" 2
call :assert "int main(){int x=2;return x++,x;}" 3
call :assert "int main(){int x=2;return x--;}" 2
call :assert "int main(){int x=2;return x--,x;}" 1
call :assert "int main(){int x=2;return x+=3,x;}" 5
call :assert "int main(){int x=3;return x&=5,x;}" 1
REM call :assert "int main(){int x=5;return x%=3,x;}" 2
call :assert "int main(){int x=2;return x*=3,x;}" 6
call :assert "int main(){int x=5;return x|=3,x;}" 7
call :assert "int main(){int x=2;return x<<=3,x;}" 16
call :assert "int main(){int x=16;return x>>=3,x;}" 2
call :assert "int main(){int x=2;return x-=3,x;}" -1
REM call :assert "int main(){int x=5;return x^=3,x;}" 6
call :assert "int main(){int x=2;return ++x;}" 3
call :assert "int main(){int x=2;return --x;}" 1
call :assert "int main(){int x=2,y=3;return y+++--x;}" 4
call :assert "int main(){int x=2,y=3,z;return z=y+++--x,z+y;}" 8
popd

exit /b

:assert
set input=%1
set expected=%2
cc.exe %input% > tmp.asm || goto :error
nasm -f win64 tmp.asm -o tmp.obj || goto :error
link /nologo /entry:main /out:tmp.exe tmp.obj || goto :error
tmp.exe
set actual=%errorlevel%
if %actual% == %expected% (
	echo %input% ^=^> %actual%
) else (
	echo %input% got %actual%, expected %expected%
	nasm -g -F cv8 -f win64 tmp.asm -o tmp.obj || goto :error
	link /pdb:tmp.pdb /debug /nologo /entry:main /out:tmp.exe tmp.obj || goto :error
	goto :error
)
exit /b

:error
echo %input%
type tmp.asm
popd
(goto) 2>nul || exit /b %errorlevel%


REM nasm -g -F cv8 -f win64 tmp.asm -o tmp.obj 
REM link /pdb:tmp.pdb /debug /nologo /entry:main /out:tmp.exe tmp.obj 
