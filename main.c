#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "bpb.h"
#include "directory.h"
#include <wchar.h>
#include <locale.h>
#include <time.h>

#define ATTR_LONG_NAME 0x0F

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
// Função para formatar o nome do arquivo/diretório
void formatar_nome(char *nome_formatado, const uint8_t *nome_fat32)
{
    char nome[9] = {0};     // Nome do arquivo (8 caracteres)
    char extensao[4] = {0}; // Extensão do arquivo (3 caracteres)

    // Extrai o nome (primeiros 8 caracteres)
    strncpy(nome, (const char *)nome_fat32, 8);
    nome[8] = '\0';

    // Remove espaços extras do nome
    for (int i = 7; i >= 0; i--)
    {
        if (nome[i] == ' ')
        {
            nome[i] = '\0';
        }
        else
        {
            break;
        }
    }

    // Extrai a extensão (últimos 3 caracteres)
    strncpy(extensao, (const char *)nome_fat32 + 8, 3);
    extensao[3] = '\0';

    // Remove espaços extras da extensão
    for (int i = 2; i >= 0; i--)
    {
        if (extensao[i] == ' ')
        {
            extensao[i] = '\0';
        }
        else
        {
            break;
        }
    }

    // Combina nome e extensão com um ponto
    if (extensao[0] != '\0')
    {
        sprintf(nome_formatado, "%s.%s", nome, extensao);
    }
    else
    {
        strcpy(nome_formatado, nome);
    }
}
// Exibe os atributos de um arquivo ou diretório
int dir_attr(const char *name, FILE *disk, Directory *parent)
{
    if (!name || !disk || !parent)
    {
        fprintf(stderr, "Erro: Parâmetros inválidos para dir_attr().\n");
        return -1;
    }

    // Converter o nome para o formato 8.3
    char formatted_name[12] = {0};
    normalizar_nome(formatted_name, name);

    // Obter o cluster inicial do diretório atual
    uint32_t cluster = parent->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    // Ler o diretório atual
    fseek(disk, offset, SEEK_SET);
    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        fprintf(stderr, "Erro: Falha ao alocar memória para leitura do diretório.\n");
        return -1;
    }
    fread(entries, cluster_size, 1, disk);

    // Procurar o arquivo ou diretório pelo nome
    int found = 0;
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break; // Fim das entradas válidas
        }

        if (entries[i].DIR_Name[0] == 0xE5)
        {
            continue; // Entrada excluída, ignorar
        }

        char entry_name[12] = {0};
        strncpy(entry_name, (const char *)entries[i].DIR_Name, 11);

        if (strncmp(entry_name, formatted_name, 11) == 0)
        {
            // Arquivo ou diretório encontrado
            found = 1;

            // Formata o nome para exibição
            char nome_formatado[13] = {0}; // 8 (nome) + 1 (ponto) + 3 (extensão) + 1 (null)
            formatar_nome(nome_formatado, entries[i].DIR_Name);

            // Exibir o nome do arquivo/diretório
            printf("Nome do arquivo: %s\n", nome_formatado);

            // Exibir os atributos
            if (entries[i].DIR_Attr & ATTR_DIRECTORY)
            {
                printf("Atributos: Diretorio\n");
            }
            else
            {
                printf("Atributos: Arquivo\n");
            }

            // Decodificar e exibir a data de criação
            if (entries[i].DIR_CrtDate != 0)
            {
                printf("Data de criação: %04d-%02d-%02d\n",
                       (entries[i].DIR_CrtDate >> 9) + 1980,
                       (entries[i].DIR_CrtDate >> 5) & 0x0F,
                       entries[i].DIR_CrtDate & 0x1F);
            }
            else
            {
                printf("Data de criação: Não disponível\n");
            }

            // Decodificar e exibir a hora de criação
            if (entries[i].DIR_CrtTime != 0)
            {
                printf("Hora de criação: %02d:%02d:%02d\n",
                       (entries[i].DIR_CrtTime >> 11),
                       (entries[i].DIR_CrtTime >> 5) & 0x3F,
                       (entries[i].DIR_CrtTime & 0x1F) * 2);
            }
            else
            {
                printf("Hora de criação: Não disponível\n");
            }

            // Decodificar e exibir a data de última modificação
            if (entries[i].DIR_WrtDate != 0)
            {
                printf("Data de última modificação: %04d-%02d-%02d\n",
                       (entries[i].DIR_WrtDate >> 9) + 1980,
                       (entries[i].DIR_WrtDate >> 5) & 0x0F,
                       entries[i].DIR_WrtDate & 0x1F);
            }
            else
            {
                printf("Data de última modificação: Não disponível\n");
            }

            // Decodificar e exibir a hora de última modificação
            if (entries[i].DIR_WrtTime != 0)
            {
                printf("Hora de última modificação: %02d:%02d:%02d\n",
                       (entries[i].DIR_WrtTime >> 11),
                       (entries[i].DIR_WrtTime >> 5) & 0x3F,
                       (entries[i].DIR_WrtTime & 0x1F) * 2);
            }
            else
            {
                printf("Hora de última modificação: Não disponível\n");
            }

            // Exibir tamanho do arquivo (se não for diretório)
            if (!(entries[i].DIR_Attr & ATTR_DIRECTORY))
            {
                printf("Tamanho do arquivo: %u bytes%s\n", entries[i].DIR_FileSize, entries[i].DIR_FileSize == 0 ? " (vazio)" : "");
            }
            else
            {
                printf("Tamanho: N/A (Diretório)\n");
            }

            break;
        }
    }

    if (!found)
    {
        printf("Arquivo ou diretório '%s' não encontrado.\n", name);
    }

    free(entries);
    return found ? 0 : -1;
}
// Exibe os arquivos de um diretório com suporte a caracteres acentuados
int ls(FILE *disk, Directory *dir)
{
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    fseek(disk, offset, SEEK_SET);
    Directory *entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);

    char lfn_name[256] = "";
    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break;
        }

        if (entries[i].DIR_Name[0] == 0xE5 || (strncmp((const char *)entries[i].DIR_Name, ".          ", 11) == 0) || (strncmp((const char *)entries[i].DIR_Name, "..         ", 11) == 0))
        {
            continue;
        }

        if (!(entries[i].DIR_Attr & (ATTR_ARCHIVE | ATTR_DIRECTORY)))
        {
            continue;
        }

        // Se for uma entrada de nome longo, armazenamos o nome
        if (entries[i].DIR_Attr == ATTR_LONG_NAME)
        {
            LongDirEntry *lfn_entry = (LongDirEntry *)&entries[i];
            wchar_t lfn_part[14] = {0};
            memcpy(lfn_part, lfn_entry->LDIR_Name1, sizeof(lfn_entry->LDIR_Name1));
            memcpy(lfn_part + 5, lfn_entry->LDIR_Name2, sizeof(lfn_entry->LDIR_Name2));
            memcpy(lfn_part + 11, lfn_entry->LDIR_Name3, sizeof(lfn_entry->LDIR_Name3));
            wcstombs(lfn_name, lfn_part, sizeof(lfn_name));
            continue;
        }

        // Se não houver nome longo armazenado, usamos o nome curto
        char name[13] = "";
        if (lfn_name[0] != '\0')
        {
            strncpy(name, lfn_name, sizeof(name) - 1);
            lfn_name[0] = '\0';
        }
        else
        {
            strncpy(name, (const char *)entries[i].DIR_Name, 8);
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
            if (entries[i].DIR_Name[8] != ' ')
            {
                strcat(name, ".");
                strncat(name, (const char *)entries[i].DIR_Name + 8, 3);
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
        }

        // Exibir diretórios em verde
        if (entries[i].DIR_Attr == ATTR_DIRECTORY)
        {
            printf(green " %s\n" reset, name);
        }
        else
        {
            printf(" %s\n", name);
        }
    }

    free(entries);
    return 0;
}
// Função para mudar de diretório
int cd(const char *dir_name, FILE *disk, Directory *current_dir)
{
    uint32_t cluster = current_dir->DIR_FstClusLO | (current_dir->DIR_FstClusHI << 16);
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = cluster_to_offset(cluster);

    if (strcmp(dir_name, "..") == 0)
    {
        if (cluster == g_bpb.BPB_RootClus)
        {
            printf("Já está no diretório raiz.\n");
            return -1;
        }

        // Ler a entrada do diretório pai
        fseek(disk, offset, SEEK_SET);
        Directory *entries = malloc(cluster_size);
        fread(entries, cluster_size, 1, disk);

        for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
        {
            if (entries[i].DIR_Name[0] == 0)
                break;

            if (entries[i].DIR_Attr == ATTR_DIRECTORY && strncmp((char *)entries[i].DIR_Name, "..", 2) == 0)
            {
                uint32_t parent_cluster = entries[i].DIR_FstClusLO | (entries[i].DIR_FstClusHI << 16);
                free(entries);

                // Atualiza a estrutura do diretório atual com o diretório pai
                offset = cluster_to_offset(parent_cluster);
                fseek(disk, offset, SEEK_SET);
                fread(current_dir, sizeof(Directory), 1, disk);
                return 0;
            }
        }

        free(entries);
        printf("Erro: Diretório pai não encontrado.\n");
        return -1;
    }

    // Ler o diretório atual para encontrar o novo diretório
    fseek(disk, offset, SEEK_SET);
    Directory *entries = malloc(cluster_size);
    fread(entries, cluster_size, 1, disk);

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
            break;

        if (entries[i].DIR_Attr & ATTR_DIRECTORY)
        {
            char name[12] = {0};
            strncpy(name, (const char *)entries[i].DIR_Name, 11);
            trim_whitespace(name);

            if (strcmp(name, dir_name) == 0)
            {
                memcpy(current_dir, &entries[i], sizeof(Directory));
                free(entries);
                return 0;
            }
        }
    }

    printf("Diretório '%s' não encontrado.\n", dir_name);
    free(entries);
    return -1;
}
// Função para remover espaços em branco do início e do fim de uma string
void trim_whitespace(char *str)
{
    char *end;

    // Remove espaços do início
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
    {
        return; // Se a string estiver vazia, nada a fazer
    }

    // Remove espaços do final
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Adiciona terminador nulo no final da string limpa
    *(end + 1) = '\0';
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
            // Verifica se é um arquivo e não um diretório
            if (entries[i].DIR_Attr & ATTR_DIRECTORY)
            {
                free(entries);
                return -2; // Código de erro para "é um diretório"
            }

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

    free(entries);
    return found ? 0 : -1; // 0 para sucesso, -1 para arquivo não encontrado
}
void normalizar_nome_dir(char *destino, const char *origem)
{
    memset(destino, ' ', 11); // Preenche com espaços
    destino[11] = '\0';       // Terminador nulo

    int i = 0, j = 0;
    // Copiar o nome antes do ponto (máximo de 8 caracteres)
    while (origem[i] != '.' && origem[i] != '\0' && j < 8)
    {
        destino[j++] = toupper((unsigned char)origem[i]); // Converte para maiúsculas
        i++;
    }

    // Se houver uma extensão, copiar após o ponto
    if (origem[i] == '.')
    {
        i++;
        j = 8; // Extensão começa na posição 8
        while (origem[i] != '\0' && j < 11)
        {
            destino[j++] = toupper((unsigned char)origem[i]); // Converte para maiúsculas
            i++;
        }
    }
}
// Função para criar um arquivo vazio no diretório atual
int touch(const char *name, FILE *disk, Directory *actual_cluster)
{
    // Verifica se o nome do arquivo tem uma extensão
    if (strchr(name, '.') == NULL)
    {
        printf("Erro: O arquivo '%s' deve ter uma extensão.\n", name);
        return -1;
    }

    // Verifica se o nome do arquivo é muito longo
    if (strlen(name) > 255)
    {
        printf("Erro: O nome do arquivo não pode ter mais de 255 caracteres.\n");
        return -1;
    }

    // Verifica se o arquivo já existe
    if (nome_existe(name, disk, actual_cluster))
    {
        return -2;
    }

    // Encontra uma entrada vazia no diretório atual
    uint32_t cluster = actual_cluster->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = malloc(max_entries * sizeof(Directory));
    fread(entries, sizeof(Directory), max_entries, disk);

    // Normaliza o nome do arquivo para o formato FAT32 (8.3)
    char formatted_name[12] = {0};
    normalizar_nome(formatted_name, name);

    // Procura uma entrada vazia para criar o arquivo
    for (int i = 0; i < max_entries; i++)
    {
        if (entries[i].DIR_Name[0] == 0x00 || entries[i].DIR_Name[0] == 0xE5)
        {
            memset(entries[i].DIR_Name, 0, sizeof(entries[i].DIR_Name));
            strncpy((char *)entries[i].DIR_Name, formatted_name, 11);
            entries[i].DIR_Attr = 0x20; // Atributo de arquivo

            // Preenche as datas e horas de criação/modificação
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);

            entries[i].DIR_CrtDate = ((tm_info->tm_year - 80) << 9) | ((tm_info->tm_mon + 1) << 5) | tm_info->tm_mday;
            entries[i].DIR_CrtTime = (tm_info->tm_hour << 11) | (tm_info->tm_min << 5) | (tm_info->tm_sec / 2);
            entries[i].DIR_WrtDate = entries[i].DIR_CrtDate; // Data de modificação é a mesma da criação
            entries[i].DIR_WrtTime = entries[i].DIR_CrtTime; // Hora de modificação é a mesma da criação

            entries[i].DIR_FstClusHI = 0;
            entries[i].DIR_FstClusLO = 0;
            entries[i].DIR_FileSize = 0;

            // Escreve a entrada atualizada no disco
            fseek(disk, offset, SEEK_SET);
            fwrite(entries, sizeof(Directory), max_entries, disk);

            free(entries);
            return 0; // Sucesso
        }
    }

    // Se não houver espaço no diretório
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

