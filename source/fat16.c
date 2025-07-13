#include "../include/fat16.h"

// Inicializa o sistema de arquivos (formatar)
int fat16_init(fat16_fs_t *fs, const char *partition_name) {
    fs->partition_file = fopen(partition_name, "wb+");
    if (!fs->partition_file) {
        perror("Erro ao criar arquivo de partição");
        return -1;
    }
    
    if (fat16_format(fs) != 0) {
        fclose(fs->partition_file);
        return -1;
    }
    
    strcpy(fs->current_path, "/");
    return 0;
}

// Carrega um sistema de arquivos existente
int fat16_load(fat16_fs_t *fs, const char *partition_name) {
    fs->partition_file = fopen(partition_name, "rb+");
    if (!fs->partition_file) {
        perror("Erro ao abrir arquivo de partição");
        return -1;
    }
    
    // Carrega a FAT
    if (fat16_read_fat(fs) != 0) {
        fclose(fs->partition_file);
        return -1;
    }
    
    strcpy(fs->current_path, "/");
    return 0;
}

// Formata o sistema de arquivos
int fat16_format(fat16_fs_t *fs) {
    uint8_t buffer[CLUSTER_SIZE];
    
    // Inicializa o boot block (cluster 0) com 0xbb
    memset(buffer, 0xbb, CLUSTER_SIZE);
    if (fat16_write_cluster(fs, BOOT_BLOCK_CLUSTER, buffer) != 0) {
        return -1;
    }
    
    // Inicializa a FAT
    memset(fs->fat, 0, sizeof(fs->fat));
    
    // Marca clusters especiais na FAT
    fs->fat[BOOT_BLOCK_CLUSTER] = FAT_BOOT_BLOCK;
    
    // Marca clusters da FAT
    for (int i = FAT_START_CLUSTER; i < FAT_START_CLUSTER + FAT_SIZE_CLUSTERS; i++) {
        fs->fat[i] = FAT_TABLE;
    }
    
    // Marca o diretório root
    fs->fat[ROOT_DIR_CLUSTER] = FAT_END_OF_FILE;
    
    // Escreve a FAT no disco
    if (fat16_write_fat(fs) != 0) {
        return -1;
    }
    
    // Inicializa o diretório root
    memset(buffer, 0, CLUSTER_SIZE);
    if (fat16_write_cluster(fs, ROOT_DIR_CLUSTER, buffer) != 0) {
        return -1;
    }
    
    // Inicializa o resto da partição com zeros
    memset(buffer, 0, CLUSTER_SIZE);
    for (uint16_t i = DATA_START_CLUSTER; i < TOTAL_CLUSTERS; i++) {
        if (fat16_write_cluster(fs, i, buffer) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// Fecha o sistema de arquivos
void fat16_close(fat16_fs_t *fs) {
    if (fs->partition_file) {
        fclose(fs->partition_file);
        fs->partition_file = NULL;
    }
}

// Lê um cluster do disco
int fat16_read_cluster(fat16_fs_t *fs, uint16_t cluster_num, void *buffer) {
    if (cluster_num >= TOTAL_CLUSTERS) {
        return -1;
    }
    
    long offset = cluster_num * CLUSTER_SIZE;
    if (fseek(fs->partition_file, offset, SEEK_SET) != 0) {
        return -1;
    }
    
    if (fread(buffer, CLUSTER_SIZE, 1, fs->partition_file) != 1) {
        return -1;
    }
    
    return 0;
}

// Escreve um cluster no disco
int fat16_write_cluster(fat16_fs_t *fs, uint16_t cluster_num, const void *buffer) {
    if (cluster_num >= TOTAL_CLUSTERS) {
        return -1;
    }
    
    long offset = cluster_num * CLUSTER_SIZE;
    if (fseek(fs->partition_file, offset, SEEK_SET) != 0) {
        return -1;
    }
    
    if (fwrite(buffer, CLUSTER_SIZE, 1, fs->partition_file) != 1) {
        return -1;
    }
    
    fflush(fs->partition_file);
    return 0;
}

// Lê a FAT do disco
int fat16_read_fat(fat16_fs_t *fs) {
    uint8_t buffer[CLUSTER_SIZE];
    uint16_t *fat_ptr = fs->fat;
    
    for (int i = 0; i < FAT_SIZE_CLUSTERS; i++) {
        if (fat16_read_cluster(fs, FAT_START_CLUSTER + i, buffer) != 0) {
            return -1;
        }
        
        memcpy(fat_ptr, buffer, CLUSTER_SIZE);
        fat_ptr += CLUSTER_SIZE / sizeof(uint16_t);
    }
    
    return 0;
}

// Escreve a FAT no disco
int fat16_write_fat(fat16_fs_t *fs) {
    uint8_t buffer[CLUSTER_SIZE];
    uint16_t *fat_ptr = fs->fat;
    
    for (int i = 0; i < FAT_SIZE_CLUSTERS; i++) {
        memcpy(buffer, fat_ptr, CLUSTER_SIZE);
        
        if (fat16_write_cluster(fs, FAT_START_CLUSTER + i, buffer) != 0) {
            return -1;
        }
        
        fat_ptr += CLUSTER_SIZE / sizeof(uint16_t);
    }
    
    return 0;
}

// Encontra um cluster livre
uint16_t fat16_find_free_cluster(fat16_fs_t *fs) {
    for (uint16_t i = DATA_START_CLUSTER; i < TOTAL_CLUSTERS; i++) {
        if (fs->fat[i] == FAT_FREE) {
            return i;
        }
    }
    return 0;
}

// Separa o caminho em diretório pai e nome do arquivo
void fat16_parse_path(const char *path, char *parent_path, char *filename) {
    char *last_slash = strrchr(path, '/');
    
    if (last_slash == NULL || last_slash == path) {
        // Arquivo no diretório root
        strcpy(parent_path, "/");
        strcpy(filename, last_slash ? last_slash + 1 : path);
    } else {
        // Arquivo em subdiretório
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        strcpy(filename, last_slash + 1);
    }
}

// Encontra uma entrada de diretório
int fat16_find_directory_entry(fat16_fs_t *fs, const char *path, dir_entry_t *entry, uint16_t *parent_cluster) {
    if (strcmp(path, "/") == 0) {
        // Diretório root
        if (parent_cluster) *parent_cluster = ROOT_DIR_CLUSTER;
        return 0;
    }
    
    char temp_path[256];
    strcpy(temp_path, path);
    
    // Remove '/' do final se existir
    int len = strlen(temp_path);
    if (len > 1 && temp_path[len - 1] == '/') {
        temp_path[len - 1] = '\0';
    }
    
    // Divide o caminho em tokens
    char *token = strtok(temp_path, "/");
    uint16_t current_cluster = ROOT_DIR_CLUSTER;
    
    while (token != NULL) {
        data_cluster_t cluster_data;
        if (fat16_read_cluster(fs, current_cluster, &cluster_data) != 0) {
            return -1;
        }
        
        int found = 0;
        for (size_t i = 0; i < MAX_DIR_ENTRIES; i++) {
            if (cluster_data.dir[i].filename[0] == 0) {
                // Entrada vazia
                break;
            }
            
            if (strncmp((char *)cluster_data.dir[i].filename, token, MAX_FILENAME_SIZE) == 0) {
                if (entry) {
                    *entry = cluster_data.dir[i];
                }
                if (parent_cluster) {
                    *parent_cluster = current_cluster;
                }
                current_cluster = cluster_data.dir[i].first_block;
                found = 1;
                break;
            }
        }
        
        if (!found) {
            return -1;
        }
        
        token = strtok(NULL, "/");
    }
    
    return 0;
}

// Adiciona uma entrada de diretório
int fat16_add_directory_entry(fat16_fs_t *fs, uint16_t parent_cluster, const char *name, uint8_t attributes, uint16_t first_block, uint32_t size) {
    data_cluster_t cluster_data;
    
    if (fat16_read_cluster(fs, parent_cluster, &cluster_data) != 0) {
        return -1;
    }
    
    // Encontra uma entrada livre
    for (size_t i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (cluster_data.dir[i].filename[0] == 0) {
            // Entrada livre encontrada
            memset(&cluster_data.dir[i], 0, sizeof(dir_entry_t));
            strncpy((char *)cluster_data.dir[i].filename, name, MAX_FILENAME_SIZE);
            cluster_data.dir[i].attributes = attributes;
            cluster_data.dir[i].first_block = first_block;
            cluster_data.dir[i].size = size;
            
            return fat16_write_cluster(fs, parent_cluster, &cluster_data);
        }
    }
    
    return -1;
}

// Remove uma entrada de diretório
int fat16_remove_directory_entry(fat16_fs_t *fs, uint16_t parent_cluster, const char *name) {
    data_cluster_t cluster_data;
    
    if (fat16_read_cluster(fs, parent_cluster, &cluster_data) != 0) {
        return -1;
    }
    
    // Encontra a entrada
    for (size_t i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (cluster_data.dir[i].filename[0] == 0) {
            break;
        }
        
        if (strncmp((char *)cluster_data.dir[i].filename, name, MAX_FILENAME_SIZE) == 0) {
            // Move todas as entradas subsequentes para preencher o espaço
            for (size_t j = i; j < MAX_DIR_ENTRIES - 1; j++) {
                cluster_data.dir[j] = cluster_data.dir[j + 1];
            }
            
            // Limpa a última entrada
            memset(&cluster_data.dir[MAX_DIR_ENTRIES - 1], 0, sizeof(dir_entry_t));
            
            return fat16_write_cluster(fs, parent_cluster, &cluster_data);
        }
    }
    
    return -1;
}

// Verifica se um diretório está vazio
int fat16_is_directory_empty(fat16_fs_t *fs, uint16_t cluster) {
    data_cluster_t cluster_data;
    
    if (fat16_read_cluster(fs, cluster, &cluster_data) != 0) {
        return 0;
    }
    
    return cluster_data.dir[0].filename[0] == 0;
}