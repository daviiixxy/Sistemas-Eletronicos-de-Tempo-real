CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lpthread -lrt

# Diretórios
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = inc

# Arquivos fonte
COMMON_SRC = $(SRC_DIR)/common.c
SENSOR_PROCESS_SRC = $(SRC_DIR)/sensor_process.c
SENSOR_MANAGER_SRC = $(SRC_DIR)/sensor_manager.c
DATA_PROCESSOR_SRC = $(SRC_DIR)/data_processor.c
CONTROL_INTERFACE_SRC = $(SRC_DIR)/control_interface.c
MAIN_SRC = $(SRC_DIR)/main.c

# Executáveis
TARGETS = $(BIN_DIR)/sensor_process \
          $(BIN_DIR)/sensor_manager \
          $(BIN_DIR)/data_processor \
          $(BIN_DIR)/control_interface \
          $(BIN_DIR)/sensor_system

# Objetos
COMMON_OBJ = $(BUILD_DIR)/common.o

.PHONY: all clean clean-all directories

all: directories $(TARGETS)

directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) fifos

# Regra para common.o
$(COMMON_OBJ): $(COMMON_SRC) $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Executáveis
$(BIN_DIR)/sensor_process: $(SENSOR_PROCESS_SRC) $(COMMON_OBJ) $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< $(COMMON_OBJ) -o $@ $(LDFLAGS)

$(BIN_DIR)/sensor_manager: $(SENSOR_MANAGER_SRC) $(COMMON_OBJ) $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< $(COMMON_OBJ) -o $@ $(LDFLAGS)

$(BIN_DIR)/data_processor: $(DATA_PROCESSOR_SRC) $(COMMON_OBJ) $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< $(COMMON_OBJ) -o $@ $(LDFLAGS)

$(BIN_DIR)/control_interface: $(CONTROL_INTERFACE_SRC) $(COMMON_OBJ) $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< $(COMMON_OBJ) -o $@ $(LDFLAGS)

$(BIN_DIR)/sensor_system: $(MAIN_SRC) $(COMMON_OBJ) $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< $(COMMON_OBJ) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

clean-all: clean
	rm -rf fifos
	rm -f /tmp/sensor_data_fifo /tmp/control_fifo
	rm -f /dev/shm/sensor_system_shm
	rm -f /dev/mqueue/sensor_mq

# Ajuda
help:
	@echo "Makefile para Sistema de Monitoramento de Sensores"
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  all        - Compila todos os executáveis (padrão)"
	@echo "  clean      - Remove arquivos compilados"
	@echo "  clean-all  - Remove arquivos compilados e recursos IPC"
	@echo "  help       - Mostra esta mensagem"

