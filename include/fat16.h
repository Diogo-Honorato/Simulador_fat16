#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Constantes do sistema de arquivos
#define SECTOR_SIZE 512
#define CLUSTER_SIZE 1024
#define SECTORS_PER_CLUSTER 2
#define TOTAL_CLUSTERS 4096
#define PARTITION_SIZE (SECTOR_SIZE * SECTORS_PER_CLUSTER * TOTAL_CLUSTERS)
#define FAT_SIZE_BYTES (TOTAL_CLUSTERS * 2)
#define FAT_SIZE_CLUSTERS (FAT_SIZE_BYTES / CLUSTER_SIZE)
#define MAX_DIR_ENTRIES (CLUSTER_SIZE / sizeof(dir_entry_t))
#define MAX_FILENAME_SIZE 18

// Valores especiais da FAT
#define FAT_FREE 0x0000
#define FAT_FILE_START 0x0001
#define FAT_FILE_END 0xFFFC
#define FAT_BOOT_BLOCK 0xFFFD
#define FAT_TABLE 0xFFFE
#define FAT_END_OF_FILE 0xFFFF

// Atributos de arquivo
#define ATTR_FILE 0
#define ATTR_DIRECTORY 1

// Posições na partição
#define BOOT_BLOCK_CLUSTER 0
#define FAT_START_CLUSTER 1
#define ROOT_DIR_CLUSTER (FAT_START_CLUSTER + FAT_SIZE_CLUSTERS)
#define DATA_START_CLUSTER (ROOT_DIR_CLUSTER + 1)

// Estrutura de entrada de diretório (32 bytes)
typedef struct {
    uint8_t filename[18];
    uint8_t attributes;
    uint8_t reserved[7];
    uint16_t first_block;
    uint32_t size;
} dir_entry_t;

// União para manipular clusters como dados ou diretório
typedef union {
    dir_entry_t dir[MAX_DIR_ENTRIES];
    uint8_t data[CLUSTER_SIZE];
} data_cluster_t;

// Estrutura principal do sistema de arquivos
typedef struct {
    FILE *partition_file;
    uint16_t fat[TOTAL_CLUSTERS];
    data_cluster_t current_cluster;
    char current_path[256];
} fat16_fs_t;

// Funções principais
int fat16_init(fat16_fs_t *fs, const char *partition_name);
int fat16_load(fat16_fs_t *fs, const char *partition_name);
int fat16_format(fat16_fs_t *fs);
void fat16_close(fat16_fs_t *fs);

// Funções de manipulação de clusters
int fat16_read_cluster(fat16_fs_t *fs, uint16_t cluster_num, void *buffer);
int fat16_write_cluster(fat16_fs_t *fs, uint16_t cluster_num, const void *buffer);
int fat16_read_fat(fat16_fs_t *fs);
int fat16_write_fat(fat16_fs_t *fs);

// Funções de manipulação de arquivos e diretórios
int fat16_ls(fat16_fs_t *fs, const char *path);
int fat16_mkdir(fat16_fs_t *fs, const char *path);
int fat16_create(fat16_fs_t *fs, const char *path);
int fat16_unlink(fat16_fs_t *fs, const char *path);
int fat16_write(fat16_fs_t *fs, const char *data, const char *path);
int fat16_append(fat16_fs_t *fs, const char *data, const char *path);
int fat16_read(fat16_fs_t *fs, const char *path);

// Funções auxiliares
uint16_t fat16_find_free_cluster(fat16_fs_t *fs);
int fat16_find_directory_entry(fat16_fs_t *fs, const char *path, dir_entry_t *entry, uint16_t *parent_cluster);
int fat16_add_directory_entry(fat16_fs_t *fs, uint16_t parent_cluster, const char *name, uint8_t attributes, uint16_t first_block, uint32_t size);
int fat16_remove_directory_entry(fat16_fs_t *fs, uint16_t parent_cluster, const char *name);
void fat16_parse_path(const char *path, char *parent_path, char *filename);
int fat16_is_directory_empty(fat16_fs_t *fs, uint16_t cluster);

#endif // FAT16_H