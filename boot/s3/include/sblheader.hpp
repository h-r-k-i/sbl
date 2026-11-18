#ifndef SBLHEADER_HPP
#define SBLHEADER_HPP

#include <stdint.h>

typedef struct __attribute__((packed)) {
    char magic[8];
    uint64_t checksum;
    uint64_t flags;
    uint64_t sgalf;
    uint64_t first;
} SBL_Data;

typedef struct __attribute__((packed)) {
    uint16_t data;
    uint16_t next;
} SBL_Node;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint16_t entry_count;
} SBL_E820_Header;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint16_t extended_memory_1k;
    uint16_t extended_memory_64k;
    uint16_t configured_memory_1k;
    uint16_t configured_memory_64k;
} SBL_E801_Map;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint16_t memory_above_1M;
} SBL_88_Map;

typedef struct __attribute__((packed)) {
    char Signature[16];
    uint16_t size;
    uint32_t VGAPointer;
    uint32_t count;
    uint32_t VGAModePointer;
    uint16_t fhd;
    char resv[16];
} SBL_DisplayInfoBlock;

static_assert(sizeof(SBL_DisplayInfoBlock) % 16 == 0, "SBL Display Info Block structure is not a multiple of 16 bytes");

typedef struct __attribute__((packed)) {
    uint16_t Attributes;
    uint8_t Window_A;
    uint8_t Window_B;
    uint16_t Granularity;
    uint16_t WindowSize;
    uint16_t Segment_A;
    uint16_t Segment_B;
    uint32_t WinFuncPtr;
    uint16_t Pitch;
    uint16_t Width;
    uint16_t Height;
    uint8_t WChar;
    uint8_t YChar;
    uint8_t Planes;
    uint8_t bpp;
    uint8_t Banks;
    uint8_t MemoryModel;
    uint8_t BankSize;
    uint8_t ImagePages;
    uint8_t resv1;
    uint8_t RedMask;
    uint8_t RedPosition;
    uint8_t GreenMask;
    uint8_t GreenPosition;
    uint8_t BlueMask;
    uint8_t BluePosition;
    uint16_t resv2;
    uint8_t DirectColorAttributes;
    uint32_t FramebufferAddress;
    uint32_t OffScreenMemOffset;
    uint16_t OffScreenMemSize;
    uint8_t resv3[206];
} VBEModeInfoBlock;

#define SBL_BOOTLOADER_MAP_COUNT 13
#define SBL_BOOTLOADER_FREE      1
#define SBL_BOOTLOADER_RESERVED  2
#define SBL_BOOTLOADER_INUSE     3
#define SBL_BOOTLOADER_CRITICAL  4

#define SBL_FLAG_BMM_COMPLETE (1ULL << 62)
#define SBL_FLAG_VIS_COMPLETE (1ULL << 17)

enum class SBL_BMMIndex : uint8_t {
    IVT_BDA          = 0,
    PM_IVT_GDT       = 1,
    ContractArea     = 2,
    MemoryMap        = 3,
    BootloaderMap    = 4,
    DisplayInfoBlock = 5,
    VBEMIB           = 6,
    ACPI             = 7,
    SMBIOS           = 8,
    Identity         = 9,
    S1               = 10,
    S2               = 11,
    S3               = 12,
};

typedef struct __attribute__((packed)) {
    uint64_t Address;
    uint64_t Size;
    uint32_t Status;
} SBL_BootloaderMemoryMapEntry;

typedef struct __attribute__((packed)) {
    char Signature[16];
    uint32_t count;
    SBL_BootloaderMemoryMapEntry entries[SBL_BOOTLOADER_MAP_COUNT];
} SBL_BMM;

struct SBL_Region {
    uint32_t address;
    uint32_t size;
};

enum class SBLMapResult {
    Good,
    BadMagic,
    BadChecksum,
    MapIncomplete,
    MapNotFound,
    BadMapCount,
    BadStatus,
    BadRange,
};

static const char BMM_SIGNATURE[16] = "bmm.bmm.bmm.bmm";
static const char DISPLAY_SIGNATURE[16] = {'v','i','s','b','v','i','s','b','v','i','s','b','v','i','s','b'};
static const uint16_t MAX_NODES = 64;

