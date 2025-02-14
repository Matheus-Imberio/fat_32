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

// Muda de diretório, usado para o comando "cd"
int cd(const char *name, FILE *disk, Directory *actual_dir)
{
    if (!name || !disk || !actual_dir)
    {
        fprintf(stderr, "Erro: Parâmetros inválidos para cd().\n");
        return -1;
    }

    uint32_t cluster = actual_dir->DIR_FstClusLO;

    // Se for "cd ..", precisamos encontrar o diretório pai corretamente
    if (strcmp(name, "..") == 0)
    {
        Directory parent;
        if (find_parent_directory(disk, actual_dir, &parent) != 0)
        {
            fprintf(stderr, "Erro: Não foi possível encontrar o diretório pai.\n");
            return -1;
        }
        *actual_dir = parent;
        return 0;
    }

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

    if (fread(entries, cluster_size, 1, disk) != 1)
    {
        fprintf(stderr, "Erro: Falha na leitura do diretório.\n");
        free(entries);
        return -1;
    }

    // Convertendo o nome para FAT32 8.3
    char expected_name[12] = {0};
    convert_to_8dot3(name, expected_name);

    for (uint32_t i = 0; i < cluster_size / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0)
            break; // Diretório vazio, para a busca

        if (entries[i].DIR_Name[0] == 0xE5 || (entries[i].DIR_Attr & ATTR_VOLUME_ID))
            continue; // Entrada deletada ou Volume ID, ignora

        if (entries[i].DIR_Attr & ATTR_DIRECTORY)
        {
            char dir_name[12] = {0};
            memcpy(dir_name, entries[i].DIR_Name, 11); // Usa memcpy para segurança

            if (strncmp(dir_name, expected_name, 11) == 0)
            {
                *actual_dir = entries[i]; // Atualiza o diretório atual
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
int find_file_in_directory(FILE *disk, Directory *dir, const char *file_name, Directory *file_entry)
{
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    Directory *entries = malloc(g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec);
    if (!entries)
    {
        fprintf(stderr, "Erro ao alocar memória para leitura do diretório.\n");
        return -1;
    }

    fread(entries, sizeof(Directory), g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec / sizeof(Directory), disk);

    char formatted_name[12];
    convert_to_8dot3(file_name, formatted_name);

    for (size_t i = 0; i < g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec / sizeof(Directory); i++)
    {
        if (entries[i].DIR_Name[0] == 0 || entries[i].DIR_Name[0] == 0xE5)
        {
            continue;
        }

        if (strncmp((const char *)entries[i].DIR_Name, formatted_name, 11) == 0)
        {
            *file_entry = entries[i]; // Retorna a entrada do arquivo
            free(entries);
            return 0;
        }
    }

    free(entries);
    return -1; // Arquivo não encontrado
}
int cp(const char *source_path, const char *target_path, FILE *disk, Directory *actual_dir)
{
    FILE *source_file = NULL;
    uint8_t buffer[4096];
    size_t bytes_read, bytes_written;

    // Verifica se o caminho de origem é dentro da imagem (começa com "img/")
    if (strncmp(source_path, "img/", 4) == 0)
    {
        // Remove o prefixo "img/" para operar dentro da imagem
        const char *internal_source = source_path + 4;

        // Busca o arquivo de origem no diretório atual ou raiz
        Directory source_entry;
        if (find_file_in_directory(disk, actual_dir, internal_source, &source_entry) != 0)
        {
            fprintf(stderr, "Erro: Arquivo de origem '%s' não encontrado na imagem.\n", source_path);
            return -1;
        }

        // Verifica se o destino é dentro da imagem (começa com "img/")
        if (strncmp(target_path, "img/", 4) == 0)
        {
            // Remove o prefixo "img/" para operar dentro da imagem
            const char *internal_target = target_path + 4;

            // Verifica se o arquivo de destino já existe
            Directory target_entry;
            if (find_file_in_directory(disk, actual_dir, internal_target, &target_entry) == 0)
            {
                fprintf(stderr, "Erro: O arquivo de destino '%s' já existe na imagem.\n", target_path);
                return -1;
            }

            // Cria o arquivo de destino na imagem
            if (touch(internal_target, disk, actual_dir) != 0)
            {
                fprintf(stderr, "Erro: Falha ao criar o arquivo de destino '%s'.\n", target_path);
                return -1;
            }

            // Busca o arquivo de destino criado
            if (find_file_in_directory(disk, actual_dir, internal_target, &target_entry) != 0)
            {
                fprintf(stderr, "Erro: Arquivo de destino '%s' não encontrado na imagem.\n", target_path);
                return -1;
            }

            // Copia os dados do arquivo de origem para o destino
            uint32_t source_cluster = source_entry.DIR_FstClusLO | (source_entry.DIR_FstClusHI << 16);
            uint32_t target_cluster = target_entry.DIR_FstClusLO | (target_entry.DIR_FstClusHI << 16);
            uint32_t file_size = source_entry.DIR_FileSize;
            uint32_t bytes_remaining = file_size;

            while (bytes_remaining > 0)
            {
                uint32_t read_size = (bytes_remaining > sizeof(buffer)) ? sizeof(buffer) : bytes_remaining;
                fseek(disk, cluster_to_offset(source_cluster), SEEK_SET);
                bytes_read = fread(buffer, 1, read_size, disk);

                if (bytes_read == 0)
                {
                    fprintf(stderr, "Erro: Falha ao ler o arquivo de origem '%s'.\n", source_path);
                    return -1;
                }

                fseek(disk, cluster_to_offset(target_cluster), SEEK_SET);
                bytes_written = fwrite(buffer, 1, bytes_read, disk);

                if (bytes_written != bytes_read)
                {
                    fprintf(stderr, "Erro: Falha ao escrever no arquivo de destino '%s'.\n", target_path);
                    return -1;
                }

                bytes_remaining -= bytes_read;
                source_cluster = g_fat[source_cluster * 4] & 0x0FFFFFFF;

                // Aloca um novo cluster para o destino, se necessário
                if (bytes_remaining > 0)
                {
                    uint32_t new_cluster;
                    if (allocate_cluster(disk, &new_cluster) == -1)
                    {
                        fprintf(stderr, "Erro: Sem espaço para alocar um novo cluster.\n");
                        return -1;
                    }

                    g_fat[target_cluster * 4] = new_cluster;
                    target_cluster = new_cluster;
                }
            }

            // Atualiza o tamanho do arquivo de destino
            target_entry.DIR_FileSize = file_size;
            uint32_t cluster = actual_dir->DIR_FstClusLO;
            uint32_t offset = cluster_to_offset(cluster);
            fseek(disk, offset, SEEK_SET);
            fwrite(&target_entry, sizeof(Directory), 1, disk);

            printf("Arquivo '%s' copiado para '%s' com sucesso.\n", source_path, target_path);
            return 0;
        }
        else
        {
            // Copia o arquivo da imagem para o sistema de arquivos do host
            source_file = fopen(target_path, "wb");
            if (!source_file)
            {
                fprintf(stderr, "Erro: Não foi possível abrir o arquivo de destino '%s'.\n", target_path);
                return -1;
            }

            // Copia os dados do arquivo da imagem para o arquivo no host
            uint32_t file_cluster = source_entry.DIR_FstClusLO | (source_entry.DIR_FstClusHI << 16);
            uint32_t file_size = source_entry.DIR_FileSize;
            uint32_t bytes_remaining = file_size;

            while (bytes_remaining > 0)
            {
                uint32_t read_size = (bytes_remaining > sizeof(buffer)) ? sizeof(buffer) : bytes_remaining;
                fseek(disk, cluster_to_offset(file_cluster), SEEK_SET);
                bytes_read = fread(buffer, 1, read_size, disk);

                if (bytes_read == 0)
                {
                    fprintf(stderr, "Erro: Falha ao ler o arquivo de origem '%s'.\n", source_path);
                    fclose(source_file);
                    return -1;
                }

                bytes_written = fwrite(buffer, 1, bytes_read, source_file);
                if (bytes_written != bytes_read)
                {
                    fprintf(stderr, "Erro: Falha ao escrever no arquivo de destino '%s'.\n", target_path);
                    fclose(source_file);
                    return -1;
                }

                bytes_remaining -= bytes_read;
                file_cluster = g_fat[file_cluster * 4] & 0x0FFFFFFF;

                // Verifica se chegou ao fim do arquivo
                if (file_cluster >= 0x0FFFFFF8)
                {
                    break;
                }
            }

            fclose(source_file);
            printf("Arquivo '%s' copiado para '%s' com sucesso.\n", source_path, target_path);
            return 0;
        }
    }
    else
    {
        // Copia o arquivo do host para a imagem
        source_file = fopen(source_path, "rb");
        if (!source_file)
        {
            fprintf(stderr, "Erro: Não foi possível abrir o arquivo de origem '%s'.\n", source_path);
            return -1;
        }

        // Verifica se o arquivo já existe no diretório
        Directory file_entry;
        if (find_file_in_directory(disk, actual_dir, target_path, &file_entry) == 0)
        {
            fprintf(stderr, "Erro: O arquivo '%s' já existe na imagem.\n", target_path);
            fclose(source_file);
            return -1;
        }

        // Cria o arquivo na imagem
        if (touch(target_path, disk, actual_dir) != 0)
        {
            fprintf(stderr, "Erro: Falha ao criar o arquivo de destino '%s'.\n", target_path);
            fclose(source_file);
            return -1;
        }

        // Busca o arquivo criado na imagem
        if (find_file_in_directory(disk, actual_dir, target_path, &file_entry) != 0)
        {
            fprintf(stderr, "Erro: Arquivo de destino '%s' não encontrado na imagem.\n", target_path);
            fclose(source_file);
            return -1;
        }

        // Copia os dados do arquivo do host para a imagem
        uint32_t file_cluster = file_entry.DIR_FstClusLO | (file_entry.DIR_FstClusHI << 16);
        uint32_t file_size = 0;
        uint32_t bytes_remaining = 0;

        while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
        {
            fseek(disk, cluster_to_offset(file_cluster), SEEK_SET);
            fwrite(buffer, 1, bytes_read, disk);

            file_size += bytes_read;
            bytes_remaining += bytes_read;

            // Aloca um novo cluster se necessário
            if (bytes_remaining >= g_bpb.BPB_SecPerClus * g_bpb.BPB_BytsPerSec)
            {
                uint32_t new_cluster;
                if (allocate_cluster(disk, &new_cluster) == -1)
                {
                    fprintf(stderr, "Erro: Sem espaço para alocar um novo cluster.\n");
                    fclose(source_file);
                    return -1;
                }

                g_fat[file_cluster * 4] = new_cluster;
                file_cluster = new_cluster;
                bytes_remaining = 0;
            }
        }

        // Atualiza o tamanho do arquivo na entrada do diretório
        file_entry.DIR_FileSize = file_size;
        uint32_t cluster = actual_dir->DIR_FstClusLO;
        uint32_t offset = cluster_to_offset(cluster);
        fseek(disk, offset, SEEK_SET);
        fwrite(&file_entry, sizeof(Directory), 1, disk);

        fclose(source_file);
        printf("Arquivo '%s' copiado para '%s' com sucesso.\n", source_path, target_path);
        return 0;
    }
}
int mv(const char *source_path, const char *target_path, FILE *disk, Directory *actual_dir)
{
    // Primeiro, copia o arquivo
    if (cp(source_path, target_path, disk, actual_dir) != 0)
    {
        fprintf(stderr, "Erro: Falha ao copiar o arquivo '%s'.\n", source_path);
        return -1;
    }

    // Depois, remove o arquivo de origem
    Directory file_entry;
    if (source_path[0] == '/')
    {
        // Remove o arquivo de origem a partir do diretório raiz
        if (find_file_in_directory(disk, &g_RootDirectory, source_path + 1, &file_entry) != 0)
        {
            fprintf(stderr, "Erro: Arquivo de origem '%s' não encontrado na imagem.\n", source_path);
            return -1;
        }
    }
    else
    {
        // Remove o arquivo de origem a partir do diretório atual
        if (find_file_in_directory(disk, actual_dir, source_path, &file_entry) != 0)
        {
            fprintf(stderr, "Erro: Arquivo de origem '%s' não encontrado no diretório atual.\n", source_path);
            return -1;
        }
    }

    // Marca a entrada como excluída
    file_entry.DIR_Name[0] = 0xE5;
    uint32_t cluster = actual_dir->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);
    fwrite(&file_entry, sizeof(Directory), 1, disk);

    // Libera os clusters na FAT
    uint32_t file_cluster = file_entry.DIR_FstClusLO | (file_entry.DIR_FstClusHI << 16);
    while (file_cluster < 0x0FFFFFF8)
    {
        uint32_t next_cluster = g_fat[file_cluster * 4] & 0x0FFFFFFF;
        g_fat[file_cluster * 4] = 0x00000000; // Marca como livre
        file_cluster = next_cluster;
    }

    printf("Arquivo '%s' movido para '%s' com sucesso.\n", source_path, target_path);
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
int rm(FILE *disk, Directory *dir, const char *file_name)
{
    if (!disk || !dir || !file_name)
    {
        fprintf(stderr, "Erro: Parâmetros inválidos para rm().\n");
        return -1;
    }

    // Converter o nome do arquivo para o formato 8.3
    char formatted_name[12] = {0};
    convert_to_8dot3(file_name, formatted_name);

    // Encontrar o cluster inicial do diretório atual
    uint32_t cluster = dir->DIR_FstClusLO;
    uint32_t offset = cluster_to_offset(cluster);
    fseek(disk, offset, SEEK_SET);

    // Ler as entradas do diretório
    int max_entries = g_bpb.BPB_BytsPerSec * g_bpb.BPB_SecPerClus / sizeof(Directory);
    Directory *entries = malloc(max_entries * sizeof(Directory));
    if (!entries)
    {
        fprintf(stderr, "Erro: Falha ao alocar memória para leitura do diretório.\n");
        return -1;
    }

    fread(entries, sizeof(Directory), max_entries, disk);

    // Procurar o arquivo no diretório
    int found = 0;
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

        if (!(entries[i].DIR_Attr & ATTR_DIRECTORY))
        {
            char entry_name[12] = {0};
            memcpy(entry_name, entries[i].DIR_Name, 11);

            if (strncmp(entry_name, formatted_name, 11) == 0)
            {
                // Marcar a entrada como excluída
                entries[i].DIR_Name[0] = 0xE5;

                // Escrever a entrada modificada de volta no disco
                fseek(disk, offset + (i * sizeof(Directory)), SEEK_SET);
                fwrite(&entries[i], sizeof(Directory), 1, disk);

                // Liberar os clusters associados ao arquivo na FAT
                uint32_t file_cluster = entries[i].DIR_FstClusLO | (entries[i].DIR_FstClusHI << 16);
                while (file_cluster < 0x0FFFFFF8)
                {
                    uint32_t next_cluster;
                    memcpy(&next_cluster, &g_fat[file_cluster * 4], 4);
                    next_cluster &= 0x0FFFFFFF;

                    // Marcar o cluster como livre na FAT
                    uint32_t fat_entry = 0x00000000;
                    uint32_t fat_offset = g_bpb.BPB_RsvdSecCnt * g_bpb.BPB_BytsPerSec + file_cluster * 4;
                    fseek(disk, fat_offset, SEEK_SET);
                    fwrite(&fat_entry, sizeof(uint32_t), 1, disk);

                    file_cluster = next_cluster;
                }

                found = 1;
                break;
            }
        }
    }

    free(entries);

    if (!found)
    {
        fprintf(stderr, "Erro: Arquivo '%s' não encontrado.\n", file_name);
        return -1;
    }

    printf("Arquivo '%s' removido com sucesso.\n", file_name);
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

    printf("Diretório '%s' removido com sucesso.\n", name);
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
                        printf("Diretório '%s' removido com sucesso.\n", name);
                    }
                    else
                    {
                        printf("Erro ao remover o diretório '%s'.\n", name);
                    }
                }
            }
            else if (strncmp(comando, "rm", 2) == 0)
            {
                if (strlen(comando) <= 3)
                {
                    printf("Erro: Nome do arquivo inválido.\n");
                }
                else
                {
                    char *file_name = comando + 3; // Extrai o nome do arquivo após "rm "
                    if (rm(disk, &actual_dir, file_name) == 0)
                    {
                        printf("Arquivo '%s' removido com sucesso.\n", file_name);
                    }
                    else
                    {
                        printf("Erro ao remover o arquivo '%s'.\n", file_name);
                    }
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
            // mv <source> <target>
            else if (strncmp(comando, "mv", 2) == 0)
            {
                char source[100], target[100];
                if (sscanf(comando + 3, "%s %s", source, target) == 2)
                {
                    // Verifica se o destino é um diretório dentro da imagem (começa com "img/")
                    if (strncmp(target, "img/", 4) == 0)
                    {
                        // Remove o prefixo "img/" para operar dentro da imagem
                        const char *internal_target = target + 4;

                        // Move o arquivo dentro da imagem
                        if (mv(source, internal_target, disk, &actual_dir) != 0)
                        {
                            printf("Erro ao mover '%s' para '%s'.\n", source, target);
                        }
                        else
                        {
                            printf("Arquivo '%s' movido para '%s' com sucesso.\n", source, target);
                        }
                    }
                    else
                    {
                        // Move o arquivo para o sistema de arquivos do host
                        if (mv(source, target, disk, &actual_dir) != 0)
                        {
                            printf("Erro ao mover '%s' para '%s'.\n", source, target);
                        }
                        else
                        {
                            printf("Arquivo '%s' movido para '%s' com sucesso.\n", source, target);
                        }
                    }
                }
                else
                {
                    printf("Uso: mv <caminho_origem> <caminho_destino>\n");
                }
            }
            // cp <source><target>
            // cp <source> <target>
            else if (strncmp(comando, "cp", 2) == 0)
            {
                char source[100], target[100];
                if (sscanf(comando + 3, "%s %s", source, target) == 2)
                {
                    // Verifica se o destino é um diretório dentro da imagem (começa com "img/")
                    if (strncmp(target, "img/", 4) == 0)
                    {
                        // Remove o prefixo "img/" para operar dentro da imagem
                        const char *internal_target = target + 4;

                        // Copia o arquivo para dentro da imagem
                        if (cp(source, internal_target, disk, &actual_dir) != 0)
                        {
                            printf("Erro ao copiar '%s' para '%s'.\n", source, target);
                        }
                        else
                        {
                            printf("Arquivo '%s' copiado para '%s' com sucesso.\n", source, target);
                        }
                    }
                    else
                    {
                        // Copia o arquivo para o sistema de arquivos do host
                        if (cp(source, target, disk, &actual_dir) != 0)
                        {
                            printf("Erro ao copiar '%s' para '%s'.\n", source, target);
                        }
                        else
                        {
                            printf("Arquivo '%s' copiado para '%s' com sucesso.\n", source, target);
                        }
                    }
                }
                else
                {
                    printf("Uso: cp <arquivo_origem> <caminho_destino>\n");
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
