#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "bpb.h"
#include "directory.h"

#define green "\033[0;32m"
#define blue "\033[0;34m"
#define reset "\033[0m"
// auxiliares
// Verifica se o comando é "cluster"
int verify_cluster_command(char *command)
{
    char aux[7];
    for (int i = 0; i < 7; i++)
    {
        aux[i] = command[i];
    }
    if (strcmp(aux, "cluster") == 0)
    {
        return 0;
    }
    return 1;
}

// Verifica se o comando é "attr"
int verify_attr_command(char *command)
{
    char aux[4];
    for (int i = 0; i < 4; i++)
    {
        aux[i] = command[i];
    }
    if (strcmp(aux, "attr") == 0)
    {
        return 0;
    }
    return 1;
}

// Imprime o disco, usado para debug
void print_disk(FILE *disk)
{
    fseek(disk, 0, SEEK_SET);
    uint8_t buffer[1024];
    fread(buffer, 1024, 1, disk);
    for (int i = 0; i < 1024; i++)
    {
        printf("%x ", buffer[i]);
        if (i % 16 == 15)
        {
            printf("\n");
        }
    }
}

// Imprime a FAT, usado para debug
void print_fat()
{
    for (int i = 0; i < 1024; i++)
    {
        printf("%x ", g_fat[i]);
        if (i % 16 == 15)
        {
            printf("\n");
        }
    }
}

// Converte um cluster para um offset em bytes
uint32_t cluster_to_offset(uint32_t cluster)
{
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;
    return offset;
}

// BPB

// Lê o BPB do disco
int read_bpb(FILE *disk)
{
    fseek(disk, 0, SEEK_SET);
    fread(&g_bpb, sizeof(bpb), 1, disk);
    return 0;
}

// Lê um setor do disco
int read_sector(FILE *disk, uint32_t sector, uint32_t count, void *buffer)
{
    fseek(disk, sector * g_bpb.BPB_BytsPerSec, SEEK_SET);
    fread(buffer, g_bpb.BPB_BytsPerSec, count, disk);
    return 0;
}

