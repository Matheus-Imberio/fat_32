#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "bpb.h"
#include "fat.h"
#include "directory.h"

#define green "\033[0;32m"
#define blue "\033[0;34m"
#define reset "\033[0m"

//AUXILIAR

//Verifica se o comando é "cluster"
int verify_cluster_command(char* command) {
    char aux[7];
    for(int i = 0; i < 7; i++) {
        aux[i] = command[i];
    }
    if(strcmp(aux, "cluster") == 0) {
        return 0;
    }
    return 1;
}

//Verifica se o comando é "attr"
int verify_attr_command(char* command) {
    char aux[4];
    for(int i = 0; i < 4; i++) {
        aux[i] = command[i];
    }
    if(strcmp(aux, "attr") == 0) {
        return 0;
    }
    return 1;
}

//Imprime o disco, usado para debug
void print_disk(FILE* disk) {
    fseek(disk, 0, SEEK_SET);
    uint8_t buffer[1024];
    fread(buffer, 1024, 1, disk);
    for(int i = 0; i < 1024; i++) {
        printf("%x ", buffer[i]);
        if(i % 16 == 15) {
            printf("\n");
        }
    }
}

//Imprime a FAT, usado para debug
void print_fat() {
    for(int i = 0; i < 1024; i++) {
        printf("%x ", g_fat[i]);
        if(i % 16 == 15) {
            printf("\n");
        }
    }
}

//Converte um cluster para um offset em bytes
uint32_t cluster_to_offset(uint32_t cluster) {
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;
    return offset;
}


//BPB

// Lê o BPB do disco
int read_bpb(FILE* disk) {
    fseek(disk, 0, SEEK_SET);
    fread(&g_bpb, sizeof(bpb), 1, disk);
    return 0;
}

// Lê um setor do disco
int read_sector(FILE* disk, uint32_t sector, uint32_t count, void* buffer) {
    fseek(disk, sector * g_bpb.BPB_BytsPerSec, SEEK_SET);
    fread(buffer, g_bpb.BPB_BytsPerSec, count, disk);
    return 0;
}

// Imprime informações do BPB para o comando "info"
int info(FILE* disk) {
    fseek(disk, 0, SEEK_SET);
    fread(&g_bpb, sizeof(bpb), 1, disk);
    printf("BPB_BytsPerSec: %d\n", g_bpb.BPB_BytsPerSec);
    printf("BPB_SecPerClus: %d\n", g_bpb.BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: %d\n", g_bpb.BPB_RsvdSecCnt);
    printf("BPB_NumFATs: %d\n", g_bpb.BPB_NumFATs);
    printf("BPB_RootEntCnt: %d\n", g_bpb.BPB_RootEntCnt);
    printf("BPB_TotSec16: %d\n", g_bpb.BPB_TotSec16);
    printf("BPB_Media: %d\n", g_bpb.BPB_Media);
    printf("BPB_FATSz16: %d\n", g_bpb.BPB_FATSz16);
    printf("BPB_SecPerTrk: %d\n", g_bpb.BPB_SecPerTrk);
    printf("BPB_NumHeads: %d\n", g_bpb.BPB_NumHeads);
    printf("BPB_HiddSec: %d\n", g_bpb.BPB_HiddSec);
    printf("BPB_TotSec32: %d\n", g_bpb.BPB_TotSec32);
    printf("BPB_FATSz32: %d\n", g_bpb.BPB_FATSz32);
    printf("BPB_ExtFlags: %d\n", g_bpb.BPB_ExtFlags);
    printf("BPB_FSVer: %d\n", g_bpb.BPB_FSVer);
    printf("BPB_RootClus: %d\n", g_bpb.BPB_RootClus);
    printf("BPB_FSInfo: %d\n", g_bpb.BPB_FSInfo);
    printf("BPB_BkBootSec: %d\n", g_bpb.BPB_BkBootSec);
    printf("BPB_Reserved:");
    for(int i = 0; i < 12; i++) {
        printf("%d", g_bpb.BPB_Reserved[i]);
    }
    printf("\n");
    printf("BS_DrvNum: %d\n", g_bpb.BS_DrvNum);
    printf("BS_Reserved1: %d\n", g_bpb.BS_Reserved1);
    printf("BS_BootSig: %d\n", g_bpb.BS_BootSig);
    printf("BS_VolID: %d\n", g_bpb.BS_VolID);
    printf("BS_VolLab: %s\n", g_bpb.BS_VolLab);

    return 0;
}

//DIRECTORY

// Lê o diretório raiz
int read_root_directory(FILE* disk) {
    uint32_t cluster = g_bpb.BPB_RootClus;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);
    Directory entries[16];
    fread(entries, sizeof(Directory), 16, disk);

    g_RootDirectory.DIR_Attr = ATTR_DIRECTORY;
    g_RootDirectory.DIR_FstClusLO = cluster;
    g_RootDirectory.DIR_Name[0] = '/';
    return 0;
}

