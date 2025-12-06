CC = gcc
CFLAGS =
LDFLAGS = -pthread

# Цели по умолчанию
all: client server

# Общая библиотека
COMMON_SRC = common.c
COMMON_OBJ = common.o
COMMON_HDR = common.h

# Компиляция библиотеки
$(COMMON_OBJ): $(COMMON_SRC) $(COMMON_HDR)
	$(CC) $(CFLAGS) -c $(COMMON_SRC) -o $(COMMON_OBJ)

# Клиент
CLIENT_SRC = client.c
CLIENT_OBJ = client.o
CLIENT_EXE = client

$(CLIENT_OBJ): $(CLIENT_SRC) $(COMMON_HDR)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

$(CLIENT_EXE): $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CLIENT_OBJ) $(COMMON_OBJ) -o $(CLIENT_EXE) $(LDFLAGS)

# Сервер
SERVER_SRC = server.c
SERVER_OBJ = server.o
SERVER_EXE = server

$(SERVER_OBJ): $(SERVER_SRC) $(COMMON_HDR)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

$(SERVER_EXE): $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(SERVER_OBJ) $(COMMON_OBJ) -o $(SERVER_EXE) $(LDFLAGS)

# Очистка
clean:
	rm -f $(CLIENT_EXE) $(SERVER_EXE) $(CLIENT_OBJ) $(SERVER_OBJ) $(COMMON_OBJ)

# Пересборка
rebuild: clean all

# Установка
install: all
	cp $(CLIENT_EXE) $(SERVER_EXE) /usr/local/bin/

# Тесты
test: all
	@echo "Запуск тестов..."

.PHONY: all clean rebuild install test