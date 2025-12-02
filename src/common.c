#include "common.h"

// Função para log com cores
void log_message(const char *color, const char *component, const char *message)
{
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline

    printf("%s[%s] %s%s %s%s\n", color, time_str, COLOR_CYAN, component,
           COLOR_RESET, message);
    fflush(stdout);
}

// Retorna nome do tipo de sensor
const char *sensor_type_name(sensor_type_t type)
{
    switch (type) {
    case SENSOR_TEMPERATURE:
        return "TEMPERATURA";
    case SENSOR_HUMIDITY:
        return "UMIDADE";
    case SENSOR_PRESSURE:
        return "PRESSAO";
    default:
        return "DESCONHECIDO";
    }
}

// Cleanup de recursos (chamado no exit)
void cleanup_resources(void)
{
    // Remove FIFOs
    unlink(FIFO_SENSOR_DATA);
    unlink(FIFO_CONTROL);

    // Remove memória compartilhada
    shm_unlink(SHM_NAME);

    // Remove fila de mensagens
    mq_unlink(MQ_NAME);
}