// Imprime informações do BPB para o comando "info"
int info(FILE *disk)
{
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
    for (int i = 0; i < 12; i++)
    {
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

// DIRECTORY

// Lê o diretório raiz
int read_root_directory(FILE *disk)
{
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
int pwd()
{
    printf("Diretório atual: %s\n", g_RootDirectory.DIR_Name);
    return 0;
}

// Exibe os atributos de um diretório
int dir_attr(char *name, FILE *disk)
{
    uint32_t cluster = g_RootDirectory.DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;
    fseek(disk, offset, SEEK_SET);
    Directory *entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break;
        }
        if (entries[i].DIR_Name[0] == 0xE5)
        {
            continue;
        }
        if (entries[i].DIR_Attr == ATTR_VOLUME_ID)
        {
            continue;
        }
        if (entries[i].DIR_Attr == ATTR_DIRECTORY)
        {
            char aux[12];
            for (int j = 0; entries[i].DIR_Name[j] != ' '; j++)
            {
                aux[j] = entries[i].DIR_Name[j];
            }
            if (strcmp(aux, name) == 0)
            {
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
int ls(FILE *disk, Directory *dir)
{
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    fseek(disk, offset, SEEK_SET);
    Directory *entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        // Fim da lista de diretórios
        if (entries[i].DIR_Name[0] == 0)
        {
            break;
        }

        // Ignorar entradas removidas e "." ou ".."
        if (entries[i].DIR_Name[0] == 0xE5 ||
            (strncmp((const char *)entries[i].DIR_Name, ".          ", 11) == 0) ||
            (strncmp((const char *)entries[i].DIR_Name, "..         ", 11) == 0))
        {
            continue;
        }

        // Ignorar entradas inválidas (sem atributos ou com atributos estranhos)
        if (!(entries[i].DIR_Attr & (ATTR_ARCHIVE | ATTR_DIRECTORY)))
        {
            continue;
        }

        // Nome do arquivo/diretório formatado
        char name[13]; // Nome de arquivo FAT32 é no máximo 12 caracteres + '\0'
        memset(name, 0, sizeof(name));

        // Copiar o nome básico (primeiros 8 caracteres)
        strncpy(name, (const char *)entries[i].DIR_Name, 8);
        // Remover espaços no final do nome básico
        for (int j = 7; j >= 0; j--)
        {
            if (name[j] == ' ')
            {
                name[j] = '\0';
            }
            else
            {
                break;
            }
        }
        // Adicionar extensão, se existir (caracteres 8 a 11)
        if (entries[i].DIR_Name[8] != ' ')
        {
            strcat(name, ".");
            strncat(name, (const char *)entries[i].DIR_Name + 8, 3);
            // Remover espaços no final da extensão
            for (int j = strlen(name) - 1; j >= 0; j--)
            {
                if (name[j] == ' ')
                {
                    name[j] = '\0';
                }
                else
                {
                    break;
                }
            }
        }
        // Validar nome (apenas caracteres ASCII visíveis)
        int valid = 1;
        for (size_t j = 0; j < strlen(name); j++)
        {
            if (name[j] < 32 || name[j] > 126)
            {
                valid = 0;
                break;
            }
        }
        if (!valid)
        {
            continue; // Ignorar entrada com caracteres inválidos
        }

        // Exibir diretórios em verde
        if (entries[i].DIR_Attr == ATTR_DIRECTORY)
        {
            printf(green " %s\n" reset, name);
        }
        else
        {
            // Exibir arquivos normais
            printf(" %s\n", name);
        }
    }

    free(entries); // Liberação de memória
    return 0;
}

// Muda de diretório, usado para o comando "cd"
int cd(const char *name, FILE *disk, Directory *actual_dir)
{
    uint32_t cluster = actual_dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    fseek(disk, offset, SEEK_SET);

    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro: Falha ao alocar memória para leitura do diretório.\n");
        return -1;
    }

    fread(entries, cluster_size, 1, disk);

    // Convertendo o nome para o formato FAT32 8.3
    char expected_name[12];
    convert_to_8dot3(name, expected_name);

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break; // Diretório vazio, não encontrado
        }
        if (entries[i].DIR_Name[0] == 0xE5 || (entries[i].DIR_Attr & ATTR_VOLUME_ID))
        {
            continue; // Entrada deletada ou volume ID, ignora
        }
        if (entries[i].DIR_Attr & ATTR_DIRECTORY)
        {
            char aux[12] = {0}; // Garante que o buffer está zerado
            strncpy(aux, (char *)entries[i].DIR_Name, 11);
            aux[11] = '\0'; // Termina corretamente a string

            if (strncmp(aux, expected_name, 11) == 0)
            {
                *actual_dir = entries[i];
                free(entries);
                return 0;
            }
        }
    }

    free(entries);
    return -1; // Diretório não encontrado
}

// FAT

// Lê a FAT do disco
int read_fat(FILE *disk)
{
    uint32_t fat_size = g_bpb.BPB_FATSz32 * g_bpb.BPB_BytsPerSec;
    g_fat = malloc(fat_size);
    if (!g_fat)
    {
        fprintf(stderr, "Erro ao alocar memória para a FAT.\n");
        return -1;
    }
    uint32_t fat_start = g_bpb.BPB_RsvdSecCnt * g_bpb.BPB_BytsPerSec;
    fseek(disk, fat_start, SEEK_SET);
    fread(g_fat, fat_size, 1, disk);
    return 0;
}

// Lê um cluster do disco para o comando "cluster"
int read_cluster(FILE *disk, uint32_t cluster)
{
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    uint8_t *buffer = malloc(cluster_size);
    fseek(disk, offset, SEEK_SET);
    fread(buffer, cluster_size, 1, disk);

    printf("Cluster %d:\n", cluster);
    for (uint32_t i = 0; i < cluster_size; i++)
    {
        uint8_t byte = buffer[i];
        printf("%c ", byte);
        if (i % 16 == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
    free(buffer);
    return 0;
}

void convert_to_8dot3(const char *input, char *output)
{
    memset(output, ' ', 11);

    int i = 0, j = 0;
    // Copiar o nome antes do ponto (máximo de 8 caracteres)
    while (input[i] != '.' && input[i] != '\0' && j < 8)
    {
        output[j++] = input[i];
        i++;
    }

    // Se houver uma extensão, copiar após o ponto
    if (input[i] == '.')
    {
        i++;
        j = 8; // Extensão começa na posição 8
        while (input[i] != '\0' && j < 11)
        {
            output[j++] = input[i];
            i++;
        }
    }
}

int rm(FILE *disk, Directory *dir, const char *filename)
{
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    if (fseek(disk, offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Erro ao buscar o cluster do diretório no disco.\n");
        return -1;
    }

    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro ao alocar memória para o diretório.\n");
        return -1;
    }

    if (fread(entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao ler o cluster do diretório.\n");
        free(entries);
        return -1;
    }

    int found = 0;
    char formatted_name[12] = {0};
    convert_to_8dot3(filename, formatted_name);

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break;
        }

        if (entries[i].DIR_Name[0] == 0xE5)
        {
            continue;
        }

        char entry_name[12] = {0};
        strncpy(entry_name, (const char *)entries[i].DIR_Name, 11);

        if (strncmp(entry_name, formatted_name, 11) == 0)
        {
            entries[i].DIR_Name[0] = 0xE5;
            found = 1;
            break;
        }
    }

    if (found)
    {
        if (fseek(disk, offset, SEEK_SET) != 0 || fwrite(entries, cluster_size, 1, disk) != 1)
        {
            fprintf(stderr, "Erro ao atualizar o diretório no disco.\n");
            free(entries);
            return -1;
        }
    }
    else
    {
        printf("Arquivo '%s' não encontrado.\n", filename);
    }

    free(entries);
    return found ? 0 : -1;
}
// Função para criar um arquivo vazio no diretório atual
int touch(const char *name, FILE *disk, Directory *actual_cluster)
{
    // Verifica se o nome do arquivo não excede 255 caracteres
    if (strlen(name) > 255)
    {
        printf("Erro: O nome do arquivo não pode ter mais de 255 caracteres.\n");
        return -1;
    }

    // Calcula informações do diretório atual
    uint32_t cluster = actual_cluster->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    fseek(disk, offset, SEEK_SET);
    Directory *entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);

    // Procura uma entrada vazia no diretório
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0x00 || entries[i].DIR_Name[0] == 0xE5)
        {
            // Preenche a entrada com o nome no formato 8.3
            char formatted_name[12] = {0};
            convert_to_8dot3(name, formatted_name);
            memset(entries[i].DIR_Name, 0, sizeof(entries[i].DIR_Name));
            strncpy((char *)entries[i].DIR_Name, formatted_name, 11);

            // Preenche outros atributos
            entries[i].DIR_Attr = 0x20; // Arquivo
            entries[i].DIR_FstClusHI = 0;
            entries[i].DIR_FstClusLO = 0;
            entries[i].DIR_FileSize = 0;

            // Atualiza o diretório no disco
            fseek(disk, offset, SEEK_SET);
            fwrite(entries, cluster_size, 1, disk);

            free(entries);
            return 0;
        }
    }

    free(entries);
    printf("Erro: Não há espaço disponível no diretório.\n");
    return -1;
}
// Directory *find_directory(FILE *disk, const char *dir_name) {
//     uint32_t cluster = g_RootDirectory.DIR_FstClusLO; // Começar da raiz
//     uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
//     uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
//     uint32_t offset = data_start + (cluster - 2) * cluster_size;

