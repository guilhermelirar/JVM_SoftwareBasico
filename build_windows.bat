@echo off
if not exist obj mkdir obj
if not exist bin mkdir bin

echo Compilando...
gcc -std=c11 -Wall -Wextra -g -Iinclude -c src/*.c
move *.o obj/
gcc obj/*.o -o bin/lexibid.exe

echo Concluido! O executavel esta em bin/lexibid.exe
pause
