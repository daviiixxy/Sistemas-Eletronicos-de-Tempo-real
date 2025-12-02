#include "common.h"

// Variável de condição para sincronização
pthread_cond_t data_ready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int new_data_available = 0;

// Thread para ler mensagens da fila POSIX
void *message_reader_thread(void *arg)
{
    mqd_t mq = *(mqd_t *) arg;
    char component[] = "MSG_READER";

    log_message(COLOR_CYAN, component, "Thread leitora de mensagens iniciada");

    char buffer[MAX_MESSAGE_SIZE];
    unsigned int priority;

    while (1) {
        ssize_t bytes_read =
            mq_receive(mq, buffer, MAX_MESSAGE_SIZE, &priority);

        if (bytes_read >= 0) {
            buffer[bytes_read] = '\0';

            // Sinalizar com variável de condição
            pthread_mutex_lock(&cond_mutex);
            new_data_available = 1;
            pthread_cond_signal(&data_ready);
            pthread_mutex_unlock(&cond_mutex);

            char msg[512];
            int len =
                snprintf(msg, sizeof(msg), "Mensagem recebida: %s", buffer);
            if (len >= (int) sizeof(msg) - 1) {
                // Mensagem truncada, adicionar indicador
                msg[sizeof(msg) - 4] = '.';
                msg[sizeof(msg) - 3] = '.';
                msg[sizeof(msg) - 2] = '.';
                msg[sizeof(msg) - 1] = '\0';
            }
            log_message(COLOR_CYAN, component, msg);
        } else if (errno != EAGAIN) {
            perror("Erro ao receber mensagem");
            break;
        }

        msleep(100); // 100ms
    }

    return NULL;
}

// Thread para processar dados quando disponíveis (usando variável de condição)
void *data_processor_thread(void *arg __attribute__((unused)))
{
    char component[] = "DATA_PROC_THREAD";

    log_message(COLOR_MAGENTA, component, "Thread processadora iniciada");

    while (1) {
        // Aguardar sinal de variável de condição
        pthread_mutex_lock(&cond_mutex);

        while (!new_data_available) {
            pthread_cond_wait(&data_ready, &cond_mutex);
        }

        new_data_available = 0;
        pthread_mutex_unlock(&cond_mutex);

        // Processar dados
        log_message(COLOR_MAGENTA, component,
                    "Processando novos dados disponíveis");
    }

    return NULL;
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    log_message(COLOR_BLUE, "CONTROL", "Iniciando interface de controle");

    // Criar/Abrir fila de mensagens POSIX
    struct mq_attr attr = {
        .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = MAX_MESSAGE_SIZE};

    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (mq == (mqd_t) -1) {
        perror("Erro ao criar/abrir fila de mensagens");
        exit(1);
    }

    // Criar/Abrir FIFO de controle
    mkfifo(FIFO_CONTROL, 0666);
    int control_fd = open(FIFO_CONTROL, O_RDWR);
    if (control_fd == -1) {
        perror("Erro ao abrir FIFO de controle");
        mq_close(mq);
        exit(1);
    }

    // Criar threads
    pthread_t reader_thread, processor_thread;

    if (pthread_create(&reader_thread, NULL, message_reader_thread, &mq) != 0) {
        perror("Erro ao criar thread leitora");
        exit(1);
    }

    if (pthread_create(&processor_thread, NULL, data_processor_thread, NULL) !=
        0) {
        perror("Erro ao criar thread processadora");
        exit(1);
    }

    // Ler comandos do FIFO de controle
    log_message(COLOR_BLUE, "CONTROL", "Aguardando comandos de controle...");

    control_message_t cmd;
    while (read(control_fd, &cmd, sizeof(control_message_t)) > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Comando recebido: %d (sensor=%d)",
                 cmd.command, cmd.sensor_id);
        log_message(COLOR_GREEN, "CONTROL", msg);

        if (cmd.command == 3) { // Shutdown
            break;
        }
    }

    // Cleanup
    pthread_cancel(reader_thread);
    pthread_cancel(processor_thread);
    pthread_join(reader_thread, NULL);
    pthread_join(processor_thread, NULL);

    close(control_fd);
    mq_close(mq);

    log_message(COLOR_BLUE, "CONTROL", "Interface de controle encerrada");

    return 0;
}