//     // Percorrer os clusters até encontrar o diretório
//     while (cluster != 0) {
//         if (fseek(disk, offset, SEEK_SET) != 0) {
//             fprintf(stderr, "Erro ao buscar o cluster do diretório no disco.\n");
//             return NULL;
//         }

//         Directory *entries = malloc(cluster_size);
//         if (!entries) {
//             fprintf(stderr, "Erro ao alocar memória para o diretório.\n");
//             return NULL;
//         }

//         if (fread(entries, cluster_size, 1, disk) != 1) {
//             fprintf(stderr, "Erro ao ler o cluster do diretório.\n");
//             free(entries);
//             return NULL;
//         }

//         // Verifica as entradas do diretório
//         for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++) {
//             if (entries[i].DIR_Name[0] == 0) {
//                 free(entries);
//                 return NULL;
//             }

//             if (entries[i].DIR_Name[0] == 0xE5) {
//                 continue;
//             }

//             char entry_name[12] = {0};
//             strncpy(entry_name, (const char *)entries[i].DIR_Name, 11);

//             // Verifica se o nome corresponde ao nome do diretório
//             if (strncmp(entry_name, dir_name, 11) == 0) {
//                 Directory *found_directory = &entries[i]; // Guarda o diretório encontrado
//                 free(entries); // Agora podemos liberar a memória depois de usá-la
//                 return found_directory;
//             }
//         }

