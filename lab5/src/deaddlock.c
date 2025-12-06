#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Функция для потока 1
void* функция_потока1(void* arg) {
    printf("Поток 1: Пытаюсь захватить мьютекс1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Поток 1: Захватил мьютекс1\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Поток 1: Пытаюсь захватить мьютекс2...\n");
    pthread_mutex_lock(&mutex2); // Здесь произойдет дедлок!
    printf("Поток 1: Захватил мьютекс2\n");
    
    // Критическая секция
    printf("Поток 1: Вошел в критическую секцию\n");
    sleep(1);
    printf("Поток 1: Покинул критическую секцию\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для потока 2
void* функция_потока2(void* arg) {
    printf("Поток 2: Пытаюсь захватить мьютекс2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Поток 2: Захватил мьютекс2\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Поток 2: Пытаюсь захватить мьютекс1...\n");
    pthread_mutex_lock(&mutex1); // Здесь произойдет дедлок!
    printf("Поток 2: Захватил мьютекс1\n");
    
    // Критическая секция
    printf("Поток 2: Вошел в критическую секцию\n");
    sleep(1);
    printf("Поток 2: Покинул критическую секцию\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t поток1, поток2;
    
    printf("=== Демонстрация Взаимной Блокировки (Deadlock) ===\n");
    printf("Эта программа демонстрирует классический сценарий взаимной блокировки.\n");
    printf("Два потока пытаются захватить два мьютекса в разном порядке.\n\n");
    
    // Создаем потоки
    if (pthread_create(&поток1, NULL, функция_потока1, NULL) != 0) {
        perror("Ошибка создания потока1");
        return 1;
    }
    
    if (pthread_create(&поток2, NULL, функция_потока2, NULL) != 0) {
        perror("Ошибка создания потока2");
        return 1;
    }
    
    // Даем потокам время выполниться
    sleep(5);
    
    // Проверяем, живы ли потоки
    printf("\n=== Проверка состояния потоков ===\n");
    printf("Если программа зависла здесь - произошла взаимная блокировка!\n");
    printf("Основной поток: Ожидаю завершения потоков...\n");
    
    // Попытка присоединить потоки (скорее всего, заблокируется)
    pthread_join(поток1, NULL);
    pthread_join(поток2, NULL);
    
    printf("Это сообщение скорее всего никогда не будет напечатано из-за дедлока!\n");
    
    return 0;
}