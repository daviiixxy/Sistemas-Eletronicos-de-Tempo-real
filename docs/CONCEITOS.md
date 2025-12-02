# Conceitos Demonstrados no Projeto

Este documento detalha como cada conceito de Sistemas Operacionais Embarcados é demonstrado no projeto.

## 1. Programação Concorrente

### Processos (fork, exec, wait, exit)

**Localização**: `sensor_manager.c`, `main.c`

**Demonstração**:
- **fork()**: O `sensor_manager` cria múltiplos processos filhos usando `fork()` para cada sensor
- **exec()**: Cada processo filho executa `sensor_process` usando `execv()`, substituindo a imagem do processo
- **wait()**: O processo supervisor (`main.c`) usa `waitpid()` para aguardar processos filhos terminarem
- **exit()**: Processos filhos terminam com `exit()` após completar suas tarefas

**Código de exemplo**:
```c
// sensor_manager.c - Criação de processo com fork/exec
pid_t pid = fork();
if (pid == 0) {
    // Processo filho
    execv("./bin/sensor_process", args);
    exit(1);
} else {
    // Processo pai
    child_pids[num_children++] = pid;
}
```

### Threads POSIX (pthread_create, join, mutexes)

**Localização**: `data_processor.c`, `control_interface.c`

**Demonstração**:
- **pthread_create()**: Criação de múltiplas threads produtoras e consumidoras
- **pthread_join()**: Aguardar threads terminarem antes de encerrar o programa
- **pthread_exit()**: Threads terminam explicitamente retornando da função

**Código de exemplo**:
```c
// data_processor.c - Criação de threads
pthread_t producer;
pthread_create(&producer, NULL, producer_thread, &fifo_fd);
pthread_join(producer, NULL);
```

## 2. Sincronização

### Semáforos POSIX (sem_init, sem_wait, sem_post)

**Localização**: `data_processor.c` - Buffer circular

**Demonstração**:
- **sem_init()**: Inicialização de semáforos para controlar slots vazios e cheios no buffer
- **sem_wait()**: Produtor aguarda slot vazio, consumidor aguarda slot cheio
- **sem_post()**: Sinaliza quando slot é preenchido ou esvaziado

**Código de exemplo**:
```c
// Inicialização
sem_init(&buf->empty_slots, 0, BUFFER_SIZE);
sem_init(&buf->full_slots, 0, 0);

// Produtor
sem_wait(&shared_buffer->empty_slots);  // Aguarda slot vazio
// ... escreve no buffer ...
sem_post(&shared_buffer->full_slots);   // Sinaliza slot cheio

// Consumidor
sem_wait(&shared_buffer->full_slots);   // Aguarda slot cheio
// ... lê do buffer ...
sem_post(&shared_buffer->empty_slots);  // Sinaliza slot vazio
```

### Mutexes (pthread_mutex)

**Localização**: `data_processor.c` - Proteção do buffer compartilhado

**Demonstração**:
- **pthread_mutex_init()**: Inicialização do mutex
- **pthread_mutex_lock()**: Entra na seção crítica antes de acessar buffer
- **pthread_mutex_unlock()**: Sai da seção crítica após modificar buffer

**Código de exemplo**:
```c
pthread_mutex_lock(&shared_buffer->mutex);
// Seção crítica - acesso ao buffer
shared_buffer->buffer[write_pos] = data;
shared_buffer->write_pos = (write_pos + 1) % BUFFER_SIZE;
pthread_mutex_unlock(&shared_buffer->mutex);
```

### Variáveis de Condição (pthread_cond)

**Localização**: `control_interface.c`

**Demonstração**:
- **pthread_cond_init()**: Inicialização da variável de condição
- **pthread_cond_wait()**: Thread aguarda sinal de dados disponíveis
- **pthread_cond_signal()**: Sinaliza thread esperando quando dados chegam

**Código de exemplo**:
```c
// Thread esperando
pthread_mutex_lock(&cond_mutex);
while (!new_data_available) {
    pthread_cond_wait(&data_ready, &cond_mutex);
}
pthread_mutex_unlock(&cond_mutex);

// Thread sinalizando
pthread_mutex_lock(&cond_mutex);
new_data_available = 1;
pthread_cond_signal(&data_ready);
pthread_mutex_unlock(&cond_mutex);
```

### Exclusão Mútua / Seção Crítica

**Localização**: Todo o código que acessa recursos compartilhados

**Demonstração**:
- Buffer circular protegido por mutex
- Contadores compartilhados protegidos
- Estruturas de dados críticas sincronizadas

## 3. Comunicação Entre Processos (IPC)

### Pipes

**Localização**: `main.c` (implícito via fork)

**Demonstração**:
- Processos pai-filho criados com `fork()` podem comunicar via pipes
- FIFOs (named pipes) são usados para comunicação entre processos não relacionados

### Named Pipes (FIFOs)

**Localização**: `sensor_process.c`, `data_processor.c`, `control_interface.c`

**Demonstração**:
- **mkfifo()**: Criação de FIFOs nomeados
- **open()**: Abertura de FIFO para leitura/escrita
- **read()/write()**: Comunicação unidirecional entre processos

**Código de exemplo**:
```c
// Criar FIFO
mkfifo(FIFO_SENSOR_DATA, 0666);

// Escritor
int fd = open(FIFO_SENSOR_DATA, O_WRONLY);
write(fd, &data, sizeof(data));

// Leitor
int fd = open(FIFO_SENSOR_DATA, O_RDONLY);
read(fd, &data, sizeof(data));
```

