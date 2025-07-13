COMPILADORC = gcc
CFLAGS = -Wall -Wextra -O2 -g 
LDFLAGS = -lm -lrt
EXECUTABLE = fat16

# Diretórios
HEADER_DIR = include
SRC_DIR = source
OBJ_DIR = objects

SRCS = $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))


all: obj_dirs $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	@$(COMPILADORC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | obj_dirs
	@mkdir -p $(@D)
	@$(COMPILADORC) $(CFLAGS) -I$(HEADER_DIR) -c $< -o $@

run: $(EXECUTABLE)
	./$(EXECUTABLE)

obj_dirs:
	@mkdir -p $(OBJ_DIR)


clean:
	@rm -rf $(OBJ_DIR) $(EXECUTABLE)
	@rm -f fat.part

leak:
	@valgrind --leak-check=full --show-leak-kinds=all ./$(EXECUTABLE)

test:
	@if [ ! -x ./test.sh ]; then \
		echo "Adicionando permissão de execução ao test.sh..."; \
		chmod +x ./test_fat16.sh; \
	fi
	@./test_fat16.sh


.PHONY: all clean leak