// Função para verificar se um nome já existe no diretório
int nome_existe_mkdir(const char *name, FILE *disk, Directory *actual_cluster)
{
    uint32_t cluster = actual_cluster->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = malloc(max_entries * sizeof(Directory));
    fread(entries, sizeof(Directory), max_entries, disk);

    char nome_normalizado[12] = {0};
    normalizar_nome_dir(nome_normalizado, name);

    for (int i = 0; i < max_entries; i++)
    {
        if (entries[i].DIR_Name[0] == 0)
            break; // Não há mais entradas válidas

        // Ignora entradas excluídas ou especiais
        if (entries[i].DIR_Name[0] == 0xE5 || (strncmp((const char *)entries[i].DIR_Name, ".          ", 11) == 0) || (strncmp((const char *)entries[i].DIR_Name, "..         ", 11) == 0))
        {
            continue;
        }

        // Verifica se o nome já existe (arquivo ou diretório)
        char nome_existente[12] = {0};
        strncpy(nome_existente, (const char *)entries[i].DIR_Name, 11);
        nome_existente[11] = '\0'; // Garante terminador nulo

        // Comparação case insensitive para nomes curtos (8.3)
        if (strncasecmp(nome_existente, nome_normalizado, 11) == 0)
        {
            free(entries);
            return 1; // Já existe
        }
    }

    free(entries);
    return 0; // Não encontrado
}

