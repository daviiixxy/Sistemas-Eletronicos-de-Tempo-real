# Guia Rápido de Início

## Compilação e Execução Rápida

```bash
# 1. Navegar para o diretório do projeto
cd ProjetoFinal

# 2. Compilar o projeto
make clean && make all

# 3. Executar o sistema completo
./bin/sensor_system
```

## O que Esperar

Ao executar, você verá:

1. **Logs coloridos** de cada componente do sistema
2. **Processos de sensores** coletando dados (temperatura, umidade, pressão)
3. **Threads produtoras** recebendo dados via FIFO
4. **Threads consumidoras** processando dados em paralelo
5. **Comunicação IPC** funcionando entre componentes

## Encerrar o Sistema

Pressione `Ctrl+C` para encerrar gracefulmente. O sistema:
- Enviará SIGTERM para todos os processos filhos
- Aguardará processos terminarem
- Limpará recursos IPC (FIFOs, memória compartilhada, filas)

## Estrutura de Arquivos

```
ProjetoFinal/
├── README.md              # Documentação completa
├── CONCEITOS.md          # Explicação detalhada dos conceitos
├── APRESENTACAO.md       # Slides para apresentação
├── QUICKSTART.md         # Este arquivo
├── Makefile              # Build do projeto
├── common.h/.c           # Código comum e utilitários
├── main.c                # Processo supervisor
├── sensor_manager.c      # Gerenciador de sensores
├── sensor_process.c      # Processo individual de sensor
├── data_processor.c     # Processador de dados (threads)
└── control_interface.c  # Interface de controle
```

## Executar Componentes Individuais

Para debug ou teste individual:

```bash
# Apenas gerenciador de sensores
./bin/sensor_manager

# Apenas processador de dados
./bin/data_processor

# Apenas interface de controle
./bin/control_interface
```

## Limpeza

```bash
# Limpar arquivos compilados
make clean

# Limpar tudo (incluindo recursos IPC)
make clean-all
```

## Requisitos

- Linux (Ubuntu/Debian recomendado)
- GCC
- Bibliotecas: pthread, rt (geralmente incluídas)

**Importante**: Não é necessário executar com `sudo` - o projeto funciona com permissões de usuário normal.

## Troubleshooting

### Erro: "FIFO não encontrado"
- Execute `make clean-all` e tente novamente
- Certifique-se de executar `sensor_system` primeiro (ele cria os FIFOs)

### Erro: "Permissão negada"
- Verifique permissões de execução: `chmod +x bin/*`
- Alguns recursos IPC podem precisar de permissões adequadas

### Erro: "Fila de mensagens não encontrada"
- Execute `make clean-all` para limpar recursos antigos
- Certifique-se de executar como o mesmo usuário

## Próximos Passos

1. Leia `README.md` para documentação completa
2. Leia `CONCEITOS.md` para entender os conceitos implementados
3. Revise `APRESENTACAO.md` para preparar a apresentação
4. Explore o código fonte para entender a implementação

