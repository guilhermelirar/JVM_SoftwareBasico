# JVM (Trabalho de Software Básico)
2026/01 - Software Básico 
Guilherme Lira Ribeiro - 231013558

Este trabalho contém códigos para a geração de dois executáveis:

- `jcdump`: leitor e exibidor de arquivos .class de Java 8
- `java-sb`: implementação parcial da máquina virtual Java para Java 8

---

## Compilação:

Compilado em C 99 (`-std=c99`), com inclusão do diretório `./include/` (`-Iinclude`).

#### Windows 

Executar script "`build_windows.bat`" na raiz do projeto, cujo conteúdo 
está listado abaixo:

```bat 
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
pause```

#### Linux (requer Make)

- `make`: compila o código e gera o binário em `./bin/jcdump`
- `make clean`: remove o diretório de build e de arquivos objeto


## Uso do Leitor-Exibidor

A partir do diretório raiz do projeto:

```sh
./bin/jcdump<.exe> <arquivo.class> [arquivo de saída]
```

- `<arquivo.class>`: caminho para o .class a ser lido e exibido (deve conter `.class`).
- `[arquivo de saída]`: argumento opcional para o arquivo onde os dados do .class
 devem ser exibidos. Se omitido, os dados serão exibidos na saída padrão (`stdout`).

## Uso do java-sb

A máquina virtual java java-sb pode ser executada como:
```sh
./bin/jav-sb<.exe> <path/to/Class>
```
Para o java-sb, "Class" é o nome do arquivo **sem a extensão ".class"** 
(o programa resolve o caminho internamnte)

---