// Função para criar um novo diretório
int mkdir(const char *name, Directory *parent, FILE *disk)
{
    // Verifica se o diretório já existe
    if (nome_existe_mkdir(name, disk, parent))
    {
        printf("Erro: O diretório '%s' já existe.\n", name);
        fflush(stdout);
        return -1;
    }

    // Verifica se há espaço no diretório atual
    uint32_t cluster = parent->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = malloc(max_entries * sizeof(Directory));
    fread(entries, sizeof(Directory), max_entries, disk);

    int found_empty = 0;
    for (int i = 0; i < max_entries; i++)
    {
        if (entries[i].DIR_Name[0] == 0 || entries[i].DIR_Name[0] == 0xE5)
        {
            found_empty = 1;
            break;
        }
    }

    if (!found_empty)
    {
        printf("Erro: O diretório atual está cheio. Não é possível criar '%s'.\n", name);
        free(entries);
        return -1;
    }

    // Aloca um novo cluster para o diretório
    uint32_t new_cluster;
    if (allocate_cluster(disk, &new_cluster) == -1)
    {
        fprintf(stderr, "Erro: Sem espaço para criar um novo diretório.\n");
        free(entries);
        return -1;
    }

    // Prepara a nova entrada de diretório
    Directory new_dir = {0};
    normalizar_nome((char *)new_dir.DIR_Name, name);
    new_dir.DIR_Attr = ATTR_DIRECTORY;

    // Preenche as datas e horas de criação/modificação
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    new_dir.DIR_CrtDate = ((tm_info->tm_year - 80) << 9) | ((tm_info->tm_mon + 1) << 5) | tm_info->tm_mday;
    new_dir.DIR_CrtTime = (tm_info->tm_hour << 11) | (tm_info->tm_min << 5) | (tm_info->tm_sec / 2);
    new_dir.DIR_WrtDate = new_dir.DIR_CrtDate; // Data de modificação é a mesma da criação
    new_dir.DIR_WrtTime = new_dir.DIR_CrtTime; // Hora de modificação é a mesma da criação

    new_dir.DIR_FstClusLO = (uint16_t)(new_cluster & 0xFFFF);
    new_dir.DIR_FstClusHI = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    // Encontra uma entrada vazia para armazenar o novo diretório
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

    // Cria as entradas padrão "." e ".."
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    Directory *buffer = calloc(cluster_size / sizeof(Directory), sizeof(Directory));

    memcpy(buffer[0].DIR_Name, ".          ", 11);
    buffer[0].DIR_Attr = ATTR_DIRECTORY;
    buffer[0].DIR_FstClusLO = new_dir.DIR_FstClusLO;
    buffer[0].DIR_FstClusHI = new_dir.DIR_FstClusHI;

    memcpy(buffer[1].DIR_Name, "..         ", 11);
    buffer[1].DIR_Attr = ATTR_DIRECTORY;
    buffer[1].DIR_FstClusLO = parent->DIR_FstClusLO;
    buffer[1].DIR_FstClusHI = parent->DIR_FstClusHI;

    fseek(disk, cluster_to_offset(new_cluster), SEEK_SET); // Corrigido
    fwrite(buffer, cluster_size, 1, disk);
    free(buffer);

    return 0;
}
// Função para normalizar um nome FAT32 (formato 8.3)
void normalizar_nome(char *destino, const char *origem)
{
    memset(destino, 0, 12);
    convert_to_8dot3(origem, destino); // Converte para formato FAT32
}

