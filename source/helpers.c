#include <malloc.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Used with ISFS_GetFile.
static fstats stats ATTRIBUTE_ALIGN(32);

void hexDump(void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = addr;

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("  %s\n", buff);

            // Output the offset.
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  %s\n", buff);
}

// The author apologizes for the amount of error catching here.
// In the end, blame Nintendo.
u8 *ISFS_GetFile(const char *path, u32 *size) {
    *size = 0;
    printf("ISFS_GetFile: reading %s\n", path);
    s32 fd = ISFS_Open(path, ISFS_OPEN_READ);
    u8 *buf = NULL;

    if (fd >= 0) {
        memset(&stats, 0, sizeof(fstats));

        s32 ret = ISFS_GetFileStats(fd, &stats);
        if (ret >= 0) {
            s32 length = stats.file_length;
            buf = (u8 *)memalign(32, length);

            if (buf) {
                u32 tmp_size = ISFS_Read(fd, (char *)buf, length);

                if (tmp_size == length) {
                    // We were successful.
                    *size = tmp_size;
                } else {
                    // If positive, the file could not be fully read.
                    // If negative, it is most likely an underlying /dev/fs
                    // error.
                    if (tmp_size >= 0) {
                        printf("ISFS_GetFile: only able to read %d out of "
                               "%d bytes!\n",
                               tmp_size, length);
                    } else {
                        printf("ISFS_GetFile: ISFS_Open reported error %d\n",
                               tmp_size);
                    }

                    free(buf);
                }
            } else {
                printf("ISFS_GetFile: failed to allocate buffer!\n");
            }
        } else {
            printf("ISFS_GetFile: unable to retrieve file stats (error %d)\n",
                   ret);
        }
        ISFS_Close(fd);
    } else {
        printf("ISFS_GetFile: unable to open file (error %d)\n", fd);
    }
    return buf;
}
