#include <ogcsys.h>

typedef struct _dolheader {
    u32 section_pos[18];
    void *section_start[18];
    u32 section_size[18];
    void *bss_start;
    u32 bss_size;
    void *entry_point;
    u32 padding[7];
} __attribute__((packed)) dolheader;

bool load_channel_metadata(u64 titleID);
void *load_channel_dol();
