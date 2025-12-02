#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Função auxiliar para sleep em microsegundos
static inline void msleep(long milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Cores para output (opcional)
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"

// Constantes do sistema
#define MAX_SENSORS 5
#define BUFFER_SIZE 100
#define MAX_MESSAGE_SIZE 256
#define FIFO_SENSOR_DATA "/tmp/sensor_data_fifo"
#define FIFO_CONTROL "/tmp/control_fifo"
#define SHM_NAME "/sensor_system_shm"
#define MQ_NAME "/sensor_mq"

// Tipos de sensores
typedef enum {
    SENSOR_TEMPERATURE = 0,
    SENSOR_HUMIDITY,
    SENSOR_PRESSURE,
    SENSOR_TYPE_COUNT
} sensor_type_t;

// Estrutura de dados do sensor
typedef struct {
    sensor_type_t type;
    int sensor_id;
    float value;
    time_t timestamp;
    int active;
} sensor_data_t;

// Estrutura de controle
typedef struct {
    int command; // 0=stop, 1=start, 2=status, 3=shutdown
    int sensor_id;
    char message[MAX_MESSAGE_SIZE];
} control_message_t;

// Buffer circular para produtor-consumidor
typedef struct {
    sensor_data_t buffer[BUFFER_SIZE];
    int read_pos;
    int write_pos;
    int count;
    sem_t empty_slots;
    sem_t full_slots;
    pthread_mutex_t mutex;
} circular_buffer_t;

// Funções utilitárias
void log_message(const char *color, const char *component, const char *message);
const char *sensor_type_name(sensor_type_t type);
void cleanup_resources(void);

#endif // COMMON_H
