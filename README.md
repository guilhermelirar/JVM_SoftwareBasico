# Trabalho de Software Básico
2026/01 - Software Básico 
Guilherme Lira Ribeiro - 231013558

Este trabalho contém códigos para a geração de dois executáveis:

- `jcdump`: leitor e exibidor de arquivos .class de Java 8
- `java-sb`: implementação parcial da máquina virtual Java para Java 8

---

## Leitor Exibidor de .class

### Compilação:

Compilado em C 99 (`-std=c99`), com inclusão do diretório `./include/` (`-Iinclude`).

#### Windows 

Executar script "`build_windows.bat`" na raiz do projeto, cujo conteúdo 
está listado abaixo:

```bat 
@echo off
if not exist obj mkdir obj
if not exist bin mkdir bin

echo Compilando...
gcc -std=c99 -Wall -Wextra -g -Iinclude -c src/*.c
move *.o obj/
gcc obj/*.o -o bin/jcdump.exe

echo Concluido! O executavel esta em bin/jcdump.exe
pause
```

#### Linux (requer Make)

- `make`: compila o código e gera o binário em `./bin/jcdump`
- `make clean`: remove o diretório de build e de arquivos objeto



### Uso

A partir do diretório raiz do projeto:

```sh
./bin/jcdump <arquivo.class> [arquivo de saída]
```

- `<arquivo.class>`: caminho para o .class a ser lido e exibido (deve conter `.class`).
- `[arquivo de saída]`: argumento opcional para o arquivo onde os dados do .class
 devem ser exibidos. Se omitido, os dados serão exibidos na saída padrão (`stdout`).

---

