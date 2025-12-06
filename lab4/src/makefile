CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lpthread

# Все цели по умолчанию
all: client server

# Клиент
client: client.o
	$(CC) $(CFLAGS) -o client client.o $(LDFLAGS)

client.o: client.c
	$(CC) $(CFLAGS) -c -o client.o client.c

# Сервер
server: server.o
	$(CC) $(CFLAGS) -o server server.o $(LDFLAGS)

server.o: server.c
	$(CC) $(CFLAGS) -c -o server.o server.c

# Очистка
clean:
	rm -f *.o client server

# Пересборка
rebuild: clean all

# Запуск тестов
test_server: server
	./server --port 20001 --tnum 4

test_client: client
	./client --k 1000 --mod 12345 --servers servers.txt

# Файлы для зависимостей
.PHONY: all clean rebuild test_server test_client