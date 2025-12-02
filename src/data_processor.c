#include "common.h"

// Buffer compartilhado (produtor-consumidor)
circular_buffer_t *shared_buffer = NULL;
volatile int processor_running = 1;

// Inicializar buffer circular
void init_buffer(circular_buffer_t *buf)
{
    buf->read_pos = 0;
    buf->write_pos = 0;
    buf->count = 0;
    sem_init(&buf->empty_slots, 0, BUFFER_SIZE);
    sem_init(&buf->full_slots, 0, 0);
    pthread_mutex_init(&buf->mutex, NULL);
}

// Produtor: lê dados do FIFO e coloca no buffer
void *producer_thread(void *arg)
{
    int fifo_fd = *(int *) arg;
    char component[] = "PRODUTOR";

    log_message(COLOR_CYAN, component, "Thread produtora iniciada");

    while (processor_running) {
        sensor_data_t data;
        ssize_t bytes_read = read(fifo_fd, &data, sizeof(sensor_data_t));

        if (bytes_read == sizeof(sensor_data_t)) {
            // Aguardar slot vazio (semáforo)
            sem_wait(&shared_buffer->empty_slots);

            // Entrar na seção crítica (mutex)
            pthread_mutex_lock(&shared_buffer->mutex);

            // Escrever no buffer circular
            shared_buffer->buffer[shared_buffer->write_pos] = data;
            shared_buffer->write_pos =
                (shared_buffer->write_pos + 1) % BUFFER_SIZE;
            shared_buffer->count++;

            // Sair da seção crítica
            pthread_mutex_unlock(&shared_buffer->mutex);

            // Sinalizar slot cheio
            sem_post(&shared_buffer->full_slots);

            char msg[128];
            snprintf(msg, sizeof(msg), "Dados recebidos: Sensor-%d %s=%.2f",
                     data.sensor_id, sensor_type_name(data.type), data.value);
            log_message(COLOR_CYAN, component, msg);
        } else if (bytes_read == 0) {
            // FIFO fechado
            break;
        } else if (bytes_read == -1 && errno != EAGAIN) {
            perror("Erro ao ler FIFO");
            break;
        }
    }

    log_message(COLOR_YELLOW, component, "Thread produtora encerrada");
    return NULL;
}

// Consumidor: processa dados do buffer
void *consumer_thread(void *arg)
{
    int thread_id = *(int *) arg;
    char component[32];
    snprintf(component, sizeof(component), "CONSUMIDOR-%d", thread_id);

    log_message(COLOR_MAGENTA, component, "Thread consumidora iniciada");

    int processed = 0;
    while (processor_running) {
        // Aguardar slot cheio (semáforo)
        sem_wait(&shared_buffer->full_slots);

        // Entrar na seção crítica (mutex)
        pthread_mutex_lock(&shared_buffer->mutex);

        // Ler do buffer circular
        sensor_data_t data = shared_buffer->buffer[shared_buffer->read_pos];
        shared_buffer->read_pos = (shared_buffer->read_pos + 1) % BUFFER_SIZE;
        shared_buffer->count--;

        // Sair da seção crítica
        pthread_mutex_unlock(&shared_buffer->mutex);

        // Sinalizar slot vazio
        sem_post(&shared_buffer->empty_slots);

        // Processar dados (simular análise)
        float processed_value = data.value * 1.1; // Exemplo de processamento

        processed++;
        if (processed % 5 == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Processado: Sensor-%d %s=%.2f -> %.2f (total=%d)",
                     data.sensor_id, sensor_type_name(data.type), data.value,
                     processed_value, processed);
            log_message(COLOR_MAGENTA, component, msg);
        }

        // Simular tempo de processamento
        msleep(100); // 100ms
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Thread consumidora encerrada (processados=%d)",
             processed);
    log_message(COLOR_YELLOW, component, msg);

    return NULL;
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    log_message(COLOR_BLUE, "DATA_PROC", "Iniciando processador de dados");

    // Criar/Abrir FIFO para leitura (se não existir)
    if (mkfifo(FIFO_SENSOR_DATA, 0666) == -1 && errno != EEXIST) {
        perror("Erro ao criar FIFO");
        exit(1);
    }

    // Abrir FIFO (bloqueante para garantir que há escritor)
    int fifo_fd = open(FIFO_SENSOR_DATA, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Erro ao abrir FIFO");
        exit(1);
    }

    // Criar memória compartilhada para o buffer
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Erro ao criar memória compartilhada");
        close(fifo_fd);
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(circular_buffer_t)) == -1) {
        perror("Erro ao definir tamanho da memória compartilhada");
        close(shm_fd);
        close(fifo_fd);
        exit(1);
    }

    shared_buffer = (circular_buffer_t *) mmap(NULL, sizeof(circular_buffer_t),
                                               PROT_READ | PROT_WRITE,
                                               MAP_SHARED, shm_fd, 0);
    if (shared_buffer == MAP_FAILED) {
        perror("Erro ao mapear memória compartilhada");
        close(shm_fd);
        close(fifo_fd);
        exit(1);
    }

    // Inicializar buffer
    init_buffer(shared_buffer);

    // Criar threads produtoras e consumidoras
    pthread_t producer;
    pthread_t consumers[3];
    int consumer_ids[3] = {1, 2, 3};

    // Thread produtora
    if (pthread_create(&producer, NULL, producer_thread, &fifo_fd) != 0) {
        perror("Erro ao criar thread produtora");
        exit(1);
    }

    // Threads consumidoras (modelo produtor-consumidor)
    for (int i = 0; i < 3; i++) {
        if (pthread_create(&consumers[i], NULL, consumer_thread,
                           &consumer_ids[i]) != 0) {
            perror("Erro ao criar thread consumidora");
            exit(1);
        }
    }

    log_message(COLOR_BLUE, "DATA_PROC", "Todas as threads criadas");

    // Aguardar threads (em produção, aguardaria indefinidamente)
    sleep(30);

    processor_running = 0;

    // Aguardar threads terminarem
    pthread_join(producer, NULL);
    for (int i = 0; i < 3; i++) {
        pthread_join(consumers[i], NULL);
    }

    // Cleanup
    munmap(shared_buffer, sizeof(circular_buffer_t));
    close(shm_fd);
    close(fifo_fd);

    log_message(COLOR_BLUE, "DATA_PROC", "Processador encerrado");

    return 0;
}
