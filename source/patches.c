#include <assert.h>
#include <gccore.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "memory_layout.h"
#include "patches.h"

#define AHBPROT_ENABLED read32(HW_AHBPROT) != 0xFFFFFFFF
#define ES_MODULE_START (u32 *)0x933E0000
#define ES_MODULE_END (u32 *)0x93FFFFFF

// This patch allows us to read tickets/TMDs/so forth.
static const u16 isfs_permissions_old[] = {0x428B, 0xD001, 0x2566};
static const u16 isfs_permissions_patch[] = {0x428B, 0xE001, 0x2566};

// This patch is used to allow us to identify regardless of our UID.
// The patched function appears to obtain UID - ES_LaunchTitle appears to
// utilize it as well. While almost all all of this is just function prologue,
// it is unique to ES_Identify/ES_DiVerify.
static const u16 es_identify_old[] = {
    0xb5f0, // push { r4, r5, r6, r7, lr }
    0x465f, // mov r7, r11
    0x4656, // mov r6, r10
    0x464d, // mov r5, r9
    0x4644, // mov r4, r8
    0xb4f0, // push { r4, r5, r6, r7 }
    0xb088, // sub sp,#0x20
    0x9006  // str r0,[sp,#local_2c]
};

static const u16 es_identify_patch[] = {
    0x2000, // mov r0, #0x0 ; return 0
    0x4770, // bx lr        ; branch to previous
    0xbf00, // nop
    0xbf00, // nop
    0xbf00, // nop
    0xbf00, // (well, you get the gist.)
    0xbf00, //
    0xbf00  //
};

// This patch allows us to gain access to the AHBPROT register.
static const u16 ticket_check_old[] = {
    0x685B,         // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052, // movls r2, 0x1D8
    0x189B,         // adds r3, r3, r2; add offset of access rights field in TMD
    0x681B,         // ldr r3, [r3]   ; load access rights (haxxme!)
    0x4698,         // mov r8, r3  ; store it for the DVD video bitcheck later
    0x07DB          // lsls r3, r3, #31; check AHBPROT bit
};
static const u16 ticket_check_patch[] = {
    0x685B,         // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052, // movls r2, 0x1D8
    0x189B,         // adds r3, r3, r2; add offset of access rights field in TMD
    0x23FF,         // li r3, 0xFF  ; <--- 0xFF gives us all access bits
    0x4698,         // mov r8, r3  ; store it for the DVD video bitcheck later
    0x07DB          // lsls r3, r3, #31; check AHBPROT bit
};

// If a new IOS patch is added, please update accordingly.
#define ISFS_PERMISSIONS_SIZE sizeof(isfs_permissions_patch)
#define ES_IDENTIFY_SIZE sizeof(es_identify_patch)
#define TICKET_CHECK_SIZE sizeof(ticket_check_patch)

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

bool patch_ahbprot_reset(void) {
    // Check if we've already disabled AHBPROT.
    // AHBPROT may already be disabled, depending on the user's IOS.
    if (AHBPROT_ENABLED) {
        return true;
    }

    // We'll need to disable MEM2 protections in order to write over IOS.
    disable_memory_protections();

    bool patched =
        patch_memory_range(ES_MODULE_START, ES_MODULE_END, ticket_check_old,
                           ticket_check_patch, TICKET_CHECK_SIZE);

    if (!patched) {
        printf("unable to find and patch ES memory!\n");
        return false;
    }

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

bool apply_patches() {
    // Ensure once more that we can patch memory.
    // IOS reloading may have changed state.
    disable_memory_protections();

    bool isfs_patch =
        patch_memory_range(ES_MODULE_START, ES_MODULE_END, isfs_permissions_old,
                           isfs_permissions_patch, ISFS_PERMISSIONS_SIZE);
    if (!isfs_patch) {
        printf("unable to find and patch ISFS permissions!\n");
        return false;
    }

    bool es_patch =
        patch_memory_range(ES_MODULE_START, ES_MODULE_END, es_identify_old,
                           es_identify_patch, ES_IDENTIFY_SIZE);
    if (!es_patch) {
        printf("unable to find and patch ES_Identify!\n");
        return false;
    }

    return true;
}
