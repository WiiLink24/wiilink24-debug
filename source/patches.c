#include <gccore.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "memory_layout.h"
#include "patches.h"

#define AHBPROT_DISABLED read32(HW_AHBPROT) == 0xFFFFFFFF
#define IOS_MEMORY_START (u32 *)0x933E0000
#define IOS_MEMORY_END (u32 *)0x93FFFFFF

void disable_memory_protections() { write16(MEM2_PROT, 2); }

bool patch_memory_range(u32 *start, u32 *end, const u16 original_patch[],
                        const u16 new_patch[], u32 patch_size) {
    bool patched = false;

    for (u32 *patchme = start; patchme < end; ++patchme) {
        if (memcmp(patchme, original_patch, patch_size) == 0) {
            // Copy our new patch over the existing, and flush.
            memcpy(patchme, new_patch, patch_size);
            DCFlushRange(patchme, patch_size);

            // While this realistically won't do anything for some parts,
            // it's worth a try...
            ICInvalidateRange(patchme, patch_size);

            printf("patched %d bytes at %p\n", patch_size, patchme);

            patched = true;
        }
    }

    return patched;
}

bool patch_ios_range(const u16 original_patch[], const u16 new_patch[],
                     u32 patch_size) {
    return patch_memory_range(IOS_MEMORY_START, IOS_MEMORY_END, original_patch,
                              new_patch, patch_size);
}

bool reload_current_ios() {
    s32 current_ios = IOS_GetVersion();
    if (current_ios < 0) {
        printf("unable to get current IOS version! (error %d)\n", current_ios);
        return false;
    }

    s32 ios_result = IOS_ReloadIOS(current_ios);
    if (ios_result < 0) {
        printf("unable to reload IOS version! (error %d)\n", ios_result);
        return false;
    }

    return true;
}

bool patch_ahbprot_reset() {
    // We'll need to disable MEM2 protections in order to write over IOS.
    disable_memory_protections();

    bool patched = patch_ios_range(ticket_check_old, ticket_check_patch,
                                   TICKET_CHECK_SIZE);
    if (!patched) {
        printf("unable to find and patch ES memory!\n");
        return false;
    }

    if (!reload_current_ios()) {
        printf("failed to reload IOS!\n");
        return false;
    }

    return true;
}

bool apply_patches() {
    // Check if we've already disabled AHBPROT.
    // AHBPROT may already be disabled, depending on the user's IOS.
    if (AHBPROT_DISABLED) {
        bool ahbprot_fix = patch_ahbprot_reset();
        if (!ahbprot_fix) {
            // patch_ahbprot_reset should log its own errors.
            return false;
        }
    }

    // Next, ensure that we can patch memory.
    // IOS reloading may have changed state.
    disable_memory_protections();

    bool isfs_patched = patch_ios_range(
        isfs_permissions_old, isfs_permissions_patch, ISFS_PERMISSIONS_SIZE);
    if (!isfs_patched) {
        printf("unable to find and patch ISFS permissions!\n");
        return false;
    }

    bool es_patched =
        patch_ios_range(es_identify_old, es_identify_patch, ES_IDENTIFY_SIZE);
    if (!es_patched) {
        printf("unable to find and patch ES_Identify!\n");
        return false;
    }

    bool ios_patched =
        patch_ios_range(ios_verify_old, ios_verify_patch, IOS_VERIFY_SIZE);
    if (!ios_patched) {
        printf("unable to find and patch IOSC_VerifyPublicKeySign!\n");
        return false;
    }

    return true;
}
