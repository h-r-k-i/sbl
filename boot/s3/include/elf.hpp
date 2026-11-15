/*******************************************************************************************
 * 
 * FILENAME:    ./boot/s3/include/elf.hpp
 * NAME:        SBL, Stage 3, ELF Header
 * DESCRIPTION: ELF header structures and constants for the SBL Stage 3 bootloader.
 * AUTHOR:      Haruki Tokumei, Haruki Media Group
 * DATE:        14 November 2026
 * COPYRIGHT:   Haruki Media Group.
 * LICENSE:     MIT License (./LICENSE)
 *******************************************************************************************/


#ifndef ELF_HPP
#define ELF_HPP

#include <stdint.h>

#define ELF_32 1
#define ELF_64 2

#define ELF_LITTLE_ENDIAN 1
#define ELF_BIG_ENDIAN 2 // not like the x86 bootloader gives a shit about bigend but still

#define ET_EXEC 2
#define ET_DYN 3

#define EM_86 0x03
#define EM_8664 0x3E

#define PN_XNUM 0xFFFF

#define PT_LOAD 0x01
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

#define SBL_MAX_REGION 8192 // max bytes SBL will check

#define MAJOR 26
#define MINOR 9

struct ELFHeader {
    uint8_t magic[4]; // Magic number
    uint8_t class_;   // 32-bit or 64-bit
    uint8_t data;     // Endianness
    uint8_t version;  // ELF version
    uint8_t osabi;    // Operating System ABI identification
    uint8_t abiversion; // ABI version
    uint8_t pad[7];   // Padding bytes

} __attribute__((packed));

struct ELF32Header {
    ELFHeader e_ident;
    uint16_t e_type;      // Object file type
    uint16_t e_machine;   // Architecture
    uint32_t e_version;   // Object file version
    uint32_t e_entry;     // Entry point virtual address
    uint32_t e_phoff;     // Program header table file offset
    uint32_t e_shoff;     // Section header table file offset
    uint32_t e_flags;     // Processor-specific flags
    uint16_t e_ehsize;    // ELF header size in bytes
    uint16_t e_phentsize; // Program header table entry size
    uint16_t e_phnum;     // Program header table entry count
    uint16_t e_shentsize; // Section header table entry size
    uint16_t e_shnum;     // Section header table entry count
    uint16_t e_shstrndx;  // Section header string table index
} __attribute__((packed));

struct ELF64Header {
    ELFHeader e_ident;
    uint16_t e_type;      // Object file type
    uint16_t e_machine;   // Architecture
    uint32_t e_version;   // Object file version
    uint64_t e_entry;     // Entry point virtual address
    uint64_t e_phoff;     // Program header table file offset
    uint64_t e_shoff;     // Section header table file offset
    uint32_t e_flags;     // Processor-specific flags
    uint16_t e_ehsize;    // ELF header size in bytes
    uint16_t e_phentsize; // Program header table entry size
    uint16_t e_phnum;     // Program header table entry count
    uint16_t e_shentsize; // Section header table entry size
    uint16_t e_shnum;     // Section header table entry count
    uint16_t e_shstrndx;  // Section header string table index
} __attribute__((packed));

struct pheader32 {
    uint32_t p_type;   // Segment type
    uint32_t p_offset; // Segment file offset
    uint32_t p_vaddr;  // Segment virtual address
    uint32_t p_paddr;  // Segment physical address
    uint32_t p_filesz; // Segment size in file
    uint32_t p_memsz;  // Segment size in memory
    uint32_t p_flags;  // Segment flags
    uint32_t p_align;  // Segment alignment
} __attribute__((packed));

struct pheader64 {
    uint32_t p_type;   // Segment type
    uint32_t p_flags;  // Segment flags
    uint64_t p_offset; // Segment file offset
    uint64_t p_vaddr;  // Segment virtual address
    uint64_t p_paddr;  // Segment physical address
    uint64_t p_filesz; // Segment size in file
    uint64_t p_memsz;  // Segment size in memory
    uint64_t p_align;  // Segment alignment
} __attribute__((packed));

struct SBLHeader {
    uint8_t magic[8]; // "NEKONEK0"
    uint16_t major; // contract year
    uint16_t minor; // contract month
    uint32_t size; // header size
    uint64_t flags; // flags for SBL
    uint64_t stack; // stack size
    uint8_t padding[24]; // zeroed out
    uint64_t checksum; // crc32 checksum of the header

} __attribute__((packed));

