#ifndef SHELL_H
#define SHELL_H
#include "../include/fat16.h"

#define MAX_COMMAND_LENGTH 512
#define PARTITION_FILE "fat.part"

char* extract_quoted_string(const char* input);
void process_command(fat16_fs_t* fs, const char* command);

#endif