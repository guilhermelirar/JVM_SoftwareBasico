@echo off
if not exist obj\common mkdir obj\common
if not exist obj\display mkdir obj\display
if not exist bin mkdir bin

echo Compilando Common...
gcc -std=c11 -Wall -Wextra -g -Iinclude -c src/common/*.c
move *.o obj\common\

echo Compilando Display...
gcc -std=c11 -Wall -Wextra -g -Iinclude -c src/display/*.c
move *.o obj\display\

echo Linkando JCDUMP...
gcc obj\common\*.o obj\display\*.o -o bin\jcdump.exe

echo Concluido! O executavel esta em bin\jcdump.exe
pause
