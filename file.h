#include <stdint.h>

typedef struct
{
  uint16_t cluster;
  uint32_t fileSize;
  uint16_t createTime;
  uint16_t writeTime;
  uint16_t createDate;
  uint16_t writeDate;
  uint16_t lastAccDate;
  char name[11];
  uint8_t attr;
} stat_t;

typedef struct
{
  uint32_t cluster;
  uint32_t sector;
  uint32_t uint8_t;
  stat_t *stat;
} file_t;

int file_attr(file_t *file);
int touch(char *name);