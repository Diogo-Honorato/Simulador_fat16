#include "../include/fat16.h"

// Lista o conteúdo de um diretório
int fat16_ls(fat16_fs_t *fs, const char *path) {
    uint16_t cluster;
    dir_entry_t entry;
    
    if (path == NULL || strlen(path) == 0) {
        cluster = ROOT_DIR_CLUSTER;
    } else {
        if (fat16_find_directory_entry(fs, path, &entry, NULL) != 0) {
            printf("Diretório não encontrado: %s\n", path);
            return -1;
        }
        
        if (entry.attributes != ATTR_DIRECTORY) {
            printf("'%s' não é um diretório\n", path);
            return -1;
        }
        
        cluster = entry.first_block;
    }
    
    data_cluster_t cluster_data;
    if (fat16_read_cluster(fs, cluster, &cluster_data) != 0) {
        printf("Erro ao ler diretório\n");
        return -1;
    }
    
    printf("Conteúdo do diretório %s:\n", path ? path : "/");
    printf("%-18s %-10s %-8s %s\n\n", "Nome", "Tipo", "Tamanho", "Cluster");

    
    for (size_t i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (cluster_data.dir[i].filename[0] == 0) {
            break;
        }
        
        printf("%-18s %-10s %-8u %u\n",
               cluster_data.dir[i].filename,
               cluster_data.dir[i].attributes == ATTR_DIRECTORY ? "DIR" : "FILE",
               cluster_data.dir[i].size,
               cluster_data.dir[i].first_block);
    }
    
    return 0;
}

// Cria um diretório
int fat16_mkdir(fat16_fs_t *fs, const char *path) {
    char parent_path[256];
    char dirname[MAX_FILENAME_SIZE + 1];
    
    fat16_parse_path(path, parent_path, dirname);
    
    // Verifica se o diretório pai existe
    dir_entry_t parent_entry;
    uint16_t parent_cluster;
    
    if (strcmp(parent_path, "/") == 0) {
        parent_cluster = ROOT_DIR_CLUSTER;
    } else {
        if (fat16_find_directory_entry(fs, parent_path, &parent_entry, &parent_cluster) != 0) {
            printf("Diretório pai não encontrado: %s\n", parent_path);
            return -1;
        }
        parent_cluster = parent_entry.first_block;
    }
    
    // Verifica se o diretório já existe
    dir_entry_t existing_entry;
    if (fat16_find_directory_entry(fs, path, &existing_entry, NULL) == 0) {
        printf("Diretório já existe: %s\n", path);
        return -1;
    }
    
    // Encontra um cluster livre
    uint16_t free_cluster = fat16_find_free_cluster(fs);
    if (free_cluster == 0) {
        printf("Erro: Não há clusters livres\n");
        return -1;
    }
    
    // Marca o cluster como fim de arquivo na FAT
    fs->fat[free_cluster] = FAT_END_OF_FILE;
    
    // Inicializa o cluster do diretório
    data_cluster_t cluster_data;
    memset(&cluster_data, 0, sizeof(cluster_data));
    
    if (fat16_write_cluster(fs, free_cluster, &cluster_data) != 0) {
        printf("Erro ao escrever cluster do diretório\n");
        return -1;
    }
    
    // Adiciona a entrada no diretório pai
    if (fat16_add_directory_entry(fs, parent_cluster, dirname, ATTR_DIRECTORY, free_cluster, 0) != 0) {
        printf("Erro ao adicionar entrada no diretório pai\n");
        fs->fat[free_cluster] = FAT_FREE;
        return -1;
    }
    
    // Atualiza a FAT no disco
    if (fat16_write_fat(fs) != 0) {
        printf("Erro ao atualizar FAT\n");
        return -1;
    }
    
    printf("Diretório criado: %s\n", path);
    return 0;
}

