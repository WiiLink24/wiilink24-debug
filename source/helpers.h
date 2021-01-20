#include <ogcsys.h>

#define TITLE_UPPER(x) ((u32)((x) >> 32))
#define TITLE_LOWER(x) ((u32)(x)&0xFFFFFFFF)

void hexDump(void *addr, int len);
void *ISFS_GetFile(const char *path, u32 *size);