//         free(entries);

//         // Buscar o próximo cluster, usando a tabela FAT
//         cluster = read_fat(disk); // read_fat deve retornar o próximo cluster.
//         offset = data_start + (cluster - 2) * cluster_size;
//     }

//     return NULL; // Diretório não encontrado
// }
uint32_t get_directory_offset(uint32_t first_cluster)
{
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    return data_start + (first_cluster - 2) * cluster_size;
}

uint32_t find_directory(FILE *disk, const char *target_dir_name)
{
    if (strcmp(target_dir_name, "/") == 0)
    {
        // Diretório raiz
        return g_bpb.BPB_RootClus;
    }

    char formatted_target_dir_name[12] = {0};
    convert_to_8dot3(target_dir_name, formatted_target_dir_name);

    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;

    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro ao alocar memória para busca de diretório.\n");
        return 0;
    }

    uint32_t current_cluster = g_bpb.BPB_RootClus;
    while (current_cluster != 0xFFFFFFFF)
    {
        uint32_t offset = get_directory_offset(current_cluster);

        if (fseek(disk, offset, SEEK_SET) != 0 || fread(entries, cluster_size, 1, disk) != 1)
        {
            fprintf(stderr, "Erro ao acessar o cluster do diretório.\n");
            free(entries);
            return 0;
        }

        for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
        {
            if (entries[i].DIR_Name[0] == 0)
                break; // Diretório vazio
            if (entries[i].DIR_Name[0] == 0xE5)
                continue; // Entrada excluída

            char entry_name[12] = {0};
            strncpy(entry_name, (const char *)entries[i].DIR_Name, 11);

            if (strncmp(entry_name, formatted_target_dir_name, 11) == 0 && (entries[i].DIR_Attr & 0x10))
            {
                uint32_t target_cluster = entries[i].DIR_FstClusLO;
                free(entries);
                return target_cluster; // Retorna o cluster inicial do diretório
            }
        }

        current_cluster = 0xFFFFFFFF; // Caso simples: assume que não há encadeamento
    }

    fprintf(stderr, "Diretório '%s' não encontrado.\n", target_dir_name);
    free(entries);
    return 0;
}