// Exibe o diretório atual
int pwd() {
    printf("Diretório atual: %s\n", g_RootDirectory.DIR_Name);
    return 0;
}

// Exibe os atributos de um diretório
int dir_attr(char* name, FILE* disk){
    uint32_t cluster = g_RootDirectory.DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;
    fseek(disk, offset, SEEK_SET);
    Directory* entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);
    for(uint32_t i = 0; i < cluster_size / sizeof(Directory); i++) {
        if(entries[i].DIR_Name[0] == 0) {
            break;
        }
        if(entries[i].DIR_Name[0] == 0xE5) {
            continue;
        }
        if(entries[i].DIR_Attr == ATTR_VOLUME_ID) {
            continue;
        }
        if(entries[i].DIR_Attr == ATTR_DIRECTORY) {
            char aux[12];
            for(int j = 0; entries[i].DIR_Name[j]!=' ' ; j++) {
                aux[j] = entries[i].DIR_Name[j];
            }
            if(strcmp(aux, name) == 0) {
                printf("Nome: %s\n", entries[i].DIR_Name);
                printf("Atributos: %d\n", entries[i].DIR_Attr);
                printf("Data de criação: %d\n", entries[i].DIR_CrtDate);
                printf("Hora de criação: %d\n", entries[i].DIR_CrtTime);
                printf("Data de último acesso: %d\n", entries[i].DIR_LstAccDate);
                printf("Data de último acesso: %d\n", entries[i].DIR_WrtDate);
                printf("Data de último acesso: %d\n", entries[i].DIR_WrtTime);
                printf("Tamanho: %d\n", entries[i].DIR_FileSize);
                return 0;
            }
        }

    }
    return -1;
}

// Exibe os arquivos de um diretório
int ls(FILE* disk, Directory* dir) {
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;
    fseek(disk, offset, SEEK_SET);
    Directory* entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);
    for(uint32_t i = 0; i < cluster_size / sizeof(Directory); i++) {
        if(entries[i].DIR_Name[0] == 0) {
            break;
        }
        if(entries[i].DIR_Name[0] == 0xE5) {
            continue;
        }
        if(entries[i].DIR_Attr == ATTR_VOLUME_ID) {
            continue;
        }
        if(entries[i].DIR_Attr == ATTR_DIRECTORY) {
            printf(green " %s\n" reset, entries[i].DIR_Name);
        } else {
            for(int j = 0; j < 11; j++) {
                if(entries[i].DIR_Name[j] == ' ' && entries[i].DIR_Name[j + 1] != ' ') {
                    printf(".");
                }
                else if(entries[i].DIR_Name[j] != ' '){
                    printf("%c", entries[i].DIR_Name[j]);
                }
            }
        }
        // printf("Attr: %d\n", entries[i].DIR_Attr);
        printf(" \n");
    }
    return 0;
}

// Muda de diretório, usado para o comando "cd"
int cd(const char* name, FILE* disk, Directory* actual_dir) {
    uint32_t cluster = actual_dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;
    fseek(disk, offset, SEEK_SET);
    Directory* entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);
    for(uint32_t i = 0; i < cluster_size / sizeof(Directory); i++) {
        if(entries[i].DIR_Name[0] == 0) {
            break;
        }
        if(entries[i].DIR_Name[0] == 0xE5) {
            continue;
        }
        if(entries[i].DIR_Attr == ATTR_VOLUME_ID) {
            continue;
        }
        if(entries[i].DIR_Attr == ATTR_DIRECTORY) {
            char aux[12];
            for(int j = 0; entries[i].DIR_Name[j]!=' ' ; j++) {
                aux[j] = entries[i].DIR_Name[j];
            }
            if(strcmp(aux, name) == 0) {
                *actual_dir = entries[i];
                return 0;
            }
        }
    }
    return -1;
}

//FAT

// Lê a FAT do disco
int read_fat(FILE* disk) {
    uint32_t fat_size = g_bpb.BPB_FATSz32 * g_bpb.BPB_BytsPerSec;
    g_fat = malloc(fat_size);
    if(!g_fat) {
        fprintf(stderr, "Erro ao alocar memória para a FAT.\n");
        return -1;
    }
    uint32_t fat_start = g_bpb.BPB_RsvdSecCnt * g_bpb.BPB_BytsPerSec;
    fseek(disk, fat_start, SEEK_SET);
    fread(g_fat, fat_size, 1, disk);
    return 0;
}