static_assert(sizeof(SBLHeader) == 0x40, "SBLHeader size is not 64 bytes");

static_assert(sizeof(ELFHeader) == 0x10, "ELFHeader size is not 16 bytes");
static_assert(sizeof(ELF32Header) == 0x34, "ELF32Header size is not 52 bytes");
static_assert(sizeof(ELF64Header) == 0x40, "ELF64Header size is not 64 bytes");

enum class ELFHeaderValidationResult {
    BadMagic,
    BadClass,
    BadData,
    BadVersion,
    Good
};

enum class ElfLongHeaderValidationResult {
    BadType,
    UnsupportedType,
    BadMachine,
    BadVersion,
    BadEntryPoint,
    BadProgramHeaderOffset,
    BadSectionHeaderOffset,
    BadHeaderSize,
    BadProgramHeaderEntrySize,
    BadProgramHeaderEntryCount,
    BadSectionHeaderStringTableIndex,
    UnsupportedCombination,
    UnsupportedProgramHeaderEntryCount,
    UnsupportedOther,
    Good
};

enum class ProgramHeaderValidationResult {
    UnloadableSegment,
    InvalidFlags,
    InvalidOffset,
    InvalidSegmentSize,
    InvalidMemoryLocation,
    InvalidAlignment,
    OverlappingSegments,
    Good
};

ELFHeaderValidationResult ValidateELFHeader(const ELFHeader& header);

// assumes that the ELFHeader has already been validated and is good
ElfLongHeaderValidationResult ValidateELF32Header(const ELF32Header& header, const uint64_t fileSize);

// assumes that the ELFHeader has already been validated and is good
ElfLongHeaderValidationResult ValidateELF64Header(const ELF64Header& header, const uint64_t fileSize);

template<typename R>
ProgramHeaderValidationResult ValidateProgramHeader(const R& header, const uint64_t fileSize) {
    constexpr uint64_t ADDRESS_SPACE_SIZE = uint64_t{1} << 32;

    if (header.p_type != PT_LOAD) return ProgramHeaderValidationResult::UnloadableSegment;
    if (header.p_flags & ~(PF_X | PF_W | PF_R)) return ProgramHeaderValidationResult::InvalidFlags;
    if (header.p_offset > fileSize || header.p_filesz > fileSize - header.p_offset) return ProgramHeaderValidationResult::InvalidOffset;
    if (header.p_filesz > header.p_memsz) return ProgramHeaderValidationResult::InvalidSegmentSize;
    if (header.p_paddr >= ADDRESS_SPACE_SIZE || header.p_memsz > ADDRESS_SPACE_SIZE - header.p_paddr) return ProgramHeaderValidationResult::InvalidMemoryLocation;
    if (header.p_vaddr != header.p_paddr) return ProgramHeaderValidationResult::InvalidMemoryLocation;
    if (header.p_align > 1) {
        if ((header.p_align & (header.p_align - 1)) != 0) return ProgramHeaderValidationResult::InvalidAlignment;
        if ((header.p_vaddr % header.p_align) != (header.p_offset % header.p_align)) return ProgramHeaderValidationResult::InvalidAlignment;
    }
    return ProgramHeaderValidationResult::Good;
}

template<typename R>
ProgramHeaderValidationResult ValidateProgramHeaderOverlap(const R* headers, const uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        const R& a = headers[i];
        if (a.p_type != PT_LOAD || a.p_memsz == 0) continue;
        const uint64_t aStart = a.p_paddr;
        const uint64_t aEnd = aStart + a.p_memsz;

        for (uint16_t j = i + 1; j < count; j++) {
            const R& b = headers[j];
            if (b.p_type != PT_LOAD || b.p_memsz == 0) continue;
            const uint64_t bStart = b.p_paddr;
            const uint64_t bEnd = bStart + b.p_memsz;

            if (aStart < bEnd && bStart < aEnd) return ProgramHeaderValidationResult::OverlappingSegments;
        }
    }
    return ProgramHeaderValidationResult::Good;
}

uint64_t crc64(uint8_t* data, uint64_t len);

#endif // ELF_HPP