// Verifica se um nome já existe no diretório atual
int nome_existe(const char *name, FILE *disk, Directory *actual_cluster)
{
    uint32_t cluster = actual_cluster->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = malloc(max_entries * sizeof(Directory));
    fread(entries, sizeof(Directory), max_entries, disk);

    char nome_normalizado[12] = {0};
    normalizar_nome(nome_normalizado, name);

    for (int i = 0; i < max_entries; i++)
    {
        if (entries[i].DIR_Name[0] == 0)
            break; // Não há mais entradas válidas

        // Ignora entradas excluídas ou especiais
        if (entries[i].DIR_Name[0] == 0xE5 || (strncmp((const char *)entries[i].DIR_Name, ".          ", 11) == 0) || (strncmp((const char *)entries[i].DIR_Name, "..         ", 11) == 0))
        {
            continue;
        }

        // Verifica se é uma entrada de nome longo (LFN)
        if (entries[i].DIR_Attr == ATTR_LONG_NAME)
        {
            LongDirEntry *lfn_entry = (LongDirEntry *)&entries[i];
            wchar_t lfn_part[14] = {0};
            memcpy(lfn_part, lfn_entry->LDIR_Name1, sizeof(lfn_entry->LDIR_Name1));
            memcpy(lfn_part + 5, lfn_entry->LDIR_Name2, sizeof(lfn_entry->LDIR_Name2));
            memcpy(lfn_part + 11, lfn_entry->LDIR_Name3, sizeof(lfn_entry->LDIR_Name3));

            char lfn_name[256] = {0};
            setlocale(LC_ALL, "en_US.UTF-8");
            wcstombs(lfn_name, lfn_part, sizeof(lfn_name));

            // Comparação exata para nomes longos
            if (strcmp(lfn_name, name) == 0)
            {
                free(entries);
                return 1; // Já existe
            }
        }
        else
        {
            // Verifica se é uma entrada de nome curto (8.3)
            char nome_existente[12] = {0};
            strncpy(nome_existente, (const char *)entries[i].DIR_Name, 11);
            trim_whitespace(nome_existente);

            // Comparação case sensitive para nomes curtos (8.3)
            if (strcmp(nome_existente, nome_normalizado) == 0)
            {
                free(entries);
                return 1; // Já existe
            }
        }
    }

    free(entries);
    return 0; // Não encontrado
}

