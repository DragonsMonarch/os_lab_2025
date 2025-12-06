#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

// Общие структуры данных
struct Server{
    char ip[255];
    int port;
};

struct TaskData {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Общие функции
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);

// Функции для работы с сетью
int CreateAndConnectSocket(const char *ip, int port);
int CreateAndBindSocket(int port);

// Функции для работы с файлами
struct Server* ReadServersFromFile(const char *filename, int *count);
int ParseTaskFromBuffer(const char *buffer, struct TaskData *task);
void SerializeTaskToBuffer(const struct TaskData *task, char *buffer);

#endif