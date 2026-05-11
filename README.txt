Leitor Exibidor de .class

2026/01 - Software Básico 
Guilherme Lira Ribeiro - 231013558

+---------------------------------------------------------+

Compilação (Windows):

Executar script "build_windows.bat"
Conteúdo:
  
  @echo off
  if not exist obj mkdir obj
  if not exist bin mkdir bin

  echo Compilando...
  gcc -std=c11 -Wall -Wextra -g -Iinclude -c src/*.c
  move *.o obj/
  gcc obj/*.o -o bin/lexibid.exe

  echo Concluido! O executavel esta em bin/lexibid.exe
  pause

+--------------------------------------------------------+

Executar: bin/lexibid <arquivo.class> [arquivo de saída]
  arquivo.class (obrigatório): caminho para arquivo.class
  arquivo de saída (opcional): onde saída será gravada 
    (default: saída padrão)