int mv(FILE *disk, Directory *dir, const char *source_name, const char *target_dir_name)
{
    char formatted_source_name[12] = {0};
    convert_to_8dot3(source_name, formatted_source_name);

    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;

    // Buscar o cluster inicial do diretório de origem
    uint32_t offset = get_directory_offset(dir->DIR_FstClusLO);

    // Ler o diretório de origem
    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro ao alocar memória para o diretório de origem.\n");
        return -1;
    }

    if (fseek(disk, offset, SEEK_SET) != 0 || fread(entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao ler o diretório de origem.\n");
        free(entries);
        return -1;
    }

    int found_source = 0;
    Directory file_entry;

    // Encontrar o arquivo de origem no diretório atual
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
            break; // Fim do diretório
        if (entries[i].DIR_Name[0] == 0xE5)
            continue; // Entrada excluída

        if (strncmp((const char *)entries[i].DIR_Name, formatted_source_name, 11) == 0)
        {
            file_entry = entries[i];
            entries[i].DIR_Name[0] = 0xE5; // Marcar como excluído
            found_source = 1;
            break;
        }
    }

    if (!found_source)
    {
        fprintf(stderr, "Arquivo '%s' não encontrado no diretório de origem.\n", source_name);
        free(entries);
        return -1;
    }

    // Localizar o cluster inicial do diretório de destino
    uint32_t target_cluster = find_directory(disk, target_dir_name);
    if (target_cluster == 0)
    {
        fprintf(stderr, "Diretório de destino '%s' não encontrado.\n", target_dir_name);
        free(entries);
        return -1;
    }

    uint32_t target_offset = get_directory_offset(target_cluster);

    // Ler o diretório de destino
    Directory *target_entries = malloc(cluster_size);
    if (!target_entries)
    {
        fprintf(stderr, "Erro ao alocar memória para o diretório de destino.\n");
        free(entries);
        return -1;
    }

    if (fseek(disk, target_offset, SEEK_SET) != 0 || fread(target_entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao ler o diretório de destino.\n");
        free(entries);
        free(target_entries);
        return -1;
    }

    // Procurar espaço livre no diretório de destino
    int added = 0;
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (target_entries[i].DIR_Name[0] == 0)
        {
            // Encontrou espaço livre, copia o arquivo para o diretório de destino
            memcpy(&target_entries[i], &file_entry, sizeof(Directory));
            added = 1;
            break;
        }
    }

    if (!added)
    {
        fprintf(stderr, "Não há espaço suficiente no diretório de destino '%s'.\n", target_dir_name);
        free(entries);
        free(target_entries);
        return -1;
    }

    // Escrever as alterações no diretório de destino
    if (fseek(disk, target_offset, SEEK_SET) != 0 || fwrite(target_entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao salvar alterações no diretório de destino.\n");
        free(entries);
        free(target_entries);
        return -1;
    }

    // Escrever as alterações no diretório de origem
    if (fseek(disk, offset, SEEK_SET) != 0 || fwrite(entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao salvar alterações no diretório de origem.\n");
        free(entries);
        free(target_entries);
        return -1;
    }

    printf("Arquivo '%s' movido para '%s' com sucesso.\n", source_name, target_dir_name);

    free(entries);
    free(target_entries);
    return 0;
}
int cp(FILE *disk, Directory *dir, const char *source_name, const char *target_dir_name)
{
    char formatted_source_name[12] = {0};
    convert_to_8dot3(source_name, formatted_source_name);

    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;

    // Buscar o cluster inicial do diretório de origem
    uint32_t offset = get_directory_offset(dir->DIR_FstClusLO);

    // Ler o diretório de origem
    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro ao alocar memória para o diretório de origem.\n");
        return -1;
    }

    if (fseek(disk, offset, SEEK_SET) != 0 || fread(entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao ler o diretório de origem.\n");
        free(entries);
        return -1;
    }

    int found_source = 0;
    Directory file_entry;

    // Encontrar o arquivo de origem no diretório atual
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
            break; // Fim do diretório
        if (entries[i].DIR_Name[0] == 0xE5)
            continue; // Entrada excluída

        if (strncmp((const char *)entries[i].DIR_Name, formatted_source_name, 11) == 0)
        {
            file_entry = entries[i]; // Copiar a entrada do arquivo
            found_source = 1;
            break;
        }
    }

    if (!found_source)
    {
        fprintf(stderr, "Arquivo '%s' não encontrado no diretório de origem.\n", source_name);
        free(entries);
        return -1;
    }

    // Localizar o cluster inicial do diretório de destino
    uint32_t target_cluster = find_directory(disk, target_dir_name);
    if (target_cluster == 0)
    {
        fprintf(stderr, "Diretório de destino '%s' não encontrado.\n", target_dir_name);
        free(entries);
        return -1;
    }

    uint32_t target_offset = get_directory_offset(target_cluster);

    // Ler o diretório de destino
    Directory *target_entries = malloc(cluster_size);
    if (!target_entries)
    {
        fprintf(stderr, "Erro ao alocar memória para o diretório de destino.\n");
        free(entries);
        return -1;
    }

    if (fseek(disk, target_offset, SEEK_SET) != 0 || fread(target_entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao ler o diretório de destino.\n");
        free(entries);
        free(target_entries);
        return -1;
    }

    // Procurar espaço livre no diretório de destino
    int added = 0;
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (target_entries[i].DIR_Name[0] == 0)
        {
            // Encontrou espaço livre, copia o arquivo para o diretório de destino
            memcpy(&target_entries[i], &file_entry, sizeof(Directory));
            added = 1;
            break;
        }
    }

    if (!added)
    {
        fprintf(stderr, "Não há espaço suficiente no diretório de destino '%s'.\n", target_dir_name);
        free(entries);
        free(target_entries);
        return -1;
    }

    // Escrever as alterações no diretório de destino
    if (fseek(disk, target_offset, SEEK_SET) != 0 || fwrite(target_entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao salvar alterações no diretório de destino.\n");
        free(entries);
        free(target_entries);
        return -1;
    }

    printf("Arquivo '%s' copiado para '%s' com sucesso.\n", source_name, target_dir_name);

    free(entries);
    free(target_entries);
    return 0;
}
int rename_file(FILE *disk, Directory *dir, const char *file_name, const char *new_name)
{
    char formatted_old_name[12] = {0};
    char formatted_new_name[12] = {0};

    // Converter o nome do arquivo para o formato 8.3
    convert_to_8dot3(file_name, formatted_old_name);
    convert_to_8dot3(new_name, formatted_new_name);

    // Verificar se as extensões dos nomes são iguais
    if (strncmp(formatted_old_name + 8, formatted_new_name + 8, 3) != 0)
    {
        fprintf(stderr, "Erro: O novo nome '%s' deve manter a mesma extensão que '%s'.\n", new_name, file_name);
        return -1;
    }

    // Procurar o arquivo no diretório atual
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    if (fseek(disk, offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Erro ao buscar o cluster do diretório no disco.\n");
        return -1;
    }

    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro ao alocar memória para o diretório.\n");
        return -1;
    }

    if (fread(entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro ao ler o cluster do diretório.\n");
        free(entries);
        return -1;
    }

    int found = 0;

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break;
        }

        if (entries[i].DIR_Name[0] == 0xE5)
        {
            continue;
        }

        char entry_name[12] = {0};
        strncpy(entry_name, (const char *)entries[i].DIR_Name, 11);

        if (strncmp(entry_name, formatted_old_name, 11) == 0)
        {
            // Encontrou o arquivo, renomear
            memcpy(entries[i].DIR_Name, formatted_new_name, 11);
            found = 1;
            break;
        }
    }

    if (found)
    {
        // Escrever as alterações no disco
        if (fseek(disk, offset, SEEK_SET) != 0 || fwrite(entries, cluster_size, 1, disk) != 1)
        {
            fprintf(stderr, "Erro ao salvar as alterações no disco.\n");
            free(entries);
            return -1;
        }

        printf("Arquivo renomeado com sucesso: %s -> %s\n", file_name, new_name);
    }
    else
    {
        fprintf(stderr, "Arquivo '%s' não encontrado.\n", file_name);
    }

    free(entries);
    return found ? 0 : -1;
}
// Aloca um cluster vazio na FAT
int allocate_cluster(FILE *disk, uint32_t *new_cluster)
{
    for (uint32_t i = 2; i < g_bpb.BPB_TotSec32 / g_bpb.BPB_SecPerClus; i++)
    {
        uint32_t entry;
        memcpy(&entry, &g_fat[i * 4], 4);
        if ((entry & 0x0FFFFFFF) == 0)
        { // Cluster vazio encontrado
            uint32_t eoc = 0x0FFFFFFF;
            memcpy(&g_fat[i * 4], &eoc, 4);

            // Atualiza a FAT no disco
            uint32_t fat_start = g_bpb.BPB_RsvdSecCnt * g_bpb.BPB_BytsPerSec;
            fseek(disk, fat_start + (i * 4), SEEK_SET);
            fwrite(&eoc, 4, 1, disk);

            *new_cluster = i;
            return 0;
        }
    }
    return -1; // Sem espaço livre
}

