#include "common.h"

// Variável global para sinal de término
volatile sig_atomic_t running = 1;

// Handler de sinal para graceful shutdown
void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        running = 0;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <sensor_id> <sensor_type>\n", argv[0]);
        fprintf(stderr, "\nTipos de sensor válidos:\n");
        fprintf(stderr, "  0 = TEMPERATURA\n");
        fprintf(stderr, "  1 = UMIDADE\n");
        fprintf(stderr, "  2 = PRESSAO\n");
        fprintf(stderr, "\nExemplo: %s 1 0  (sensor ID 1, tipo TEMPERATURA)\n",
                argv[0]);
        fprintf(stderr, "\nNota: Este programa normalmente é executado pelo "
                        "sensor_manager.\n");
        fprintf(stderr, "      Para executar manualmente, certifique-se de que "
                        "data_processor está rodando.\n");
        exit(1);
    }

    int sensor_id = atoi(argv[1]);
    sensor_type_t sensor_type = (sensor_type_t) atoi(argv[2]);

    // Configurar handler de sinais
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    char component[64];
    snprintf(component, sizeof(component), "SENSOR-%d", sensor_id);
    log_message(COLOR_GREEN, component, "Processo iniciado");

    // Aguardar FIFO ser criado e ter um leitor
    int fifo_fd = -1;
    int retries = 20; // Aumentar tentativas
    while (retries-- > 0 &&
           (fifo_fd = open(FIFO_SENSOR_DATA, O_WRONLY | O_NONBLOCK)) == -1) {
        if (errno == ENOENT) {
            // FIFO não existe ainda, aguardar
            msleep(500);
        } else if (errno == ENXIO) {
            // FIFO existe mas não há leitor, aguardar
            msleep(500);
        } else {
            // Outro erro, tentar novamente
            msleep(500);
        }
    }

    // Se ainda não conseguiu abrir, tentar modo bloqueante uma última vez
    if (fifo_fd == -1) {
        log_message(COLOR_YELLOW, component,
                    "Aguardando leitor do FIFO (data_processor)...");
        fifo_fd = open(FIFO_SENSOR_DATA, O_WRONLY);
        if (fifo_fd == -1) {
            if (errno == ENXIO) {
                fprintf(stderr,
                        "Erro: FIFO existe mas não há leitor. Execute "
                        "'data_processor' primeiro ou use 'sensor_system' para "
                        "iniciar todo o sistema.\n");
            } else {
                perror("Erro ao abrir FIFO");
            }
            exit(1);
        }
    } else {
        // Se abriu em modo non-blocking, mudar para bloqueante
        int flags = fcntl(fifo_fd, F_GETFL);
        fcntl(fifo_fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    // Abrir fila de mensagens POSIX
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t) -1) {
        perror("Erro ao abrir fila de mensagens");
        close(fifo_fd);
        exit(1);
    }

    // Simular coleta de dados do sensor
    float base_value = 0.0;
    switch (sensor_type) {
    case SENSOR_TEMPERATURE:
        base_value = 25.0; // 25°C
        break;
    case SENSOR_HUMIDITY:
        base_value = 60.0; // 60%
        break;
    case SENSOR_PRESSURE:
        base_value = 1013.25; // hPa
        break;
    default:
        base_value = 0.0;
    }

    int count = 0;
    while (running) {
        // Simular variação de leitura
        float variation = ((float) rand() / RAND_MAX) * 5.0 - 2.5;
        float value = base_value + variation;

        sensor_data_t data = {.type = sensor_type,
                              .sensor_id = sensor_id,
                              .value = value,
                              .timestamp = time(NULL),
                              .active = 1};

        // Enviar via FIFO (pipe nomeado)
        if (write(fifo_fd, &data, sizeof(sensor_data_t)) == -1) {
            perror("Erro ao escrever no FIFO");
            break;
        }

        // Enviar via fila de mensagens POSIX (alternativa)
        char msg[MAX_MESSAGE_SIZE];
        snprintf(msg, sizeof(msg), "SENSOR-%d:%.2f", sensor_id, value);
        if (mq_send(mq, msg, strlen(msg) + 1, 0) == -1) {
            if (errno != EAGAIN) {
                perror("Erro ao enviar mensagem");
            }
        }

        count++;
        if (count % 10 == 0) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "%s: %.2f (leitura #%d)",
                     sensor_type_name(sensor_type), value, count);
            log_message(COLOR_GREEN, component, log_msg);
        }

        // Simular intervalo de leitura (1 segundo)
        sleep(1);
    }

    log_message(COLOR_YELLOW, component, "Encerrando processo...");

    close(fifo_fd);
    mq_close(mq);

    return 0;
}