// Cria um arquivo
int fat16_create(fat16_fs_t *fs, const char *path) {
    char parent_path[256];
    char filename[MAX_FILENAME_SIZE + 1];
    
    fat16_parse_path(path, parent_path, filename);
    
    // Verifica se o diretório pai existe
    dir_entry_t parent_entry;
    uint16_t parent_cluster;
    
    if (strcmp(parent_path, "/") == 0) {
        parent_cluster = ROOT_DIR_CLUSTER;
    } else {
        if (fat16_find_directory_entry(fs, parent_path, &parent_entry, &parent_cluster) != 0) {
            printf("Diretório pai não encontrado: %s\n", parent_path);
            return -1;
        }
        parent_cluster = parent_entry.first_block;
    }
    
    // Verifica se o arquivo já existe
    dir_entry_t existing_entry;
    if (fat16_find_directory_entry(fs, path, &existing_entry, NULL) == 0) {
        printf("Arquivo já existe: %s\n", path);
        return -1;
    }
    
    // Encontra um cluster livre
    uint16_t free_cluster = fat16_find_free_cluster(fs);
    if (free_cluster == 0) {
        printf("Erro: Não há clusters livres\n");
        return -1;
    }
    
    // Marca o cluster como fim de arquivo na FAT
    fs->fat[free_cluster] = FAT_END_OF_FILE;
    
    // Inicializa o cluster do arquivo
    data_cluster_t cluster_data;
    memset(&cluster_data, 0, sizeof(cluster_data));
    
    if (fat16_write_cluster(fs, free_cluster, &cluster_data) != 0) {
        printf("Erro ao escrever cluster do arquivo\n");
        return -1;
    }
    
    // Adiciona a entrada no diretório pai
    if (fat16_add_directory_entry(fs, parent_cluster, filename, ATTR_FILE, free_cluster, 0) != 0) {
        printf("Erro ao adicionar entrada no diretório pai\n");
        fs->fat[free_cluster] = FAT_FREE; // Libera o cluster
        return -1;
    }
    
    // Atualiza a FAT no disco
    if (fat16_write_fat(fs) != 0) {
        printf("Erro ao atualizar FAT\n");
        return -1;
    }
    
    printf("Arquivo criado: %s\n", path);
    return 0;
}

// Remove um arquivo ou diretório
int fat16_unlink(fat16_fs_t *fs, const char *path) {
    char parent_path[256];
    char name[MAX_FILENAME_SIZE + 1];
    
    fat16_parse_path(path, parent_path, name);
    
    // Encontra a entrada
    dir_entry_t entry;
    uint16_t parent_cluster;
    
    if (fat16_find_directory_entry(fs, path, &entry, &parent_cluster) != 0) {
        printf("Arquivo ou diretório não encontrado: %s\n", path);
        return -1;
    }
    
    // Se for diretório, verifica se está vazio
    if (entry.attributes == ATTR_DIRECTORY) {
        if (!fat16_is_directory_empty(fs, entry.first_block)) {
            printf("Erro: Diretório não está vazio: %s\n", path);
            return -1;
        }
    }
    
    // Libera os clusters na FAT
    uint16_t current_cluster = entry.first_block;
    while (current_cluster != FAT_END_OF_FILE && current_cluster >= FAT_FILE_START && current_cluster <= FAT_FILE_END) {
        uint16_t next_cluster = fs->fat[current_cluster];
        fs->fat[current_cluster] = FAT_FREE;
        current_cluster = next_cluster;
    }
    
    // Remove a entrada do diretório pai
    if (fat16_remove_directory_entry(fs, parent_cluster, name) != 0) {
        printf("Erro ao remover entrada do diretório pai\n");
        return -1;
    }
    
    // Atualiza a FAT no disco
    if (fat16_write_fat(fs) != 0) {
        printf("Erro ao atualizar FAT\n");
        return -1;
    }
    
    printf("Removido: %s\n", path);
    return 0;
}

