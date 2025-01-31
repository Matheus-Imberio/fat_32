#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "file.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

typedef struct __attribute__((packed))
{
  uint8_t DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t DIR_NTRes;
  uint8_t DIR_CrtTimeTenth;
  uint16_t DIR_CrtTime;
  uint16_t DIR_CrtDate;
  uint16_t DIR_LstAccDate;
  uint16_t DIR_FstClusHI;
  uint16_t DIR_WrtTime;
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClusLO;
  uint32_t DIR_FileSize;
} Directory;

typedef struct
{
  uint8_t LDIR_Ord;        // Ordem da entrada (bit 6 indica a última entrada)
  uint16_t LDIR_Name1[5];  // Primeiros 5 caracteres do nome
  uint8_t LDIR_Attr;       // Atributo (sempre 0x0F para LFN)
  uint8_t LDIR_Type;       // Sempre 0x00
  uint8_t LDIR_Chksum;     // Checksum do nome curto associado
  uint16_t LDIR_Name2[6];  // Próximos 6 caracteres do nome
  uint16_t LDIR_FstClusLO; // Sempre 0 para LFN
  uint16_t LDIR_Name3[2];  // Últimos 2 caracteres do nome
} LongDirEntry;

uint8_t *g_fat = NULL;

int read_fat(FILE *disk);
int read_cluster(FILE *disk, uint32_t cluster);

Directory g_RootDirectory;

int read_root_directory(FILE *disk);
Directory *find_file(const char *name);
int read_file(FILE *disk, Directory *file, void *buffer);
int pwd();
int dir_attr(char *name, FILE *disk);
int cd(const char *name, FILE *disk, Directory *actual_dir);
int touch(const char *name, FILE *disk, Directory *actual_cluster);
int ls(FILE *disk, Directory *dir);
int mv(FILE *disk, Directory *dir, const char *source_name, const char *target_dir_name);
int cp(FILE *disk, Directory *dir, const char *source_name, const char *target_dir_name);
int rename_file(FILE *disk, Directory *dir, const char *file_name, const char *new_name);
int allocate_cluster(FILE *disk, uint32_t *new_cluster);
int mkdir(const char *name, Directory *parent, FILE *disk);
void convert_to_8dot3(const char *input, char *output);
void trim_whitespace(char *str);
