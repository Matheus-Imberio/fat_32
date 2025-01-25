#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

uint8_t* g_fat = NULL;

int read_fat(FILE* disk);
int read_cluster(FILE* disk, uint32_t cluster);