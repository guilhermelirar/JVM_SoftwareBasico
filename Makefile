CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g
INCLUDE = -Iinclude

OBJ_DIR = obj
BIN_DIR = bin

# --- SRC ---
# O que é comum a ambos
COMMON_SRC = $(wildcard src/common/*.c)

DISPLAY_SRC = $(wildcard src/display/*.c)
JVM_SRC     = $(wildcard src/jvm/*.c)

# --- OBJ ---
# patsubst garante que src/common/file.c vire obj/common/file.o
COMMON_OBJ  = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(COMMON_SRC))
DISPLAY_OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(DISPLAY_SRC))
JVM_OBJ     = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(JVM_SRC))

# --- BINÁRIOS ---
BIN_LEITOR = $(BIN_DIR)/jcdump
BIN_JVM    = $(BIN_DIR)/java-sb

# --- REGRAS PRINCIPAIS ---
all: $(BIN_LEITOR) $(BIN_JVM)

# Compila o Leitor-Exibidor (Common + Display)
$(BIN_LEITOR): $(COMMON_OBJ) $(DISPLAY_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@

# Compila a JVM (Common + JVM)
$(BIN_JVM): $(COMMON_OBJ) $(JVM_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@

# Regra genérica para compilar qualquer .c em .o mantendo a pasta
# O @mkdir garante que se src/common existe, obj/common será criada
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

# --- UTILITÁRIOS ---
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Exemplo de uso: make debug_leitor
debug_leitor: $(BIN_LEITOR)
	gdb --args ./$(BIN_LEITOR) referencia/Teste.class

.PHONY: all clean