// Lê um cluster do disco para o comando "cluster"
int read_cluster(FILE* disk, uint32_t cluster) {
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    uint8_t* buffer = malloc(cluster_size);
    fseek(disk, offset, SEEK_SET);
    fread(buffer, cluster_size, 1, disk);

    printf("Cluster %d:\n", cluster);
    for(uint32_t i = 0; i < cluster_size; i++) {
        uint8_t byte = buffer[i];
        printf("%c ", byte);
        if(i % 16 == 15) {
            printf("\n");
        }
        
    }
    printf("\n");
    free(buffer);
    return 0;
}

// Remove um arquivo do sistema FAT32
int rm(const char* filename, FILE* disk, Directory* actual_dir) {
    uint32_t cluster = actual_dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    fseek(disk, offset, SEEK_SET);
    Directory* entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++) {
        // Verifica se chegamos no final das entradas do diretório
        if (entries[i].DIR_Name[0] == 0) {
            break;
        }
        // Ignora entradas inválidas ou deletadas
        if (entries[i].DIR_Name[0] == 0xE5 || entries[i].DIR_Attr == ATTR_VOLUME_ID) {
            continue;
        }

        // Constrói o nome do arquivo sem os espaços
        char aux[12] = {0};
        for (int j = 0; j < 11; j++) {
            if (entries[i].DIR_Name[j] != ' ') {
                aux[j] = entries[i].DIR_Name[j];
            } else {
                break;
            }
        }

        // Compara com o nome fornecido
        if (strcmp(aux, filename) == 0) {
            // Marca a entrada como deletada
            entries[i].DIR_Name[0] = 0xE5;

            // Escreve as alterações no disco
            fseek(disk, offset, SEEK_SET);
            fwrite(entries, cluster_size, 1, disk);

            printf("Arquivo '%s' removido.\n", filename);
            free(entries);
            return 0;
        }
    }

    printf("Arquivo '%s' não encontrado.\n", filename);
    free(entries);
    return -1;
}


// MAIN
int main(int argc, char *argv[]) {
    // Verifica se o número de argumentos está correto
    if (argc != 2) {
        printf("Sintaxe: %s <disk image> \n", argv[0]);
        return -1;
    }

    // Abre a imagem do disco
    FILE* disk = fopen(argv[1], "rb");
    if (!disk) {
        fprintf(stderr, "Não foi possível abrir a imagem %s!\n", argv[1]);
        return -1;
    }

    read_bpb(disk);
    read_fat(disk);
    read_root_directory(disk);
    Directory actual_dir = g_RootDirectory;
    Directory last_dir = g_RootDirectory;

    char comando[100];
    // Loop para o shell
    while (1) {
        printf("fatshell:[%s]> ", argv[1]);
        
        if (fgets(comando, sizeof(comando), stdin) != NULL) {
            comando[strcspn(comando, "\n")] = 0;
            // info
            if(strcmp(comando, "info") == 0) {
                printf("Lendo informações...\n");
                info(disk);
            }
            // cluster <num>
            else if(verify_cluster_command(comando) == 0) {
                uint8_t num = atoi(comando + 8);
                read_cluster(disk, num);
            }
            // pwd
            else if(strcmp(comando, "pwd") == 0) {
                pwd();
            }
            // attr <nome>
            else if(verify_attr_command(comando) == 0) {
                char* name = comando + 5;
                dir_attr(name, disk);
            }
            // rm <file>
            else if (strncmp(comando, "rm", 2) == 0) {
                if (rm(comando + 3, disk, &actual_dir) == -1) {
                    printf("Falha ao remover o arquivo. Certifique-se de que ele existe.\n");
                }
            }
            // ls
            else if(strcmp(comando, "ls") == 0) {
                ls(disk, &actual_dir);
            }
            // cd <nome>
            else if(strncmp(comando, "cd", 2) == 0) {
                Directory aux = actual_dir;
                if(strcmp(comando + 3, ".") == 0) {
                    continue;
                }
                else if(strcmp(comando + 3, "..") == 0) {
                    actual_dir = last_dir;
                    last_dir = aux;
                } 
                else {
                    if(cd(comando + 3, disk, &actual_dir) == -1) {
                        printf("Diretório não encontrado.\n");
                    }
                    else{
                        last_dir = aux;
                    }
                }

            }
            // exit
            else if(strcmp(comando, "exit") == 0) {
                printf("Saindo...\n");
                break;
            }

        } else {
            printf("Erro ao ler o comando.\n");
            break;
        }
    }
    fclose(disk);
    return 0;
}
