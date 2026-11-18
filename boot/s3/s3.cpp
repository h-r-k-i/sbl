/*******************************************************************************************
 * 
 * FILENAME:    ./boot/s3/s3.cpp
 * NAME:        SBL, Stage 3
 * DESCRIPTION: help
 *              
 * AUTHOR:      Haruki Tokumei, Haruki Media Group
 * DATE:        1 February 2026
 * COPYRIGHT:   Haruki Media Group.
 * LICENSE:     MIT License (./LICENSE)
 *******************************************************************************************/

#include "s3.hpp"
#include "suite.hpp"
#include "disk.hpp"
#include "pwm.hpp"
#include "fatfsd.hpp"
#include "elf.hpp"
#include "sblheader.hpp"
#include "protected.hpp"

#ifdef BUILD_DEBUG
    #define DEBUG
#endif // BUILD_DEBUG

void s3_main(uint32_t eax, uint32_t ebx, uint32_t edx) {
    uint16_t globalResult_16 = 0;
    // hang();
    volatile uint16_t * video = (uint16_t *)0xB8000;
    char * str1 = "        SBL: Loading Neko...               \0";
    uint16_t index = 0;
    uint16_t a = (0x0F << 8) | 'A';

    while (str1[index] != '\0') video[index] =  (0x0F << 8) | str1[++index];

    if (eax == SBL) {
        DisableCursor();

        // video[0] = (0x0F << 8) | VERT_RIGHT;
        // video[1] = (0x0F << 8) | RIGHT_ARROW;
        // video[2] = (0x0F << 8) | SPACE;
        // video[3] = (0x0F << 8) | SEVEN;

        // video[0 + 80] = (0x0F << 8) | VERT_RIGHT;
        // video[1 + 80] = (0x0F << 8) | TRIANGLE_RIGHT;
        // video[2 + 80] = (0x0F << 8) | SPACE;
        // video[3 + 80] = (0x0F << 8) | SEVEN;

        // video[0 + 80 * 2] = (0x0F << 8) | VERT_RIGHT;
        // video[1 + 80 * 2] = (0x0F << 8) | GREATER_THAN;
        // video[2 + 80 * 2] = (0x0F << 8) | SPACE;
        // video[3 + 80 * 2] = (0x0F << 8) | SEVEN;

        // video[0 + 80 * 3] = (0x0F << 8) | VERT_RIGHT;
        // video[1 + 80 * 3] = (0x0F << 8) | SPACE;
        // video[2 + 80 * 3] = (0x0F << 8) | LEFT_BRACKET;
        // video[3 + 80 * 3] = (0x0F << 8) | SEVEN;
        // video[4 + 80 * 3] = (0x0F << 8) | RIGHT_BRACKET;

        printk("Please type the 8.3 directory of the kernel you want to boot,\n");
        printk("or enter Q to shut down the device.\n");
        printk("Do not enter drive letters.\n");
        printk("Max length is 255 characters.\n");

        bool ValidCommand = false;

        char inputBuffer[256] = {0};
        Disk disk(edx & 0xFF, globalResult_16);
        uint32_t KernelLoadPoint = 0x0000;

        while (!ValidCommand) {
            printk("sbl.harukimedia.org# ");

            gets(inputBuffer, 256);
            printk("\n");
            char* exitCode = "Q";
            if (SBL_memcmp(inputBuffer, exitCode, 2) == true) {
                printk("Shutting down the device...\n");
                ShutDownPC();
                printk("Failed to shut down the device (what the fuck)\n");
                hang();
            }
            uint8_t sector[512];
            disk.GetSectors(0, 1, sector);
            MBR* mbr = (MBR*)(sector);

            for (auto a : mbr->Partitions) {
                if (a.isValid() == false) continue;
                if (a.IsExtended()) {
                    // todo: handle extended partitions
                    continue;
                }

                uint8_t vbrSector[512];
                disk.GetSectors(a.StartLBA, 1, vbrSector);
                VBR* vbr = (VBR*)(vbrSector);
                uint8_t* buffer = (uint8_t*)0x6000;

                FAT12 fat12(disk, a.StartLBA);
                FAT16 fat16(disk, a.StartLBA);
                FAT32 fat32(disk, a.StartLBA);

                FAT* fat = nullptr;

                switch (vbr->GetFATType()) {
                    case FATType::FAT12: fat = &fat12; break;
                    case FATType::FAT16: fat = &fat16; break;
                    case FATType::FAT32: fat = &fat32; break;
                    default: break;
                }
                if (fat != nullptr) {
                    auto packet = fat->ResolvePath(inputBuffer);
                    if (packet.directoryState == DirectoryState::Found) {
                        printk("Found %s.\n", inputBuffer);
                        if (packet.isDirectory) {
                            printk("The file %s is a directory, not a file.\n", inputBuffer);
                            continue;
                        }
                        if (packet.size < sizeof(ELFHeader) || packet.size < sizeof(SBLHeader)) {
                            printk("What the fuck are you presenting to me?\n");
                            continue;
                        }
                        FileContext context = {0, sizeof(ELFHeader)};
                        if (!fat->StreamFile(packet, buffer, sizeof(ELFHeader), context)) {
                            printk("1. Failed to read file %s.\n", inputBuffer);
                            continue;
                        }
                        ELFHeader* elfHeader = (ELFHeader*)buffer;
                        ELFHeaderValidationResult validationResult = ValidateELFHeader(*elfHeader);
                        if (validationResult != ELFHeaderValidationResult::Good) {
                            printk("Invalid ELF header for %s.\n", inputBuffer);
                            continue;
                        }
                        bool isSBLFile = false;
                        uint16_t sblSeekSize = [](uint64_t x, uint64_t y) -> uint16_t {
                            return (x < y) ? x : y;
                        }(SBL_MAX_REGION, packet.size);

                        buffer = (uint8_t*)0xF000;
                        context.size = sblSeekSize;
                        if (!fat->StreamFile(packet, buffer, sblSeekSize, context)) {
                            printk("Failed to read %s into memory.\n", inputBuffer);
                            continue;
                        }
                        SBLHeader* sblHeader = (SBLHeader*)buffer;
                        for (uint16_t i = 0; i <= sblSeekSize - sizeof(SBLHeader); i++) {
                            sblHeader = (SBLHeader*)(buffer + i);
                            if (!(sblHeader->magic[0] == 'N' && sblHeader->magic[1] == 'E' && sblHeader->magic[2] == 'K' && sblHeader->magic[3] == 'O' &&
                                sblHeader->magic[4] == 'N' && sblHeader->magic[5] == 'E' && sblHeader->magic[6] == 'K' && sblHeader->magic[7] == 'O')) continue;
                            uint64_t calculatedChecksum = crc64((uint8_t*)sblHeader, sizeof(SBLHeader) - sizeof(uint64_t));
                            if (calculatedChecksum != sblHeader->checksum) continue;
                            isSBLFile = true;
                            break;
                        }
                        if (!isSBLFile) {
                            printk("The file %s does not follow the Arc-C contract.\n", inputBuffer);
                            continue;
                        }
                        if ((~(((SBL_Data*)ebx)->flags | ((SBL_Data*)ebx)->sgalf) & sblHeader->flags) != 0) {
                            printk("This bootloader does not support the flags of %s.\n", inputBuffer);
                            continue;
                        }

                        buffer = (uint8_t*)0x6000;
                        context = {0, sizeof(ELFHeader)};
                        if (!fat->StreamFile(packet, buffer, sizeof(ELFHeader), context)) {
                            printk("2. Failed to read file %s.\n", inputBuffer);
                            continue;
                        }
                        elfHeader = (ELFHeader*)buffer;
                        if (elfHeader->class_ != 1) {
                            printk("The file %s is not a 32-bit ELF file.\n", inputBuffer);
                            continue;
                        }
                        context.size = sizeof(ELF32Header);
                        if (!fat->StreamFile(packet, buffer, sizeof(ELF32Header), context)) {
                            printk("3. Failed to read file %s.\n", inputBuffer);
                            continue;
                        }
                        ELF32Header* elf32Header = (ELF32Header*)buffer;
                        auto validationResult2 = ValidateELF32Header(*elf32Header, packet.size);
                        if (validationResult2 != ElfLongHeaderValidationResult::Good) {
                            printk("The file %s has an invalid ELF32 header.\n", inputBuffer);
                            continue;
                        }
                        KernelLoadPoint = elf32Header->e_entry;
                        context.offset = elf32Header->e_phoff;
                        context.size = elf32Header->e_phnum * elf32Header->e_phentsize;
                        uint16_t programHeaderCount = elf32Header->e_phnum;
                        if (!fat->StreamFile(packet, buffer, context.size, context)) {
                            printk("Failed to read program headers of %s.\n", inputBuffer);
                            continue;
                        }
                        pheader32* programHeaders = (pheader32*)buffer;
                        bool hasUnloadableSegment = false;
                        for (uint16_t i = 0; i < programHeaderCount; i++) {
                            auto validationResult3 = ValidateProgramHeader(programHeaders[i], packet.size);
                            if (programHeaders[i].p_type != PT_LOAD) continue;
                            if (validationResult3 != ProgramHeaderValidationResult::Good) {
                                hasUnloadableSegment = true;
                                break;
                            }

                            // todo: validate that the memory region isn't bad
                            if (programHeaders[i].p_paddr < 0x100000) {
                                hasUnloadableSegment = true;
                                break;
                            }

                        }
                        if (hasUnloadableSegment) {
                            printk("The file %s has an unloadable data segment in its program headers.\n", inputBuffer);
                            continue;
                        }

                        if (ValidateProgramHeaderOverlap(programHeaders, programHeaderCount) != ProgramHeaderValidationResult::Good) {
                            printk("The file %s has overlapping loadable segments in its program headers.\n", inputBuffer);
                            continue;
                        }

                        for (uint16_t i = 0; i < programHeaderCount; i++) {
                            if (programHeaders[i].p_type != PT_LOAD) continue;
                            context.offset = programHeaders[i].p_offset;
                            context.size = programHeaders[i].p_filesz;
                            if (!fat->StreamFile(packet, (uint8_t*)programHeaders[i].p_paddr, context.size, context)) {
                                printk("Failed to read segment %d of %s.\n", i, inputBuffer);
                                hasUnloadableSegment = true;
                                break;
                            }
                            if (programHeaders[i].p_memsz > programHeaders[i].p_filesz) {
                                uint32_t bssSize = programHeaders[i].p_memsz - programHeaders[i].p_filesz;
                                uint8_t* bssStart = (uint8_t*)(programHeaders[i].p_paddr + programHeaders[i].p_filesz);
                                for (uint32_t j = 0; j < bssSize; j++) bssStart[j] = 0;
                            }
                        }
                        if (hasUnloadableSegment) {
                            printk("Failed to load all segments of %s.\n", inputBuffer);
                            continue;
                        }

                        printk("File read successfully.\n");
                        ValidCommand = true;
                        break;
                    }
                    else printk("Did not find %s\n", inputBuffer);
                }
            }
        }

        if (ValidCommand) {
            for (uint16_t i = 0; i < 6; i++) {
                printk("Booting %s in %d seconds...\n", inputBuffer, 5 - i);
                if (i != 5) LiteralWait();
            }
            SBL_Region pmArea;
            if (SBL_GetPMIVTGDTArea(ebx, pmArea) != SBLMapResult::Good) {
                printk("Failed to get PM_IVT_GDT area from bootloader map.\n");
                hang();
            }
            RunKernel(KernelLoadPoint, eax, ebx, (uint16_t)(pmArea.address));
            printk("Failed to jump to kernel. This shouldn't appear.\n");
            printk("Please report this to issue@harukimedia.org.\n");
        }
        else {
            printk("Gave up, somehow.\n");
            printk("You really shouldn't see this message.\nPlease report it to issue@harukimedia.org lmao.\n");
        }



        // printk(VERT_RIGHT);
        // printk(RIGHT_ARROW);
        // printk(VERT_RIGHT);
        // printk(VERT_RIGHT);
        // printk("\n");

        hang(); // if we return from enableLongMode() then something went wrong and we should hang ourselves
    }
    else {
        str1 = "what????????????????";
        index = 0;
        while (str1[index] != '\0') {
            video[index] = (0x0F << 8) | str1[index];
            index++;
        }
        hang();
    }
}

void hang() {
    // #ifdef DEBUG
    //     printk("SBL: hang called.\n");
    // #endif // DEBUG
    // ((uint16_t*)(0xB8000))[0] = (0x0F << 8) | 'A';
    while (1) ;
}