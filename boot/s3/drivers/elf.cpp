/*******************************************************************************************
 * 
 * FILENAME:    ./boot/s3/drivers/elf.cpp
 * NAME:        SBL, Stage 3, ELF Driver
 * DESCRIPTION: ELF driver for the SBL Stage 3 bootloader
 * AUTHOR:      Haruki Tokumei, Haruki Media Group
 * DATE:        14 November 2026
 * COPYRIGHT:   Haruki Media Group.
 * LICENSE:     MIT License (./LICENSE)
 *******************************************************************************************/

#include "elf.hpp"

ELFHeaderValidationResult ValidateELFHeader(const ELFHeader& header) {
    if (header.magic[0] != 0x7F || header.magic[1] != 'E' || header.magic[2] != 'L' || header.magic[3] != 'F') return ELFHeaderValidationResult::BadMagic;
    if (header.class_ != ELF_32 && header.class_ != ELF_64) return ELFHeaderValidationResult::BadClass;
    if (header.data != ELF_LITTLE_ENDIAN) return ELFHeaderValidationResult::BadData; // Only support little-endian for now
    if (header.version != 1) return ELFHeaderValidationResult::BadVersion; // Only version 1 really exists
    return ELFHeaderValidationResult::Good;
}

ElfLongHeaderValidationResult ValidateELF32Header(const ELF32Header& header, const uint64_t fileSize) {
    if (header.e_type == ET_DYN) return ElfLongHeaderValidationResult::UnsupportedType;
    if (header.e_type != ET_EXEC) return ElfLongHeaderValidationResult::BadType;
    if (header.e_machine == EM_8664) return ElfLongHeaderValidationResult::UnsupportedCombination; // 64-bit architecture with 32-bit header is not supported
    if (header.e_machine != EM_86) return ElfLongHeaderValidationResult::BadMachine;
    if (header.e_version != 1) return ElfLongHeaderValidationResult::BadVersion;
    // if (header.e_entry > ) // was about to check if a uint32_t may be greater than uint32_max whoops
    if (header.e_phoff > fileSize) return ElfLongHeaderValidationResult::BadProgramHeaderOffset;
    if (header.e_shoff > fileSize) return ElfLongHeaderValidationResult::BadSectionHeaderOffset;
    if (header.e_ehsize != sizeof(ELF32Header)) return ElfLongHeaderValidationResult::BadHeaderSize;
    if (header.e_phentsize != sizeof(pheader32)) return ElfLongHeaderValidationResult::BadProgramHeaderEntrySize;
    if (header.e_phnum == 0) return ElfLongHeaderValidationResult::BadProgramHeaderEntryCount;
    if (header.e_phnum == PN_XNUM) return ElfLongHeaderValidationResult::UnsupportedProgramHeaderEntryCount; // extended program header count is not supported
    if (uint64_t(header.e_phnum) * header.e_phentsize + header.e_phoff > fileSize) return ElfLongHeaderValidationResult::BadProgramHeaderEntryCount;
    return ElfLongHeaderValidationResult::Good;
}

ElfLongHeaderValidationResult ValidateELF64Header(const ELF64Header& header, const uint64_t fileSize) {
    if (header.e_type == ET_DYN) return ElfLongHeaderValidationResult::UnsupportedType;
    if (header.e_type != ET_EXEC) return ElfLongHeaderValidationResult::BadType;
    if (header.e_machine == EM_86) return ElfLongHeaderValidationResult::UnsupportedCombination; // 32-bit architecture with 64-bit header is not supported
    if (header.e_machine != EM_8664) return ElfLongHeaderValidationResult::BadMachine;
    if (header.e_version != 1) return ElfLongHeaderValidationResult::BadVersion;
    if (header.e_entry > UINT32_MAX) return ElfLongHeaderValidationResult::BadEntryPoint; // only support 32-bit entry points for now
    if (header.e_phoff > fileSize) return ElfLongHeaderValidationResult::BadProgramHeaderOffset;
    if (header.e_shoff > fileSize) return ElfLongHeaderValidationResult::BadSectionHeaderOffset;
    if (header.e_ehsize != sizeof(ELF64Header)) return ElfLongHeaderValidationResult::BadHeaderSize;
    if (header.e_phentsize != sizeof(pheader64)) return ElfLongHeaderValidationResult::BadProgramHeaderEntrySize;
    if (header.e_phnum == 0) return ElfLongHeaderValidationResult::BadProgramHeaderEntryCount;
    if (header.e_phnum == PN_XNUM) return ElfLongHeaderValidationResult::UnsupportedProgramHeaderEntryCount; // extended program header count is not supported
    if (uint64_t(header.e_phnum) * header.e_phentsize > fileSize - header.e_phoff) return ElfLongHeaderValidationResult::BadProgramHeaderEntryCount;
    return ElfLongHeaderValidationResult::Good;
}

uint64_t crc64(uint8_t* data, uint64_t len) {
    uint64_t crc = 0;
    uint64_t poly = 0x42F0E1EBA9EA3693;

    for (uint64_t i = 0; i < len; i++) {
        crc ^= (uint64_t)data[i] << 56ULL;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & (1ULL << 63)) crc = ((crc << 1) ^ poly) & 0xFFFFFFFFFFFFFFFF;
            else crc = (crc << 1) & 0xFFFFFFFFFFFFFFFF;
        }
    }

    return crc;
}