int find_parent_directory(FILE *disk, Directory *actual_dir, Directory *parent_dir)
{
    if (!disk || !actual_dir || !parent_dir)
    {
        return -1;
    }

    uint32_t cluster = actual_dir->DIR_FstClusLO;

    if (cluster == g_bpb.BPB_RootClus)
    {
        fprintf(stderr, "Erro: Já está na raiz, não pode subir mais.\n");
        return -1;
    }

    uint32_t data_start = (g_bpb.BPB_RsvdSecCnt + g_bpb.BPB_NumFATs * g_bpb.BPB_FATSz32) * g_bpb.BPB_BytsPerSec;
    uint32_t cluster_size = g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec;
    uint32_t offset = data_start + (cluster - 2) * cluster_size;

    fseek(disk, offset, SEEK_SET);

    Directory *entries = malloc(cluster_size);
    if (!entries)
    {
        return -1;
    }

    fread(entries, cluster_size, 1, disk);

    // O segundo item do diretório sempre é "..", o diretório pai
    *parent_dir = entries[1];

    free(entries);
    return 0;
}

int rmdir(const char *name, Directory *parent, FILE *disk)
{
    if (!name || !parent || !disk)
    {
        fprintf(stderr, "Erro: Parâmetros inválidos para rmdir().\n");
        return -1;
    }

    // Converter o nome para o formato 8.3
    char expected_name[12] = {0};
    convert_to_8dot3(name, expected_name);

    // Encontrar o diretório no diretório pai
    uint32_t cluster = parent->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = malloc(max_entries * sizeof(Directory));
    if (!entries)
    {
        fprintf(stderr, "Erro: Falha ao alocar memória para leitura do diretório.\n");
        return -1;
    }

    fread(entries, sizeof(Directory), max_entries, disk);

    Directory *dir_to_remove = NULL;
    int dir_index = -1;

    for (int i = 0; i < max_entries; i++)
    {
        if (entries[i].DIR_Name[0] == 0)
        {
            break; // Fim das entradas válidas
        }

        if (entries[i].DIR_Name[0] == 0xE5)
        {
            continue; // Entrada deletada, ignorar
        }

        if (entries[i].DIR_Attr & ATTR_DIRECTORY)
        {
            char dir_name[12] = {0};
            memcpy(dir_name, entries[i].DIR_Name, 11);

            if (strncmp(dir_name, expected_name, 11) == 0)
            {
                dir_to_remove = &entries[i];
                dir_index = i;
                break;
            }
        }
    }

    if (!dir_to_remove)
    {
        fprintf(stderr, "Erro: Diretório '%s' não encontrado.\n", name);
        free(entries);
        return -1;
    }

    // Verificar se o diretório está vazio
    uint32_t dir_cluster = dir_to_remove->DIR_FstClusLO | (dir_to_remove->DIR_FstClusHI << 16);
    uint32_t dir_offset = cluster_to_offset(dir_cluster);
    fseek(disk, dir_offset, SEEK_SET);

    Directory *dir_entries = malloc(g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec);
    if (!dir_entries)
    {
        fprintf(stderr, "Erro: Falha ao alocar memória para leitura do diretório.\n");
        free(entries);
        return -1;
    }

    fread(dir_entries, sizeof(Directory), g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec / sizeof(Directory), disk);

    int is_empty = 1;
    for (unsigned long i = 0; i < g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec / sizeof(Directory); i++)
    {
        if (dir_entries[i].DIR_Name[0] == 0)
        {
            break; // Fim das entradas válidas
        }

        if (dir_entries[i].DIR_Name[0] == 0xE5)
        {
            continue; // Entrada deletada, ignorar
        }

        // Ignorar as entradas "." e ".."
        if (strncmp((const char *)dir_entries[i].DIR_Name, ".          ", 11) == 0 ||
            strncmp((const char *)dir_entries[i].DIR_Name, "..         ", 11) == 0)
        {
            continue;
        }

        // Se houver qualquer outra entrada, o diretório não está vazio
        is_empty = 0;
        break;
    }

    if (!is_empty)
    {
        fprintf(stderr, "Erro: O diretório '%s' não está vazio.\n", name);
        free(entries);
        free(dir_entries);
        return -1;
    }

    // Marcar o cluster como livre na FAT
    uint32_t fat_entry = 0x00000000;
    uint32_t fat_offset = g_bpb.BPB_RsvdSecCnt * g_bpb.BPB_BytsPerSec + dir_cluster * 4;
    fseek(disk, fat_offset, SEEK_SET);
    fwrite(&fat_entry, sizeof(uint32_t), 1, disk);

    // Marcar a entrada do diretório como disponível (0xE5)
    entries[dir_index].DIR_Name[0] = 0xE5;
    fseek(disk, offset + (dir_index * sizeof(Directory)), SEEK_SET);
    fwrite(&entries[dir_index], sizeof(Directory), 1, disk);

    free(entries);
    free(dir_entries);
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
    (void)last_dir;

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
                    if (nome_existe_mkdir(name, disk, &actual_dir))
                    {
                        printf("Erro: O diretório '%s' já existe.\n", name);
                    }
                    else
                    {
                        mkdir(name, &actual_dir, disk);
                    }
                }
            }
            // pwd
            else if (strcmp(comando, "pwd") == 0)
            {
                printf("Diretório atual: %s\n", current_path);
            }

            // Verifica se o comando é "attr <nome>"
            else if (verify_attr_command(comando) == 0)
            {
                // Extrai o nome do arquivo ou diretório
                char *name = comando + 5; // Pula "attr " para obter o nome

                // Remove espaços em branco no início e no final do nome (se houver)
                while (*name == ' ')
                    name++; // Remove espaços no início
                char *end = name + strlen(name) - 1;
                while (end > name && *end == ' ')
                    end--;         // Remove espaços no final
                *(end + 1) = '\0'; // Termina a string corretamente

                // Verifica se o nome não está vazio
                if (strlen(name) == 0)
                {
                    printf("Erro: Nome do arquivo ou diretório não especificado.\n");
                }
                else
                {
                    // Chama a função dir_attr com o diretório atual
                    dir_attr(name, disk, &actual_dir); // Passa o endereço de actual_dir
                }
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

                if (touch(nomeArquivo, disk, &actual_dir) == -2)
                {
                    printf(" Erro: O arquivo '%s' já existe.\n", nomeArquivo);
                }
            }
            // rmdir
            else if (strncmp(comando, "rmdir", 5) == 0)
            {
                if (strlen(comando) <= 6)
                {
                    printf("Erro: Nome do diretório inválido.\n");
                }
                else
                {
                    char *name = comando + 6; // Extrai o nome do diretório após "rmdir "
                    if (rmdir(name, &actual_dir, disk) == 0)
                    {
                    }
                    else
                    {
                        printf("Erro ao remover o diretório '%s'.\n", name);
                    }
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
                int resultado = rm(disk, &actual_dir, nomeArquivo);

                if (resultado == -2)
                {
                    printf("Erro: '%s' é um diretório e não pode ser removido com rm.\n", nomeArquivo);
                }
                else if (resultado == -1)
                {
                    printf("Erro: Arquivo '%s' não encontrado.\n", nomeArquivo);
                }
                else if (resultado != 0)
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
                        // Encontrar o diretório pai
                        Directory parent_dir;
                        if (find_parent_directory(disk, &actual_dir, &parent_dir) == 0)
                        {
                            last_dir = actual_dir;   // Salva o diretório atual como último diretório
                            actual_dir = parent_dir; // Atualiza o diretório atual para o diretório pai

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
                                actual_dir = g_RootDirectory; // Restaura o diretório atual para o diretório raiz
                            }
                        }
                        else
                        {
                            printf("Erro: Não foi possível encontrar o diretório pai.\n");
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
