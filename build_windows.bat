@echo off
:: Cria as pastas necessárias para organizar os objetos e os binários
if not exist obj\common mkdir obj\common
if not exist obj\display mkdir obj\display
if not exist obj\jvm mkdir obj\jvm
if not exist bin mkdir bin

echo =========================================
echo Compilando modulos comuns (Common)...
echo =========================================
gcc -std=c99 -Wall -Wextra -g -Iinclude -c src/common/*.c
move *.o obj\common\ >nul

echo =========================================
echo Compilando Leitor-Exibidor (Display)...
echo =========================================
gcc -std=c99 -Wall -Wextra -g -Iinclude -c src/display/*.c
move *.o obj\display\ >nul

echo =========================================
echo Compilando interpretador e engine (JVM)...
echo =========================================
gcc -std=c99 -Wall -Wextra -g -Iinclude -c src/jvm/*.c
move *.o obj\jvm\ >nul

echo =========================================
echo Linkando os binarios finais...
echo =========================================

echo Gerando bin\jcdump.exe...
gcc obj\common\*.o obj\display\*.o -o bin\jcdump.exe

echo Gerando bin\java-sb.exe...
:: Adicionado a flag -lm no final caso o GCC no Windows precise para as funções matematicas
gcc obj\common\*.o obj\jvm\*.o -lm -o bin\java-sb.exe

echo =========================================
echo Fim do processo! Executaveis gerados em:
echo  - bin\jcdump.exe
echo  - bin\java-sb.exe
echo =========================================
pause
