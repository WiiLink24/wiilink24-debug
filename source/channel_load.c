#include <malloc.h>
#include <ogc/es.h>
#include <ogcsys.h>
#include <stdio.h>
#include <string.h>

#include "certs_dat.h"
#include "channel_load.h"
#include "common.h"
#include "helpers.h"
#include "memory_layout.h"

u8 channelios;
u16 bootindex;
u32 bootid = 0xFFFFFFFF;
u64 channelId;

// Much of the following code is derived (from)
// https://github.com/conanac/wiilauncher/blob/4f001975d658d18a27c40043520084a5556463ee/source/identify.c.
bool load_channel_metadata(u64 titleId) {
    char filename[256];

    // Load the TMD, ticket stored on NAND, and console certificates. We'll
    // present this to ES.
    if (ISFS_Initialize() < 0) {
        return false;
    }

    u32 tikSize = 0;
    snprintf(filename, sizeof(filename), "/ticket/%08x/%08x.tik",
             TITLE_UPPER(titleId), TITLE_LOWER(titleId));
    u8 *tikbuf = ISFS_GetFile(filename, &tikSize);
    if (tikbuf == NULL || tikSize == 0) {
        printf("failed reading ticket!\n");
        ISFS_Deinitialize();
        return false;
    }

    u32 tmdSize = 0;
    snprintf(filename, sizeof(filename), "/title/%08x/%08x/content/title.tmd",
             TITLE_UPPER(titleId), TITLE_LOWER(titleId));
    u8 *tmdbuf = ISFS_GetFile(filename, &tmdSize);
    if (tmdbuf == NULL || tmdSize == 0) {
        printf("failed reading TMD!\n");
        ISFS_Deinitialize();
        return false;
    }

    u32 keyId = 0;
    s32 ret = ES_Identify((signed_blob *)certs_dat, certs_dat_size,
                          (signed_blob *)tmdbuf, tmdSize, (signed_blob *)tikbuf,
                          tikSize, &keyId);
    ISFS_Deinitialize();

    if (ret < 0) {
        printf("failed telling ES we have changed titles! (error %d)\n", ret);
        return false;
    }

    // Strip the signature from the TMD. We do not care about it.
    // This is 1:1 to what ES_GetStoredTMD would return if it were to be used.
    signed_blob *signedTmd = (signed_blob *)tmdbuf;
    tmd *titleTmd = SIGNATURE_PAYLOAD(signedTmd);

    // Collect information we can deal with.
    channelios = titleTmd->sys_version & 0xFF;
    bootindex = titleTmd->boot_index;
    for (int i = 0; i < titleTmd->num_contents; i++) {
        // Find the content ID matching the index.
        if (titleTmd->contents[i].index == bootindex)
            bootid = titleTmd->contents[i].cid;
    }

    if (bootid == 0xFFFFFFFF) {
        printf("unable to find a bootable DOL.\n"
               "Are you sure the installed ticket is valid?\n");
        return false;
    }

    channelId = titleId;

    printf("ready to roll with IOS %d.\n", TITLE_LOWER(titleTmd->sys_version));
    printf("we will boot index %d (content ID %08x).\n", titleTmd->boot_index,
           bootid);

    return true;
}

void *load_channel_dol() {
    char filename[256];

    // Prepare to read.
    if (ISFS_Initialize()) {
        return NULL;
    }

    snprintf(filename, sizeof(filename), "/title/%08x/%08x/content/%08x.app",
             TITLE_UPPER(channelId), TITLE_LOWER(channelId), bootid);

    u32 dolSize = 0;
    u8 *doldata = ISFS_GetFile(filename, &dolSize);
    if (doldata == NULL || dolSize == 0) {
        printf("failed to load channel DOL!\n");
        return NULL;
    }

    // TODO: determine if compression is needed

    // Copy the header from the DOL we've just retrieved from NAND.
    static dolheader dol_header[1] ATTRIBUTE_ALIGN(32);
    memcpy(dol_header, doldata, sizeof(dolheader));

    // Keep note of the entrypoint for later.
    void *bootentrypoint = dol_header->entry_point;

    // The entrypoint isn't always an absolute address.
    printf("entrypoint noted as being at 0x%p.\n", bootentrypoint);

    // Zero out BSS ahead of time.
    memset(dol_header->bss_start, 0, dol_header->bss_size);

    // Copy executable segments.
    for (u32 i = 0; i < 18; i++) {
        u32 data_pos = dol_header->section_pos[i];
        void *data_start = dol_header->section_start[i];
        u32 data_size = dol_header->section_size[i];

        // Section is disabled.
        if (!data_size)
            continue;
        // Sections can't possibly be before their header.
        if (sizeof(dolheader) > data_pos)
            continue;

        // Ensure the section has a proper start address.
        if ((u32)data_start <= 0x80000000)
            data_start += 0x80000000;

        // Copy from the read position to the read start for the read length.
        memcpy(data_start, (void *)(doldata + data_pos), data_size);

        // Flush as much as possible.
        DCFlushRange(data_start, data_size);
        ICInvalidateRange(data_start, data_size);
        printf("section [%d] pos=0x%08x start=0x%p size=0x%08x\n", i + 1,
               data_pos, data_start, data_size);
    }

    // TODO: apply patches here

    ISFS_Deinitialize();

    memset(doldata, 0, dolSize);
    free(doldata);

    // Set disc ID in memory, while channelId is accessible.
    *Disc_ID = TITLE_LOWER(channelId);

    return bootentrypoint;
}
