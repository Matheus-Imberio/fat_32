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
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

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

Directory g_RootDirectory;

int read_root_directory(FILE* disk);
Directory* find_file(const char* name);
int read_file(FILE* disk, Directory* file, void* buffer);
int pwd();
int dir_attr(char* name, FILE* disk);
int ls(FILE* disk, Directory* dir);
int cd(const char* name, FILE* disk, Directory* actual_dir);
int rm(FILE *disk, Directory *dir, const char *filename);
int touch(const char *name, FILE *disk, Directory *actual_cluster);