// Cria um novo diretório
int mkdir(const char *name, Directory *parent, FILE *disk)
{
    uint32_t new_cluster;
    if (allocate_cluster(disk, &new_cluster) == -1)
    {
        fprintf(stderr, "Erro: Sem espaço para criar um novo diretório.\n");
        return -1;
    }

    // Criar a entrada do novo diretório no formato 8.3
    Directory new_dir = {0};
    convert_to_8dot3(name, (char *)new_dir.DIR_Name);
    new_dir.DIR_Attr = ATTR_DIRECTORY;
    new_dir.DIR_FstClusLO = (uint16_t)(new_cluster & 0xFFFF);
    new_dir.DIR_FstClusHI = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    // Encontrar espaço livre no diretório pai
    uint32_t cluster = parent->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    // Ajustar número de entradas lidas de acordo com o tamanho do cluster
    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = calloc(max_entries, sizeof(Directory));

    fread(entries, sizeof(Directory), max_entries, disk);
    for (int i = 0; i < max_entries; i++)
    {
        if (entries[i].DIR_Name[0] == 0 || entries[i].DIR_Name[0] == 0xE5)
        {
            entries[i] = new_dir;
            fseek(disk, offset + (i * sizeof(Directory)), SEEK_SET);
            fwrite(&new_dir, sizeof(Directory), 1, disk);
            break;
        }
    }
    free(entries);

    // Criar o cluster do novo diretório e inicializar as entradas "." e ".."
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    Directory *buffer = calloc(cluster_size / sizeof(Directory), sizeof(Directory));

    // Entrada "."
    memcpy(buffer[1].DIR_Name, "..         ", 11);
    buffer[0].DIR_Name[11] = '\0';
    buffer[0].DIR_Attr = ATTR_DIRECTORY;
    buffer[0].DIR_FstClusLO = new_dir.DIR_FstClusLO;
    buffer[0].DIR_FstClusHI = new_dir.DIR_FstClusHI;

    // Entrada ".."

    memcpy(buffer[1].DIR_Name, "..         ", 11);
    buffer[1].DIR_Name[11] = '\0';
    buffer[1].DIR_Attr = ATTR_DIRECTORY;
    buffer[1].DIR_FstClusLO = parent->DIR_FstClusLO;
    buffer[1].DIR_FstClusHI = parent->DIR_FstClusHI;

    // Escrever no disco
    fseek(disk, cluster_to_offset(new_cluster), SEEK_SET);
    fwrite(buffer, cluster_size, 1, disk);
    free(buffer);

    return 0;
}

