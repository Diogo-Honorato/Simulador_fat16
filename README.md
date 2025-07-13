# Simulador de Sistema de Arquivos FAT16

Este projeto implementa um simulador de sistema de arquivos FAT16 simplificado conforme especificado no trabalho prático da disciplina de Sistemas Operacionais.

## Características do Sistema

- **Tamanho da partição**: 4MB (4096 clusters × 1024 bytes/cluster)
- **Tamanho do cluster**: 1024 bytes (2 setores de 512 bytes)
- **FAT**: 16 bits por entrada (8192 bytes total)
- **Diretório root**: 1 cluster (32 entradas máximo)
- **Arquivos suportados**: Máximo 18 caracteres no nome

## Compilação

### Pré-requisitos
- GCC (GNU Compiler Collection)
- Make
- Sistema operacional Linux

### Comandos de Compilação

```bash
# Compilar o projeto
make

# Executar o programa
make run

# Limpar arquivos compilados
make clean

# Executar teste básico
make test

# Verificar vazamentos de memória (requer valgrind)
make debug
```

## Uso do Simulador

### Executando o Programa

```bash
./fat16
```

### Comandos Disponíveis

| Comando | Descrição | Exemplo |
|---------|-----------|---------|
| `init` | Inicializa/formata o sistema de arquivos | `init` |
| `load` | Carrega um sistema de arquivos existente | `load` |
| `ls [caminho]` | Lista conteúdo do diretório | `ls /` ou `ls /meudir` |
| `mkdir <caminho>` | Cria um diretório | `mkdir /meudir` |
| `create <caminho>` | Cria um arquivo vazio | `create /arquivo.txt` |
| `write "dados" <caminho>` | Escreve dados no arquivo | `write "Olá mundo" /arquivo.txt` |
| `append "dados" <caminho>` | Anexa dados ao arquivo | `append " mais texto" /arquivo.txt` |
| `read <caminho>` | Lê o conteúdo do arquivo | `read /arquivo.txt` |
| `unlink <caminho>` | Remove arquivo ou diretório | `unlink /arquivo.txt` |
| `help` | Mostra ajuda | `help` |
| `exit` | Sai do programa | `exit` |

### Exemplo de Uso

```bash
fat16: init
Inicializando sistema de arquivos...
Sistema de arquivos inicializado com sucesso!

fat16: mkdir /documentos
Diretório criado: /documentos

fat16: create /documentos/teste.txt
Arquivo criado: /documentos/teste.txt

fat16: write "Este é um arquivo de teste" /documentos/teste.txt
Dados escritos no arquivo: /documentos/teste.txt

fat16: read /documentos/teste.txt
Conteúdo do arquivo /documentos/teste.txt (26 bytes):
----------------------------------------
Este é um arquivo de teste
----------------------------------------

fat16: ls /
Conteúdo do diretório /:
Nome               Tipo       Tamanho  Cluster
--------------------------------------------------
documentos         DIR        0        10

fat16: ls /documentos
Conteúdo do diretório /documentos:
Nome               Tipo       Tamanho  Cluster
--------------------------------------------------
teste.txt          FILE       26       11
```

## Detalhes Técnicos

### Layout da Partição

| Seção | Clusters | Tamanho | Descrição |
|-------|----------|---------|-----------|
| Boot Block | 0 | 1024 bytes | Inicializado com 0xbb |
| FAT | 1-8 | 8192 bytes | Tabela de alocação |
| Root Directory | 9 | 1024 bytes | Diretório raiz |
| Data Area | 10-4095 | ~4MB | Dados dos arquivos |

### Valores da FAT

- `0x0000`: Cluster livre
- `0x0001-0xFFFC`: Ponteiro para próximo cluster
- `0xFFFD`: Boot block
- `0xFFFE`: Cluster da FAT
- `0xFFFF`: Fim do arquivo

### Limitações

1. **Tamanho máximo do nome**: 18 caracteres
2. **Entradas por diretório**: 32 máximo
3. **Tamanho da partição**: 4MB fixo
4. **Não suporta subdiretorios aninhados profundos**

## Arquivo de Partição

O sistema cria automaticamente um arquivo chamado `fat.part` que representa a partição virtual. Este arquivo:
- Tem exatamente 4MB de tamanho
- Contém todas as estruturas do sistema de arquivos
- Pode ser carregado em execuções posteriores com o comando `load`

## Tratamento de Erros

O simulador trata diversos tipos de erro:
- Arquivo/diretório não encontrado
- Diretório não vazio (ao tentar remover)
- Espaço insuficiente no disco
- Erro de leitura/escrita
- Diretório cheio (máximo 32 entradas)

## Observações Importantes

1. **Inicialização**: Sempre execute `init` antes de usar o sistema pela primeira vez
2. **Persistência**: Os dados são salvos automaticamente no arquivo `fat.part`
3. **Consistência**: O sistema mantém consistência entre FAT e entradas de diretório
4. **Memória**: Apenas a FAT e um cluster são mantidos em memória por vez

## Desenvolvimento

### Estrutura de Dados Principal

```c
typedef struct {
    FILE *partition_file;           // Arquivo da partição
    uint16_t fat[TOTAL_CLUSTERS];   // Tabela FAT em memória
    data_cluster_t current_cluster; // Cluster atual
    char current_path[256];         // Caminho atual
} fat16_fs_t;
```

### Entrada de Diretório

```c
typedef struct {
    uint8_t filename[18];    // Nome do arquivo
    uint8_t attributes;      // Atributos (0=arquivo, 1=diretório)
    uint8_t reserved[7];     // Reservado
    uint16_t first_block;    // Primeiro cluster
    uint32_t size;           // Tamanho em bytes
} dir_entry_t;
```

## Testes

Para testar o sistema:

```bash
# Teste básico
make test

# Teste manual completo
./fat16
init
mkdir /teste
create /teste/arquivo.txt
write "teste de conteudo" /teste/arquivo.txt
read /teste/arquivo.txt
ls /teste
unlink /teste/arquivo.txt
unlink /teste
exit
```

## Troubleshooting

### Partição Corrompida
- Execute `init` para recriar o sistema de arquivos
- Ou delete o arquivo `fat.part` e execute `init`