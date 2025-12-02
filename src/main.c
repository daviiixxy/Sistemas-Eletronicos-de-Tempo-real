#include "common.h"

// PIDs dos processos principais
pid_t sensor_mgr_pid = -1;
pid_t data_proc_pid = -1;
pid_t control_pid = -1;

// Handler para sinais de término
void signal_handler(int sig __attribute__((unused)))
{
    log_message(COLOR_YELLOW, "MAIN",
                "Recebido sinal de término, encerrando sistema...");

    // Enviar SIGTERM para processos filhos
    if (sensor_mgr_pid > 0)
        kill(sensor_mgr_pid, SIGTERM);
    if (data_proc_pid > 0)
        kill(data_proc_pid, SIGTERM);
    if (control_pid > 0)
        kill(control_pid, SIGTERM);

    // Aguardar processos terminarem
    if (sensor_mgr_pid > 0)
        waitpid(sensor_mgr_pid, NULL, 0);
    if (data_proc_pid > 0)
        waitpid(data_proc_pid, NULL, 0);
    if (control_pid > 0)
        waitpid(control_pid, NULL, 0);

    // Cleanup de recursos
    cleanup_resources();

    exit(0);
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    // Registrar handler de sinais
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    log_message(COLOR_BLUE, "MAIN",
                "=== Sistema de Monitoramento de Sensores ===");
    log_message(COLOR_BLUE, "MAIN", "Iniciando componentes do sistema...");

    // Criar diretórios necessários
    mkdir("fifos", 0755);

    // Criar FIFOs
    mkfifo(FIFO_SENSOR_DATA, 0666);
    mkfifo(FIFO_CONTROL, 0666);

    // Criar fila de mensagens POSIX
    struct mq_attr attr = {
        .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = MAX_MESSAGE_SIZE};
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t) -1) {
        perror("Erro ao criar fila de mensagens");
    } else {
        mq_close(mq);
    }

    // Criar processo de processamento de dados
    data_proc_pid = fork();
    if (data_proc_pid == 0) {
        // Processo filho - executar data_processor
        char *args[] = {"./bin/data_processor", NULL};
        execv(args[0], args);
        perror("Erro ao executar data_processor");
        exit(1);
    } else if (data_proc_pid == -1) {
        perror("Erro ao criar processo data_processor");
        exit(1);
    }

    sleep(1); // Aguardar data_processor inicializar

    // Criar processo gerenciador de sensores
    sensor_mgr_pid = fork();
    if (sensor_mgr_pid == 0) {
        // Processo filho - executar sensor_manager
        char *args[] = {"./bin/sensor_manager", NULL};
        execv(args[0], args);
        perror("Erro ao executar sensor_manager");
        exit(1);
    } else if (sensor_mgr_pid == -1) {
        perror("Erro ao criar processo sensor_manager");
        kill(data_proc_pid, SIGTERM);
        exit(1);
    }

    sleep(1);

    // Criar processo de interface de controle
    control_pid = fork();
    if (control_pid == 0) {
        // Processo filho - executar control_interface
        char *args[] = {"./bin/control_interface", NULL};
        execv(args[0], args);
        perror("Erro ao executar control_interface");
        exit(1);
    } else if (control_pid == -1) {
        perror("Erro ao criar processo control_interface");
        kill(sensor_mgr_pid, SIGTERM);
        kill(data_proc_pid, SIGTERM);
        exit(1);
    }

    log_message(COLOR_GREEN, "MAIN", "Todos os processos criados com sucesso");
    log_message(COLOR_GREEN, "MAIN",
                "Sistema em execução. Pressione Ctrl+C para encerrar.");

    // Processo supervisor aguarda indefinidamente
    // Monitora processos filhos e aguarda sinais
    while (1) {
        sleep(1);

        // Verificar se processos ainda estão rodando
        if (waitpid(sensor_mgr_pid, NULL, WNOHANG) > 0) {
            log_message(COLOR_YELLOW, "MAIN", "sensor_manager terminou");
            sensor_mgr_pid = -1;
        }
        if (waitpid(data_proc_pid, NULL, WNOHANG) > 0) {
            log_message(COLOR_YELLOW, "MAIN", "data_processor terminou");
            data_proc_pid = -1;
        }
        if (waitpid(control_pid, NULL, WNOHANG) > 0) {
            log_message(COLOR_YELLOW, "MAIN", "control_interface terminou");
            control_pid = -1;
        }

        // Se todos os processos terminaram, sair
        if (sensor_mgr_pid == -1 && data_proc_pid == -1 && control_pid == -1) {
            log_message(COLOR_BLUE, "MAIN", "Todos os processos terminaram");
            break;
        }
    }

    cleanup_resources();
    log_message(COLOR_BLUE, "MAIN", "Sistema encerrado");

    return 0;
}
