#ifndef VMODE_HPP
#define VMODE_HPP

/*******************************************************************************************
 * 
 * FILENAME:    ./boot/s3/include/vmode.hpp
 * NAME:        SBL, Stage 3, Video Mode Switch || Header
 * DESCRIPTION: Header file for video mode switching functions in the SBL Stage 3 bootloader.
 * AUTHOR:      Haruki Tokumei, Haruki Media Group
 * DATE:        18 November 2026
 * COPYRIGHT:   Haruki Media Group.
 * LICENSE:     MIT License (./LICENSE)
 *******************************************************************************************/

#include <stdint.h>

extern "C" void SetVideoMode(uint16_t mode);

#endif // VMODE_HPP