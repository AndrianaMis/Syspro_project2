CC := gcc
CFLAGS = -Wall -g -pthread -Iinclude
LDFLAGS = -pthread

BUILD_DIR := build
BIN_DIR := bin

SERVER_SRC=src/jobExecutorServer.c src/queue.c 
COMMANDER_SRC=src/jobCommander.c

SERVER_OBJ := $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SERVER_SRC))
COMMANDER_OBJ := $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(COMMANDER_SRC))

SERVER_EXEC := $(BIN_DIR)/jobExecutorServer
COMMANDER_EXEC := $(BIN_DIR)/jobCommander

.PHONY: clean

all: $(SERVER_EXEC) $(COMMANDER_EXEC)

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $@ $(LDFLAGS)

$(COMMANDER_EXEC): $(COMMANDER_OBJ)
	$(CC) $(COMMANDER_OBJ) -o $@ $(LDFLAGS)


$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(BUILD_DIR)/*.o $(SERVER_EXEC) $(COMMANDER_EXEC)

jobCommander: $(COMMANDER_EXEC)
	./$(COMMANDER_EXEC)

jobExecutorServer: $(SERVER_EXEC)
	./$(SERVER_EXEC)

