#include "common.h"

#define MAX_CHILDREN MAX_SENSORS
pid_t child_pids[MAX_CHILDREN];
int num_children = 0;

// Handler para sinais de filhos terminados
void sigchld_handler(int sig __attribute__((unused)))
{
    int status;
    pid_t pid;

    // Coletar todos os processos filhos terminados (wait sem bloquear)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < num_children; i++) {
            if (child_pids[i] == pid) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "Processo filho %d terminou (status=%d)", pid,
                         WEXITSTATUS(status));
                log_message(COLOR_YELLOW, "SENSOR_MGR", msg);
                child_pids[i] = -1;
                break;
            }
        }
    }
}

// Criar processo de sensor usando fork e exec
pid_t create_sensor_process(int sensor_id, sensor_type_t sensor_type)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("Erro no fork");
        return -1;
    }

    if (pid == 0) {
        // Processo filho - executar sensor_process
        char id_str[16], type_str[16];
        snprintf(id_str, sizeof(id_str), "%d", sensor_id);
        snprintf(type_str, sizeof(type_str), "%d", (int) sensor_type);

        char *args[] = {"./bin/sensor_process", id_str, type_str, NULL};

        // Usar exec para substituir imagem do processo
        if (execv(args[0], args) == -1) {
            perror("Erro no execv");
            exit(1);
        }
    } else {
        // Processo pai
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Criado processo sensor %d (PID=%d, tipo=%s)", sensor_id, pid,
                 sensor_type_name(sensor_type));
        log_message(COLOR_BLUE, "SENSOR_MGR", msg);

        if (num_children < MAX_CHILDREN) {
            child_pids[num_children++] = pid;
        }
    }

    return pid;
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    log_message(COLOR_BLUE, "SENSOR_MGR", "Iniciando gerenciador de sensores");

    // Criar diretórios necessários
    mkdir("fifos", 0755);

    // Criar FIFOs (se não existirem)
    if (mkfifo(FIFO_SENSOR_DATA, 0666) == -1 && errno != EEXIST) {
        perror("Erro ao criar FIFO de dados");
        exit(1);
    }
    if (mkfifo(FIFO_CONTROL, 0666) == -1 && errno != EEXIST) {
        perror("Erro ao criar FIFO de controle");
        exit(1);
    }

    // Criar fila de mensagens POSIX (se não existir)
    struct mq_attr attr = {
        .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = MAX_MESSAGE_SIZE};
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t) -1) {
        perror("Erro ao criar fila de mensagens");
        exit(1);
    } else {
        mq_close(mq);
    }

    // Configurar handler para SIGCHLD (filhos terminados)
    signal(SIGCHLD, sigchld_handler);

    // Criar processos de sensores usando fork/exec
    create_sensor_process(1, SENSOR_TEMPERATURE);
    msleep(100); // Pequeno delay entre criações

    create_sensor_process(2, SENSOR_TEMPERATURE);
    msleep(100); // Pequeno delay entre criações

    create_sensor_process(3, SENSOR_HUMIDITY);
    msleep(100); // Pequeno delay entre criações

    create_sensor_process(4, SENSOR_PRESSURE);

    log_message(COLOR_BLUE, "SENSOR_MGR", "Todos os sensores criados");

    // Aguardar um tempo antes de encerrar (em produção, aguardaria
    // indefinidamente)
    sleep(30);

    // Enviar SIGTERM para todos os filhos
    log_message(COLOR_YELLOW, "SENSOR_MGR", "Encerrando processos filhos...");
    for (int i = 0; i < num_children; i++) {
        if (child_pids[i] != -1) {
            kill(child_pids[i], SIGTERM);
        }
    }

    // Aguardar todos os filhos terminarem
    for (int i = 0; i < num_children; i++) {
        if (child_pids[i] != -1) {
            waitpid(child_pids[i], NULL, 0);
        }
    }

    log_message(COLOR_BLUE, "SENSOR_MGR", "Gerenciador encerrado");

    return 0;
}