// MAIN
int main(int argc, char *argv[])
{
    // Verifica se o número de argumentos está correto
    if (argc != 2)
    {
        printf("Sintaxe: %s <disk image> \n", argv[0]);
        return -1;
    }

    // Abre a imagem do disco
    FILE *disk = fopen(argv[1], "rb+");
    if (!disk)
    {
        fprintf(stderr, "Não foi possível abrir a imagem %s!\n", argv[1]);
        return -1;
    }

    read_bpb(disk);
    read_fat(disk);
    read_root_directory(disk);
    Directory actual_dir = g_RootDirectory;
    Directory last_dir = g_RootDirectory;

    char comando[100];
    char current_path[256] = "/"; // Caminho inicial

    // Loop para o shell
    while (1)
    {
        printf("fatshell:[%s:%s]> ", argv[1], current_path);

        if (fgets(comando, sizeof(comando), stdin) != NULL)
        {
            comando[strcspn(comando, "\n")] = 0;

            // info
            if (strcmp(comando, "info") == 0)
            {
                printf("Lendo informações...\n");
                info(disk);
            }
            // cluster <num>
            else if (verify_cluster_command(comando) == 0)
            {
                uint8_t num = atoi(comando + 8);
                read_cluster(disk, num);
            }
            // MKDIR <nome>
            else if (strncmp(comando, "mkdir", 5) == 0)
            {
                if (strlen(comando) <= 6)
                {
                    printf("Erro: Nome do diretório inválido.\n");
                }
                else
                {
                    char *name = comando + 6;
                    mkdir(name, &actual_dir, disk);
                }
            }
            // pwd
            else if (strcmp(comando, "pwd") == 0)
            {
                printf("Diretório atual: %s\n", current_path);
            }

            // attr <nome>
            else if (verify_attr_command(comando) == 0)
            {
                char *name = comando + 5;
                dir_attr(name, disk);
            }
            // ls
            else if (strcmp(comando, "ls") == 0)
            {
                ls(disk, &actual_dir);
            }
            // touch <filename.file>
            else if (strncmp(comando, "touch", 5) == 0)
            {
                char nomeArquivo[13];
                sscanf(comando + 6, "%s", nomeArquivo);

                if (touch(nomeArquivo, disk, &actual_dir) != 0)
                {
                    printf("Erro ao criar o arquivo '%s'.\n", nomeArquivo);
                }
            }
            // rm <name>
            else if (strncmp(comando, "rm", 2) == 0)
            {
                // Remover espaços em branco no nome do arquivo, caso haja
                char nomeArquivo[13]; // 8+1 (ponto)+3

                // Extrai o nome do arquivo após "rm "
                sscanf(comando + 3, "%12s", nomeArquivo); // "rm" ocupa os 2 primeiros caracteres

                // Chama a função rm para excluir o arquivo
                if (rm(disk, &actual_dir, nomeArquivo) != 0)
                {
                    printf("Erro ao remover o arquivo '%s'.\n", nomeArquivo);
                }
            }
            // cd <name>
            else if (strncmp(comando, "cd", 2) == 0)
            {
                Directory aux = actual_dir;

                if (strcmp(comando + 3, ".") == 0)
                {
                    // Permanece no mesmo diretório
                    continue;
                }
                else if (strcmp(comando + 3, "..") == 0)
                {
                    // Voltar para o diretório anterior
                    if (strcmp(current_path, "/") == 0)
                    {
                        printf("Já está no diretório raiz.\n");
                    }
                    else
                    {
                        actual_dir = last_dir;
                        last_dir = aux;

                        // Atualizar o caminho para o diretório pai
                        char *last_slash = strrchr(current_path, '/');
                        if (last_slash && last_slash != current_path)
                        {
                            *last_slash = '\0'; // Remove o último componente do caminho
                        }
                        else if (last_slash == current_path)
                        {
                            // Caso especial: voltando ao diretório raiz
                            strcpy(current_path, "/");
                        }
                    }
                }
                else
                {
                    // Tentar mudar para o diretório especificado
                    if (cd(comando + 3, disk, &actual_dir) == -1)
                    {
                        printf("Diretório '%s' não encontrado.\n", comando + 3);
                    }
                    else
                    {
                        last_dir = aux;

                        // Atualizar o caminho para o novo diretório
                        size_t temp_path_len = strlen(current_path);
                        size_t comando_len = strlen(comando + 3);

                        if (temp_path_len + comando_len + 2 > sizeof(current_path))
                        {
                            printf("Erro: Caminho muito longo para ser armazenado.\n");
                        }
                        else
                        {
                            if (strcmp(current_path, "/") == 0)
                            {
                                snprintf(current_path, sizeof(current_path), "/%s", comando + 3);
                            }
                            else
                            {
                                strncat(current_path, "/", sizeof(current_path) - strlen(current_path) - 1);
                                strncat(current_path, comando + 3, sizeof(current_path) - strlen(current_path) - 1);
                            }
                        }
                    }
                }
            }

            // mv<source><target>
            else if (strncmp(comando, "mv", 2) == 0)
            {
                char source[100], target[100];
                if (sscanf(comando + 3, "%s %s", source, target) == 2)
                {
                    if (mv(disk, &actual_dir, source, target) != 0)
                    {
                        printf("Erro ao mover '%s' para '%s'.\n", source, target);
                    }
                }
                else
                {
                    printf("Uso: mv <caminho_origem> <diretório_destino>\n");
                }
            }
            // cp <source><target>
            else if (strncmp(comando, "cp", 2) == 0)
            {
                char source[100], target[100];
                if (sscanf(comando + 3, "%s %s", source, target) == 2)
                {
                    // Chama a função cp para copiar o arquivo
                    if (cp(disk, &actual_dir, source, target) != 0)
                    {
                        printf("Erro ao copiar '%s' para '%s'.\n", source, target);
                    }
                }
                else
                {
                    printf("Uso: cp <arquivo_origem> <diretório_destino>\n");
                }
            }
            // rename <file> <newfilename>
            else if (strncmp(comando, "rename", 6) == 0)
            {
                char file_name[100], new_name[100];
                if (sscanf(comando + 7, "%s %s", file_name, new_name) == 2)
                {
                    if (rename_file(disk, &actual_dir, file_name, new_name) != 0)
                    {
                        printf("Erro ao renomear '%s' para '%s'.\n", file_name, new_name);
                    }
                }
                else
                {
                    printf("Uso: rename <arquivo> <novo_nome>\n");
                }
            }

            // exit
            else if (strcmp(comando, "exit") == 0)
            {
                printf("Finalizado!\n");
                break;
            }
        }
        else
        {
            printf("Erro ao ler o comando.\n");
            break;
        }
    }
    fclose(disk);
    return 0;
}
