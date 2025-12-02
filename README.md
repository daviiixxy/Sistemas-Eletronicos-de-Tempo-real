# Sistema de Monitoramento de Sensores Distribuído

## Descrição do Projeto

Este projeto implementa um sistema distribuído de monitoramento de sensores que demonstra os principais conceitos de programação concorrente, sincronização e comunicação entre processos estudados na disciplina de Sistemas Operacionais Embarcados.

O sistema simula um ambiente embarcado com múltiplos sensores (temperatura, umidade, pressão) que coletam dados continuamente, um processador central que analisa esses dados, e uma interface de controle que permite monitoramento e configuração em tempo real.

## Arquitetura do Sistema

O sistema é composto por 4 processos principais:

1. **Sensor Manager** (`sensor_manager.c`): Gerencia múltiplos processos de sensores usando `fork()` e `exec()`
2. **Data Processor** (`data_processor.c`): Processa dados dos sensores usando threads POSIX e modelo produtor-consumidor
3. **Control Interface** (`control_interface.c`): Interface de controle usando comunicação IPC
4. **Main Supervisor** (`main.c`): Processo supervisor que coordena todos os componentes

## Conceitos Demonstrados

### 1. Programação Concorrente
- ✅ **Processos**: `fork()`, `exec()`, `wait()`, `exit()`
- ✅ **Threads POSIX**: `pthread_create()`, `pthread_join()`, `pthread_exit()`

### 2. Sincronização
- ✅ **Semáforos POSIX**: `sem_init()`, `sem_wait()`, `sem_post()`
- ✅ **Mutexes**: `pthread_mutex_init()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()`
- ✅ **Variáveis de Condição**: `pthread_cond_init()`, `pthread_cond_wait()`, `pthread_cond_signal()`
- ✅ **Exclusão Mútua**: Seções críticas protegidas

### 3. Comunicação Entre Processos (IPC)
- ✅ **Pipes**: Comunicação unidirecional entre processos
- ✅ **Named Pipes (FIFOs)**: Comunicação persistente entre processos
- ✅ **Filas de Mensagens POSIX**: `mq_open()`, `mq_send()`, `mq_receive()`
- ✅ **Memória Compartilhada POSIX**: `shm_open()`, `mmap()`, `munmap()`
- ✅ **Sinais**: `signal()`, `kill()`, `sigaction()`

### 4. Estruturas Adicionais (Pontos Extras)
- ✅ **Modelo Produtor-Consumidor**: Threads produtoras e consumidoras com buffer compartilhado
- ✅ **Algoritmos de Sincronização**: Implementação de buffer circular com sincronização
- ✅ **Simulação de Condições de Corrida**: Demonstração de problemas sem sincronização

## Estrutura do Projeto

```
ProjetoFinal/
├── README.md                 # Este arquivo
├── Makefile                  # Build do projeto
├── inc/                      # Headers
│   └── common.h             # Definições comuns e utilitários
├── src/                      # Código fonte
│   ├── main.c               # Processo supervisor principal
│   ├── sensor_manager.c     # Gerenciador de processos de sensores
│   ├── sensor_process.c     # Processo individual de sensor
│   ├── data_processor.c     # Processador de dados (threads)
│   ├── control_interface.c  # Interface de controle
│   └── common.c             # Implementação de utilitários
├── build/                    # Diretório de build (gerado)
├── bin/                      # Executáveis (gerado)
└── fifos/                    # Named pipes (gerado)
```

## Requisitos

- Sistema Linux (Ubuntu/Debian recomendado)
- GCC (GNU Compiler Collection)
- pthread (biblioteca POSIX threads)
- rt (biblioteca POSIX real-time, geralmente incluída)

**Nota**: Este projeto **NÃO requer privilégios de root (sudo)** para executar, pois não usa escalonamento de tempo real. Todos os recursos IPC utilizados funcionam com permissões de usuário normal.

## Compilação

```bash
cd ProjetoFinal
make clean
make
```

## Execução

```bash
# Executar o sistema completo (processos podem rodar em qualquer core)
./bin/sensor_system

# Opcional: Forçar todos os processos no mesmo core (apenas para testes)
# taskset -c 0 ./bin/sensor_system
```

**Nota**: Não é necessário usar `taskset` - o sistema funciona perfeitamente com processos distribuídos entre múltiplos cores. Veja `NOTAS_TECNICAS.md` para mais detalhes.

## Funcionalidades

1. **Coleta de Dados**: Múltiplos processos de sensores coletam dados simulados
2. **Processamento Concorrente**: Threads processam dados usando modelo produtor-consumidor
3. **Comunicação IPC**: Diferentes mecanismos IPC conectam os componentes
4. **Monitoramento em Tempo Real**: Interface permite visualizar dados e controlar o sistema
5. **Sincronização**: Todos os recursos compartilhados são protegidos adequadamente

## Demonstração de Conceitos

### Processos e Fork
- O `sensor_manager` cria processos filhos para cada sensor usando `fork()` e `exec()`
- Cada processo filho executa independentemente e comunica via IPC

### Threads e Concorrência
- O `data_processor` cria múltiplas threads para processamento paralelo
- Threads produtoras coletam dados, threads consumidoras processam

### Sincronização
- Semáforos controlam acesso ao buffer compartilhado
- Mutexes protegem estruturas de dados críticas
- Variáveis de condição sincronizam threads produtoras/consumidoras

### IPC
- Pipes conectam processos pai-filho
- FIFOs permitem comunicação bidirecional
- Filas de mensagens POSIX para comunicação assíncrona
- Memória compartilhada para dados de alta frequência

## Limpeza

```bash
make clean          # Remove arquivos compilados
make clean-all      # Remove também FIFOs e memória compartilhada
```

## Notas de Desenvolvimento

- O sistema foi projetado para demonstrar os conceitos de forma didática
- Logs detalhados mostram o funcionamento interno do sistema
- O código inclui comentários explicativos sobre cada mecanismo utilizado

## Autores

[Seu Nome] - Sistemas Operacionais Embarcados - UEA

