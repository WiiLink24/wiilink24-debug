#include <gccore.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "memory_layout.h"
#include "patches.h"

#define AHBPROT_ENABLED read32(HW_AHBPROT) != 0xFFFFFFFF
#define IOS_MEMORY_START (u32 *)0x933E0000
#define IOS_MEMORY_END (u32 *)0x93FFFFFF

// This patch allows us to read tickets/TMDs/so forth.
static const u16 isfs_permissions_old[] = {0x428B, 0xD001, 0x2566};
static const u16 isfs_permissions_patch[] = {0x428B, 0xE001, 0x2566};

// This patch is used to allow us to identify regardless of our UID.
// We patch the start of this in order to be IOS-agnostic, as immediately
// following is a branch whose address is not guaranteed to always be the same.
// For this reason, we may also patch ES_DiVerifyWithTicketView's condition
// within the main ES Ioctlv handler.
// (No issue there. We will never call it, and IOS should be reloaded soon.)
static const u16 es_identify_old[] = {
    0x68cc, // ldr r4, [r1, #0xc]
    0x69a6, // ldr r6, [r4, #0x18]
    0x6868, // ldr r0, [r5, #0x4] ; unfortunately, no clue what value this is
    0x2803  // cmp r0, #0x3
};
static const u16 es_identify_patch[] = {
    0x68cc, // ldr r4, [r1, #0xc]
    0x69a6, // ldr r6, [r4, #0x18]
    0x2003, // mov r0, #0x3 ; always true
    0x2803  // cmp r0, #0x3
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

// This patch returns success to all signatures.
static const u16 ios_verify_old[] = {
    0xb5f0, // push { r4, r5, r6, r7, lr }
    0x4657, // mov r7, r10
    0x464e, // mov r6, r9
    0x4645, // mov r5, r8
    0xb4e0, // push { r5, r6, r7 }
    0xb083, // sub sp, #0xc
    0x2400  // mov r4, #0x0
};
static const u16 ios_verify_patch[] = {
    0x2000, // mov r0, #0x0
    0x4770, // bx lr
    0xb000, // nop
    0xb000, // nop
    0xb000, // nop
    0xb000, // nop
    0xb000  // nop
};

// If a new IOS patch is added, please update accordingly.
#define ISFS_PERMISSIONS_SIZE sizeof(isfs_permissions_patch)
#define ES_IDENTIFY_SIZE sizeof(es_identify_patch)
#define IOS_VERIFY_SIZE sizeof(ios_verify_patch)
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

bool patch_ios_range(const u16 original_patch[], const u16 new_patch[],
                     u32 patch_size) {
    return patch_memory_range(IOS_MEMORY_START, IOS_MEMORY_END, original_patch,
                              new_patch, patch_size);
}

bool patch_ahbprot_reset(void) {
    // Check if we've already disabled AHBPROT.
    // AHBPROT may already be disabled, depending on the user's IOS.
    if (AHBPROT_ENABLED) {
        return true;
    }

    // We'll need to disable MEM2 protections in order to write over IOS.
    disable_memory_protections();

    bool patched = patch_ios_range(ticket_check_old, ticket_check_patch,
                                   TICKET_CHECK_SIZE);

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
