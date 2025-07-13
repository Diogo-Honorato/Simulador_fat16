#include "../include/shell.h"

// Função para extrair string entre aspas
char* extract_quoted_string(const char* input) {
    char* start = strchr(input, '"');
    if (!start) return NULL;
    
    start++; // Pula a primeira aspa
    char* end = strchr(start, '"');
    if (!end) return NULL;
    
    size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, start, len);
    result[len] = '\0';
    
    return result;
}

// Função para processar comandos
void process_command(fat16_fs_t* fs, const char* command) {
    char cmd[MAX_COMMAND_LENGTH];
    strncpy(cmd, command, MAX_COMMAND_LENGTH - 1);
    cmd[MAX_COMMAND_LENGTH - 1] = '\0';
    
    // Remove newline se existir
    char* newline = strchr(cmd, '\n');
    if (newline) *newline = '\0';
    
    // Tokeniza o comando
    char* token = strtok(cmd, " ");
    if (!token) return;
    
    if (strcmp(token, "init") == 0) {
        printf("Inicializando sistema de arquivos...\n");
        if (fat16_init(fs, PARTITION_FILE) == 0) {
            printf("Sistema de arquivos inicializado com sucesso!\n");
        } else {
            printf("Erro ao inicializar sistema de arquivos!\n");
        }
        
    } else if (strcmp(token, "load") == 0) {
        printf("Carregando sistema de arquivos...\n");
        if (fat16_load(fs, PARTITION_FILE) == 0) {
            printf("Sistema de arquivos carregado com sucesso!\n");
        } else {
            printf("Erro ao carregar sistema de arquivos!\n");
        }
        
    } else if (strcmp(token, "ls") == 0) {
        token = strtok(NULL, "");
        if (token) {
            // Remove espaços em branco do início
            while (*token == ' ') token++;
        }
        fat16_ls(fs, token);
        
    } else if (strcmp(token, "mkdir") == 0) {
        token = strtok(NULL, "");
        if (token) {
            while (*token == ' ') token++;
            fat16_mkdir(fs, token);
        } else {
            printf("Uso: mkdir <caminho>\n");
        }
        
    } else if (strcmp(token, "create") == 0) {
        token = strtok(NULL, "");
        if (token) {
            while (*token == ' ') token++;
            fat16_create(fs, token);
        } else {
            printf("Uso: create <caminho>\n");
        }
        
    } else if (strcmp(token, "unlink") == 0) {
        token = strtok(NULL, "");
        if (token) {
            while (*token == ' ') token++;
            fat16_unlink(fs, token);
        } else {
            printf("Uso: unlink <caminho>\n");
        }
        
    } else if (strcmp(token, "write") == 0) {
        // Reconstrói o comando para processar aspas
        char* rest_of_command = strtok(NULL, "");
        if (rest_of_command) {
            while (*rest_of_command == ' ') rest_of_command++;
            
            char* data = extract_quoted_string(rest_of_command);
            if (!data) {
                printf("Uso: write \"dados\" <caminho>\n");
                return;
            }
            
            // Encontra o caminho após as aspas
            char* path_start = strchr(rest_of_command, '"');
            if (path_start) {
                path_start = strchr(path_start + 1, '"');
                if (path_start) {
                    path_start++;
                    while (*path_start == ' ') path_start++;
                    
                    if (*path_start) {
                        fat16_write(fs, data, path_start);
                    } else {
                        printf("Uso: write \"dados\" <caminho>\n");
                    }
                }
            }
            
            free(data);
        } else {
            printf("Uso: write \"dados\" <caminho>\n");
        }
        
    } else if (strcmp(token, "append") == 0) {
        // Reconstrói o comando para processar aspas
        char* rest_of_command = strtok(NULL, "");
        if (rest_of_command) {
            while (*rest_of_command == ' ') rest_of_command++;
            
            char* data = extract_quoted_string(rest_of_command);
            if (!data) {
                printf("Uso: append \"dados\" <caminho>\n");
                return;
            }
            
            // Encontra o caminho após as aspas
            char* path_start = strchr(rest_of_command, '"');
            if (path_start) {
                path_start = strchr(path_start + 1, '"');
                if (path_start) {
                    path_start++;
                    while (*path_start == ' ') path_start++;
                    
                    if (*path_start) {
                        fat16_append(fs, data, path_start);
                    } else {
                        printf("Uso: append \"dados\" <caminho>\n");
                    }
                }
            }
            
            free(data);
        } else {
            printf("Uso: append \"dados\" <caminho>\n");
        }
        
    } else if (strcmp(token, "read") == 0) {
        token = strtok(NULL, "");
        if (token) {
            while (*token == ' ') token++;
            fat16_read(fs, token);
        } else {
            printf("Uso: read <caminho>\n");
        }
        
    } else if (strcmp(token, "help") == 0) {
        printf("Comandos disponíveis:\n");
        printf("  init                        - Inicializar sistema de arquivos\n");
        printf("  load                        - Carregar sistema de arquivos\n");
        printf("  ls [caminho]                - Listar diretório\n");
        printf("  mkdir <caminho>             - Criar diretório\n");
        printf("  create <caminho>            - Criar arquivo\n");
        printf("  unlink <caminho>            - Remover arquivo/diretório\n");
        printf("  write \"dados\" <caminho>     - Escrever dados em arquivo\n");
        printf("  append \"dados\" <caminho>    - Anexar dados a arquivo\n");
        printf("  read <caminho>              - Ler conteúdo de arquivo\n");
        printf("  help                        - Mostrar esta ajuda\n");
        printf("  exit                        - Sair do programa\n");
        
    } else if (strcmp(token, "exit") == 0) {
        printf("Saindo...\n");
        exit(0);
        
    } else {
        printf("Comando não reconhecido: %s\n", token);
        printf("Digite 'help' para ver os comandos disponíveis.\n");
    }
}