// Escreve dados em um arquivo (sobrescreve)
int fat16_write(fat16_fs_t *fs, const char *data, const char *path) {
    dir_entry_t entry;
    uint16_t parent_cluster;
    
    if (fat16_find_directory_entry(fs, path, &entry, &parent_cluster) != 0) {
        printf("Arquivo não encontrado: %s\n", path);
        return -1;
    }
    
    if (entry.attributes != ATTR_FILE) {
        printf("'%s' não é um arquivo\n", path);
        return -1;
    }
    
    size_t data_len = strlen(data);
    
    // Calcula quantos clusters são necessários
    size_t clusters_needed = (data_len + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    if (clusters_needed == 0) clusters_needed = 1;
    
    // Libera clusters existentes
    uint16_t current_cluster = entry.first_block;
    while (current_cluster != FAT_END_OF_FILE && current_cluster >= FAT_FILE_START && current_cluster <= FAT_FILE_END) {
        uint16_t next_cluster = fs->fat[current_cluster];
        fs->fat[current_cluster] = FAT_FREE;
        current_cluster = next_cluster;
    }
    
    // Aloca novos clusters
    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;
    
    for (size_t i = 0; i < clusters_needed; i++) {
        uint16_t free_cluster = fat16_find_free_cluster(fs);
        if (free_cluster == 0) {
            printf("Erro: Não há clusters livres suficientes\n");
            return -1;
        }
        
        if (i == 0) {
            first_cluster = free_cluster;
        } else {
            fs->fat[prev_cluster] = free_cluster;
        }
        
        fs->fat[free_cluster] = FAT_END_OF_FILE;
        prev_cluster = free_cluster;
    }
    
    // Escreve os dados
    const char *data_ptr = data;
    current_cluster = first_cluster;
    
    for (size_t i = 0; i < clusters_needed; i++) {
        data_cluster_t cluster_data;
        memset(&cluster_data, 0, sizeof(cluster_data));
        
        size_t bytes_to_write = (i == clusters_needed - 1) ? 
                               (data_len - i * CLUSTER_SIZE) : CLUSTER_SIZE;
        
        if (bytes_to_write > 0) {
            memcpy(cluster_data.data, data_ptr, bytes_to_write);
            data_ptr += bytes_to_write;
        }
        
        if (fat16_write_cluster(fs, current_cluster, &cluster_data) != 0) {
            printf("Erro ao escrever dados\n");
            return -1;
        }
        
        current_cluster = fs->fat[current_cluster];
    }
    
    // Atualiza a entrada do diretório
    char parent_path[256];
    char filename[MAX_FILENAME_SIZE + 1];
    fat16_parse_path(path, parent_path, filename);
    
    if (fat16_remove_directory_entry(fs, parent_cluster, filename) != 0) {
        printf("Erro ao atualizar entrada do diretório\n");
        return -1;
    }
    
    if (fat16_add_directory_entry(fs, parent_cluster, filename, ATTR_FILE, first_cluster, data_len) != 0) {
        printf("Erro ao adicionar entrada atualizada\n");
        return -1;
    }
    
    // Atualiza a FAT no disco
    if (fat16_write_fat(fs) != 0) {
        printf("Erro ao atualizar FAT\n");
        return -1;
    }
    
    printf("Dados escritos no arquivo: %s\n", path);
    return 0;
}

// Anexa dados a um arquivo
int fat16_append(fat16_fs_t *fs, const char *data, const char *path) {
    dir_entry_t entry;
    uint16_t parent_cluster;
    
    if (fat16_find_directory_entry(fs, path, &entry, &parent_cluster) != 0) {
        printf("Arquivo não encontrado: %s\n", path);
        return -1;
    }
    
    if (entry.attributes != ATTR_FILE) {
        printf("'%s' não é um arquivo\n", path);
        return -1;
    }
    
    // Lê o conteúdo atual do arquivo
    char *current_data = malloc(entry.size + 1);
    if (!current_data) {
        printf("Erro de memória\n");
        return -1;
    }
    
    uint16_t current_cluster = entry.first_block;
    size_t bytes_read = 0;
    
    while (current_cluster != FAT_END_OF_FILE && current_cluster >= FAT_FILE_START && current_cluster <= FAT_FILE_END) {
        data_cluster_t cluster_data;
        if (fat16_read_cluster(fs, current_cluster, &cluster_data) != 0) {
            printf("Erro ao ler dados do arquivo\n");
            free(current_data);
            return -1;
        }
        
        size_t bytes_to_read = (entry.size - bytes_read > CLUSTER_SIZE) ? 
                              CLUSTER_SIZE : (entry.size - bytes_read);
        
        memcpy(current_data + bytes_read, cluster_data.data, bytes_to_read);
        bytes_read += bytes_to_read;
        
        current_cluster = fs->fat[current_cluster];
    }
    
    current_data[entry.size] = '\0';
    
    // Combina dados antigos com novos
    size_t new_size = entry.size + strlen(data);
    char *combined_data = malloc(new_size + 1);
    if (!combined_data) {
        printf("Erro de memória\n");
        free(current_data);
        return -1;
    }
    
    strcpy(combined_data, current_data);
    strcat(combined_data, data);
    
    free(current_data);
    
    // Escreve os dados combinados
    int result = fat16_write(fs, combined_data, path);
    free(combined_data);
    
    if (result == 0) {
        printf("Dados anexados ao arquivo: %s\n", path);
    }
    
    return result;
}

// Lê o conteúdo de um arquivo
int fat16_read(fat16_fs_t *fs, const char *path) {
    dir_entry_t entry;
    
    if (fat16_find_directory_entry(fs, path, &entry, NULL) != 0) {
        printf("Arquivo não encontrado: %s\n", path);
        return -1;
    }
    
    if (entry.attributes != ATTR_FILE) {
        printf("'%s' não é um arquivo\n", path);
        return -1;
    }
    
    printf("Conteúdo do arquivo %s (%u bytes):\n\n", path, entry.size);

    
    if (entry.size == 0) {
        printf("(arquivo vazio)\n");
        return 0;
    }
    
    uint16_t current_cluster = entry.first_block;
    size_t bytes_read = 0;
    
    while (current_cluster != FAT_END_OF_FILE && current_cluster >= FAT_FILE_START && current_cluster <= FAT_FILE_END) {
        data_cluster_t cluster_data;
        if (fat16_read_cluster(fs, current_cluster, &cluster_data) != 0) {
            printf("Erro ao ler dados do arquivo\n");
            return -1;
        }
        
        size_t bytes_to_print = (entry.size - bytes_read > CLUSTER_SIZE) ? 
                               CLUSTER_SIZE : (entry.size - bytes_read);
        
        for (size_t i = 0; i < bytes_to_print; i++) {
            putchar(cluster_data.data[i]);
        }
        
        bytes_read += bytes_to_print;
        current_cluster = fs->fat[current_cluster];
    }
    
    printf("\n\n");
    return 0;
}