static bool SignatureMatches(const char* p) {
    for (uint8_t i = 0; i < sizeof(BMM_SIGNATURE); i++) if (p[i] != BMM_SIGNATURE[i]) return false;
    return true;
}

static bool SignatureMatchesDisplay(const char* p) {
    for (uint8_t i = 0; i < sizeof(DISPLAY_SIGNATURE); i++) if (p[i] != DISPLAY_SIGNATURE[i]) return false;
    return true;
}

SBLMapResult SBL_FindBMM(uint32_t sblData, SBL_BMM*& bmm) {
    bmm = nullptr;
    SBL_Data* header = (SBL_Data*)sblData;

    const char* magic = "NEKONEKO";
    for (uint8_t i = 0; i < 8; i++) if (header->magic[i] != magic[i]) return SBLMapResult::BadMagic;

    uint64_t sum = *(uint64_t*)(header->magic) + header->flags + header->sgalf + header->checksum;
    if (sum != 0xFFFFFFFFFFFFFFFFULL) return SBLMapResult::BadChecksum;

    if ((header->flags & SBL_FLAG_BMM_COMPLETE) == 0) return SBLMapResult::MapIncomplete;

    uint16_t nodeAddr = (uint16_t)header->first;
    for (uint16_t n = 0; n < MAX_NODES && nodeAddr != 0; n++) {
        SBL_Node* node = (SBL_Node*)(uint32_t)nodeAddr;
        if (node->data != 0 && SignatureMatches((const char*)(uint32_t)node->data)) {
            bmm = (SBL_BMM*)(uint32_t)node->data;
            if (bmm->count != SBL_BOOTLOADER_MAP_COUNT) return SBLMapResult::BadMapCount;
            return SBLMapResult::Good;
        }
        nodeAddr = node->next;
    }
    return SBLMapResult::MapNotFound;
}

SBLMapResult SBL_FindVisualArea(uint32_t sblData, SBL_DisplayInfoBlock*& vib) {
    vib = nullptr;
    SBL_Data* header = (SBL_Data*)sblData;

    const char* magic = "NEKONEKO";
    for (uint8_t i = 0; i < 8; i++) if (header->magic[i] != magic[i]) return SBLMapResult::BadMagic;

    uint64_t sum = *(uint64_t*)(header->magic) + header->flags + header->sgalf + header->checksum;
    if (sum != 0xFFFFFFFFFFFFFFFFULL) return SBLMapResult::BadChecksum;

    if ((header->flags & SBL_FLAG_VIS_COMPLETE) == 0) return SBLMapResult::MapIncomplete;

    uint16_t nodeAddr = (uint16_t)header->first;
    for (uint16_t n = 0; n < MAX_NODES && nodeAddr != 0; n++) {
        SBL_Node* node = (SBL_Node*)(uint32_t)nodeAddr;
        if (node->data != 0 && SignatureMatchesDisplay((const char*)(uint32_t)node->data)) {
            vib = (SBL_DisplayInfoBlock*)(uint32_t)node->data;
            if (vib->fhd == 0xFFFF) return SBLMapResult::BadStatus;
            if (vib->size != sizeof(SBL_DisplayInfoBlock)) return SBLMapResult::BadRange;
            return SBLMapResult::Good;
        }
        nodeAddr = node->next;
    }
    return SBLMapResult::MapNotFound;
}

SBLMapResult SBL_GetPMIVTGDTArea(uint32_t sblData, SBL_Region& region) {
    region = {0, 0};

    SBL_BMM* bmm;
    SBLMapResult result = SBL_FindBMM(sblData, bmm);
    if (result != SBLMapResult::Good) return result;

    SBL_BootloaderMemoryMapEntry& entry = bmm->entries[(uint8_t)SBL_BMMIndex::PM_IVT_GDT];
    if (entry.Status != SBL_BOOTLOADER_CRITICAL) return SBLMapResult::BadStatus;

    if (entry.Size == 0 || entry.Address + entry.Size > 0x10000) return SBLMapResult::BadRange;

    region.address = (uint32_t)entry.Address;
    region.size = (uint32_t)entry.Size;
    return SBLMapResult::Good;
}

#endif // SBLHEADER_HPP