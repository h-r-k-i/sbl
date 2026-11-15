/*******************************************************************************************
 * 
 * FILENAME:    ./boot/s3/include/protected.hpp
 * NAME:        SBL, Stage 3, Protected Mode Wrapper
 * DESCRIPTION: cotains a single function that wraps the protected mode entry point for
 *              the bootloader lmao
 * AUTHOR:      Haruki Tokumei, Haruki Media Group
 * DATE:        14 November 2026
 * COPYRIGHT:   Haruki Media Group.
 * LICENSE:     MIT License (./LICENSE)
 *******************************************************************************************/

#ifndef PROTECTED_HPP
#define PROTECTED_HPP

#include <stdint.h>

extern "C" void RunKernel(uint32_t kernelEntryPoint, uint32_t magicNumber, uint32_t bootloaderMapAddress, uint16_t GDT_IDT_Area);

#endif // PROTECTED_HPP