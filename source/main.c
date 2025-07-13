#include "../include/shell.h"
#include "../include/fat16.h"

int main() {
    fat16_fs_t fs;
    char command[MAX_COMMAND_LENGTH];
    
    // Inicializa estrutura do sistema de arquivos
    memset(&fs, 0, sizeof(fs));
    
    printf("Simulador de Sistema de Arquivos FAT16\n");
    printf("Digite 'help' para ver os comandos disponíveis.\n");
    printf("Digite 'init' para inicializar um novo sistema de arquivos.\n");
    printf("Digite 'load' para carregar um sistema de arquivos existente.\n\n");
    
    while (1) {
        printf("~fat16$ ");
        fflush(stdout);
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        process_command(&fs, command);
    }
    
    fat16_close(&fs);
    
    return 0;
}