### Filas de Mensagens POSIX (mq_send, mq_receive)

**Localização**: `sensor_process.c`, `control_interface.c`

**Demonstração**:
- **mq_open()**: Criação/abertura de fila de mensagens
- **mq_send()**: Envio de mensagens assíncronas
- **mq_receive()**: Recebimento de mensagens
- **mq_close()**: Fechamento da fila

**Código de exemplo**:
```c
// Criar fila
mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0666, &attr);

// Enviar mensagem
mq_send(mq, message, strlen(message) + 1, 0);

// Receber mensagem
mq_receive(mq, buffer, MAX_SIZE, &priority);
```

### Memória Compartilhada POSIX (shm_open, mmap)

**Localização**: `data_processor.c`

**Demonstração**:
- **shm_open()**: Criação de objeto de memória compartilhada
- **mmap()**: Mapeamento da memória compartilhada no espaço de endereçamento
- **munmap()**: Desmapeamento da memória
- **shm_unlink()**: Remoção do objeto de memória compartilhada

**Código de exemplo**:
```c
// Criar memória compartilhada
int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, sizeof(circular_buffer_t));

// Mapear
circular_buffer_t* buffer = mmap(NULL, sizeof(circular_buffer_t),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

// Usar buffer compartilhado
buffer->write_pos = 0;

// Desmapear
munmap(buffer, sizeof(circular_buffer_t));
```

### Sinais (signal, kill)

**Localização**: `sensor_process.c`, `sensor_manager.c`, `main.c`

**Demonstração**:
- **signal()**: Registro de handlers para sinais (SIGTERM, SIGINT, SIGCHLD)
- **kill()**: Envio de sinais para processos (SIGTERM para encerramento graceful)
- **sigaction()**: Alternativa mais robusta (pode ser usado)

**Código de exemplo**:
```c
// Registrar handler
signal(SIGTERM, signal_handler);

// Enviar sinal
kill(child_pid, SIGTERM);

// Handler
void signal_handler(int sig) {
    if (sig == SIGTERM) {
        running = 0;  // Encerramento graceful
    }
}
```

## 4. Estruturas e Mecanismos Adicionais (Pontos Extras)

### Modelo Produtor-Consumidor

**Localização**: `data_processor.c`

**Demonstração**:
- Thread produtora lê dados do FIFO e coloca no buffer circular
- Múltiplas threads consumidoras processam dados do buffer
- Sincronização usando semáforos e mutexes
- Buffer circular com posições de leitura/escrita

**Características**:
- Produtor não bloqueia se buffer cheio (comportamento configurável)
- Consumidores processam dados em paralelo
- Sincronização adequada previne condições de corrida

### Algoritmos de Sincronização Clássicos

**Demonstração**:
- **Buffer Circular**: Implementação clássica com índices de leitura/escrita
- **Produtor-Consumidor**: Padrão clássico de sincronização
- **Exclusão Mútua**: Proteção de seções críticas

### Simulação de Condições de Corrida

**Observação**: O código atual previne condições de corrida através de sincronização adequada. Para demonstrar problemas de corrida, seria necessário executar versões sem proteção (não incluídas por segurança).

## Arquitetura de Comunicação

```
┌─────────────────┐
│  Sensor Process │──┐
│   (fork/exec)   │  │
└─────────────────┘  │
                     │ FIFO
┌─────────────────┐  │
│  Sensor Process │──┤
│   (fork/exec)   │  │
└─────────────────┘  │
                     ├──►┌──────────────────┐
┌─────────────────┐  │   │ Data Processor   │
│  Sensor Process │──┤   │ (Threads Produtor)│
│   (fork/exec)   │  │   └──────────────────┘
└─────────────────┘  │          │
                     │          │ Memória
┌─────────────────┐  │          │ Compartilhada
│  Sensor Process │──┘          │
│   (fork/exec)   │             │
└─────────────────┘             ▼
                     ┌──────────────────┐
                     │ Buffer Circular   │
                     │ (Semáforos/Mutex)│
                     └──────────────────┘
                              │
                              │
                     ┌────────▼─────────┐
                     │ Threads Consumidor│
                     │ (Processamento)   │
                     └───────────────────┘
                              │
                              │ Fila de Mensagens POSIX
                              ▼
                     ┌──────────────────┐
                     │ Control Interface │
                     │ (Variáveis Cond.) │
                     └──────────────────┘
```

## Resumo de Conceitos por Arquivo

| Arquivo | Conceitos Demonstrados |
|---------|----------------------|
| `main.c` | fork, wait, kill, sinais, coordenação de processos |
| `sensor_manager.c` | fork, exec, wait, SIGCHLD handler |
| `sensor_process.c` | FIFOs, filas de mensagens POSIX, sinais |
| `data_processor.c` | threads, semáforos, mutexes, memória compartilhada, produtor-consumidor |
| `control_interface.c` | threads, variáveis de condição, filas de mensagens POSIX, FIFOs |

## Pontos de Atenção

1. **Sincronização**: Todos os recursos compartilhados são protegidos adequadamente
2. **Cleanup**: Recursos IPC são limpos ao encerrar (FIFOs, memória compartilhada, filas)
3. **Graceful Shutdown**: Sinais são tratados para encerramento ordenado
4. **Logging**: Sistema de logs colorido facilita visualização do funcionamento

